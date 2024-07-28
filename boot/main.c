#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "uart.h"
#include "button.h"
#include "led.h"

#define LOG_TAG "main"
#define LOG_LVL ELOG_LVL_INFO
#include "elog.h"


extern void bl_lowlevel_init(void);
extern void bootloader_main(uint32_t boot_delay);
extern bool verify_application(void);


static bool button_trap_boot(void)
{
    if (bl_button_pressed())
    {
        bl_delay_ms(100);
        return bl_button_pressed();
    }

    return false;
}

static void button_wait_release(void)
{
    while (bl_button_pressed())
    {
        bl_delay_ms(100);
    }
}

int main(void)
{
    bl_lowlevel_init();

#if DEBUG
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG);
    elog_start();
#endif

    bl_delay_init();
    bl_uart_init();
    bl_button_init();
    bl_led_init();

    log_d("button: %d", bl_button_pressed());

    bool trap_boot = false;
    if (button_trap_boot())
    {
        log_w("button pressed, trap into boot");
        trap_boot = true;
    }
    else if (!verify_application())
    {
        log_w("application verify failed, trap into boot");
        trap_boot = true;
    }

    if (trap_boot)
    {
        bl_led_on();
        button_wait_release();
    }

    bootloader_main(trap_boot ? 0 : 3);

    return 0;
}
