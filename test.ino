// #include <LiquidCrystal_I2C.h>
// LiquidCrystal_I2C lcd(0x27, 20, 4);

#define test

#ifdef test
#define LOOP_TEST loop
#else
#define LOOP_MAIN loop
#endif

#define SENSOR_PIN A0
#define BUTTON_PIN 5

enum State {
  idle,
  inflate,
  deflate,
  hold,
  emergency
};
State state = idle;

const int pump_pin = 6;
const int valve_pin = 7;

const int stop_pin = 4;

unsigned long time_state = 0;  //test time

const int pressure_t = 150;          //Assume pressure threshold
const int pressure_r = 10;           //Assume pressure release threshold
const unsigned long hold_ms = 4000;  //Assume pressure hold time


#define MMHG100ADC 508
#define MMHG0ADC 257

float readPressure() {
  int adc = analogRead(SENSOR_PIN);
  constexpr float k = 100.0 / (MMHG100ADC - MMHG0ADC);
  constexpr float h = MMHG0ADC * k;
  return adc * k - h;
}

void emergencyState() {
  state = emergency;
  digitalWrite(pump_pin, LOW);
  digitalWrite(valve_pin, HIGH);
}

bool stopBottom() {
  return (digitalRead(stop_pin) == LOW);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(pump_pin, OUTPUT);
  pinMode(valve_pin, OUTPUT);
  pinMode(stop_pin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  //lcd for show the information
  // lcd.init();
  // lcd.backlight();

  digitalWrite(pump_pin, LOW);
  digitalWrite(valve_pin, LOW);
}

void LOOP_TEST() {
  float p = readPressure();
  // Serial.println(p);
  delay(50);
  switch (state) {
    case idle:
      if (digitalRead(BUTTON_PIN) == LOW) {
        digitalWrite(pump_pin, HIGH);
        digitalWrite(valve_pin, HIGH);
        digitalWrite(LED_BUILTIN, HIGH);
        state = inflate;
      }
    case (inflate):
      if (p > 150) {
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(pump_pin, LOW);
        digitalWrite(valve_pin, LOW);
        state = idle;
      }
  }
}
