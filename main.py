import serial
import json
import keyboard
import os

PORT = "COM9"  # COM port Bluetooth 
BAUDRATE = 115200

def get_speed_data():
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)

    #print("Reading speed data over Bluetooth...\n")
    while True:
        line = ser.readline().decode('utf-8').strip()
        if not line:
            continue
        try:
            data = json.loads(line)
            #print(f"Speed: {data['speed']:.2f} km/h")
            return data['speed']
        except json.JSONDecodeError:
            print(f"Raw line: {line}")

def move_forward():
    speed = get_speed_data()
    
    os.system('cls') 
    if 0 < speed <= 25:
        print(f"Speed {speed}: Move forward!")
        keyboard.press('w')
        keyboard.release('shift')
    elif speed > 25:
        print(f"Speed {speed}: Run!")
        keyboard.press('w')
        keyboard.press('shift')
    else:
        print(f"Speed {speed}: Stop moving.")
        keyboard.release('w')
        keyboard.release('shift')

    
if __name__ == "__main__":
    try: 
        while True:
            move_forward()
    except KeyboardInterrupt:
        print("Exiting...")
        keyboard.release('w')
        keyboard.release('shift')
