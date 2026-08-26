# Joystick Memory Game (Arduino)

A Simon-Says style memory game built on Arduino using a joystick, a 16x2 LCD, four LEDs, and a buzzer. Watch the sequence of directions play out, then repeat it back correctly to advance — each round adds one more step, and the game speeds up as you go.

## Features

- **Joystick-driven input** — push up/down/left/right to answer, center-return debouncing prevents double registers
- **16x2 LCD display** — shows round number, step progress, score, and game state
- **Custom LCD icons** — smiley, sad face, and trophy characters for correct/game-over/high-score screens
- **Per-direction tones** — each direction has a distinct pitch, and the pitch is replayed exactly when you press it, so the game can be learned by ear
- **Success jingle** — a short ascending chime plays when a round is completed
- **Difficulty ramp** — playback speed increases each round, with a floor so it never becomes unreadable
- **Idle LED animation** — a chase animation runs on the LEDs while waiting at the start screen
- **Countdown** — a "3, 2, 1" countdown with rising beeps before each round's sequence plays
- **Persistent high score** — best score is saved to EEPROM and survives power cycles

## Hardware Required

| Component | Qty |
|---|---|
| Arduino Uno (or compatible) | 1 |
| 16x2 LCD (HD44780-compatible) | 1 |
| 10kΩ potentiometer (LCD contrast) | 1 |
| Analog joystick module (2-axis + button) | 1 |
| LEDs | 4 |
| 220Ω resistors (for LEDs) | 4 |
| Passive buzzer | 1 |
| Breadboard + jumper wires | — |

> **Note:** Use a *passive* buzzer, not an active one — the game uses `tone()` to control pitch, which active buzzers ignore.

## Wiring

**LCD (4-bit mode):**

| LCD Pin | Arduino Pin |
|---|---|
| RS | 12 |
| E | 11 |
| D4 | 5 |
| D5 | 4 |
| D6 | 3 |
| D7 | 2 |
| R/W | GND |
| VSS | GND |
| VDD | 5V |
| V0 (contrast) | Wiper of 10kΩ pot (other legs to 5V / GND) |
| A (backlight +) | 5V |
| K (backlight −) | GND |

**Joystick:**

| Joystick Pin | Arduino Pin |
|---|---|
| GND | GND |
| +5V | 5V |
| VRx | A0 |
| VRy | A1 |
| SW | A2 |

**LEDs** (each through a 220Ω resistor to GND):

| Direction | Arduino Pin |
|---|---|
| Up | 6 |
| Down | 7 |
| Left | 8 |
| Right | 9 |

**Buzzer:**

| Buzzer Pin | Arduino Pin |
|---|---|
| + | 10 |
| − | GND |

## Installation

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) if you don't already have it.
2. Wire up the components as described above.
3. Open `memory_game.ino` in the Arduino IDE.
4. Select your board and port under **Tools**.
5. Click **Upload**.

No external libraries beyond the built-in `LiquidCrystal` and `EEPROM` libraries are required.

## How to Play

1. Power on — the LCD shows the title screen and your current best score while the LEDs chase in a loop.
2. Press the joystick button to start.
3. A "Get ready... 3, 2, 1" countdown plays.
4. Watch the sequence: each step lights an LED and plays a tone.
5. Repeat the sequence by pushing the joystick in the same directions, in order.
6. Get it right → a success jingle plays, the sequence grows by one step, and it gets a little faster.
7. Get it wrong → game over screen, with a new high score fanfare if you beat your best.
8. Press the joystick button again to retry.

## Tuning

- **Joystick thresholds:** cheap joystick modules vary in resting/extreme analog values. If directions misfire or don't register, print `analogRead(VRx)` / `analogRead(VRy)` to the Serial Monitor while moving the stick and adjust the `300` / `724` thresholds in `readDirectionBlocking()` accordingly.
- **Difficulty curve:** adjust the ramp in `playSequenceToUser()` — `stepDelay = 350 - (roundCount * 15)`, floored at `150` — to make the game speed up faster or slower.
- **Tone pitches:** `TONE_UP`, `TONE_DOWN`, `TONE_LEFT`, `TONE_RIGHT` are defined near the top of the sketch if you want to change the sound palette.

## License

Feel free to use, modify, and share.
