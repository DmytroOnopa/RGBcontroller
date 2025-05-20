#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#include <EEPROM.h>

#define LED_PIN     8
#define NUM_LEDS    13

#define BTN_MODE     A0
#define BTN_BRIGHTNESS A1
#define BTN_COLOR    A2

#define MODE_LAMP 0
#define MODE_RAINBOW 1
#define MODE_MANUAL 2
#define MODE_POLICE 3
#define MODE_WAVE 4
#define MODE_CHASE 5
#define MODE_CASCADE 6

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

Bounce btnMode = Bounce();
Bounce btnBright = Bounce();
Bounce btnColor = Bounce();

uint8_t mode = 0; // 0: Lamp, 1: Rainbow, 2: ManualColor, 3: Police
uint8_t brightnessLevel = 2;
const uint8_t brightnessValues[] = {40, 90, 140, 200, 255};

uint16_t hue = 0;
uint8_t animationStep = 0;

// Для поліції
uint8_t policePhase = 0;
uint8_t blinkCount = 0;
bool lightsOn = false;
unsigned long lastUpdate = 0;

void saveSettings() {
  EEPROM.update(0, mode);
  EEPROM.update(1, brightnessLevel);
  EEPROM.update(2, hue >> 8);
  EEPROM.update(3, hue & 0xFF);
}

void loadSettings() {
  mode = EEPROM.read(0);
  brightnessLevel = EEPROM.read(1);
  hue = (EEPROM.read(2) << 8) | EEPROM.read(3);
  if (brightnessLevel >= sizeof(brightnessValues)) brightnessLevel = 2;
}

void setup() {
  strip.begin();
  loadSettings();
  strip.setBrightness(brightnessValues[brightnessLevel]);
  strip.show();

  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_BRIGHTNESS, INPUT_PULLUP);
  pinMode(BTN_COLOR, INPUT_PULLUP);

  btnMode.attach(BTN_MODE);
  btnMode.interval(25);

  btnBright.attach(BTN_BRIGHTNESS);
  btnBright.interval(25);

  btnColor.attach(BTN_COLOR);
  btnColor.interval(25);
}

void loop() {
  btnMode.update();
  btnBright.update();
  btnColor.update();

  if (btnMode.fell()) {
    mode++;
    if (mode > 6) mode = 0;
    animationStep = 0;
    policePhase = 0;
    blinkCount = 0;
    lightsOn = false;
    saveSettings();
  }

// Зміна яскравості у всіх режимах, окрім ручної настройки кольору (mode != 2)
if (mode != 2) {
  if (btnBright.fell()) {
    if (brightnessLevel > 0) {
      fadeToBrightness(brightnessValues[brightnessLevel], brightnessValues[brightnessLevel - 1]);
      brightnessLevel--;
      saveSettings();
    }
  }
  if (btnColor.fell()) {
    if (brightnessLevel < sizeof(brightnessValues) - 1) {
      fadeToBrightness(brightnessValues[brightnessLevel], brightnessValues[brightnessLevel + 1]);
      brightnessLevel++;
      saveSettings();
    }
  }
}

  // У ручному режимі — A1/A2 змінюють колір
  if (mode == 2) {
    if (btnBright.fell()) {
      hue += 256 * 10;
      if (hue > 65535) hue = 0;
      saveSettings();
    }
    if (btnColor.fell()) {
      if (hue < 256 * 10) {
        hue = 65535;
      } else {
        hue -= 256 * 10;
      }
      saveSettings();
    }
  }

  switch (mode) {
    case 0: modeLamp(); break;
    case 1: animationRainbow(); break;
    case 2: modeManualColor(); break;
    case 3: animationPolice(); break;
    case 4: animationWave(); break;
    case 5: animationChase(); break;  // Нова анімація
    case 6: animationCascade(); break;
  }
}

void fadeToBrightness(uint8_t from, uint8_t to) {
  int step = (to > from) ? 5 : -5;
  for (int b = from; b != to; b += step) {
    strip.setBrightness(b);
    strip.show();
    delay(15);
  }
  strip.setBrightness(to);
  strip.show();
}

void modeLamp() {
  uint32_t color = strip.Color(255, 255, 255); // чисто біле світло
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void animationRainbow() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t pixelHue = hue + (i * 65536L / NUM_LEDS);
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
  strip.show();
  hue += 256;
  delay(30);
}

void modeManualColor() {
  uint32_t color = strip.ColorHSV(hue, 255, 255);
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void animationPolice() {
  unsigned long now = millis();
  if (now - lastUpdate < 100) return;
  lastUpdate = now;

  strip.clear();

  if (policePhase == 0) { // три ліво
    if (lightsOn) {
      for (int i = 0; i < NUM_LEDS / 2; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 255));
      }
    }
  } else { // три право
    if (lightsOn) {
      for (int i = NUM_LEDS / 2; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 0));
      }
    }
  }

  strip.show();
  lightsOn = !lightsOn;

  if (!lightsOn) {
    blinkCount++;
    if (blinkCount >= 3) {
      policePhase = (policePhase + 1) % 2;
      blinkCount = 0;
    }
  }
}

void animationWave() {
  static uint16_t baseHue = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t pixelHue = (baseHue + i * 100) % 65536;
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
  strip.show();
  baseHue += 256; // повільне переливання
}

void animationChase() {
  static int pos = 0;
  uint32_t color = strip.ColorHSV(hue, 255, 255);

  strip.clear();
  strip.setPixelColor(pos, color);
  strip.show();

  pos++;
  if (pos >= NUM_LEDS) pos = 0;  // Крутимось по колу

  delay(100);
}

void animationCascade() {
  static uint32_t pixels[NUM_LEDS];  // масив для збереження кольорів

  // Здвигаємо всі вниз
  for (int i = NUM_LEDS - 1; i > 0; i--) {
    pixels[i] = pixels[i - 1];
  }

  // На першому пікселі (верху) — іноді створюємо нову краплю
  if (random(0, 5) == 0) {
    pixels[0] = strip.ColorHSV(hue, 255, 255);
  } else {
    pixels[0] = 0; // без краплі
  }

  // Малюємо
  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t color = pixels[i];

    // Поступово зменшуємо яскравість для ефекту згасання
    uint8_t r = (uint8_t)(color >> 16) & 0xFF;
    uint8_t g = (uint8_t)(color >> 8) & 0xFF;
    uint8_t b = (uint8_t)(color) & 0xFF;

    // гаснемо трохи
    r = (r > 20) ? r - 20 : 0;
    g = (g > 20) ? g - 20 : 0;
    b = (b > 20) ? b - 20 : 0;

    pixels[i] = strip.Color(r, g, b);
    strip.setPixelColor(i, pixels[i]);
  }

  strip.show();
  delay(70);
}