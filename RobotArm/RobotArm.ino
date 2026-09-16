#include <Bonezegei_ULN2003_Stepper.h>
#include <ESP32RotaryEncoder.h>
#include <ESP32Servo.h>

#define FORWARD 0
#define REVERSE 1

#define RX2 16
#define TX2 17
// Servo pins
#define SERVO1_PIN 15
#define SERVO2_PIN 2
#define SERVO3_PIN 22
#define SERVO4_PIN 23

// Rotary movement amounts
const long STEPPER_STEPS_PER_CLICK = 50;
const int SERVO_DEGREES_PER_CLICK = 5;

// Stepper motors
Bonezegei_ULN2003_Stepper Stepper2(32, 33, 25, 26);
Bonezegei_ULN2003_Stepper Stepper1(27, 14, 12, 13);

// Rotary encoder: CLK, DT, pushbutton
RotaryEncoder rotaryEncoder(18, 19, 21);

// Servos
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Selected device:
// 1 = Stepper 1
// 2 = Stepper 2
// 3 = Servo 1
// 4 = Servo 2
// 5 = Servo 3
// 6 = Servo 4
uint8_t selectedMotor = 1;

// Encoder event variables
volatile bool toggleMotorRequested = false;

volatile bool encoderTurnRequested = false;
volatile long latestEncoderValue = 0;

long processedEncoderValue = 0;

// Servo positions
int servo1Angle = 90;
int servo2Angle = 90;
int servo3Angle = 90;
int servo4Angle = 90;

HardwareSerial SerialSecond(2);

// ======================================================
// Encoder callbacks
// ======================================================

void buttonCallback(unsigned long duration) {
  toggleMotorRequested = true;
}


void knobCallback(long value) {
  Serial.print("Encoder raw value: ");
  Serial.println(value);
  latestEncoderValue = value;
  encoderTurnRequested = true;
}


// ======================================================
// Display selected motor
// ======================================================

void printSelectedMotor() {
  Serial.println();

  switch (selectedMotor) {
    case 1:
      Serial.println("Selected: STEPPER 1");
      Serial.println("Pins: 32, 33, 25, 26");
      Serial.println("Rotary: 50 steps per click");
      Serial.println("Serial: signed step count");
      SerialSecond.println("1");
      break;

    case 2:
      Serial.println("Selected: STEPPER 2");
      Serial.println("Pins: 27, 14, 12, 13");
      Serial.println("Rotary: 50 steps per click");
      Serial.println("Serial: signed step count");
      SerialSecond.println("2");
      break;

    case 3:
      Serial.println("Selected: MG90S SERVO 1");
      Serial.println("Signal pin: GPIO 15");
      Serial.println("Rotary: 5 degrees per click");
      Serial.println("Serial: absolute angle 0-180");
      SerialSecond.println("3");
      break;

    case 4:
      Serial.println("Selected: MG90S SERVO 2");
      Serial.println("Signal pin: GPIO 2");
      Serial.println("Rotary: 5 degrees per click");
      Serial.println("Serial: absolute angle 0-180");
      SerialSecond.println("4");
      break;

    case 5:
      Serial.println("Selected: MG90S SERVO 3");
      Serial.println("Signal pin: GPIO 4");
      Serial.println("Rotary: 5 degrees per click");
      Serial.println("Serial: absolute angle 0-180");
      SerialSecond.println("5");
      break;

    case 6:
      Serial.println("Selected: MG90S SERVO 4");
      Serial.println("Signal pin: GPIO 23");
      Serial.println("Rotary: 5 degrees per click");
      Serial.println("Serial: absolute angle 0-180");
      SerialSecond.println("6");
      break;
  }
}


// ======================================================
// Stepper control
// ======================================================

void moveStepper(long steps) {
  if (steps == 0) {
    return;
  }

  uint8_t direction;
  unsigned long stepCount;

  if (steps > 0) {
    direction = FORWARD;
    stepCount = (unsigned long)steps;
  } else {
    direction = REVERSE;
    stepCount = (unsigned long)(-(steps + 1)) + 1;
  }

  Serial.print("Moving STEPPER ");
  Serial.print(selectedMotor);

  if (direction == FORWARD) {
    Serial.print(" FORWARD ");
  } else {
    Serial.print(" REVERSE ");
  }

  Serial.print(stepCount);
  Serial.println(" steps...");

  if (selectedMotor == 1) {
    Stepper1.step(direction, stepCount);
  } else if (selectedMotor == 2) {
    Stepper2.step(direction, stepCount);
  }

  Serial.print("DONE: STEPPER ");
  Serial.println(selectedMotor);
}


// ======================================================
// Servo helpers
// ======================================================

int constrainServoAngle(long angle) {
  if (angle < 0) {
    return 0;
  }

  if (angle > 180) {
    return 180;
  }

  return (int)angle;
}


