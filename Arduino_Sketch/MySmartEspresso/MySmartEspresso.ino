/*
My Smart Espresso
Arduino Sketch
v: 0.1

Description:
Simply read T using 100k Thermostor and display values in LCD through serial port


The circuit:
Arduino MEGA2560
LCD display (16x2): NHD-0216K3Z-NSW-BBW
Voltage divider between:
100k Resistor 
100k Thermistor

Arduino TX (pin 18) to NHD RX (pin 1)
Arduino GND to NHD VSS (pin 2)
Arduino 5V to NHD VDD (pin 3)
Arduino GND to 100k
Arduino 5V to Thermistor
Arduino A0 to common 100k and thermistor leg

v0.1: send a simple text message

*/
//Define general constants
const float pi = 3.141593;
const float P_FS = 12; //max pressure scale 
const float V_P_FS = 4.5; //voltage at 12 Bar(g)
const float V_P_Zero = 0.47; //voltage at 0 Bar(g) 
const int F = 60; //mains frequency (50 or 60Hz)
const int n_cycles = 50; //number of cycles per period of control


//Declare variables
int ThermistorPin = 0;
int PressurePin = 1;
int SSRPin = 2;
int Vo;
int AR1, P1;
float m1, n1; //proportional constants to calculate P
float R1 = 10000;
float logR2, R2, T;
float T_float_lpf, T_float_old=20;
float V1;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
bool heating = false;

//timer variables
unsigned long elapsetime = 0;
unsigned long previousMillis = 0;
long t_sample = 500; // in millisecons

unsigned long SSRmillis = 0;
unsigned long SSRelapsed = 0;
unsigned long SSRprevious = 0;
float SSRperiod = (float)n_cycles/(float)F*1000.0; //100 x period in milliseconds
float SSROnTime = 0; //percent of period that SSR is ON

//Declare variables for digital low pass filter (two chanels)
float dt = (float)t_sample/1000; //interval in s for low pass filter
float fc1 = 0.05;
float RC1, alpha1;

//declare and initialize variables for serial comm:
String inputString = "";
//String textString = "";
boolean stringComplete = false;
String SSR_SP = "";

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600); //to Serial Monitor
  Serial1.begin(9600); //to LCD
  Serial2.begin(9600); //to BT HC-06

  Serial1.write(0xFE);
  Serial1.write(0x41);//Turn On
  Serial1.write(0xFE);
  Serial1.write(0x46);//cursor home
  Serial1.print("Temperature (C): ");

  //measure initial T to initialize low pass filter variables
  //Calculate R for thermistor from voltage divider
  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T_float_old = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T_float_old = T_float_old - 273.15;

  //calculate linear eq for pressure
  m1 = P_FS/(V_P_FS-V_P_Zero);
  n1 = -m1*V_P_Zero;

  //Set SSR pin mode to out
  pinMode(SSRPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  /*
   * SERIAL COMM EVENTS
   */
  
  if(stringComplete){
    stringComplete = false; //reset input string
    SSR_SP = inputString;
    SSROnTime = SSR_SP.toFloat();
    SSROnTime = constrain(SSROnTime, 0.0, 100.0);
    Serial.print("SP= ");
    Serial.println(SSROnTime);

    //reset inputString
    inputString = "";
  }
  
  /*
   * TIMER EVENTS
   */
  //Timer events that happens based on mains period
  SSRmillis = millis(); //current program millis
  SSRelapsed = SSRmillis - SSRprevious;
  
  if(SSRelapsed >= SSRperiod){
    //the control period, 10x the mains period is up
    //reset counter
    SSRprevious = SSRmillis;
  }
  if(SSRelapsed >= SSROnTime*SSRperiod/100){
    //turn off SSR;
    digitalWrite(SSRPin, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    heating = false;
    //Serial.println(" OFF");
  }
  else{
    digitalWrite(SSRPin, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    heating = true;
    //Serial.println(" ON");
  }


  //Timer events that happens every time t_sample is up:
  unsigned long currentMillis = millis();
  elapsetime = currentMillis - previousMillis;
  if(elapsetime >= t_sample){
    //refresh time is 0.5s
    
    Serial1.write(0xFE);
    Serial1.write(0x51);//clear screen comand
    
    //Read analog pins
    Vo = analogRead(ThermistorPin);
    AR1 = analogRead(PressurePin);
  
    Serial.print("Vo :");
    //Serial.print(Vo);
  
    Serial.print(", A1 :");
    //Serial.print(AR1);
  
    //Calculate R for thermistor from voltage divider
    R2 = R1 * (1023.0 / (float)Vo - 1.0);
    Serial.print(", R2 :");
    //Serial.print(R2);
    
    logR2 = log(R2);
    T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
    T = T - 273.15;
    Serial.print(T);
    
    //Apply low pass filter to T
    RC1 = 1/(2*pi*fc1);
    alpha1 = dt/(dt+RC1);
    T_float_lpf = alpha1*T + (1-alpha1)*T_float_old; //value for T after applying low pass filter
    T_float_old = T_float_lpf;
    T = T_float_lpf;
    
    Serial.print(", Temperature: "); 
    Serial.print(T);
    Serial.print(" C"); 
  
    //Calculate Voltage for pressure:
    V1 = float(AR1)/1023.0*5.0;
    //Calculate Pressure:
    P1 = m1*V1 +n1;
    Serial.print(", V1:");
    Serial.print(V1);
    Serial.print(", P1:");
    Serial.print(P1);
    Serial.println(" Bar_g");
    
    //Print to LCD:
    Serial1.write(0xFE);
    Serial1.write(0x51);//cursor blink on
    Serial1.print(" T= ");
    Serial1.print(T);
    Serial1.print(" degC");
    Serial1.write(0xFE);//move cursor to second line
    Serial1.write(0x45);
    Serial1.write(0x28);
    Serial1.print("P= ");
    Serial1.print(P1);
    Serial1.print(" bar_g");
  
    //Print to BT:
    Serial2.print(T);
    Serial2.print(" ");
    Serial2.println(P1);
  
    //Mirror BT to Serial monitor
    if (Serial2.available()){
      Serial.write(Serial2.read());
    }
    
    //read from Serial monitor and write to BT
    //if (Serial.available()){
     // Serial2.write(Serial.read());
    //}
  
  
    //SSR heater output
    /*
    if(!heating){
      digitalWrite(SSRPin, HIGH);
      digitalWrite(LED_BUILTIN, HIGH);
      heating = true;
    }
    else {
      digitalWrite(SSRPin, LOW);
      digitalWrite(LED_BUILTIN, LOW);
      heating = LOW;
    }*/
    
    
  
    //delay(500);
    
  
    
    //reset timer
    previousMillis = currentMillis;
  }
}


void serialEvent(){
  while (Serial.available()){
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n'){
      stringComplete = true;
    }
  }
}
