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
#define I2C_SDA 16
#define I2C_SCL 17
#define LCD_ADDR 0x27

#define MAX_COUNTING 4096
#define GPIO_PULSE 15
#define GPIO_PWM 12

#define SAMPLE_MS 2000

SemaphoreHandle_t semaphore_count;
//Contador de pulsos que va por interrupciones
void Pulso(uint gpio, uint32_t event_mask) {
    BaseType_t to_higher_priority_task = false;
    xSemaphoreGiveFromISR(semaphore_count, &to_higher_priority_task);
    portYIELD_FROM_ISR(to_higher_priority_task);
}


//Contador de pulsos
void Conteo_LCD(void *params) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // Variable para imprimir el mensaje
    char str[16];

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLE_MS));

        // Cantidad de pulsos
        uint32_t Pulsos = uxSemaphoreGetCount(semaphore_count);

        // Calculo frecuencia
        float Frecuencia = (float)Pulsos * (1000.0f / SAMPLE_MS);

        // Imprimo por Monitor Serie
        printf("Frecuencia: %.2f Hz\n", Frecuencia);
        
        // Armo un string con la variable de contador y la incremento
        sprintf(str, "Frecuencia: %.2f Hz", Frecuencia);
        // Muevo el cursor al comienzo de la primera fila
        lcd_set_cursor(0, 0);
        // Imprimo el mensaje
        lcd_string(str);

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

    // Inicializacion de tareas
    xTaskCreate(Conteo_LCD,"Conteo_LCD",configMINIMAL_STACK_SIZE * 2, NULL,tskIDLE_PRIORITY + 1,NULL);

        //Defino interrupccion por flanco ascendente
    gpio_set_irq_enabled_with_callback(
        GPIO_PULSE,           // El número del pin GPIO a vigilar.
        GPIO_IRQ_EDGE_RISE,   // El tipo de evento: flanco de subida (LOW -> HIGH).
        true,                 // Habilita (true) la interrupción.
        Pulso        // Función que se llama cuando ocurre la interrupción.
    );

    pwm_user_init(GPIO_PWM, 250);
 

    // Inicializo el I2C con un clock de 100 KHz
    i2c_init(I2C_PORT, 100000);
    // Habilito la funcion de I2C en los GPIOs
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    // Habilito pull-ups
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // Inicializo LCD
    lcd_init(I2C_PORT, LCD_ADDR);
    // Limpio pantalla
    lcd_clear();



    /*
    Ejemplito de I2C

    char buffer[32];
    int temperatura = 25;
    float voltaje = 3.3;

    sprintf(buffer, "Temp: %dC V: %.2f", temperatura, voltaje);
    Después de esta línea, buffer contendrá: "Temp: 25C V: 3.30"

    lcd_string(buffer);   //Aca lo mande por I2C al display
    */




    //Scheduler
    vTaskStartScheduler();
    while (true);
}