void moveServo(long requestedAngle) {
  if (requestedAngle < 0 || requestedAngle > 180) {
    Serial.println("ERROR: Servo angle must be between 0 and 180.");
    return;
  }

  int targetAngle = (int)requestedAngle;

  switch (selectedMotor) {
    case 3:
      servo1Angle = targetAngle;
      servo1.write(servo1Angle);

      Serial.print("SERVO 1 angle: ");
      Serial.println(servo1Angle);
      break;

    case 4:
      servo2Angle = targetAngle;
      servo2.write(servo2Angle);

      Serial.print("SERVO 2 angle: ");
      Serial.println(servo2Angle);
      break;

    case 5:
      servo3Angle = targetAngle;
      servo3.write(servo3Angle);

      Serial.print("SERVO 3 angle: ");
      Serial.println(servo3Angle);
      break;

    case 6:
      servo4Angle = targetAngle;
      servo4.write(servo4Angle);

      Serial.print("SERVO 4 angle: ");
      Serial.println(servo4Angle);
      break;

    default:
      Serial.println("ERROR: No servo selected.");
      break;
  }
}


// ======================================================
// Rotary control
// ======================================================

void processRotaryTurn(long encoderChange) {
  if (encoderChange == 0) {
    return;
  }

  if (selectedMotor == 1 || selectedMotor == 2) {
    long requestedSteps =
        encoderChange * STEPPER_STEPS_PER_CLICK;

    moveStepper(requestedSteps);
    return;
  }

  switch (selectedMotor) {
    case 3:
      servo1Angle = constrainServoAngle(
          servo1Angle +
          (encoderChange * SERVO_DEGREES_PER_CLICK)
      );

      servo1.write(servo1Angle);

      Serial.print("SERVO 1 angle: ");
      Serial.println(servo1Angle);
      break;

    case 4:
      servo2Angle = constrainServoAngle(
          servo2Angle +
          (encoderChange * SERVO_DEGREES_PER_CLICK)
      );

      servo2.write(servo2Angle);

      Serial.print("SERVO 2 angle: ");
      Serial.println(servo2Angle);
      break;

    case 5:
      servo3Angle = constrainServoAngle(
          servo3Angle +
          (encoderChange * SERVO_DEGREES_PER_CLICK)
      );

      servo3.write(servo3Angle);

      Serial.print("SERVO 3 angle: ");
      Serial.println(servo3Angle);
      break;

    case 6:
      servo4Angle = constrainServoAngle(
          servo4Angle +
          (encoderChange * SERVO_DEGREES_PER_CLICK)
      );

      servo4.write(servo4Angle);

      Serial.print("SERVO 4 angle: ");
      Serial.println(servo4Angle);
      break;
  }
}


// ======================================================
// Serial control
// ======================================================

void processSerialCommand(long value) {
  if (selectedMotor == 1 || selectedMotor == 2) {
    moveStepper(value);
  } else {
    moveServo(value);
  }
}


// ======================================================
// Setup
// ======================================================

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  delay(1000);

  Serial.println();
  Serial.println("Starting motor controller...");

  // Stepper 1
  Stepper1.begin();
  Stepper1.setSpeed(2);

  // Stepper 2
  Stepper2.begin();
  Stepper2.setSpeed(2);

  // Servo 1
  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo1.write(servo1Angle);

  // Servo 2
  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo2.write(servo2Angle);

  // Servo 3
  servo3.setPeriodHertz(50);
  servo3.attach(SERVO3_PIN, 500, 2400);
  servo3.write(servo3Angle);

  // Servo 4
  servo4.setPeriodHertz(50);
  servo4.attach(SERVO4_PIN, 500, 2400);
  servo4.write(servo4Angle);

  // Rotary encoder
  rotaryEncoder.setEncoderType(EncoderType::HAS_PULLUP);

  // Large range so the encoder can move in either direction.
  rotaryEncoder.setBoundaries(-1000000, 1000000, false);

  rotaryEncoder.onTurned(&knobCallback);
  rotaryEncoder.onPressed(&buttonCallback);
  rotaryEncoder.begin();

  Serial.println("=== Controller Ready ===");
  Serial.println("Turn encoder to move selected motor.");
  Serial.println("Press encoder to select the next motor.");
  Serial.println("Serial commands are also enabled.");

  printSelectedMotor();

   SerialSecond.begin(
    9600,           // baud rate
    SERIAL_8N1,
    RX2,
    TX2
  );
}


// ======================================================
// Main loop
// ======================================================

void loop() {
  // Select the next motor using the encoder button
  if (toggleMotorRequested) {
    toggleMotorRequested = false;

    selectedMotor++;

    if (selectedMotor > 6) {
      selectedMotor = 1;
    }

    Serial.println();
    Serial.println("------------------------");
    printSelectedMotor();
    Serial.println("------------------------");
  }

  // Handle encoder rotation
  if (encoderTurnRequested) {
    encoderTurnRequested = false;

    long currentEncoderValue = latestEncoderValue;
    long encoderChange =
        currentEncoderValue - processedEncoderValue;

    processedEncoderValue = currentEncoderValue;

    processRotaryTurn(encoderChange);
  }

  // Handle Serial commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0) {
      return;
    }

    char *endPointer;
    long value = strtol(
        command.c_str(),
        &endPointer,
        10
    );

    if (*endPointer != '\0') {
      Serial.print("ERROR: Invalid command: ");
      Serial.println(command);
      return;
    }

    Serial.print("Received: ");
    Serial.println(value);

    processSerialCommand(value);
  }
}