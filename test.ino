// #include <LiquidCrystal_I2C.h>
// LiquidCrystal_I2C lcd(0x27, 20, 4);

#define SENSOR_PIN A0
#define PULSE_PIN A1
#define BUTTON_PIN 5
#define PUMP_PIN 6
#define VALVE_PIN 7
#define STOP_PIN 4

#define MMHG200ADC (1024.0 * 0.8)
#define MMHG0ADC (1024.0 * 0.2)

float readPressure() {
  float adc = analogRead(SENSOR_PIN);
  return (adc - MMHG0ADC) * (200.0 / (MMHG200ADC - MMHG0ADC));
}

// read pulse amplitude, centered by zero
int readPulse() {
  return int(analogRead(PULSE_PIN));
}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(STOP_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  //lcd for show the information
  // lcd.init();
  // lcd.backlight();

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);
}

#define MAX_PULSE_RECORD 50
#define INFLATE_MAX_MMHG 150.0
#define DEFLATE_TIME 30000

// for filtering noise
#define PULSE_MAX_NOISE 20
// ignore first N pulse cycles
#define IGNORE_N_PULSE 1

void loop() {
  while (digitalRead(BUTTON_PIN) != LOW)  // wait for button
    delay(10);
  // set pump
  digitalWrite(PUMP_PIN, HIGH);
  digitalWrite(VALVE_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("pumping");
  while (readPressure() < INFLATE_MAX_MMHG)  // inflate
    delay(10);
  Serial.println("stop pump");
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);
  delay(100);
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
  float MAP = 0, DBP = pressure[i-1], SBP = pressure[IGNORE_N_PULSE];
  Serial.println("====result=====");
  {
    int max_pulse = 0;
    int MAP_index = 0;
    for (int j = IGNORE_N_PULSE; j < i; j++) {  // find MAP
      if (pulse[j] > max_pulse) {               // when pulse is max
        max_pulse = pulse[j];
        MAP_index = j;
      }
    }
    MAP = pressure[MAP_index];
    Serial.print("MAP: pulse #");
    Serial.println(MAP_index);
    float target = 0.5 * max_pulse;  //SBP
    for (int j = IGNORE_N_PULSE; j < MAP_index; j++) {  // find SBP
      float pulse0 = pulse[j];
      float pulse1 = pulse[j + 1];
      if (pulse0 <= target && target <= pulse1) {
        float pressure0 = pressure[j];
        float pressure1 = pressure[j + 1];
        SBP = pressure0 + ((pressure1 - pressure0) * ((target - pulse0)/(pulse1 - pulse0)));
        break;
      }
    }
    target = 0.8 * max_pulse;  //DBP
    for (int j = MAP_index; j < i; j++) {  // find DBP
      float pulse0 = pulse[j];
      float pulse1 = pulse[j + 1];
      if (pulse0 >= target && target >= pulse1) {
        float pressure0 = pressure[j];
        float pressure1 = pressure[j + 1];
        DBP = pressure0 + ((pressure1 - pressure0) * ((target - pulse0)/(pulse1 - pulse0)));
        break;
      }
    }
  }
  Serial.print("MAP: ");
  Serial.println(MAP);
  Serial.print("DBP: ");
  Serial.println(DBP);
  Serial.print("SBP: ");
  Serial.println(SBP);
  // while (true)
  //   ;
}
