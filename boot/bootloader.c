#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "crc32.h"
#include "ringbuffer8.h"
#include "flash_layout.h"
#include "arginfo.h"
#include "stm32f4xx.h"
#include "uart.h"
#include "led.h"
#include "button.h"
#include "norflash.h"


#define LOG_LVL     ELOG_LVL_DEBUG
#define LOG_TAG     "boot"
#include "elog.h"


#define BOOTLOADER_VERSION_MAJOR    1
#define BOOTLOADER_VERSION_MINOR    0

#define BL_TIMEOUT_MS               500ul
#define BL_UART_BUFFER_SIZE         512ul
#define BL_PACKET_HEAD_SIZE         128ul
#define BL_PACKET_PAYLOAD_SIZE      4096ul
#define BL_PACKET_PARAM_SIZE        (BL_PACKET_HEAD_SIZE + BL_PACKET_PAYLOAD_SIZE)

#define BL_EVT_RX                   (1 << 0)
#define BL_EVT_BOOT                 (1 << 10)
#define BL_EVT_POWEROFF             (1 << 11)

/* format
 *
 * | start | opcode | length | payload | crc32 |
 * | u8    | u8     | u16    | u8 * n  | u32   |
 *
 * start: 0xAA
 */

typedef enum
{
    BL_SM_IDLE,
    BL_SM_START,
    BL_SM_OPCODE,
    BL_SM_LENGTH,
    BL_SM_PARAM,
    BL_SM_CRC,
} bl_state_machine_t;

typedef enum
{
    BL_OP_NONE = 0x00,
    BL_OP_INQUIRY = 0x10,
    BL_OP_BOOT = 0x11,
    BL_OP_RESET = 0x1F,
    BL_OP_ERASE = 0x20,
    BL_OP_READ, // 功能未实现
    BL_OP_WRITE,
    BL_OP_VERIFY,
    BL_OP_END,
} bl_op_t;

typedef enum
{
    BL_INQUIRY_VERSION,
    BL_INQUIRY_BLOCK_SIZE,
} bl_inquiry_t;

typedef enum
{
    BL_ERR_OK,
    BL_ERR_OPCODE,
    BL_ERR_OVERFLOW,
    BL_ERR_TIMEOUT,
    BL_ERR_FORMAT,
    BL_ERR_VERIFY,
    BL_ERR_PARAM,
    BL_ERR_UNKNOWN = 0xff,
} bl_err_t;

typedef struct
{
    bl_op_t  opcode;
    uint16_t length;
    uint32_t crc;

    uint8_t  param[BL_PACKET_PARAM_SIZE];
    uint16_t index;
} bl_pkt_t;

typedef struct
{
    uint8_t data[16];
    uint16_t index;
} bl_rx_t;

typedef struct
{
    bl_pkt_t pkt;
    bl_rx_t rx;
    bl_state_machine_t sm;
} bl_ctrl_t;

typedef struct
{
    uint8_t subcode;
} bl_inquiry_param_t;

typedef struct
{
    uint32_t address;
    uint32_t size;
} bl_erase_param_t;

typedef struct
{
    uint32_t address;
    uint32_t size;
} bl_read_param_t;

typedef struct
{
    uint32_t address;
    uint32_t size;
    uint8_t data[];
} bl_write_param_t;

typedef struct
{
    uint32_t address;
    uint32_t size;
    uint32_t crc;
} bl_verify_param_t;


static ringbuffer8_t serial_rb;
static uint8_t serial_rb_buffer[BL_UART_BUFFER_SIZE];
static bl_ctrl_t bl_ctrl;
static uint32_t last_pkt_time;


void boot_application(uint32_t address);
void boot_application_from_argtable(void);


static void serial_recv_callback(uint8_t *data, uint32_t len)
{
    rb8_puts(serial_rb, data, len);
}

static void bl_reset(bl_ctrl_t *ctrl)
{
    ctrl->sm = BL_SM_IDLE;
    ctrl->rx.index = 0;
    ctrl->pkt.index = 0;
}

