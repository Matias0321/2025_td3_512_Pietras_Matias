#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"  

#include "lcd.h"
#include "bmp280.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define ADDR_LCD        0x27

QueueHandle_t cola;

typedef struct {
    int entero;
    float decimal;
                    } Datos;

void initCola(void) {
    cola = xQueueCreate(10, sizeof(Datos)); // 10 elementos
    if (cola == NULL) {
        // Manejar error: no se pudo crear la cola
    }
}

void task_LCD(void *pvParameters) {

    lcd_init(I2C_PORT, ADDR_LCD);
    
    char str[16];

    Datos recibido;

    while(1) {

        xQueueReceive(cola, &recibido, portMAX_DELAY);

        lcd_clear();
        // Muevo el cursor al comienzo de la primera fila
        lcd_set_cursor(0, 0);
        sprintf(str, "Temp: %f C", recibido.decimal);
        // Imprimo el mensaje
        lcd_string(str);

        lcd_set_cursor(1, 0);
        sprintf(str, "Pressure: %d Pa\n", recibido.entero);
        lcd_string(str);
        

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}



void task_BMP280(void *pvParameters) {

        // Inicializa el BMP280 usando el I2C0
        bmp280_init(i2c0);

    
    while(1) {
        
        // Obtiene parámetros de compensación
        struct bmp280_calib_param params;
        bmp280_get_calib_params(&params);

        // Obtiene valores sin compensar
        int32_t raw_temperature, raw_pressure;
        bmp280_read_raw(&raw_temperature, &raw_pressure);

        // Obtiene los valores compensados de temperatura y presión
        float temperature = bmp280_convert_temp(raw_temperature, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);

        Datos paquete;
                paquete.entero = pressure;
                paquete.decimal = temperature;

        xQueueSend(cola, &paquete, portMAX_DELAY);

        printf("Temperature: %.2f C\n", temperature);   
        printf("Pressure: %ld Pa\n", pressure);
        
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    initCola();

    xTaskCreate(task_LCD, "Imprime_LCD", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);
    xTaskCreate(task_BMP280, "Datos_BMP280", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);

    vTaskStartScheduler();
    while(1);

}

/*
        int x = 63;
        char str[16];
        sleep_ms(500);

        // Imprimo por Monitor Serie
        printf("a: %d C\n", x);

        lcd_init(I2C_PORT, ADDR);

        sprintf(str, "a: %d C", x);
        // Muevo el cursor al comienzo de la primera fila
        lcd_set_cursor(0, 0);
        // Imprimo el mensaje
        lcd_string(str);
        sleep_ms(500);
        lcd_clear();

        // Inicializa el BMP280 usando el I2C0
        bmp280_init(i2c0);

        // Obtiene parámetros de compensación
        struct bmp280_calib_param params;
        bmp280_get_calib_params(&params);

        // Obtiene valores sin compensar
        int32_t raw_temperature, raw_pressure;
        bmp280_read_raw(&raw_temperature, &raw_pressure);

        // Obtiene los valores compensados de temperatura y presión
        float temperature = bmp280_convert_temp(raw_temperature, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);

        printf("Temperature: %.2f C\n", temperature);   
        printf("Pressure: %ld Pa\n", pressure);
        */