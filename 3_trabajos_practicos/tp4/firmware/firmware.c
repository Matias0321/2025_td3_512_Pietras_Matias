#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "lcd.h"
#include "bmp280.h"

// ======== Definiciones ========
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define ADDR_LCD 0x27

#define BTN_PIN     15     // Pulsador
#define LED_PWM_PIN 16     // LED controlado por PWM
#define TEMP_REF    25.0f  // Setpoint de temperatura

// ======== Estructuras y colas ========
typedef struct {
    int entero;
    float decimal;
} Datos;

QueueHandle_t cola;       // Para enviar datos del sensor a LCD
QueueHandle_t colaError;  // Para enviar error a PWM
SemaphoreHandle_t sem_btn;

// ======== Variables globales ========
volatile int pantalla = 0; // 0 = Temp+Presion, 1 = Setpoint+Error

// ======== Inicialización de colas ========
void initCola(void) {
    cola = xQueueCreate(10, sizeof(Datos));
    if (cola == NULL) {
        // Manejo de error
        while(1);
    }
}

// ======== ISR del botón ========
void btn_isr(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(sem_btn, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void initButton(void) {
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, false);
    gpio_pull_up(BTN_PIN);
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_isr);
}

// ======== Inicialización PWM ========
void initPWM(void) {
    gpio_set_function(LED_PWM_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_PWM_PIN);
    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
}

// ======== Tarea botón ========
void task_Button(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(sem_btn, portMAX_DELAY) == pdTRUE) {
            pantalla = !pantalla;
        }
    }
}

// ======== Tarea LCD ========
void task_LCD(void *pvParameters) {
    lcd_init(I2C_PORT, ADDR_LCD);
    char str[16];
    Datos recibido;

    while (1) {
        xQueueReceive(cola, &recibido, portMAX_DELAY);

        lcd_clear();
        if (pantalla == 0) {
            lcd_set_cursor(0, 0);
            sprintf(str, "Temp: %.2f C", recibido.decimal);
            lcd_string(str);

            lcd_set_cursor(1, 0);
            sprintf(str, "Presion: %d Pa", recibido.entero);
            lcd_string(str);
        } else {
            float error = TEMP_REF - recibido.decimal;

            lcd_set_cursor(0, 0);
            sprintf(str, "Ref: %.1f C", TEMP_REF);
            lcd_string(str);

            lcd_set_cursor(1, 0);
            sprintf(str, "Error: %.2f", error);
            lcd_string(str);

            xQueueSend(colaError, &error, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ======== Tarea PWM ========
void task_PWM(void *pvParameters) {
    float error;
    while (1) {
        if (xQueueReceive(colaError, &error, portMAX_DELAY) == pdTRUE) {
            float abs_err = fabs(error);
            if (abs_err == 0) {
                pwm_set_gpio_level(LED_PWM_PIN, 65535); // Máximo brillo
            } else {
                float nivel = 65535 * (1.0f / (1.0f + abs_err));
                pwm_set_gpio_level(LED_PWM_PIN, (uint16_t)nivel);
            }
        }
    }
}

// ======== Tarea BMP280 ========
void task_BMP280(void *pvParameters) {
    bmp280_init(i2c0);
    while (1) {
        struct bmp280_calib_param params;
        bmp280_get_calib_params(&params);

        int32_t raw_temperature, raw_pressure;
        bmp280_read_raw(&raw_temperature, &raw_pressure);

        float temperature = bmp280_convert_temp(raw_temperature, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);

        Datos paquete;
        paquete.entero = pressure;
        paquete.decimal = temperature;

        xQueueSend(cola, &paquete, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ======== Main ========
int main() {
    stdio_init_all();

    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    initCola();
    colaError = xQueueCreate(10, sizeof(float));
    sem_btn = xSemaphoreCreateBinary();

    initButton();
    initPWM();

    xTaskCreate(task_LCD, "LCD", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);
    xTaskCreate(task_BMP280, "BMP280", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);
    xTaskCreate(task_Button, "Boton", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(task_PWM, "PWM_LED", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}