static void bl_response(bl_op_t op, uint8_t *data, uint16_t length)
{
    const uint8_t head = 0xAA;

    uint32_t crc = 0;
    crc = crc32_update(crc, (uint8_t *)&head, 1);
    crc = crc32_update(crc, (uint8_t *)&op, 1);
    crc = crc32_update(crc, (uint8_t *)&length, 2);
    crc = crc32_update(crc, data, length);

    bl_uart_write((uint8_t *)&head, 1);
    bl_uart_write((uint8_t *)&op, 1);
    bl_uart_write((uint8_t *)&length, 2);
    bl_uart_write(data, length);
    bl_uart_write((uint8_t *)&crc, 4);
}

static void bl_response_ack(bl_op_t op, bl_err_t err)
{
    bl_response(op, (uint8_t *)&err, 1);
}

static void bl_op_inquiry_handler(uint8_t *data, uint16_t length)
{
    log_i("inquery");

    bl_inquiry_param_t *inquiry = (void *)data;

    if (length != sizeof(bl_inquiry_param_t))
    {
        log_w("length mismatch %d != %d", length, sizeof(bl_inquiry_param_t));
        bl_response_ack(BL_OP_INQUIRY, BL_ERR_PARAM);
        return;
    }

    log_i("subcode: %02X", inquiry->subcode);
    switch (inquiry->subcode)
    {
        case BL_INQUIRY_VERSION:
        {
            uint8_t version[] = { BOOTLOADER_VERSION_MAJOR, BOOTLOADER_VERSION_MINOR };
            bl_response(BL_OP_INQUIRY, version, sizeof(version));
            break;
        }
        case BL_INQUIRY_BLOCK_SIZE:
        {
            uint16_t size = BL_PACKET_PAYLOAD_SIZE;
            bl_response(BL_OP_INQUIRY, (uint8_t *)&size, sizeof(size));
            break;
        }
        default:
        {
            bl_response_ack(BL_OP_INQUIRY, BL_ERR_PARAM);
            break;
        }
    }
}

static void bl_op_boot_handler(uint8_t *data, uint16_t length)
{
    log_i("boot");

    bl_response_ack(BL_OP_BOOT, BL_ERR_OK);

    boot_application_from_argtable();
}

static void bl_op_reset_handler(uint8_t *data, uint16_t length)
{
    log_i("reset");

    bl_response_ack(BL_OP_RESET, BL_ERR_OK);

    NVIC_SystemReset();
}

static void bl_op_erase_handler(uint8_t *data, uint16_t length)
{
    log_i("erase");

    bl_erase_param_t *erase = (void *)data;

    if (length != sizeof(bl_erase_param_t))
    {
        log_w("length mismatch %d != %d", length, sizeof(bl_erase_param_t));
        bl_response_ack(BL_OP_ERASE, BL_ERR_PARAM);
        return;
    }

    if (erase->address >= FLASH_BOOT_ADDRESS &&
        erase->address < FLASH_BOOT_ADDRESS + FLASH_BOOT_SIZE)
    {
        log_w("address 0x%08X is protected", erase->address);
        bl_response_ack(BL_OP_ERASE, BL_ERR_UNKNOWN);
        return;
    }

    log_i("erase 0x%08X, size %d", erase->address, erase->size);
    bl_norflash_erase(erase->address, erase->size);

    bl_response_ack(BL_OP_ERASE, BL_ERR_OK);
}

static void bl_op_read_handler(uint8_t *data, uint16_t length)
{
    log_i("read");

    // bl_read_param_t *read = (void *)data;

    // if (length != sizeof(bl_read_param_t))
    // {
    //     bl_response_ack(BL_OP_READ, BL_ERR_PARAM);
    //     return;
    // }

    // log_i("read 0x%08X, size %d", read->address, read->size);
}

static void bl_op_write_handler(uint8_t *data, uint16_t length)
{
    log_i("write");

    bl_write_param_t *write = (void *)data;

    if (length != sizeof(bl_write_param_t) + write->size)
    {
        log_w("length mismatch %d != %d", length, sizeof(bl_write_param_t) + write->size);
        bl_response_ack(BL_OP_WRITE, BL_ERR_PARAM);
        return;
    }

    if (write->address >= FLASH_BOOT_ADDRESS &&
        write->address < FLASH_BOOT_ADDRESS + FLASH_BOOT_SIZE)
    {
        log_w("address 0x%08X is protected", write->address);
        bl_response_ack(BL_OP_ERASE, BL_ERR_UNKNOWN);
        return;
    }

    log_i("write 0x%08X, size %d", write->address, write->size);
    bl_norflash_write(write->address, write->data, write->size);

    bl_response_ack(BL_OP_WRITE, BL_ERR_OK);
}

