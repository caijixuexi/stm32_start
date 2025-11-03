#ifndef __USART_H__
#define __USART_H__

#include "stm32f4xx.h"
#include <stdbool.h>
#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "dma.h"
#include "ringbuffer8.h"
typedef enum usart_number{
    USART_1 = 1,
    USART_2,
    USART_3,
    UART_4,
    UART_5,
    USART_6,
    UART_7,
    UART_8
}usart_number_t;

typedef struct usart_init_t
{   usart_number_t usart_number;
    uint32_t baud_rate;
    uint32_t data_bits;
    uint32_t stop_bits;
    uint32_t parity;
    uint32_t mode;
    uint32_t gpiox;
    uint32_t gpio_pin_rx;
    uint32_t gpio_pin_tx;
}usart_t;

usart_t usart1 = {
    .usart_number = USART_1,
    .baud_rate = 115200,
    .data_bits = USART_WordLength_8b ,
    .stop_bits = USART_StopBits_1,
    .parity = USART_Parity_No,
    .mode = USART_Mode_Tx | USART_Mode_Rx,
    .gpiox = GPIOA,
    .gpio_pin_rx = GPIO_Pin_9,
    .gpio_pin_tx = GPIO_Pin_10,
};
void usart_Init(usart_t* usart);
void usart_Transmit(USART_TypeDef* USARTx, uint8_t byte);
uint8_t usart_Receive(USART_TypeDef* USARTx);
#endif
