#include <LiquidCrystal.h>
#include <EEPROM.h>

// ---- LCD ----
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// ---- Joystick ----
const int VRx = A0;
const int VRy = A1;
const int SW  = A2;

// ---- LEDs ----
const int ledUp    = 6;
const int ledDown  = 7;
const int ledLeft  = 8;
const int ledRight = 9;

// ---- Buzzer ----
const int buzzerPin = 10;

// ---- Game state ----
const int MAX_ROUNDS = 100;
int sequence[MAX_ROUNDS];
int roundCount = 0; // index of the highest step currently in play
int highScore = 0;
const int EEPROM_ADDR = 0;

enum Dir { NONE = 0, UP, DOWN, LEFT, RIGHT };

// Distinct tone per direction
const int TONE_UP    = 523; // C5
const int TONE_DOWN  = 392; // G4
const int TONE_LEFT  = 440; // A4
const int TONE_RIGHT = 349; // F4
const int TONE_WRONG = 150;

// ---- Custom LCD characters ----
byte smileyChar[8] = {
  0b00000,
  0b01010,
  0b01010,
  0b00000,
  0b10001,
  0b01110,
  0b00000,
  0b00000
};
byte sadChar[8] = {
  0b00000,
  0b01010,
  0b01010,
  0b00000,
  0b01110,
  0b10001,
  0b00000,
  0b00000
};
byte trophyChar[8] = {
  0b11111,
  0b10101,
  0b11111,
  0b00100,
  0b00100,
  0b01110,
  0b00000,
  0b00000
};
const byte CHAR_SMILEY = 0;
const byte CHAR_SAD    = 1;
const byte CHAR_TROPHY = 2;

void flashInputFeedback(int dir, bool correct, bool playSound = true);

void setup() {
  lcd.begin(16, 2);
  lcd.createChar(CHAR_SMILEY, smileyChar);
  lcd.createChar(CHAR_SAD, sadChar);
  lcd.createChar(CHAR_TROPHY, trophyChar);

  pinMode(ledUp, OUTPUT);
  pinMode(ledDown, OUTPUT);
  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);
  pinMode(SW, INPUT_PULLUP);

  randomSeed(analogRead(A3)); // floating pin for random entropy

  EEPROM.get(EEPROM_ADDR, highScore);
  if (highScore < 0 || highScore > 1000) highScore = 0; // guard against unset/corrupt EEPROM

  showWelcome();
  waitForButtonPress();
  startNewGame();
}

void loop() {
  playSequenceToUser();
  bool success = getPlayerSequence();

  if (success) {
    lcd.clear();
    lcd.write(CHAR_SMILEY);
    lcd.print(" Correct!");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.print(roundCount + 1);

    delay(400); // little pause before the jingle
    playSuccessJingle();
    delay(800); // let the jingle land before moving on

    roundCount++;
    addRandomStep();
  } else {
    gameOverSequence();
    startNewGame();
  }
}

// ---------- Setup / welcome ----------

void showWelcome() {
  lcd.clear();
  lcd.print("Memory Game!");
  lcd.setCursor(0, 1);
  lcd.write(CHAR_TROPHY);
  lcd.print(" Best: ");
  lcd.print(highScore);
}

// Idle LED chase animation plays while waiting for the player to
// press the joystick button to start.
void waitForButtonPress() {
  int ledSequence[] = {ledUp, ledRight, ledDown, ledLeft};
  int idx = 0;

  while (digitalRead(SW) == HIGH) {
    digitalWrite(ledSequence[idx], HIGH);
    delay(120);
    digitalWrite(ledSequence[idx], LOW);
    idx = (idx + 1) % 4;
  }

  delay(200); // debounce
  while (digitalRead(SW) == LOW) delay(10);
}

void startNewGame() {
  roundCount = 0;
  for (int i = 0; i < MAX_ROUNDS; i++) sequence[i] = 0;
  addRandomStep();
}

void addRandomStep() {
  sequence[roundCount] = random(1, 5); // 1-4 => UP/DOWN/LEFT/RIGHT
}

// ---------- Playback ----------

void countdown() {
  for (int i = 3; i >= 1; i--) {
    lcd.clear();
    lcd.print("Get ready...");
    lcd.setCursor(0, 1);
    lcd.print(i);
    tone(buzzerPin, 300 + i * 100, 150);
    delay(500);
  }
  noTone(buzzerPin);
}

void playSequenceToUser() {
  lcd.clear();
  lcd.print("Watch!");
  lcd.setCursor(0, 1);
  lcd.print("Round: ");
  lcd.print(roundCount + 1);
  delay(500);

  countdown();

  // Difficulty ramp: playback speeds up as rounds go on, with a floor
  // so it never becomes unreadable.
  int stepDelay = 350 - (roundCount * 15);
  if (stepDelay < 150) stepDelay = 150;

  lcd.clear();
  lcd.print("Watch!");
  lcd.setCursor(0, 1);
  lcd.print("Round: ");
  lcd.print(roundCount + 1);
  delay(300);

  for (int i = 0; i <= roundCount; i++) {
    flashDirection(sequence[i], stepDelay);
    delay(200);
  }
}