static void bl_op_verify_handler(uint8_t *data, uint16_t length)
{
    log_i("verify");
    bl_verify_param_t *verify = (void *)data;

    if (length != sizeof(bl_verify_param_t))
    {
        log_w("length mismatch %d != %d", length, sizeof(bl_verify_param_t));
        bl_response_ack(BL_OP_VERIFY, BL_ERR_PARAM);
        return;
    }

    log_i("verify 0x%08X, size %d", verify->address, verify->size);
    uint32_t crc = crc32_update(0, (uint8_t *)verify->address, verify->size);

    log_i("crc: %08X verify: %08X", crc, verify->crc);
    if (crc == verify->crc)
    {
        bl_response_ack(BL_OP_VERIFY, BL_ERR_OK);
    }
    else
    {
        bl_response_ack(BL_OP_VERIFY, BL_ERR_VERIFY);
    }
}

static void bl_pkt_handler(bl_pkt_t *pkt)
{
    log_i("opcode: %02X, length: %d", pkt->opcode, pkt->length);
    switch (pkt->opcode)
    {
        case BL_OP_INQUIRY:
            bl_op_inquiry_handler(pkt->param, pkt->length);
            break;
        case BL_OP_BOOT:
            bl_op_boot_handler(pkt->param, pkt->length);
            break;
        case BL_OP_RESET:
            bl_op_reset_handler(pkt->param, pkt->length);
            break;
        case BL_OP_ERASE:
            bl_op_erase_handler(pkt->param, pkt->length);
            break;
        case BL_OP_READ:
            bl_op_read_handler(pkt->param, pkt->length);
            break;
        case BL_OP_WRITE:
            bl_op_write_handler(pkt->param, pkt->length);
            break;
        case BL_OP_VERIFY:
            bl_op_verify_handler(pkt->param, pkt->length);
            break;
        default:
            break;
    }
}

static bool bl_pkt_verify(bl_pkt_t *pkt)
{
    const uint8_t head = 0xAA;

    uint32_t crc = 0;
    crc = crc32_update(crc, (uint8_t *)&head, 1);
    crc = crc32_update(crc, (uint8_t *)&pkt->opcode, 1);
    crc = crc32_update(crc, (uint8_t *)&pkt->length, 2);
    crc = crc32_update(crc, pkt->param, pkt->length);

    return crc == pkt->crc;
}

static bool bl_recv_handler(bl_ctrl_t *ctrl, uint8_t data)
{
    bool fullpkt = false;

    bl_rx_t *rx = &ctrl->rx;
    bl_pkt_t *pkt = &ctrl->pkt;

    rx->data[rx->index++] = data;

    switch (ctrl->sm)
    {
        case BL_SM_IDLE:
        {
            log_d("sm idle");

            rx->index = 0;
            if (rx->data[0] == 0xAA)
            {
                ctrl->sm = BL_SM_START;
            }
            break;
        }
        case BL_SM_START:
        {
            log_d("sm start");

            rx->index = 0;
            pkt->opcode = (bl_op_t)rx->data[0];
            ctrl->sm = BL_SM_OPCODE;
            break;
        }
        case BL_SM_OPCODE:
        {
            log_d("sm opcode");

            if (rx->index == 2)
            {
                rx->index = 0;
                uint16_t length = *(uint16_t *)rx->data;
                if (length <= BL_PACKET_PARAM_SIZE)
                {
                    pkt->length = length;
                    if (length == 0) ctrl->sm = BL_SM_CRC;
                    else             ctrl->sm = BL_SM_PARAM;
                }
                else
                {
                    bl_response_ack(pkt->opcode, BL_ERR_OVERFLOW);
                    bl_reset(ctrl);
                }
            }
            break;
        }
        case BL_SM_PARAM:
        {
            log_d("sm param");

            rx->index = 0;
            if (pkt->index < pkt->length)
            {
                pkt->param[pkt->index++] = rx->data[0];
                if (pkt->index == pkt->length)
                {
                    ctrl->sm = BL_SM_CRC;
                }
            }
            else
            {
                bl_response_ack(pkt->opcode, BL_ERR_OVERFLOW);
                bl_reset(ctrl);
            }
            break;
        }
        case BL_SM_CRC:
        {
            log_d("sm crc");

            if (rx->index == 4)
            {
                rx->index = 0;
                pkt->crc = *(uint32_t *)rx->data;
                if (bl_pkt_verify(pkt))
                {
                    fullpkt = true;
                }
                else
                {
                    log_w("crc mismatch");
                    bl_response_ack(pkt->opcode, BL_ERR_VERIFY);
                    bl_reset(ctrl);
                }
            }
            break;
        }
        default:
        {
            rx->index = 0;
            bl_response_ack(BL_OP_NONE, BL_ERR_OPCODE);
            bl_reset(ctrl);
            break;
        }
    }

    return fullpkt;
}

