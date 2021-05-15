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
int ThermistorPin = 0;
int PressurePin = 1;
int Vo;
int AR1;
float R1 = 10000;
float logR2, R2, T;
float V1;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600); //to Serial Monitor
  Serial1.begin(9600); //to LCD

  Serial1.write(0xFE);
  Serial1.write(0x41);//Turn On
  Serial1.write(0xFE);
  Serial1.write(0x46);//cursor home
  Serial1.print("Temperature (C): ");
}

void loop() {
  //Read analog pins
  Vo = analogRead(ThermistorPin);
  AR1 = analogRead(PressurePin);

  Serial.print("Vo :");
  Serial.print(Vo);

  Serial.print(", A1 :");
  Serial.print(AR1);

  //Calculate R for thermistor from voltage divider
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  Serial.print(", R2 :");
  Serial.print(R2);
  
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  
  Serial.print(", Temperature: "); 
  Serial.print(T);
  Serial.print(" C"); 

  //Calculate Voltage for pressure:
  V1 = float(AR1)/1023.0*5.0;
  Serial.print(", V1:");
  Serial.println(V1);
  
  //Print to LCD:
  Serial1.write(0xFE);
  Serial1.write(0x51);//cursor blink on
  Serial1.print(" Temperature (C): ");
  Serial1.write(0xFE);//move cursor to second line
  Serial1.write(0x45);
  Serial1.write(0x28);
  Serial1.print(T);
  

  delay(500);
  Serial1.write(0xFE);
  Serial1.write(0x51);//clear screen comand
}
