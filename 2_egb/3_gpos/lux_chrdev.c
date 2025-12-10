#include <linux/module.h>     // Módulos
#include <linux/init.h>       // Macros init/exit
#include <linux/fs.h>         // Tipos dev_t, cdev, file_operations

// 1. Variables Globales
dev_t mi_driver_dev_num;       // Almacena el Major/Minor asignado
struct cdev mi_driver_cdev;    // Estructura del dispositivo

// 2. Definición de Operaciones de Archivo (el comportamiento del driver)
// Por ahora, solo punteros nulos
static const struct file_operations mi_driver_fops = {
    .owner = THIS_MODULE,
    // .open = mi_driver_open,  // Se añadirán después
    // .read = mi_driver_read,
    // .write = mi_driver_write,
};

// --- FUNCIÓN DE INICIALIZACIÓN ---
static int __init mi_driver_init(void)
{
    int ret;

    // A. IDENTIDAD: Asignar un número Major dinámico
    ret = alloc_chrdev_region(&mi_driver_dev_num, 0, 1, "mi_chardev");
    if (ret < 0) {
        printk(KERN_ERR "Fallo en alloc_chrdev_region.\n");
        return ret;
    }
    
    // B. DISPOSITIVO: Inicializar la estructura cdev y añadirla al kernel
    cdev_init(&mi_driver_cdev, &mi_driver_fops);
    ret = cdev_add(&mi_driver_cdev, mi_driver_dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "Fallo en cdev_add. LIMPIANDO...\n");
        // ! AQUI DEBERÍA HABER UNA LIMPIEZA INTERMEDIA
        return ret;
    }

    printk(KERN_INFO "Driver cargado con Major: %d\n", MAJOR(mi_driver_dev_num));
    return 0;
}

// --- FUNCIÓN DE SALIDA (LIMPIEZA) ---
static void __exit mi_driver_exit(void)
{
    // Limpieza en orden inverso a la inicialización
    cdev_del(&mi_driver_cdev);                     // 1. Remover el cdev
    unregister_chrdev_region(mi_driver_dev_num, 1); // 2. Liberar el número Major
    printk(KERN_INFO "Driver descargado.\n");
}

module_init(mi_driver_init);
module_exit(mi_driver_exit);