static void bl_lowlevel_deinit(void)
{
#if DEBUG
    elog_deinit();
#endif

    bl_uart_deinit();
    // 停用所有中断
    __disable_irq();
}

#if CONFIG_BOOT_DELAY > 0
static void boot_tim_handler(TimerHandle_t xTimer)
{
    static uint32_t count = 0;
    uint32_t timeout = *(uint32_t *)pvTimerGetTimerID(xTimer);
    if (++count < timeout)
    {
        log_i("boot in %d seconds", timeout - count);
        return;
    }

    xTaskNotify(bl_task_handle, BL_EVT_BOOT, eSetBits);
    xTimerStop(xTimer, 0);
}
#endif

void bootloader_main(uint32_t boot_delay)
{
    bl_uart_recv_register(serial_recv_callback);
    bl_norflash_init();

    serial_rb = rb8_new(serial_rb_buffer, BL_UART_BUFFER_SIZE);

    while (1)
    {
        // 按键重启
        if (bl_button_pressed())
        {
            bl_delay_ms(5);
            if (bl_button_pressed())
            {
                log_i("button pressed, reset");
                NVIC_SystemReset();
                break;
            }
        }

        if (rb8_empty(serial_rb))
        {
            if (bl_ctrl.rx.index == 0)
            {
                last_pkt_time = bl_get_ticks();
            }
            else
            {
                if (bl_get_ticks() - last_pkt_time > BL_TIMEOUT_MS)
                {
                    log_w("recv timeout");
                    #if DEBUG
                    elog_hexdump("recv", 16, bl_ctrl.rx.data, bl_ctrl.rx.index);
                    #endif
                    bl_reset(&bl_ctrl);
                }
            }
            continue;
        }

        uint8_t data;
        rb8_get(serial_rb, &data);
        log_d("recv: %02X", data);
        if (bl_recv_handler(&bl_ctrl, data))
        {
            bl_pkt_handler(&bl_ctrl.pkt);
            bl_reset(&bl_ctrl);

            last_pkt_time = bl_get_ticks();

        #if CONFIG_BOOT_DELAY > 0
            if (boot_tim_handle)
            {
                log_i("command received, stop boot");
                xTimerStop(boot_tim_handle, portMAX_DELAY);
                boot_tim_handle = NULL;
            }
        #endif
        }
    }
}

bool verify_application(void)
{
    uint32_t address, size, crc;
    bool result = bl_arginfo_read(&address, &size, &crc);
    CHECK_RETX(result, false);

    uint32_t ccrc = crc32_update(0, (uint8_t *)address, size);
    if (ccrc != crc)
    {
        log_w("crc mismatch: %08X != %08X", ccrc, crc);
        return false;
    }

    return true;
}

void boot_application(uint32_t address)
{
    typedef int (*entry_t)(void);

    uint32_t sp = *(volatile uint32_t*)(address + 0);
    uint32_t pc = *(volatile uint32_t*)(address + 4);

    (void)sp;
    entry_t app_entry = (entry_t)pc;

    log_i("booting application at 0x%08X", address);

    bl_lowlevel_deinit();

    SCB->VTOR = address;

    app_entry();
}

void boot_application_from_argtable(void)
{
    uint32_t address;
    bool result = bl_arginfo_read(&address, NULL, NULL);
    CHECK_RET(result);

    boot_application(address);
}
