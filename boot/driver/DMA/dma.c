#include "dma.h"
void DMA1_USART1_Tx_Init(uint8_t *tx_buffer, uint32_t data_length)
{
    /* Enable DMA1 clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    /* Deinitialize DMA1 Stream 5 */
    DMA_DeInit(DMA1_Stream5);

    /* Configure DMA1 Stream 5 */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_Channel = DMA_Channel_2; // 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = 0; // Will be configured later
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = 0; // Will be configured later
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

    DMA_Init(DMA1_Stream5, &DMA_InitStructure);

    /* Enable the DMA Stream */
    //DMA_Cmd(DMA1_Stream5, ENABLE);
}