#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- LCD setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- States ---
enum State { RUNNING, PAUSED, READY, DONE };
const char* stateMessages[] = { "RUNNING", "PAUSED ", "READY  ", "DONE   " };
State currentState = READY;

// --- Events ---
enum Event {
  NONE,
  MINUTES_PRESSED,
  SECONDS_PRESSED,
  START_STOP_PRESSED,
  START_STOP_TRIPLE_PRESSED,
  TIME_ELAPSED,
  BUZZER_STOPPED
};

// --- Pin definitions ---
const int ISR_PIN               = 2;
const int MINUTES_BUTTON_PIN    = 4;
const int SECONDS_BUTTON_PIN    = 5;
const int BUZZER_PIN            = 6;
const int START_STOP_BUTTON_PIN = 7;

// --- Flags ---
volatile bool buttonTriggered = false;

// --- Timer variables ---
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

// --- Function declarations ---
void ISR_button();
Event getEvent();
void handleEvent(Event event);
void updateLCD();
void displayInitialScreen();
bool isKeyPressed(int pin);
void updateTotalSeconds();
void startBuzzer();
void stopBuzzer();

// --- State Handlers ---
void stateReadyReactOn(Event event);
void statePausedReactOn(Event event);
void stateRunningReactOn(Event event);
void stateDoneReactOn(Event event);

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

  Event event = getEvent();
  if (event != NONE)
    handleEvent(event);
  updateLCD();
}

void ISR_button() {
  buttonTriggered = true;
}

bool isKeyPressed(int pin) {
  return digitalRead(pin) == LOW;
}

Event getEvent() {
  if (buttonTriggered && now - lastDebounce > debounceDelay) {
    buttonTriggered = false;
    lastDebounce = now;

    if (isKeyPressed(SECONDS_BUTTON_PIN))
      return SECONDS_PRESSED;
    if (isKeyPressed(MINUTES_BUTTON_PIN))
      return MINUTES_PRESSED;
    if (isKeyPressed(START_STOP_BUTTON_PIN)) {
      if (now - lastClickTime <= clickWindow) {
        lastClickTime = now;
        clickCount++;
        if (clickCount == 3) {
          clickCount = 0;
          return START_STOP_TRIPLE_PRESSED;
        }
        return NONE;
      }

      lastClickTime = now;
      clickCount = 0;

      return START_STOP_PRESSED;
    }
  }

  if (currentState == RUNNING && totalSeconds > 0 && now - lastTick >= 1000) {
    totalSeconds--;
    lastTick = now;
    if (totalSeconds == 0) {
      return TIME_ELAPSED;
    }
  }

  if (currentState == DONE && buzzerCount >= 5 && now - buzzerStart >= 200) {
    return BUZZER_STOPPED;
  }

  return NONE;
}

void handleEvent(Event event) {
  switch (currentState) {
    case READY:   stateReadyReactOn(event);   break;
    case PAUSED:  statePausedReactOn(event);  break;
    case RUNNING: stateRunningReactOn(event); break;
    case DONE:    stateDoneReactOn(event);    break;
  }
}

void stateReadyReactOn(Event event) {
  switch (event) {
    case MINUTES_PRESSED:
    case SECONDS_PRESSED:
      updateTotalSeconds();
      if (totalSeconds > 0) {
        currentState = PAUSED;
      }
      break;

    default:
      break;
  }
}

void statePausedReactOn(Event event) {
  switch (event) {
    case MINUTES_PRESSED:
    case SECONDS_PRESSED:
      updateTotalSeconds();
      break;

    case START_STOP_PRESSED:
      if (totalSeconds > 0) {
        currentState = RUNNING;
      }
      break;

    case START_STOP_TRIPLE_PRESSED:
      totalSeconds = 0;
      currentState = READY;
      break;

    default:
      break;
  }
}

void stateRunningReactOn(Event event) {
  switch (event) {
    case MINUTES_PRESSED:
    case SECONDS_PRESSED:
      updateTotalSeconds();
      break;

    case START_STOP_PRESSED:
      currentState = PAUSED;
      break;

    case START_STOP_TRIPLE_PRESSED:
      totalSeconds = 0;
      currentState = READY;
      break;

    case TIME_ELAPSED:
      startBuzzer();
      currentState = DONE;
      break;

    default:
      break;
  }
}

void stateDoneReactOn(Event event) {
  switch (event) {
    case START_STOP_PRESSED:
    case BUZZER_STOPPED:
      stopBuzzer();
      currentState = READY;
      break;

    default:
      break;
  }
}

void updateTotalSeconds() {
  if (isKeyPressed(MINUTES_BUTTON_PIN)) totalSeconds += 60;
  if (isKeyPressed(SECONDS_BUTTON_PIN)) totalSeconds += 10;
}

void startBuzzer() {
  buzzerCount = 0;
  buzzerStart = now;
  tone(BUZZER_PIN, 1000, 200);
}

void stopBuzzer() {
  noTone(BUZZER_PIN);
  buzzerCount = 0;
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print(stateMessages[currentState]);

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
