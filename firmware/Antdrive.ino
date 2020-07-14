//#include <EEPROM.h>

#define MOT_1A_PIN PA7
#define MOT_1B_PIN PA2
#define MOT_2A_PIN PA6
#define MOT_2B_PIN PA3
#define MOT_SLEEP_PIN PA1
#define MOT_FAULT_PIN PA5
#define LED_PIN PA0
#define SERVO_PIN PA4
#define PPM_PIN PB1

#define DEADZONE 10
#define MIXING

#define CHANNELS 8
#define MIN_SYNC 5000
#define MIN_PULSE 600
#define MAX_PULSE 1600
int channels[CHANNELS];

volatile char counter = 0;
volatile int startMicros = 0;
static int lastUpdate = 0;

void pinChanged (bool val) {
  //bool val = digitalRead(PPM_PIN);
  lastUpdate = millis();
  digitalWrite(LED_PIN, HIGH);
  if (val) {
    startMicros = micros();
  } else {
    int duration = abs((int)micros() - startMicros);
    if (duration > MIN_SYNC) {
      counter = 0;
    } else {
      channels[counter] = map(duration, MIN_PULSE, MAX_PULSE, -255, 255);
      counter += 1;
    }
  }
}

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

uint32_t channel;

void setup() {
  //pinMode(MOT_1A_PIN, OUTPUT);
  pinMode(MOT_1B_PIN, OUTPUT);
  //pinMode(MOT_2A_PIN, OUTPUT);
  pinMode(MOT_2B_PIN, OUTPUT);
  pinMode(MOT_SLEEP_PIN, OUTPUT);
  pinMode(MOT_FAULT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(PPM_PIN, INPUT_PULLUP);

  setMotor(0, 0);
  setMotor(1, 0);
  digitalWrite(MOT_SLEEP_PIN, HIGH);

  for (int i = 0; i < CHANNELS; i++) {
    channels[CHANNELS] = 0;
  }

  /*int address = 0;
  EEPROM.get(address, MIN_PULSE);
  address += sizeof(int);
  EEPROM.get(address, MAX_PULSE);*/
  

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
  static bool lastVal = HIGH;
  bool newVal = digitalRead(PPM_PIN);
  if (newVal != lastVal) {
    pinChanged(newVal);
    lastVal = newVal;

    #ifdef MIXING
      int mot1 = min(max(channels[1]-channels[0], -255), 255);
      int mot2 = min(max(channels[1]+channels[0], -255), 255);
      setMotor(0, mot1);
      setMotor(1, mot2);
    #else
      setMotor(0, channels[0]);
      setMotor(1, channels[1]);
    #endif
  }

  if (abs((int)millis() - lastUpdate) > 200) {
    digitalWrite(LED_PIN, LOW);
    setMotor(0, 0);
    setMotor(1, 0);

    for (int i = 0; i < CHANNELS; i++) {
      channels[CHANNELS] = 0;
    }
  }
}
