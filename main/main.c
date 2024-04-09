/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueADC;

typedef struct adc {
    int axis;
    int val;
} adc_t;

int conversor(int read) {
    int result = (int) ((510 * read/4095) - 255);

    return result;
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

void x_task(void *p) {
    adc_t data;
    int vec[5] = {0, 0, 0, 0, 0};

    adc_gpio_init(28);
    
    uint16_t result;
    while(1) {
        adc_select_input(2);

        result = conversor(adc_read());

        int y = 0;
        vec[0] = vec[1];
        vec[1] = vec[2];
        vec[2] = vec[3];
        vec[4] = result;

        for(int i = 0; i < 5; i++) {
            y += vec[i];
        }

        y /= 5;

        // if(y >= -162 && y <= 162) {
        //     y = 0;
        // }

        printf("%d\n", y);
    
        data.axis = 0;
        data.val = y;
        
        xQueueSend(xQueueADC, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void y_task(void *p) {
    adc_t data;
    int vec[5] = {0, 0, 0, 0, 0};

    adc_gpio_init(26);

    uint16_t result;
    while(1) {
        adc_select_input(0);

        result = conversor(adc_read());

        int y = 0;
        vec[0] = vec[1];
        vec[1] = vec[2];
        vec[2] = vec[3];
        vec[4] = result;

        for(int i = 0; i < 5; i++) {
            y += vec[i];
        }

        y /= 5;

        // if(y >= -162 && y <= 162) {
        //     y = 0;
        // }

        printf("%d\n", y);

        data.axis = 1;
        data.val = y;
        
        xQueueSend(xQueueADC, &data, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if(xQueueReceive(xQueueADC, &data, portMAX_DELAY)) {
            write_package(data);
        }
    }
}

int main() {
    stdio_init_all();
    adc_init();

    xQueueADC = xQueueCreate(32, sizeof(adc_t));
    
    xTaskCreate(x_task, "x_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 256, NULL, 1, NULL);

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
