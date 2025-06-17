#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "hardware/adc.h"
#include "hardware/irq.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

QueueHandle_t queue;

void task_init(void *params) {

    // Inicializo la cola
    queue = xQueueCreate(1, sizeof(uint16_t));
    // Inicializo el ADC
    adc_init();
    // Inicializo el Sensor de Temperatura
    adc_set_temp_sensor_enabled(true);
    //Selecciono el pin de entrada qes el 4
    adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);

    // Elimina la tarea para liberar recursos
    vTaskDelete(NULL);
}
/*
adc_fifo_get();

adc_init();
adc_set_temp_sensor_enabled(true);
adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);
    */

void task_ADC(void *params) {

    while(1) {
        // Obtengo el valor del ADC
        uint16_t adc = adc_read();
        // Mando por cola
        xQueueSend(queue, &adc , portMAX_DELAY);
        // Bloqueo tarea para no saturar
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_print(void *params) {
    
    uint16_t adc;
    const float conversion_factor = 3.3f / (1 << 12);
    float T;
    while(1) {
        //Convierto el valor del ADC a Temperatura
         T = 27 - (adc * conversion_factor - 0.706)/0.001721;
        // Bloqueo tarea hasta que llegue el dato
        xQueueReceive(queue, &adc, portMAX_DELAY);
        // Escribo por consola

        printf("Valor del ADC: %d\n", adc);
        printf("Valor de TENSION: %f\n", adc * conversion_factor);
        printf("Valor de TEMPERATURA: %f\n", T);

    }
}


int main()
{
    stdio_init_all();
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    /* Example to turn on the Pico W LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    */

   xTaskCreate(task_init, "Crear_Cola", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
   xTaskCreate(task_ADC, "Cargar_Cola", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
   xTaskCreate(task_print, "Imprimir_Cola", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);


   vTaskStartScheduler();
   while(1);

}





