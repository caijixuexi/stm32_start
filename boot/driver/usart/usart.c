#include "usart.h"

static uint16_t GPIO_Pin_to_PinSource(uint32_t gpio_pin)
{
    switch (gpio_pin)
    {
    case GPIO_Pin_0:
        return GPIO_PinSource0;
    case GPIO_Pin_1:
        return GPIO_PinSource1;
    case GPIO_Pin_2:
        return GPIO_PinSource2;
    case GPIO_Pin_3:
        return GPIO_PinSource3;
    case GPIO_Pin_4:
        return GPIO_PinSource4;
    case GPIO_Pin_5:
        return GPIO_PinSource5;
    case GPIO_Pin_6:
        return GPIO_PinSource6;
    case GPIO_Pin_7:
        return GPIO_PinSource7;
    case GPIO_Pin_8:
        return GPIO_PinSource8;
    case GPIO_Pin_9:
        return GPIO_PinSource9;
    case GPIO_Pin_10:
        return GPIO_PinSource10;
    case GPIO_Pin_11:
        return GPIO_PinSource11;
    case GPIO_Pin_12:
        return GPIO_PinSource12;
    case GPIO_Pin_13:
        return GPIO_PinSource13;
    case GPIO_Pin_14:
        return GPIO_PinSource14;
    case GPIO_Pin_15:
        return GPIO_PinSource15;
    default:
        return GPIO_PinSource0; // 默认值或错误处理
    }
}
static uint16_t GPIO_AF_NUM(usart_number_t usart_number)
{
    switch (usart_number)
    {
    case USART_1:
        return GPIO_AF_USART1;
    case USART_2:
        return GPIO_AF_USART2;
    case USART_3:
        return GPIO_AF_USART3;
    case UART_4:
        return GPIO_AF_UART4;
    case UART_5:
        return GPIO_AF_UART5;
    case USART_6:
        return GPIO_AF_USART6;
    case UART_7:
        return GPIO_AF_UART7;
    case UART_8:
        return GPIO_AF_UART8;
    default:
        return GPIO_AF_USART1; // 默认值或错误处理
    }
}
// 修正函数返回类型
static USART_TypeDef* USART_addr(usart_number_t usart_number)
{
    switch(usart_number)
    {
        case USART_1: return USART1;
        case USART_2: return USART2;
        case USART_3: return USART3;
        case UART_4: return UART4;
        case UART_5: return UART5;
        case USART_6: return USART6;
        case UART_7: return UART7;
        case UART_8: return UART8;
        default: return USART1;
    }
}


void usart_Init(usart_t *usart)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

    /* Enable GPIO clock */
    /* check whether gpiox is valid and RCC_AHB1PeriphClockCmd is initial already*/
    /* Enable GPIO clock */
    uint32_t rcc_ahb1enr_mask = 0;

    if (usart->gpiox == GPIOA)
    {
        rcc_ahb1enr_mask = RCC_AHB1ENR_GPIOAEN;
    }
    else if (usart->gpiox == GPIOB)
    {
        rcc_ahb1enr_mask = RCC_AHB1ENR_GPIOBEN;
    }
    else if (usart->gpiox == GPIOC)
    {
        rcc_ahb1enr_mask = RCC_AHB1ENR_GPIOCEN;
    }
    else if (usart->gpiox == GPIOD)
    {
        rcc_ahb1enr_mask = RCC_AHB1ENR_GPIODEN;
    }
    else if (usart->gpiox == GPIOE)
    {
        rcc_ahb1enr_mask = RCC_AHB1ENR_GPIOEEN;
    }

    if (rcc_ahb1enr_mask && (RCC->AHB1ENR & rcc_ahb1enr_mask) == 0)
    {
        RCC->AHB1ENR |= rcc_ahb1enr_mask; // 直接操作寄存器更简洁
    }

    /* Enable USART clock */
    /* Enable USART clock based on usart number */
    switch (usart->usart_number)
    {
    case USART_1:
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        break;
    case USART_2:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // 注意：USART2在APB1
        break;
    case USART_3:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        break;
    case UART_4:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
        break;
    case UART_5:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
        break;
    case USART_6:
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
        break;
    case UART_7:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART7, ENABLE);
        break;
    case UART_8:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART8, ENABLE);
        break;
    default:
        break;
    }

    /* Configure USART Tx and Rx as alternate function */
    GPIO_InitStruct.GPIO_Pin = usart->gpio_pin_tx | usart->gpio_pin_rx;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(usart->gpiox, &GPIO_InitStruct);

    /* 配置USART引脚复用功能 */

    GPIO_PinAFConfig(usart->gpiox, GPIO_Pin_to_PinSource(usart->gpio_pin_tx), GPIO_AF_NUM(usart->usart_number));
    GPIO_PinAFConfig(usart->gpiox, GPIO_Pin_to_PinSource(usart->gpio_pin_rx), GPIO_AF_NUM(usart->usart_number));
    /* USART configuration */
    USART_InitStruct.USART_BaudRate = usart->baud_rate;
    USART_InitStruct.USART_WordLength = usart->data_bits;
    USART_InitStruct.USART_StopBits = usart->stop_bits;
    USART_InitStruct.USART_Parity = usart->parity;
    USART_InitStruct.USART_Mode = usart->mode;  
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART_addr(usart->usart_number), &USART_InitStruct);

    /* Enable USART */
    USART_Cmd(USART_addr(usart->usart_number), ENABLE);
}