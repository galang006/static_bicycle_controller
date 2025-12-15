# Static Bicycle Speed Controller

This project implements a system that measures static bicycle speed using an ESP32 microcontroller and transmits it via Bluetooth. A companion Python script then receives this speed data and translates it into keyboard commands, allowing the physical bicycle's speed to control an application or game (e.g., simulating movement with 'W' and 'Shift+W').

## Features
-   **ESP32-based Speed Measurement**: Accurately measures bicycle wheel rotation speed using an interrupt-driven sensor connected to an ESP32 microcontroller.
-   **Bluetooth Serial Communication**: Transmits real-time speed data (in km/h) from the ESP32 to a connected device via Bluetooth Serial Profile (SPP).
-   **Robust Interrupt Handling**: Employs a safe Interrupt Service Routine (ISR) to capture pulse events, ensuring minimal processing time, debouncing, and system stability.
-   **Speed Smoothing**: Implements a simple exponential moving average to provide stable and smooth speed readings.
-   **Python Data Receiver**: A Python script connects to the ESP32's Bluetooth serial port and parses incoming JSON-formatted speed data.
-   **Keyboard Command Mapping**: Translates received speed values into simulated keyboard presses (`W` for moving forward, `Shift+W` for running, no keys pressed for stopping), enabling physical interaction with software.

## Installation

This project consists of two main parts: the ESP32 firmware and the Python control script.

### ESP32 Firmware (`bicycle_reader/bicycle_reader.ino`)

1.  **Arduino IDE / PlatformIO**: Ensure you have the Arduino IDE installed with ESP32 board support, or PlatformIO configured for ESP32 development.
2.  **BluetoothSerial Library**: Install the `BluetoothSerial` library.
    *   **Arduino IDE**: Go to `Sketch > Include Library > Manage Libraries...` and search for "BluetoothSerial". Install the library by Esprssif.
3.  **Hardware Setup**:
    *   Connect your speed sensor (e.g., a Hall effect sensor or a reed switch) to **GPIO pin 25** on your ESP32. Ensure proper wiring, including power and ground.
4.  **Upload Sketch**:
    *   Open `bicycle_reader/bicycle_reader.ino` in your Arduino IDE.
    *   Select your ESP32 board (`ESP32 Dev Module` or similar) and the correct COM port for your ESP32.
    *   Click the "Upload" button to compile and flash the firmware to your ESP32 microcontroller.

### Python Script (`main.py`)

1.  **Python Installation**: Ensure you have Python 3 installed on your computer.
2.  **Install Dependencies**: Open your terminal or command prompt and install the required Python libraries using pip:
    ```bash
    pip install pyserial keyboard
    ```
3.  **Bluetooth Pairing**:
    *   On your computer, search for Bluetooth devices and pair with the ESP32. The ESP32 will broadcast itself as **"ESP32_Speedometer"**.
4.  **Identify COM Port**:
    *   After successful pairing, your operating system will assign a virtual COM port to the ESP32's Bluetooth Serial Port Profile (SPP).
    *   **Windows**: Check `Device Manager > Ports (COM & LPT)` for a port named "Standard Serial over Bluetooth link (COMx)".
    *   **macOS**: Look in `/dev/tty.Bluetooth-Incoming-Port` or similar.
    *   **Linux**: Often `/dev/rfcommX` (you might need to set up `rfcomm` binding).
5.  **Configure `main.py`**:
    *   Open `main.py` in a text editor.
    *   Modify the `PORT` variable to match the COM port identified in the previous step:
        ```python
        PORT = "COM9" # <<< Change this to your ESP32's Bluetooth COM port
        ```

## Usage

1.  **Power On ESP32**: Ensure your ESP32 with the `bicycle_reader` firmware is powered on and running. It will automatically start broadcasting speed data via Bluetooth.
2.  **Run Python Script**: Open your terminal or command prompt, navigate to the directory where `main.py` is located, and run the script:
    ```bash
    python main.py
    ```
    The script will connect to the ESP32 and start displaying the current speed and corresponding action.
3.  **Control Interaction**:
    *   **Stop/Idle (Speed `0 km/h`)**: If no pulses are detected for `TIMEOUT_MS` (2 seconds), the ESP32 reports `0 km/h`. The Python script will release both `W` and `Shift` keys.
    *   **Move Forward (Speed `0.1` to `25 km/h`)**: When the bicycle speed is detected between `0.1` and `25 km/h`, the script will press the `W` key and ensure `Shift` is released.
    *   **Run (Speed `> 25 km/h`)**: When the bicycle speed exceeds `25 km/h`, the script will press both the `W` and `Shift` keys simultaneously.
4.  **Exit**: To stop the Python script, press `Ctrl+C` in the terminal. The script includes a graceful exit that ensures all keyboard keys (`W` and `Shift`) are released before terminating.

## Code Structure

