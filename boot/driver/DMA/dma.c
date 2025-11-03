#include "dma.h"
void DMA2_USART1_Tx_Init(uint8_t *tx_buffer, uint32_t data_length)
{
    /* Enable DMA1 clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    /* Deinitialize DMA2 Stream 7 */
    DMA_DeInit(DMA2_Stream7);

    /* Configure DMA2 Stream 7 */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_4; //
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)tx_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = data_length;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream7, &DMA_InitStructure);
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);

    /* 不使能DMA流，等待需要时再使能 */
    DMA_Cmd(DMA2_Stream7, DISABLE);
}
void DMA2_USART1_Tx_Start(uint8_t *tx_buffer, uint32_t data_length)
{
    while (DMA_GetCmdStatus(DMA2_Stream7) != DISABLE)
        ;
    /* 重新配置内存地址和数据长度 */
    DMA2_Stream7->M0AR = (uint32_t)tx_buffer;
    DMA2_Stream7->NDTR = data_length;

    /*使能USART1的DMA发送请求*/
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);

    /* 启动DMA传输 */
    DMA_Cmd(DMA2_Stream7, ENABLE);
}

bool DMA2_USART1_Tx_IsComplete(void)
{
    return (DMA_GetFlagStatus(DMA2_Stream7, DMA_FLAG_TCIF7) == SET);
}

// RX相关函数实现
void DMA2_USART1_Rx_Init(uint8_t *rx_buffer, uint32_t data_length)
{
    /* Enable DMA2 clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* Deinitialize DMA2 Stream 5 */
    DMA_DeInit(DMA2_Stream5);
    while (DMA_GetCmdStatus(DMA2_Stream5) != DISABLE)
        ;

    /* Configure DMA2 Stream 5 */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_4; // USART1_RX使用通道4
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rx_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = data_length;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream5, &DMA_InitStructure);
}

void DMA2_USART1_Rx_Start(void)
{
    /* 确保DMA流已禁用 */
    while (DMA_GetCmdStatus(DMA2_Stream5) != DISABLE)
        ;

    /* 使能USART1的DMA接收请求 */
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

    /* 使能DMA流 */
    DMA_Cmd(DMA2_Stream5, ENABLE);
}

uint32_t DMA2_USART1_Rx_GetReceivedCount(void)
{
    return (USART_MAX_LEN - DMA_GetCurrDataCounter(DMA2_Stream5));
}

void DMA2_USART1_Rx_Reset(void)
{
    /* 禁用DMA流 */
    DMA_Cmd(DMA2_Stream5, DISABLE);

    /* 清除标志位 */
    DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_DMEIF5 | DMA_FLAG_FEIF5);

    /* 重新设置数据计数器 */
    DMA_SetCurrDataCounter(DMA2_Stream5, USART_MAX_LEN);

    /* 重新使能DMA流 */
    DMA_Cmd(DMA2_Stream5, ENABLE);
}

// 带ringbuffer的初始化函数实现
// 注意：这个函数需要您在stm32f4xx_it.c中实现对应的中断处理函数
void DMA2_USART1_Rx_Init_WithRingbuffer(ringbuffer8_t *ringbuf, uint8_t *rx_buffer, uint32_t data_length)
{
    // 首先调用基本初始化函数
    DMA2_USART1_Rx_Init(rx_buffer, data_length);

    // 这里可以添加ringbuffer相关的初始化代码
    // 注意：为了与ringbuffer配合使用，您需要在USART1的空闲中断中实现数据处理逻辑
}