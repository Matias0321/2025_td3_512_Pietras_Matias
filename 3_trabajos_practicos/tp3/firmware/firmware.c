#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lcd.h"
#include "helper.h"




// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define LCD_ADDR 0x27

#define MAX_COUNTING 4096
#define GPIO_PULSE 15
#define GPIO_PWM 12

#define SAMPLE_MS 250




SemaphoreHandle_t semaphore_count;

// Tarea que detecta flancos ascendentes
void Detector(void *params) {
    while (1) {
        if (gpio_get(GPIO_PULSE)) {
            xSemaphoreGive(semaphore_count);
            while (gpio_get(GPIO_PULSE));  // Espera al flanco descendente
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // retardo por si hay rebotes
    }
}



//Contador de pulsos
void Contador(void *params) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLE_MS));

        // Cantidad de pulsos
        uint32_t Pulsos = uxSemaphoreGetCount(semaphore_count);

        // Calculo frecuencia
        float Frecuencia = (float)Pulsos * (1000.0f / SAMPLE_MS);

        // Imprimo
        printf("Frecuencia: %.2f Hz\n", Frecuencia);

        // Reinicio semaforo
        xQueueReset(semaphore_count);
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

    gpio_init(GPIO_PULSE);
    gpio_set_dir(GPIO_PULSE, GPIO_IN);
    gpio_pull_down(GPIO_PULSE);


    
    //Inicializo el semaforo
    semaphore_count = xSemaphoreCreateCounting(MAX_COUNTING, 0);
    xTaskCreate(Contador,"Contador", configMINIMAL_STACK_SIZE*2, NULL,tskIDLE_PRIORITY + 1,NULL);
    xTaskCreate(Detector, "Detector", configMINIMAL_STACK_SIZE*2, NULL, tskIDLE_PRIORITY + 2, NULL);

    
    pwm_user_init(11, 150);
    pwm_user_init(GPIO_PWM, 250);


    //Scheduler
    vTaskStartScheduler();
    while (true);
}
