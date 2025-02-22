# ESP32-S3 NeoPixel Color Gradient Project 🌈

This project creates a smooth color gradient on the ESP32-S3's built-in NeoPixel (WS2812). It cycles through a spectrum of colors while printing real-time color data to the serial monitor.

## What's Inside?

- **Microcontroller**: ESP32-S3-DevKitC-1 (ESP32-WROOM-S3)
- **LED**: Built-in NeoPixel (WS2812) on GPIO48
- **Power**: USB-powered (5V)
- **IDE**: ESP-IDF with VSCode

---

## Credits

This project is based on the **RMT Transmit Example — LED Strip** from Espressif's official ESP-IDF examples. The original example demonstrates how to drive WS2812 LED strips using the RMT peripheral and a custom encoder.

- **Original Example**: [RMT Transmit Example — LED Strip](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip)
- **Supported Targets**: ESP32, ESP32-C3, ESP32-C6, ESP32-H2, ESP32-P4, ESP32-S2, ESP32-S3

This project extends the original example to:

- Add **real-time serial feedback** for debugging.
- Implement a **smooth color gradient** using HSV-to-RGB conversion.
- Remove dependencies on `LED_BUILTIN` for better compatibility with NeoPixels.

Special thanks to the following AI tools for their assistance in debugging, coding, and writing this README:

- **DeepSeek**
- **ChatGPT**
- **GitHub Copilot**
- **Phind**
- **Claude AI**

---

## JTAG vs. UART: Why UART Was Used

### The Problem with JTAG

Initially, JTAG was attempted for debugging and flashing. However:

- **OpenOCD Issues**: OpenOCD, required for JTAG, failed to connect to the ESP32-S3 on Windows despite following instructions and reinstalling drivers.
- **Known Issue**: This is a well-documented problem with the ESP32-S3-DevKitC-1 board on Windows.

### The Solution: UART

UART was used instead because:

- **Simplicity**: UART works out-of-the-box with most ESP32 boards.
- **Reliability**: No additional drivers or tools are required.
- **Cross-Platform**: UART works consistently across Windows, macOS, and Linux.

### How to Use UART

1. Connect your ESP32-S3 via USB.
2. Use the `idf.py -p COMX flash monitor` command to flash and monitor the board.
3. Replace `COMX` with your actual serial port (e.g., `COM8` on Windows).

---

## The Problem: Why This Was Tricky

### 1. NeoPixels Are Fussy

NeoPixels (WS2812 LEDs) aren't your average LEDs. They:

- Use a **single wire** for data.
- Require **nanosecond-level timing** (800ns pulses!).
- Speak a **custom protocol** that most microcontrollers can't handle without help.

### 2. PlatformIO and `LED_BUILTIN`

PlatformIO's `LED_BUILTIN` macro is designed for standard LEDs, not NeoPixels. This caused confusion because:

- `LED_BUILTIN` assumes you're using a regular GPIO pin.
- NeoPixels need **special timing** that GPIOs can't provide.

### 3. ESP32-S3's RMT Peripheral

To control NeoPixels, the **RMT peripheral** (Remote Control Transceiver) is required. It's designed for precise signal generation, but:

- The documentation is dense.
- The legacy RMT driver caused errors.
- The modern `rmt_tx.h` driver was used instead.

---

## The Solution: How It Was Fixed

### 1. Avoiding `LED_BUILTIN`

The project stopped using PlatformIO's `LED_BUILTIN` and switched to ESP-IDF for better control. This allowed:

- Direct access to the RMT peripheral.
- Explicit configuration of GPIO48 for the NeoPixel.

### 2. RMT Configuration

The RMT peripheral was set up to generate the precise signals NeoPixels need:

- **10MHz resolution**: Each "tick" is 100ns, perfect for NeoPixel timing.
- **64 memory blocks**: Reduces flickering by storing more data.

### 3. Smooth Color Gradient

Instead of just cycling through red, green, and blue, **HSV-to-RGB conversion** was implemented. This allows:

- Smooth color transitions.
- Independent control of hue, saturation, and brightness.

### 4. Real-Time Feedback

Serial logging was added to the ESP32's monitor, so you can see:

- The current **hue angle** (0-360°).
- The **GRB values** being sent to the NeoPixel.

---

## Understanding GRB Color Order

### Why GRB?

NeoPixels use a **GRB color order** instead of the more common RGB. This means:

- The first byte controls **Green**.
- The second byte controls **Red**.
- The third byte controls **Blue**.

### Example

To set the NeoPixel to **pure red**:

- Green = 0
- Red = 255
- Blue = 0

In code:

```c
uint8_t grb_data[3] = {0, 255, 0};  // [Green, Red, Blue]
```

### Why Does This Matter?

- If you use RGB order (e.g., `{255, 0, 0}`), the NeoPixel will show **green** instead of red.
- This quirk is specific to WS2812 LEDs and can trip up beginners.

---

## Key Features

- 🌈 **Color Gradient**: Smooth transitions through all colors.
- 📺 **Serial Feedback**: See color data in real-time.
- ⚙️ **Configurable**: Adjust speed and smoothness.
- 🎨 **Accurate Colors**: Uses HSV for better color control.

---

## How It Works

### The Code

Here's the heart of the project:

```c
// Convert HSV to GRB (NeoPixel format)
void hsv_to_grb(uint16_t h, uint8_t s, uint8_t v, uint8_t *grb) {
    // Magic math to convert HSV to RGB
    // ... (see full code for details)
}

// Main loop: Cycle through colors
while(1) {
    hsv_to_grb(hue, 100, 100, grb_data);  // Generate color
    rmt_transmit(led_chan, led_encoder, grb_data, sizeof(grb_data), &tx_config);  // Send to NeoPixel
    hue = (hue + HUE_STEP) % 360;  // Update hue
    vTaskDelay(pdMS_TO_TICKS(GRADIENT_SPEED));  // Wait for next step
}
```

### Serial Output Example

```c
I (1254) NeoPixel: Hue: 0° | GRB: [  0, 255,   0]  // Red
I (1354) NeoPixel: Hue: 10° | GRB: [ 42, 255,   0]  // Orange
I (1454) NeoPixel: Hue: 20° | GRB: [ 85, 255,   0]  // Yellow
...
I (7854) NeoPixel: Hue: 350° | GRB: [255,   0,  85]  // Purple
```

---

## Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/esp32-neopixel-gradient.git
cd esp32-neopixel-gradient
```

### 2. Set Up ESP-IDF

```bash
. $HOME/esp/esp-idf/export.sh
```

### 3. Build and Flash

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COM8 flash monitor
```

---

## Customization

### Adjust the Gradient

- **Speed**: Change `GRADIENT_SPEED` (lower = faster).
- **Smoothness**: Adjust `HUE_STEP` (smaller = smoother).

### Gradient Adjustment Example

```c
#define GRADIENT_SPEED 15  // Faster animation
#define HUE_STEP 1         // Smoother transitions
```

---

## Lessons Learned

### 1. NeoPixels Are Special

- They need precise timing.
- Standard GPIO control won't work.

### 2. Debugging Is Key

- Serial logging saved hours of frustration.
- Breaking the problem into smaller steps made it manageable.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
