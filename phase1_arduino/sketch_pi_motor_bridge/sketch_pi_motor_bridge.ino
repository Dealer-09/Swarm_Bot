/**
 * @file sketch_pi_motor_bridge.ino
 * @brief Phase 1 — Raspberry Pi → Arduino Motor Command Bridge
 *
 * Purpose:
 *   Receives ASCII motor commands from the Raspberry Pi over USB serial
 *   and drives the AlphaBot2-Ar motors (TB6612FNG via pins D4-D7).
 *
 * Command Protocol (Pi → Arduino, 115200 baud, newline terminated):
 *   M,<leftSpeed>,<leftDir>,<rightSpeed>,<rightDir>
 *
 *   leftSpeed / rightSpeed : 0–255
 *   leftDir  / rightDir   : F (forward) | B (backward)
 *
 * Examples:
 *   M,150,F,150,F   → move forward at speed 150
 *   M,150,B,150,B   → move backward at speed 150
 *   M,120,F,120,B   → spin left (right fwd, left bwd)
 *   M,120,B,120,F   → spin right
 *   M,0,F,0,F       → stop both motors
 *
 * Response:
 *   OK              → command accepted and applied
 *   ERR:<reason>    → malformed command
 *
 * Hardware (from pins.h / pin_mapping.csv):
 *   Motor A (Left)  — PWM: D5,  DIR: D4
 *   Motor B (Right) — PWM: D6,  DIR: D7
 *
 * Safety:
 *   - If no command received within WATCHDOG_MS, motors stop automatically.
 *   - All speeds clamped to [0, MAX_SPEED].
 *
 * @author Swarm POC
 */

// ============================================================================
// Pin Definitions (matches your pin_mapping.csv exactly)
// ============================================================================
#define MOTOR_A_PWM_PIN   5   // Left motor speed  (PWM)
#define MOTOR_A_DIR_PIN   4   // Left motor direction
#define MOTOR_B_PWM_PIN   6   // Right motor speed (PWM)
#define MOTOR_B_DIR_PIN   7   // Right motor direction

// ============================================================================
// Safety Config
// ============================================================================
#define MAX_SPEED       200   // Hard cap — never go above this (0–255)
#define WATCHDOG_MS    2000   // Stop motors if no command received in 2 seconds
#define SERIAL_BAUD   115200

// ============================================================================
// State
// ============================================================================
unsigned long lastCmdTime = 0;
bool motorsRunning = false;
String inputBuffer = "";

// ============================================================================
// Motor Helpers
// ============================================================================

void setMotor(int pwmPin, int dirPin, int speed, char dir) {
  speed = constrain(abs(speed), 0, MAX_SPEED);
  digitalWrite(dirPin, (dir == 'F') ? HIGH : LOW);
  analogWrite(pwmPin, speed);
}

void stopMotors() {
  analogWrite(MOTOR_A_PWM_PIN, 0);
  analogWrite(MOTOR_B_PWM_PIN, 0);
  motorsRunning = false;
}

// ============================================================================
// Command Parser
// ============================================================================

void parseCommand(const String& cmd) {
  // Expected: M,<leftSpd>,<leftDir>,<rightSpd>,<rightDir>
  if (cmd.length() < 5 || cmd.charAt(0) != 'M') {
    Serial.println("ERR:unknown_command");
    return;
  }

  // Split by comma
  int idx[5];
  idx[0] = cmd.indexOf(',', 0);
  idx[1] = cmd.indexOf(',', idx[0] + 1);
  idx[2] = cmd.indexOf(',', idx[1] + 1);
  idx[3] = cmd.indexOf(',', idx[2] + 1);

  if (idx[0] < 0 || idx[1] < 0 || idx[2] < 0 || idx[3] < 0) {
    Serial.println("ERR:malformed_missing_fields");
    return;
  }

  int leftSpeed   = cmd.substring(idx[0] + 1, idx[1]).toInt();
  char leftDir    = cmd.charAt(idx[1] + 1);
  int rightSpeed  = cmd.substring(idx[2] + 1, idx[3]).toInt();
  char rightDir   = cmd.charAt(idx[3] + 1);

  // Validate direction chars
  if ((leftDir != 'F' && leftDir != 'B') || (rightDir != 'F' && rightDir != 'B')) {
    Serial.println("ERR:invalid_direction_char");
    return;
  }

  // Apply
  setMotor(MOTOR_A_PWM_PIN, MOTOR_A_DIR_PIN, leftSpeed,  leftDir);
  setMotor(MOTOR_B_PWM_PIN, MOTOR_B_DIR_PIN, rightSpeed, rightDir);

  motorsRunning = (leftSpeed > 0 || rightSpeed > 0);
  lastCmdTime   = millis();

  Serial.println("OK");
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(MOTOR_A_PWM_PIN, OUTPUT);
  pinMode(MOTOR_A_DIR_PIN, OUTPUT);
  pinMode(MOTOR_B_PWM_PIN, OUTPUT);
  pinMode(MOTOR_B_DIR_PIN, OUTPUT);

  stopMotors();
  lastCmdTime = millis();

  Serial.println("READY:swarm_motor_bridge_v1");
}

// ============================================================================
// Loop
// ============================================================================

void loop() {
  // -- Read serial input (non-blocking, newline terminated) --
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        parseCommand(inputBuffer);
      }
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }

  // -- Watchdog: stop if no command received within WATCHDOG_MS --
  if (motorsRunning && (millis() - lastCmdTime > WATCHDOG_MS)) {
    stopMotors();
    Serial.println("WATCHDOG:motors_stopped");
  }
}
