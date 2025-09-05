#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- LCD setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- States ---
enum State { RUNNING, PAUSED, READY, DONE };
const char* stateMessages[] = { "RUNNING", "PAUSED ", "READY  ", "DONE   " };

// --- Pin definitions ---
const int ISR_PIN = 2;
const int MINUTES_BUTTON_PIN = 4;
const int SECONDS_BUTTON_PIN = 5;
const int START_STOP_BUTTON_PIN = 7;
const int BUZZER_PIN = 6;

// --- Flags ---
volatile bool buttonTriggered = false;

// --- Timer variables ---
State current_state;
bool buzzerActive = false;

unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 100;

int totalSeconds = 0;
unsigned long lastTick = 0;
unsigned long buzzerStart = 0;
int buzzerCount = 0;

// --- Triple click tracking ---
int clickCount = 0;
unsigned long lastClickTime = 0;
const unsigned long clickWindow = 600;

// --- Time tracker ---
unsigned long now = 0;

// --- Button commands ---
bool minutesPressed = false;
bool secondsPressed = false;
bool startStopPressed = false;

void ISR_button();
void checkButtons();
void handleStateMachine();
void handleReady();
void handlePaused();
void handleRunning();
void handleDone();
void handleBuzzer();
void updateLCD();
void displayInitialScreen();
void handleStartStop();
bool isKeyPressed(int pin);
void updateTotalSeconds();

void setup() {
  current_state = READY;

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

  checkButtons();
  handleStateMachine();
  handleBuzzer();
  updateLCD();
}

void ISR_button() {
  buttonTriggered = true;
}

bool isKeyPressed(int pin) {
  return digitalRead(pin) == LOW;
}

void checkButtons() {
  minutesPressed = false;
  secondsPressed = false;
  startStopPressed = false;

  if (buttonTriggered && now - lastDebounce > debounceDelay) {
    buttonTriggered = false;
    lastDebounce = now;

    if      (isKeyPressed(MINUTES_BUTTON_PIN))    minutesPressed   = true;
    else if (isKeyPressed(SECONDS_BUTTON_PIN))    secondsPressed   = true;
    else if (isKeyPressed(START_STOP_BUTTON_PIN)) startStopPressed = true;
  }
}

void handleStateMachine() {
  switch (current_state) {
    case READY:   handleReady();   break;
    case PAUSED:  handlePaused();  break;
    case RUNNING: handleRunning(); break;
    case DONE:    handleDone();    break;
  }
}

void updateTotalSeconds() {
  if (minutesPressed) totalSeconds += 60;
  if (secondsPressed) totalSeconds += 10;
}

void handleReady() {
  updateTotalSeconds();

  if (totalSeconds > 0 && (minutesPressed || secondsPressed)) {
    current_state = PAUSED;
  }
}

void handlePaused() {
  updateTotalSeconds();

  if (startStopPressed) {
    handleStartStop();
  }
}

void handleRunning() {
  // allow time increase while running
  updateTotalSeconds();

  if (totalSeconds > 0 && now - lastTick >= 1000) {
    totalSeconds--;
    lastTick = now;
  }

  if (totalSeconds == 0) {
    current_state = DONE;
    buzzerActive = true;
    buzzerCount = 0;
    buzzerStart = now;
  }

  if (startStopPressed) {
    handleStartStop();
  }
}

void handleDone() {
  if (!buzzerActive) {
    current_state = READY;
  }
}

void handleBuzzer() {
  if (!buzzerActive) return;

  if (isKeyPressed(START_STOP_BUTTON_PIN)) {
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

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print(stateMessages[current_state]);

  int displayMinutes = totalSeconds / 60;
  int displaySeconds = totalSeconds % 60;
  lcd.setCursor(6, 1);
  if (displayMinutes < 10) lcd.print('0');
  lcd.print(displayMinutes);
  lcd.print(':');
  if (displaySeconds < 10) lcd.print('0');
  lcd.print(displaySeconds);
}

void displayInitialScreen() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Set     ");
  lcd.setCursor(0, 1);
  lcd.print("Time: 00:00");
}

void handleStartStop() {
  if (now - lastClickTime <= clickWindow) {
    clickCount++;
  } else {
    clickCount = 1;
  }
  lastClickTime = now;

  if (clickCount == 3) {
    totalSeconds = 0;
    current_state = READY;
    buzzerActive = false;
    noTone(BUZZER_PIN);
    clickCount = 0;
    return;
  }

  if (totalSeconds > 0 && clickCount == 1) {
    if (current_state == RUNNING) current_state = PAUSED;
    else current_state = RUNNING;
  }
}
