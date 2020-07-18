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

void ppmInterrupt (bool val) {
  static char counter = 0;
  static int startMicros = 0;
  
  //bool val = digitalRead(PPM_PIN);
  if (val) {
    startMicros = micros();
  } else {
    int duration = constrain(abs((int)micros() - startMicros), PPM_MIN_PULSE, PPM_MAX_PULSE);
    if (duration > PPM_MIN_SYNC) {
      counter = 0;
    } else {
      channels[counter] = map(duration, PPM_MIN_PULSE, PPM_MAX_PULSE, -255, 255);
      counter += 1;
    }
  }
}

void pwmInterrupt (bool channel, bool val) {
  static int startMicrosA = 0;
  static int startMicrosB = 0;

  if (channel == 0) {
    if (val) {
      startMicrosA = micros();
    } else {
      int duration = constrain(abs((int)micros() - startMicrosA), PWM_MIN_PULSE, PPM_MAX_PULSE);
      channels[0] = map(duration, PWM_MIN_PULSE, PWM_MAX_PULSE, -255, 255);
    }
  } else {
    if (val) {
      startMicrosB = micros();
    } else {
      int duration = constrain(abs((int)micros() - startMicrosB), PWM_MIN_PULSE, PPM_MAX_PULSE);
      channels[1] = map(duration, PWM_MIN_PULSE, PWM_MAX_PULSE, -255, 255);
    }
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
      analogWrite(MOT_1A_PIN, 0);
      digitalWrite(MOT_1B_PIN, LOW);
    }
  } else {
    if (value > 0+DEADZONE) {
      analogWrite(MOT_2A_PIN, abs(value));
      digitalWrite(MOT_2B_PIN, LOW);
    } else if (value < 0-DEADZONE) {
      analogWrite(MOT_2A_PIN, 255-abs(255-value));
      digitalWrite(MOT_2B_PIN, HIGH);
    } else {
      analogWrite(MOT_2A_PIN, 0);
      digitalWrite(MOT_2B_PIN, LOW);
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
  pinMode(MOT_FAULT_PIN, INPUT_PULLUP);
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

  if (INPUT_MODE) {
    // Enable servo pin as input if in PWM mode
    pinMode(SERVO_PIN, INPUT_PULLUP);
  } else {
    // Attach weapon to pin and set to idle
    weapon.attach(SERVO_PIN, 1000, 2000);
    weapon.write(0);
  }

  // Flash LED to indicate ready
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  
  //attachInterrupt(PPM_PIN, pinChanged, CHANGE);
}

void loop() {
  bool haveUpdate = false;
  static int lastUpdate = 0;
  
  if (INPUT_MODE) {
    // PWM Mode
    static bool lastValA = HIGH;
    static bool lastValB = HIGH;
    
    // Check if PWM pin A has changed
    bool newValA = digitalRead(PPM_PIN);
    if (newValA != lastValA) {
      // Update variables
      pwmInterrupt(0, newValA);
      lastUpdate = millis();
      lastValA = newValA;
      haveUpdate = true;
    }

    // Check if PWM pin B has changed
    bool newValB = digitalRead(SERVO_PIN);
    if (newValB != lastValB) {
      // Update variables
      pwmInterrupt(1, newValB);
      lastUpdate = millis();
      lastValB = newValB;
      haveUpdate = true;
    }
  } else {
    // PPM Mode
    static bool lastVal = HIGH;
    
    // Check if PPM pin has changed
    bool newVal = digitalRead(PPM_PIN);
    if (newVal != lastVal) {
      // Update variables
      ppmInterrupt(newVal);
      lastUpdate = millis();
      lastVal = newVal;
      haveUpdate = true;
    }
  }

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
      // Write throttle channel to servo pin for weapon
      weapon.write(map(channels[2], -255, 255, 0, 180));
    }
  }
}
