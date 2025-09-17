#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "led.h"
int main(void)
{
while(1){
bl_led_init();
bl_led_on();

}

    return 0;
}
