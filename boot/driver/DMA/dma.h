#ifndef __DMA_H

#define __DMA_H
#include <stdbool.h>
#include "stm32f4xx.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"

void DMA1_USART1_Tx_Init(uint8_t *tx_buffer, uint32_t data_length);
void DMA1_USART2_Rx_Init(uint8_t *rx_buffer, uint32_t data_length);

#endif/* __DMA_H */