void flashDirection(int dir, int stepDelay) {
  int led = 0, freq = 0;
  switch (dir) {
    case UP:    led = ledUp;    freq = TONE_UP;    break;
    case DOWN:  led = ledDown;  freq = TONE_DOWN;  break;
    case LEFT:  led = ledLeft;  freq = TONE_LEFT;  break;
    case RIGHT: led = ledRight; freq = TONE_RIGHT; break;
  }
  digitalWrite(led, HIGH);
  tone(buzzerPin, freq, stepDelay);
  delay(stepDelay + 50);
  digitalWrite(led, LOW);
}

// ---------- Player input ----------

bool getPlayerSequence() {
  lcd.clear();
  lcd.print("Your turn!");

  for (int i = 0; i <= roundCount; i++) {
    lcd.setCursor(0, 1);
    lcd.print("Step: ");
    lcd.print(i + 1);
    lcd.print("/");
    lcd.print(roundCount + 1);
    lcd.print("   ");

    int input = readDirectionBlocking();
    bool correct = (input == sequence[i]);

    flashInputFeedback(input, correct);

    if (!correct) return false;
  }
  return true;
}

// Lights the LED for whatever the player pressed, and plays a
// short correct/incorrect blip so they get instant audio feedback
// on top of the base direction tone. Pass playSound=false to only
// flash the LED without any tone.
void flashInputFeedback(int dir, bool correct, bool playSound) {
  int led = 0, freq = 0;
  switch (dir) {
    case UP:    led = ledUp;    freq = TONE_UP;    break;
    case DOWN:  led = ledDown;  freq = TONE_DOWN;  break;
    case LEFT:  led = ledLeft;  freq = TONE_LEFT;  break;
    case RIGHT: led = ledRight; freq = TONE_RIGHT; break;
  }

  digitalWrite(led, HIGH);
  if (playSound) {
    tone(buzzerPin, freq, 250); // same pitch as playback, so it matches what they saw
  }
  delay(250);
  digitalWrite(led, LOW);
  noTone(buzzerPin);

  if (!correct) {
    delay(80);
    tone(buzzerPin, TONE_WRONG, 300); // extra low buzz layered on top, only for wrong input
    delay(300);
    noTone(buzzerPin);
  }
}

void playSuccessJingle() {
  int notes[] = {523, 659, 784, 1047}; // C5, E5, G5, C6 — quick ascending run
  int noteDur = 100;

  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, notes[i], noteDur);
    delay(noteDur + 30); // small gap between notes so they don't blur together
  }
  noTone(buzzerPin);
}

// Blocks until the joystick is pushed past a threshold in one
// direction, then waits for it to return to center before
// returning — this stops one push from registering multiple times.
int readDirectionBlocking() {
  int dir = NONE;

  while (dir == NONE) {
    int x = analogRead(VRx);
    int y = analogRead(VRy);

    if (y < 300)      dir = UP;
    else if (y > 724) dir = DOWN;
    else if (x < 300) dir = LEFT;
    else if (x > 724) dir = RIGHT;

    delay(10);
  }

  while (true) {
    int x = analogRead(VRx);
    int y = analogRead(VRy);
    if (x > 400 && x < 624 && y > 400 && y < 624) break;
    delay(10);
  }

  return dir;
}

// ---------- Game over ----------

void gameOverSequence() {
  bool newHighScore = (roundCount > highScore);
  if (newHighScore) {
    highScore = roundCount;
    EEPROM.put(EEPROM_ADDR, highScore);
  }

  lcd.clear();
  lcd.write(CHAR_SAD);
  lcd.print(" Game Over!");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(roundCount);

  tone(buzzerPin, TONE_WRONG, 200);
  delay(250);
  tone(buzzerPin, TONE_WRONG - 50, 400);
  delay(500);
  noTone(buzzerPin);

  for (int i = 0; i < 3; i++) {
    digitalWrite(ledUp, HIGH);
    digitalWrite(ledDown, HIGH);
    digitalWrite(ledLeft, HIGH);
    digitalWrite(ledRight, HIGH);
    delay(150);
    digitalWrite(ledUp, LOW);
    digitalWrite(ledDown, LOW);
    digitalWrite(ledLeft, LOW);
    digitalWrite(ledRight, LOW);
    delay(150);
  }

  delay(1500);

  if (newHighScore) {
    lcd.clear();
    lcd.write(CHAR_TROPHY);
    lcd.print(" New Best!");
    lcd.setCursor(0, 1);
    lcd.print(highScore);
    tone(buzzerPin, 1047, 150);
    delay(200);
    tone(buzzerPin, 1319, 250);
    delay(300);
    noTone(buzzerPin);
    delay(1200);
  }

  lcd.clear();
  lcd.print("Press to retry");
  waitForButtonPress();
}
