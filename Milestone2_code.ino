    #include <LiquidCrystal_I2C.h>
    LiquidCrystal_I2C lcd(0x27, 20, 4);
    
    enum State{
    idle,
    inflate,
    deflate,
    hold,
    emergency
  };
  State state = idle;

  const int button_pin = 5;
  const int pump_pin = 6;
  const int valve_pin = 7;

  const int stop_pin = 4;

  unsigned long time_state = 0; //test time
  
  const int pressure_t = 150; //Assume pressure threshold
  const int pressure_r = 10; //Assume pressure release threshold
  const unsigned long hold_ms = 4000; //Assume pressure hold time



 int readPressure(){// convert pressure to mmHg
  int adc = analogRead(A0);

  float v = adc * (5.0 / 1023.0);
  float p = 50.0 * v * v;
  return (int)p;// temporary

}
void emergencyState(){
  state = emergency;
  digitalWrite(pump_pin,LOW);
  digitalWrite(valve_pin, HIGH);
}

bool stopBottom(){
  return (digitalRead(stop_pin) == HIGH);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(button_pin, INPUT);
  pinMode(pump_pin, OUTPUT);
  pinMode(valve_pin, OUTPUT);
  pinMode(stop_pin, INPUT);

  Serial.begin(9600);

  //lcd for show the information
  lcd.init();
  lcd.backlight();

  digitalWrite(pump_pin, LOW);
  digitalWrite(valve_pin, LOW);

}

void loop() {
  if(stopBottom()){
    emergencyState();
  }

  if(state == idle){
    digitalWrite(pump_pin, LOW);
    digitalWrite(valve_pin, LOW);

    if(digitalRead(button_pin) == HIGH){
      state = inflate;
    } 
  }
  else if(state == inflate){
    digitalWrite(pump_pin, HIGH);
    digitalWrite(valve_pin, LOW);

    int p = readPressure();
    if(p >= pressure_t){ //Compare Pressure
      state = hold;
      time_state = millis();
    }
  }
  else if(state == hold){
    digitalWrite(pump_pin, LOW);
    digitalWrite(valve_pin, LOW);

    if(millis() - time_state >= hold_ms){
      state = deflate;
    }
  }
  else if(state == deflate){
    digitalWrite(pump_pin, LOW);
    digitalWrite(valve_pin, HIGH);

    int p = readPressure();
    if(p <= pressure_r){ //Compare Pressure
      state = idle;
    }
  }
  else if(state == emergency) {
    digitalWrite(pump_pin, LOW);
    digitalWrite(valve_pin, HIGH);

    if(!stopBottom() && digitalRead(button_pin) == HIGH){
    state = idle;
    }
  }
}
