#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); //detect mapping address

enum State{
    idle,
    inflate,
    deflate,
    hold,
    emergency,
    done
};
State state = idle;

int pressure = 0;

//convert state to string
const char* statePrint(State s){
  switch(s){
    case idle: return "IDLE";
    case inflate: return "INFLATE";
    case hold: return "HOLD";
    case deflate: return "DEFLATE";
    case emergency: return "EMERGENCY!!!";
  }
}

void setup(){
  lcd.init();
  lcd.backlight();
  lcd.clear();
}


void display(int d, State s){
  lcd.setCursor(0,0);
  lcd.print("Pressure = ");
  lcd.print(d);
  lcd.print(" mmHg");

  lcd.setCursor(0,1);
  lcd.print("State: ");
  lcd.print(statePrint(s));
}

void loop(){
  int p = readPressure();
  lcd.setCursor(0,0);
  lcd.print("Pressure = ");
  lcd.print(p);
  lcd.print(" mmgh");

  lcd.setCursor(0,1);
  lcd.print("State: ");
  lcd.print(statePrint(s));
}
