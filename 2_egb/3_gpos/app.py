import os, re, sys, termios, time

DEV_PATH = "/dev/lux_control" # Definimos ubicacion del archivo

set_keys = {

    "set_point": "set_point",
    "set_point_f": "set_point_f",
    "rise_time": "rise_time",
    "min": "min",
    "max": "max",
    "alpha": "alpha",
    "pid_kp": "pid_kp",
    "pid_ki": "pid_ki",
    "pid_kd": "pid_kd",
    "calib": "calib",
    "kalman_R": "kalman_R",
    "kalman_Q": "kalman_Q",
    "rtc": "rtc",

    "hour": "hour",
    "minute": "minute",
    "second": "second",
    "day": "day",
    "month": "month",
    "year": "year",
}

get_keys = {

    "User": "user_params",
    "Medicion BH1750": "bh1750",
    "logs": "logs",
    "user_params": "user_params",
    "pid_params": "pid_params",
    "filter_params": "filter_params",
    "pwm": "pwm",
    "bh1750": "bh1750",
    "temt6000": "temt6000",

    "set_point": "set_point",
    "set_point_f": "set_point_f",
    "rise_time": "rise_time",
    "min": "min",
    "max": "max",
    "alpha": "alpha",
    "pid_kp": "pid_kp",
    "pid_ki": "pid_ki",
    "pid_kd": "pid_kd",
    "calib": "calib",
    "kalman_R": "kalman_R",
    "kalman_Q": "kalman_Q",
    "rtc": "rtc",
}

def main():
    while True:
        match menu_principal():
            case "1":
                menu_set()
            case "2":
                menu_get()
            case "3":
                rta = enviar_uart("continue")
            case "4":
                rta = enviar_uart("stop")
            case "5":
                print("-------------------------------------------------")
                print("Está seguro:")
                print("-------------------------------------------------")
                print("1> Si")
                print("2> No")
                print("-------------------------------------------------")
                if input("Opción [1-2]: ").strip() == "1":
                    os.system("clear")
                    break
            case _:
                print("\nOpción invalida")
                flush_stdin()
                input("Presione tecla para continuar...")

def menu_principal():
    os.system("clear") # Limpiamos la consola
    print("-------------------------------------------------")
    print("-------------{ Aplicación UART EGB }-------------")
    print("-------------------------------------------------")
    print("Ingrese accion a realizar:")
    print("-------------------------------------------------")
    print("1> Enviar configuración")
    print("2> Obtener información")
    print("3> Enviar datos en tiempo real")
    print("4> Frenar enviado de datos en tiempo real")
    print("5> Salir")
    print("-------------------------------------------------")
    flush_stdin()
    return input("Opción [1-5]: ").strip()

def menu_set():
    print("-------------------------------------------------")
    print("Seleccione variable a configurar:")
    print("-------------------------------------------------")
    for i, key in enumerate(set_keys.keys(), start=1):
        print(f"{i}> {key}")
    print(f"0> Volver al menu anterior")
    print("-------------------------------------------------")
    flush_stdin()

    opc = input(f"Opción [0-{len(set_keys)}]: ").strip()
    try:
        msg = "set " + set_keys[list(set_keys.keys())[int(opc)-1]]
    except KeyError:
        print("\nOpción invalida")
        flush_stdin()
        return
    
    print("-------------------------------------------------")
    flush_stdin()
    enviar_uart(msg + " " + input("Ingrese el valor objetivo: ").strip())

def menu_get():
    print("-------------------------------------------------")
    print("Seleccione variable a consultar:")
    print("-------------------------------------------------")

    for i, key in enumerate(get_keys.keys(), start=1):
        print(f"{i}> {key}")
    print("0> Volver al menu anterior")
    print("-------------------------------------------------")
    flush_stdin()
    opc = input(f"Opción [0-{len(get_keys)}]: ").strip()
    try:
        msg = "get " + get_keys[list(get_keys.keys())[int(opc)-1]]
    except KeyError:
        print("\nOpción invalida")
        flush_stdin()
        return
    
    enviar_uart(msg)

def enviar_uart(msg):
    msg = msg.strip()
    print("-------------------------------------------------")
    print(f"Enviado por UART: {msg}")
    print("-------------------------------------------------")
    
    cmd = msg
    with open('/dev/lux_control', 'w') as f:    
        f.write("stop\n")
        time.sleep(0.1)
        f.write(cmd + '\n')
    time.sleep(0.5)
    with open('/dev/lux_control', 'r') as f:
        r = f.read()
    with open('/dev/lux_control', 'w') as f:    
        f.write("continue\n")
    resp = r
    print(f"Respuesta: {resp}")
    print("-------------------------------------------------")
    print("Presione tecla para continuar...")
    input()
    return resp

def flush_stdin():
    termios.tcflush(sys.stdin, termios.TCIFLUSH)

if __name__ == "__main__":
    if not os.path.exists(DEV_PATH):
        print(f"Dispositivo {DEV_PATH} no encontrado.")
    else:
        main()
        # enviar_uart("set set_point 700")
        # with open(DEV_PATH, "r") as dev:
        #     resp = dev.read().strip()
        # print(f"Respuesta: {resp}")