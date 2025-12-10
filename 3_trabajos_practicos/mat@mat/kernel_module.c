#include <linux/module.h>
#include <linux/init.h>
#include "gpio_driver.h"

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

// Etiqueta para el autor del modulo
#define AUTHOR	"Matias"

//#define PIN_LED 17

// Puntero para primer hilo
static struct task_struct *thread1;
// Puntero para segundo hilo
static struct task_struct *thread2;

/**
 * @brief Tarea para imprimir un mensaje periodicamente en el kernel
 * @param params puntero a parametros
*/
static int thread1_f(void *params) {
    // Corre mientras no haya otros procesos que lo detengan
    while(!kthread_should_stop()) {
        // Mensaje para el Kernel
        printk(KERN_INFO "%s: Led_ON\n", AUTHOR);
		gpio_set(17);
		// Demora de 0.5 segundo
		msleep(500);
    }
    return 0;
}

/**
 * @brief Tarea para imprimir un mensaje periodicamente en el kernel
 * @param params puntero a parametros
*/
static int thread2_f(void *params) {
	msleep(250);
    // Corre mientras no haya otros procesos que lo detengan
    while(!kthread_should_stop()) {
        // Mensaje para el Kernel
        printk(KERN_INFO "%s: Led_OFF\n", AUTHOR);
        gpio_clr(17);
		// Demora de 0.5 segundos
        msleep(500);
    }
    return 0;
}

/**
 * @brief Inicializa el modulo cuando se carga en el kernel
 * @return devuelve cero si salio bien
*/
static int __init kernel_thread_init(void) {

	gpio_map();
	gpio_set_dir_output(17);


    // Mensaje para el Kernel
	printk(KERN_INFO "%s: Insertando el modulo de kernel\n", AUTHOR);
    // Intento crear y correr el hilo
	thread1 = kthread_run(
        thread1_f,  // Callback
        NULL,       // Sin datos
        "thread1"   // Nombre del hilo
    );
    // Verifico si hubo error al crearlo
    if (IS_ERR(thread1)) {
        printk(KERN_ERR "%s: Error al crear thread 1\n", AUTHOR);
        return -1;
    }
    // Intento crear y correr el hilo
    thread2 = kthread_run(
        thread2_f,  // Callback
        NULL,       // Sin datos
        "thread2"   // Nombre del hilo
    );
    // Verifico si hubo error al crearlo
    if (IS_ERR(thread2)) {
        printk(KERN_ERR "%s: Error al crear thread 2\n", AUTHOR);
        // Elimino el hilo anterior
        kthread_stop(thread1);
        return -1;
    }
	return 0;
}

/**
 * @brief Llamado cuando se remueve el modulo del kernel
*/
static void __exit kernel_thread_exit(void) {
    // Mensaje para el Kernel
	pr_info("%s: Removiendo el modulo de kernel\n", AUTHOR);
    // Si se habia podido crear el hilo
	if (thread1) {
        // Detengo el hilo
        kthread_stop(thread1);
    }
    // Si se habia podido crear el hilo
    if (thread2) {
        // Detengo el hilo
        kthread_stop(thread2);
    }
}

// Registro la funcion de inicializacion y salida
module_init(kernel_thread_init);
module_exit(kernel_thread_exit);


// Informacion del modulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION("UTN FRA Tecnicas Digitales III - TP5: GPOS");
