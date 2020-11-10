#include <Servo.h>

// Pin definintions
#define MOT_1A_PIN PA7
#define MOT_1B_PIN PA2
#define MOT_2A_PIN PA6
#define MOT_2B_PIN PA3
#define MOT_SLEEP_PIN PA1
#define MOT_FAULT_PIN PA5
#define LED_PIN PA0
#define SERVO_PIN PA4
#define PPM_PIN PB1
#define CFG1_PIN PA13
#define CFG2_PIN PA14

// Deadzone for drive motors
#define DEADZONE 10

// Config options
static bool INPUT_MODE;
static bool MIXING;

// Servo object for outputting throttle channel
Servo weapon;

// PPM decoding configuration
#define PPM_MIN_SYNC 5000
#define PPM_MIN_PULSE 600
#define PPM_MAX_PULSE 1600

// PWM decoding configuration
#define PWM_MIN_PULSE 1000
#define PWM_MAX_PULSE 2000

#define CHANNELS 8
static int channels[CHANNELS];

static int lastUpdate;
static bool haveUpdate;

void ppmInterrupt () {
  static char counter = 0;
  static unsigned long startMicros = 0;

  bool val = digitalRead(PPM_PIN);
  
  if (val) {
    startMicros = micros();
  } else {
    int duration = micros() - startMicros;
    if (duration > PPM_MIN_SYNC) {
      counter = 0;
    } else {
      duration =  constrain(duration, PPM_MIN_PULSE, PPM_MAX_PULSE);
      channels[counter] = map(duration, PPM_MIN_PULSE, PPM_MAX_PULSE, -255, 255);
      counter += 1;
    }

    lastUpdate = millis();
    haveUpdate = true;
  }
}

void pwmInterruptA () {
  static unsigned long startMicros = 0;

  bool val = digitalRead(PPM_PIN);
  
  if (val) {
    startMicros = micros();
  } else {
    int duration = constrain(micros() - startMicros, PWM_MIN_PULSE, PPM_MAX_PULSE);
    channels[0] = map(duration, PWM_MIN_PULSE, PWM_MAX_PULSE, -255, 255);

    lastUpdate = millis();
    haveUpdate = true;
  }
}

void pwmInterruptB () {
  static unsigned long startMicros = 0;
  
  bool val = digitalRead(SERVO_PIN);
  
  if (val) {
    startMicros = micros();
  } else {
    int duration = constrain(micros() - startMicros, PWM_MIN_PULSE, PPM_MAX_PULSE);
    channels[1] = map(duration, PWM_MIN_PULSE, PWM_MAX_PULSE, -255, 255);

    lastUpdate = millis();
    haveUpdate = true;
  }
}

// Set motor 0 or 1 to a speed between -255 and 255
void setMotor (bool motor, int value) {
  if (motor == 0) {
    if (value > 0+DEADZONE) {
      analogWrite(MOT_1A_PIN, abs(value));
      digitalWrite(MOT_1B_PIN, LOW);
    } else if (value < 0-DEADZONE) {
      analogWrite(MOT_1A_PIN, 255-abs(value));
      digitalWrite(MOT_1B_PIN, HIGH);
    } else {
      analogWrite(MOT_1A_PIN, 255);
      digitalWrite(MOT_1B_PIN, HIGH);
    }
  } else {
    if (value > 0+DEADZONE) {
      analogWrite(MOT_2A_PIN, abs(value));
      digitalWrite(MOT_2B_PIN, LOW);
    } else if (value < 0-DEADZONE) {
      analogWrite(MOT_2A_PIN, 255-abs(value));
      digitalWrite(MOT_2B_PIN, HIGH);
    } else {
      analogWrite(MOT_2A_PIN, 255);
      digitalWrite(MOT_2B_PIN, HIGH);
    }
  }
}

void setup() {
  // Set pin modes (don't set PWM pins to OUTPUT on STM32)
  //pinMode(MOT_1A_PIN, OUTPUT);
  pinMode(MOT_1B_PIN, OUTPUT);
  //pinMode(MOT_2A_PIN, OUTPUT);
  pinMode(MOT_2B_PIN, OUTPUT);
  pinMode(MOT_SLEEP_PIN, OUTPUT);
  pinMode(MOT_FAULT_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  //pinMode(SERVO_PIN, OUTPUT);
  pinMode(PPM_PIN, INPUT_PULLUP);
  pinMode(CFG1_PIN, INPUT);
  pinMode(CFG2_PIN, INPUT);

  // Set motors to idle and disable motor driver
  setMotor(0, 0);
  setMotor(1, 0);
  digitalWrite(MOT_SLEEP_PIN, LOW);

  // Set configuration varialbles from config jumpers
  INPUT_MODE = digitalRead(CFG1_PIN);
  MIXING = digitalRead(CFG2_PIN);

  // Flash LED to indicate ready
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(100);

  if (INPUT_MODE) {
    pinMode(SERVO_PIN, INPUT_PULLUP);
    attachInterrupt(PPM_PIN, pwmInterruptA, CHANGE);
    attachInterrupt(SERVO_PIN, pwmInterruptB, CHANGE);
  } else {
    attachInterrupt(PPM_PIN, ppmInterrupt, CHANGE);
  }
}

void loop() {
  // Fault check disabled because it's always comes back positive
  // Driver still operates so the problem is between driver and microcontrollerU
  /*
  // Check for motor driver fault
  if (!digitalRead(MOT_FAULT_PIN)) {
    // If fault detected flash led and exit loop
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    return;
  }
  */

  static bool signalReady = false;
  if (!signalReady) {
    static unsigned int readyCycles = 0;
    if (haveUpdate) readyCycles += 1;
    if (readyCycles > 500) {
      if (!INPUT) {
        // Attach weapon to pin and set to idle
        weapon.attach(SERVO_PIN, 1000, 2000);
        weapon.write(0);
      }

      signalReady = true;
    }
  } else {
    // Check if no signal for a while
    if (abs((int)millis() - lastUpdate) > 200) {
      // No signal so go to failsafe
      
      // Disable LED to indicate receiver disconnected
      digitalWrite(LED_PIN, LOW);
    
      // Disable motor driver
      digitalWrite(MOT_SLEEP_PIN, LOW);
    
      // Set motors to idle
      setMotor(0, 0);
      setMotor(1, 0);
  
      if (!INPUT_MODE) {
        // Set weapon to ilde
        weapon.write(0);
      }
    } else if (haveUpdate) {
      // Still got signal so update motor and weapon outputs
      
      // Enable LED to indicate receiver connected
      digitalWrite(LED_PIN, HIGH);
    
      // Enable motor driver
      digitalWrite(MOT_SLEEP_PIN, HIGH);
    
      // Check if channel mixing is enabled
      if (MIXING) {
        // Mix channels
        int mot1 = min(max(channels[1]-channels[0], -255), 255);
        int mot2 = min(max(channels[1]+channels[0], -255), 255);
        // Set motor speeds
        setMotor(0, mot1);
        setMotor(1, mot2);
      } else {
        // Set motor speeds
        setMotor(0, channels[0]);
        setMotor(1, channels[1]);
      }
  
      if (!INPUT_MODE) {
        // PWM mode
        // Write throttle channel to servo pin for weapon
        weapon.write(map(channels[2], -255, 255, 0, 180));
      }

      haveUpdate = false;
    }
  }
}
