#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "hardware/adc.h"
#include "hardware/irq.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define SAMPLES 4
QueueHandle_t queue;

void adc_irq_handler(void) {

   
    const float conversion_factor = 3.3f / (1 << 12);

    // Deshabilito la interrupcion y detengo el ADC
    adc_irq_set_enabled(false);
    adc_run(false);

    // Variable para calcular el promedio de muestras
    uint32_t Promedio = 0;
    for(uint8_t i = 0; i < SAMPLES; i++) { Promedio += adc_fifo_get(); }
    Promedio=Promedio/SAMPLES;

    // Limpio el FIFO
    adc_fifo_drain();

    //Convierto el valor del ADC a Temperatura
    float T = 27 - (Promedio * conversion_factor - 0.706)/0.001721;


        printf("Valor del ADC: %f\n", Promedio);
        printf("Valor de TENSION: %f\n", Promedio * conversion_factor);
        printf("Valor de TEMPERATURA: %f\n", T);



}

void task_init(void *params) {


    // Inicializo el ADC
    adc_init();
    // Inicializo el Sensor de Temperatura
    adc_set_temp_sensor_enabled(true);
    //Selecciono el pin de entrada qes el 4
    adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);

    // Inicializo la interrupcion del ADC y la cantidad de lecturas necesarias
    adc_fifo_setup(true, false, SAMPLES, false, false);
    adc_irq_set_enabled(true);
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_run(true);

    // Elimina la tarea para liberar recursos
    vTaskDelete(NULL);
}

void task_ADC(void *params) {

    while(1) {

        // Habilito la interrupcion del ADC nuevamente y corro las conversiones
        adc_irq_set_enabled(true);
        adc_run(true);


        // Bloqueo tarea para no saturar
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/*void task_print(void *params) {
    
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
*/

int main()
{
    stdio_init_all();
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

   xTaskCreate(task_init, "Crear_Cola", configMINIMAL_STACK_SIZE*2, NULL, 2, NULL);
   xTaskCreate(task_ADC, "Cargar_Cola", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);
   //xTaskCreate(task_print, "Imprimir_Cola", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);


   vTaskStartScheduler();
   while(1);

}