The project is logically divided into two main components: the ESP32 microcontroller firmware for hardware interaction and the Python script for computer-side control.

### `bicycle_reader/`

This directory contains the Arduino sketch for the ESP32 microcontroller.

*   **`bicycle_reader/bicycle_reader.ino`**:
    *   **Includes**: `BluetoothSerial.h` is included to enable Bluetooth Serial Port Profile (SPP) communication.
    *   **Global Variables**:
        *   `volatile` variables (`lastPulseMicros`, `currentPulseDelta`, `newPulseAvailable`) are crucial for safely transferring raw timing data from the Interrupt Service Routine (ISR) to the main loop, minimizing processing within the ISR itself.
        *   Constants define parameters like `CIRCUM_M` (wheel circumference in meters), `DEBOUNCE_US` (debounce time for sensor pulses), `MAX_SPEED` (maximum allowed speed), and `TIMEOUT_MS` (time after which speed is considered zero).
        *   Non-volatile variables (`currentSpeed`, `smoothSpeed`, `lastPulseMillis`) are used in the main loop for calculations and state management.
    *   **`pulseISR()`**:
        *   An `IRAM_ATTR` function, meaning it's stored in Interrupt RAM for faster execution.
        *   Triggered by a `FALLING` edge on GPIO 25.
        *   Performs minimal processing: records the current microsecond timestamp, calculates the delta from the last pulse, applies simple debouncing, and sets `newPulseAvailable` to `true`. No floating-point math is done here to ensure ISR efficiency and safety.
    *   **`setup()`**:
        *   Initializes `Serial` communication at 115200 baud for debugging.
        *   Sets up GPIO 25 as an `INPUT_PULLUP`.
        *   Initializes `SerialBT` with the name "ESP32_Speedometer".
        *   Attaches `pulseISR` to the interrupt on GPIO 25.
    *   **`loop()`**:
        *   **ISR Data Handling**: Safely reads `volatile` variables from the ISR using `noInterrupts()` and `interrupts()` to prevent data corruption during simultaneous access.
        *   **Speed Calculation**: If a new pulse is available, it calculates the speed in meters per second (`mps`) and then converts it to kilometers per hour (`newSpeed`).
        *   **Validation and Timeout**: Applies `MAX_SPEED` limit and handles the case where no pulses are received for `TIMEOUT_MS` by setting `currentSpeed` to 0.0.
        *   **Speed Smoothing**: Uses a simple exponential moving average (`smoothSpeed = 0.7 * smoothSpeed + 0.3 * currentSpeed`) to provide more stable speed readings.
        *   **Output**: Every 500 milliseconds, it formats the `smoothSpeed` into a JSON string (`{"speed":X.XX}`) and sends it via both `Serial` (for debugging) and `SerialBT` (for the Python script).
        *   Includes a small `delay(10)` to prevent watchdog timer issues.

### `main.py`

This Python script runs on a computer, connects to the ESP32 via Bluetooth, and translates speed data into keyboard commands.

*   **Imports**:
    *   `serial`: For establishing and managing the serial connection to the Bluetooth COM port.
    *   `json`: For parsing the JSON formatted speed data received from the ESP32.
    *   `keyboard`: For simulating key presses and releases (`w`, `shift`).
    *   `os`: Used for clearing the console (`os.system('cls')`).
*   **`PORT` and `BAUDRATE`**:
    *   `PORT`: A string variable that *must be updated by the user* to match the virtual COM port assigned to the ESP32's Bluetooth connection on the host computer.
    *   `BAUDRATE`: Set to 115200, matching the ESP32's Bluetooth serial baud rate.
*   **`get_speed_data()`**:
    *   Initializes a `serial.Serial` object, connecting to the specified `PORT` with a timeout.
    *   Enters an infinite loop to continuously read incoming data.
    *   Reads a line from the serial port, decodes it as UTF-8, and strips whitespace.
    *   Attempts to parse the line as a JSON object. If successful, it extracts and returns the `speed` value.
    *   Includes error handling for `json.JSONDecodeError` to gracefully handle malformed or incomplete data.
*   **`move_forward()`**:
    *   Calls `get_speed_data()` to obtain the current bicycle speed.
    *   Clears the console using `os.system('cls')` for a cleaner output.
    *   Implements conditional logic to translate speed into keyboard actions:
        *   If `0 < speed <= 25`: Presses the `w` key and ensures the `shift` key is released.
        *   If `speed > 25`: Presses both the `w` and `shift` keys.
        *   If `speed <= 0`: Releases both the `w` and `shift` keys.
*   **Main Execution Block (`if __name__ == "__main__":`)**:
    *   This block ensures the `move_forward()` function is called repeatedly in an infinite loop when the script is executed directly.
    *   A `try-except KeyboardInterrupt` block is used for graceful termination. If the user presses `Ctrl+C`, the script prints an exit message and ensures that both `w` and `shift` keys are released before exiting, preventing unintended key presses after the script stops.
