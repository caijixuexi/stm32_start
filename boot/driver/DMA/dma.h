#ifndef __DMA_H
#define __DMA_H
#include <stdbool.h>
#include "stm32f4xx.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "ringbuffer8.h"

#define USART_MAX_LEN 256  // 定义USART最大接收长度
// 添加DMA配置结构体，用于传递配置参数
typedef struct {
    USART_TypeDef* usart;          // USART外设指针
    uint8_t* buffer;               // 数据缓冲区
    uint32_t buffer_size;          // 缓冲区大小
    ringbuffer8_t* ringbuffer;     // 环形缓冲区指针(可选)
} dma_usart_config_t;

// TX相关函数
extern void DMA2_USART1_Tx_Init(uint8_t *tx_buffer, uint32_t data_length);
extern void DMA2_USART1_Tx_Start(uint8_t *tx_buffer, uint32_t data_length);
extern bool DMA2_USART1_Tx_IsComplete(void);

// RX相关函数
extern void DMA2_USART1_Rx_Init(uint8_t *rx_buffer, uint32_t data_length);
extern void DMA2_USART1_Rx_Start(void);
extern uint32_t DMA2_USART1_Rx_GetReceivedCount(void);
extern void DMA2_USART1_Rx_Reset(void);

// 带ringbuffer的初始化函数
extern void DMA2_USART1_Rx_Init_WithRingbuffer(ringbuffer8_t* ringbuf, uint8_t *rx_buffer, uint32_t data_length);

#endif /* __DMA_H */