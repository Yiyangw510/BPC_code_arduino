// #include <LiquidCrystal_I2C.h>
// LiquidCrystal_I2C lcd(0x27, 20, 4);

// #define test

#ifdef test
#define LOOP_TEST loop
#else
#define LOOP_MAIN loop
#endif

#define SENSOR_PIN A0
#define PULSE_PIN A1
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


#define MMHG100ADC 512
#define MMHG0ADC 204

float readPressure() {
  float adc = analogRead(SENSOR_PIN);
  return (adc - MMHG0ADC)*(100.0/(MMHG100ADC-MMHG0ADC));
}

// read pulse amplitude, centered by zero
int readPulse() {
  return int(analogRead(PULSE_PIN));
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

#define MAX_PULSE_RECORD 50
#define INFLATE_MAX_MMHG 100
#define DEFLATE_TIME 23000

// for filtering noise
#define PULSE_MAX_NOISE 5
// ignore first N pulse cycles
#define IGNORE_N_PULSE 1

void LOOP_MAIN() {
  // set pump
  Serial.println("pumping");
  while (readPressure() < INFLATE_MAX_MMHG)  // inflate
    delay(10);
  // stop pump
  // while (readPressure() > INFLATE_MAX_MMHG - 10)  // deflate
  //   delay(10);
  int i = 0;                         // pulse number index
  float pressure[MAX_PULSE_RECORD];  // all pulse pressures
  int pulse[MAX_PULSE_RECORD];       // all pulse amplitude
  {                                  // record all pulses within DEFLATE_TIME
    Serial.println("====start=====");
    int a_1 = readPulse();
    unsigned long start_time;
    start_time = millis();
    while ((millis() < start_time + DEFLATE_TIME) && i < MAX_PULSE_RECORD) {
      // for release time or pulse record overflow
      // start of a pulse
      Serial.print("pulse #");
      Serial.print(i);
      int min_pulse = 1024;
      int max_pulse = 0;
      float peak_pressure = 0;
      {
        float p;
        int a;
        while (millis() < start_time + DEFLATE_TIME) {  // decreasing
          p = readPressure();
          a = readPulse();
          if (a < min_pulse)
            min_pulse = a;
          if (a - a_1 > PULSE_MAX_NOISE) {  // pulse increases
            a_1 = a;
            break;
          } else if (a - a_1 < -PULSE_MAX_NOISE)  // pulse decreases
            a_1 = a;
        }
        while (millis() < start_time + DEFLATE_TIME) {  // increasing
          p = readPressure();
          a = readPulse();
          if (a > max_pulse) {
            max_pulse = a;
            peak_pressure = p;
          }
          if (a - a_1 > PULSE_MAX_NOISE) {  // pulse increases
            a_1 = a;
          } else if (a - a_1 < -PULSE_MAX_NOISE) {  // pulse decreases
            a_1 = a;
            break;
          }
        }
      }
      if (millis() >= start_time + DEFLATE_TIME)
        break;
      pressure[i] = peak_pressure;
      pulse[i] = max_pulse - min_pulse;
      Serial.print(", amplitude:");
      Serial.print(pulse[i]);
      Serial.print(", preasure:");
      Serial.println(pressure[i]);
      i++;
    }
  }
  float MAP=0, DBP=0, SBP=0;
  Serial.println("====result=====");
  {
    int max_pulse_diff = 0;
    int MAP_index = 0;
    for (int j = IGNORE_N_PULSE; j < i; j++) {  // find MAP
      if (pulse[j] > max_pulse_diff) {          // when pulse is max
        max_pulse_diff = pulse[j];
        MAP_index = j;
      }
    }
    MAP = pressure[MAP_index];
    Serial.print("MAP: pulse #");
    Serial.println(MAP_index);
    int target = 0.5 * max_pulse_diff;//SBP
    int min_diff = 1024;
    for (int j = IGNORE_N_PULSE; j < MAP_index; j++) {  // find SBP
      int a = pulse[j];
      int SBP_pulse_diff = abs(a - target);
      if (SBP_pulse_diff < min_diff) {
        min_diff = SBP_pulse_diff;
        // SBP = pressure[j];
        SBP = pressure[j] + (pressure[j + 1] - pressure[j]) *
              (target - pulse[j]) / (pulse[j + 1] - pulse[j]);
      }
    }
    target = 0.8 * max_pulse_diff;//DBP
    min_diff = 1024;
    for (int j = MAP_index; j < i; j++) {  // find DBP
      int a = pulse[j];
      int DBP_pulse_diff = abs(a - target);
      if (DBP_pulse_diff < min_diff) {
        min_diff = DBP_pulse_diff;
        // DBP = pressure[j];
        DBP = pressure[j] + (pressure[j + 1] - pressure[j]) *
              (target - pulse[j]) / (pulse[j + 1] - pulse[j]);
      }
    }
  }
  Serial.print("MBP: ");
  Serial.println(MAP);
  Serial.print("DBP: ");
  Serial.println(DBP);
  Serial.print("SBP: ");
  Serial.println(SBP);
  while (true)
    ;
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
