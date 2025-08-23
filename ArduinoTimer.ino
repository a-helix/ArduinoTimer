#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Pin definitions ---
const int ISR_PIN = 2;
const int MINUTES_BUTTON_PIN = 4;
const int SECONDS_BUTTON_PIN = 5;
const int START_STOP_BUTTON_PIN = 7;
const int BUZZER_PIN = 6;

// --- Flags ---
volatile bool buttonTriggered = false;

// --- Timer variables ---
bool running = false;
bool buzzerActive = false;

unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 250;

int totalSeconds = 0;
unsigned long lastTick = 0;
unsigned long buzzerStart = 0;
int buzzerCount = 0;

// --- Triple click tracking ---
int clickCount = 0;
unsigned long lastClickTime = 0;
const unsigned long clickWindow = 600;

unsigned long now = 0;

void setup() {
  pinMode(ISR_PIN, INPUT_PULLUP);
  pinMode(MINUTES_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SECONDS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(START_STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(ISR_PIN), ISR_button, FALLING);

  displayInitialScreen();
}

void loop() {
  now = millis();
  handleISRTrigger();
  handleCountdown();
  handleBuzzer();
  updateLCD();
}

// --- ISR: only flag ---
void ISR_button() {
  buttonTriggered = true;
}

// --- Handle ISR trigger ---
void handleISRTrigger() {
  if (buttonTriggered && now - lastDebounce > debounceDelay) {
    buttonTriggered = false;
    lastDebounce = now;

    // Identify which button caused interrupt
    if (digitalRead(MINUTES_BUTTON_PIN) == LOW) {
      totalSeconds += 60;
    } 
    else if (digitalRead(SECONDS_BUTTON_PIN) == LOW) {
      totalSeconds += 10;
    } 
    else if (digitalRead(START_STOP_BUTTON_PIN) == LOW) {
      handleStartStop();
    }
  }
}

// --- Timer countdown ---
void handleCountdown() {
  if (running && totalSeconds > 0 && now - lastTick >= 1000) {
    totalSeconds--;
    lastTick = now;
  }

  if (running && totalSeconds == 0 && !buzzerActive) {
    running = false;
    buzzerActive = true;
    buzzerCount = 0;
    buzzerStart = now;
  }
}

// --- buzzer makes 5 beeps ---
void handleBuzzer() {
  if (!buzzerActive) return;

  if (digitalRead(START_STOP_BUTTON_PIN) == LOW) {
    buzzerActive = false;
    noTone(BUZZER_PIN);
    return;
  }

  if (buzzerCount < 5) {
    if (now - buzzerStart >= 300) {
      tone(BUZZER_PIN, 1000, 200);
      buzzerStart = now;
      buzzerCount++;
    }
  } else {
    if (now - buzzerStart >= 200) {
      buzzerActive = false;
      noTone(BUZZER_PIN);
    }
  }
}

// --- Update LCD ---
void updateLCD() {
  lcd.setCursor(0, 0);
  if (totalSeconds == 0 && !running) lcd.print("Ready   ");
  else if (running) lcd.print("Running ");
  else lcd.print("Paused  ");

  int displayMinutes = totalSeconds / 60;
  int displaySeconds = totalSeconds % 60;
  lcd.setCursor(6, 1);
  if (displayMinutes < 10) lcd.print('0');
  lcd.print(displayMinutes);
  lcd.print(':');
  if (displaySeconds < 10) lcd.print('0');
  lcd.print(displaySeconds);
}

// --- Initial LCD screen ---
void displayInitialScreen() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  lcd.setCursor(0, 1);
  lcd.print("Time: 00:00");
}

// --- Handle Start/Stop with triple click reset ---
void handleStartStop() {
  if (now - lastClickTime <= clickWindow) {
    clickCount++;
  } else {
    clickCount = 1;
  }
  lastClickTime = now;

  if (clickCount == 3) {
    totalSeconds = 0;
    running = false;
    buzzerActive = false;
    noTone(BUZZER_PIN);
    clickCount = 0;
    return;
  }

  if (totalSeconds > 0 && clickCount == 1) {
    running = !running;
  }
}
