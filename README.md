# Arduino RGB LED Controller with Multiple Modes

This project is an RGB LED controller built with an **Arduino Pro Micro**, a **13-LED WS2812B strip**, and **three push buttons** for user interaction. The controller supports multiple lighting modes, brightness levels, and color control, with persistent settings stored in EEPROM.

## Features

- 🌈 **Lighting Modes**:
  - `0`: Static white lamp
  - `1`: Rainbow animation
  - `2`: Manual color selection (with hue adjustment)
  - `3`: Police strobe (red & blue flashing)
  - `4`: Wave gradient animation
  - `5`: Single pixel chase animation
  - `6`: Waterfall cascade (fading drops)

- 🔘 **Button Control**:
  - `MODE` button: cycles through lighting modes
  - `BRIGHTNESS` button:
    - In all modes except ManualColor: decreases brightness
    - In ManualColor mode: increases hue
  - `COLOR` button:
    - In all modes except ManualColor: increases brightness
    - In ManualColor mode: decreases hue

- 💾 **EEPROM Saving**:
  - Mode, brightness level, and hue are saved and restored on power cycle

- 🌟 **Smooth Brightness Transitions**:
  - Smooth fade animation when changing brightness levels

## Hardware

- Arduino Pro Micro
- WS2812B LED strip (13 LEDs)
- 3 momentary push buttons
- Power supply (ensure enough current for the LED strip)

## Pin Configuration

| Pin         | Function         |
|-------------|------------------|
| `8`         | LED strip signal |
| `A0`        | Mode button      |
| `A1`        | Brightness button|
| `A2`        | Color button     |

## Libraries Used

- [`Adafruit_NeoPixel`](https://github.com/adafruit/Adafruit_NeoPixel)
- [`Bounce2`](https://github.com/thomasfredericks/Bounce2)
- `EEPROM` (built-in)

## Installation

1. Install required libraries via Library Manager or GitHub.
2. Upload the code to your Arduino Pro Micro.
3. Wire up the buttons and LEDs as described above.
4. Power the system — your light show is ready to go!

## Future Improvements

- Long-press to enter setup/config mode
- Custom mode creation via serial or button combinations
- Support for other LED types (e.g., APA102)
- Add support for color temperature presets in Lamp mode

---

### License

MIT License — feel free to use and modify!

### Author

Created with ❤️ by Genius.
