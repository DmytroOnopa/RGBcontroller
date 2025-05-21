#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#include <EEPROM.h>

#define LED_PIN     8
#define NUM_LEDS    13

#define BTN_MODE     A0
#define BTN_BRIGHTNESS A1
#define BTN_COLOR    A2

// Піни для KL1888CB (підключи як у себе):
#define SEG_A 2
#define SEG_B 3
#define SEG_C 4
#define SEG_D 5
#define SEG_E 6
#define SEG_F 7

// Константи режимів для зручності
#define MODE_LAMP 0
#define MODE_RAINBOW 1
#define MODE_MANUAL 2
#define MODE_POLICE 3
#define MODE_WAVE 4
#define MODE_CHASE 5
#define MODE_CASCADE 6
#define MODE_CENTERSPOTLIGHT 7

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

const uint8_t center = 6; // бо 7-й піксель, а індексація з нуля. Для фонарика

// Масив кодування цифр 0-9 для 7-сегментного індикатора
// Формат: бінарне представлення сегментів A-F
// 1 - сегмент увімкнено, 0 - вимкнено
// Порядок сегментів: A B C D E F
// Наприклад, 0 = A+B+C+D+E+F, 1 = B+C, тощо
const uint8_t digitSegments[10] = {
  0b111111, // 0
  0b011000, // 1
  0b110110, // 2
  0b111100, // 3
  0b011001, // 4
  0b101101, // 5
  0b101111, // 6
  0b111000, // 7
  0b111111, // 8
  0b111101  // 9
};

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

  // Ініціалізація піна сегментів 7-сегментного індикатора
  pinMode(SEG_A, OUTPUT);
  pinMode(SEG_B, OUTPUT);
  pinMode(SEG_C, OUTPUT);
  pinMode(SEG_D, OUTPUT);
  pinMode(SEG_E, OUTPUT);
  pinMode(SEG_F, OUTPUT);

  // Вивести поточний рівень яскравості одразу
  displayBrightnessLevel(brightnessLevel);
}

void loop() {
  btnMode.update();
  btnBright.update();
  btnColor.update();

  // Перемикання режиму
  if (btnMode.fell()) {
    mode++;
    if (mode > MODE_CENTERSPOTLIGHT) mode = MODE_LAMP;
    animationStep = 0;
    policePhase = 0;
    blinkCount = 0;
    lightsOn = false;
    saveSettings();
  }

  // Якщо режим НЕ ручний (не 2) — керуємо яскравістю
  if (mode != MODE_MANUAL) {
    if (btnBright.fell()) {
      if (brightnessLevel > 0) {
        fadeToBrightness(brightnessValues[brightnessLevel], brightnessValues[brightnessLevel - 1]);
        brightnessLevel--;
        saveSettings();
        displayBrightnessLevel(brightnessLevel);
      }
    }
    if (btnColor.fell()) {
      if (brightnessLevel < sizeof(brightnessValues) - 1) {
        fadeToBrightness(brightnessValues[brightnessLevel], brightnessValues[brightnessLevel + 1]);
        brightnessLevel++;
        saveSettings();
        displayBrightnessLevel(brightnessLevel);
      }
    }
  } else {
    // У режимі 2 — ручна зміна кольору
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

  // Виклик відповідного режиму
  switch (mode) {
    case MODE_LAMP: modeLamp(); break;
    case MODE_RAINBOW: animationRainbow(); break;
    case MODE_MANUAL: modeManualColor(); break;
    case MODE_POLICE: animationPolice(); break;
    case MODE_WAVE: animationWave(); break;
    case MODE_CHASE: animationChase(); break;
    case MODE_CASCADE: animationCascade(); break;
    case MODE_CENTERSPOTLIGHT: centerSpotlight(); break;
  }
}

// Функція відображення цифри 0-9 на 7-сегментному індикаторі
void displayDigit(uint8_t digit) {
  if (digit > 9) digit = 0; // обмеження

  uint8_t seg = digitSegments[digit];

  digitalWrite(SEG_A, (seg & 0b100000) ? HIGH : LOW);
  digitalWrite(SEG_B, (seg & 0b010000) ? HIGH : LOW);
  digitalWrite(SEG_C, (seg & 0b001000) ? HIGH : LOW);
  digitalWrite(SEG_D, (seg & 0b000100) ? HIGH : LOW);
  digitalWrite(SEG_E, (seg & 0b000010) ? HIGH : LOW);
  digitalWrite(SEG_F, (seg & 0b000001) ? HIGH : LOW);
}

// Вивід рівня яскравості (0-4)
void displayBrightnessLevel(uint8_t level) {
  displayDigit(level);
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

    r = (r > 20) ? r - 20 : 0;
    g = (g > 20) ? g - 20 : 0;
    b = (b > 20) ? b - 20 : 0;

    strip.setPixelColor(i, r, g, b);
  }

  strip.show();
  delay(100);
}

void centerSpotlight() {
  strip.clear();
  strip.setPixelColor(center, strip.Color(255, 255, 255));
  strip.show();
}
