#include <Wire.h>
#define Limit01 7           // Pin 7 connected to Limit switch home
#define Limit02 6           // Pin 6 connected to Limit switch end
#define ledfwd 12           // led forward
#define ledback 11          // led backward
#define ledvolt 13          // led lowvoltage

// Volt Reader 
float R1        = 100000.0; // resistance of R1 (100K) -see text!, test dulu dengan avometer nilai real ohm resitornya
float R2        = 10000.0;  // resistance of R2 (10K) - see text!, test dulu dengan avometer nilai real ohm resitornya, tambahkan kapasitor 0.22uf - 1uf 50v
int vInputmain  = A4;       // A5 input Amps
float voutmain  = 0.0;
float vinmain   = 0.0;
int voltvalmain = 0;
float inVolt    = 4.49;     // setting nilai ini agar akurasi nilai volt lebih tepat, lebih besar nilai voltase naik dan sebaliknya <<<<---------

// ACS712
int Sens = 66;              // Sensitivity in mV/A for the 30A version
const int ampsIn  = A5;     // Analog input pin
int OffsetVoltage = 2530;   // setting nilai ini agar akurasi nilai ampere lebih tepat, lebih besar nilai ampere turun dan sebaliknya <<<<---------
int RawValue      = 0;      // Init result variables
double Voltage    = 0;         
double Amps       = 0;   

// L298N
int enA   = 8;              // the PWM pin the LED is attached to /enable
int cw    = 10;             // pin in1
int ccw   = 9;              // pin in2

// LDR config
int ldrlt = A0;             // ldr left top
int ldrrt = A1;             // ldr right top
int ldrld = A2;             // ldr left down
int ldrrd = A3;             // ldr right down
int direction;              // Variable to set Rotation (CW-CCW) of the motor

// Tombol
int buttonState1 = 0;
int buttonState2 = 0;
int buttonState3 = 0;
int buttonState4 = 0;
const int buttonPin1  = 2;  // tombol forward
const int buttonPin2  = 3;  // tombol backward
const int buttonTrack = 4;  // tombol Auto direction
const int buttonAir   = 5;  // tombol Auto inv/pln

// millis
unsigned long previousMillis = 0;
const long interval = 10000;

void forward() {      // function motor forward, karena motor dc actuator gk ada encoder terpaksa runningnya di jeda 1 detik :))
    analogWrite(enA, 255);
    digitalWrite (cw, HIGH);
    digitalWrite (ccw, LOW);
    delay(1000);
    analogWrite(enA, 0);
}

void backward() {     // function motor backward
    analogWrite(enA, 255); 
    digitalWrite (cw, LOW);
    digitalWrite (ccw, HIGH);
    delay(1000);
    analogWrite(enA, 0);
}

void ledbackward(){   // indikator led backward
  digitalWrite(ledback, HIGH);
  delay(100);
  digitalWrite(ledback, LOW);
}

void ledforward(){    // indikator led forward
  digitalWrite(ledfwd, HIGH);
  delay(100);
  digitalWrite(ledfwd, LOW);
}

void ledlimit1() {    // indikator led limit home
  digitalWrite(ledback, HIGH);
  delay(100);
  digitalWrite(ledback, LOW);
}

void ledlimit2() {    // indikator led limit end
  digitalWrite(ledfwd, HIGH);
  delay(100);
  digitalWrite(ledfwd, LOW);
}

unsigned long msec = 0;
float time = 0.0;
int sample = 0;
float totalCharge = 0.0;
float averageAmps = 0.0;
float ampSeconds = 0.0;
float ampHours = 0.0;
float wHours = 0.0;

// Variable sendData ke nodemcu
int bState1 = 0;
int bState2 = 0;
int bState3 = 0;
int bState4 = 0;
int bLimit01 = 0;
int bLimit02 = 0;
int binv = 0;
int bdvert = 0;
float watts  = 0.0;
int breset = 0;

void setup() {
   pinMode(enA, OUTPUT);
   pinMode(ldrlt, INPUT);
   pinMode(ldrrt, INPUT);
   pinMode(ldrld, INPUT);
   pinMode(ldrrd, INPUT);
   pinMode(Limit01, INPUT);
   pinMode(Limit02, INPUT);
   pinMode(buttonPin1, INPUT);
   pinMode(buttonPin2, INPUT);
   pinMode(buttonTrack, INPUT);
   pinMode(buttonAir, INPUT);
   pinMode(vInputmain, INPUT);
   pinMode(ampsIn, INPUT);
   pinMode(cw, OUTPUT);
   pinMode(ccw, OUTPUT);
   pinMode(ledfwd, OUTPUT);
   pinMode(ledback, OUTPUT);
   pinMode(ledvolt, OUTPUT);
   Serial.begin(9600);
}

void loop() {

  voltreadmain();
  AmpsRead();  
  watts = Amps * vinmain;
  sample = sample + 1;
  msec = millis();
  time = (float) msec / 1000.0;
  totalCharge = totalCharge + Amps;
  averageAmps = totalCharge / sample;
  ampSeconds = averageAmps*time;
  ampHours = ampSeconds/3600;
  wHours = vinmain * ampHours;
  unsigned long currentMillis = millis();
  
  int lt = analogRead(ldrlt);                 // top left
  int rt = analogRead(ldrrt);                 // top right 
  int ld = analogRead(ldrld);                 // down left
  int rd = analogRead(ldrrd);                 // down right
  int tol = 25;                               // setting nilai toleransi ini jika posisi panel tidak sejajar dengan matahari, nilai lebih besar panel akan mendahului mahatari.
  int avt = (lt + rt) / 2;                    // average top value
  int avd = (ld + rd) / 2;                    // average down value
  int dvert = avt - avd;                      // check the top/down diference
  buttonState1 = digitalRead(buttonPin1);     // tombol forward
  buttonState2 = digitalRead(buttonPin2);     // tombol backward
  buttonState3 = digitalRead(buttonTrack);    // tombol auto/manual
  buttonState4 = digitalRead(buttonAir);      // tombol radar auto/manual

  if (buttonState3 == HIGH) {
      bState3 = 1;                          // send data ke nodemcu direction Auto
        if (-1*tol > dvert || dvert > tol){
          if (avt > avd){
            if (digitalRead(Limit02)) {}
            else {
                  ledforward();
                  forward(); // step motor forward
             }
          }
          else if (avt < avd){
            if (digitalRead(Limit01)) {}
            else {
                  ledbackward();
                  backward();
            }
          }
        }
  }
  else{
      bState3 = 0;                          // send data ke nodemcu direction Manual
      if (buttonState1 == HIGH){            // fungsi tombol forward
          if (digitalRead(Limit02)) {}
          else {
                bState1 = 1;
                ledforward();
                forward();
          }
      }
      else{
        bState1 = 0;
      }
      if (buttonState2 == HIGH){            // fungsi tombol backward
          if (digitalRead(Limit01)) {}
          else {
                bState2 = 1;
                ledbackward();    
                backward();
         }
      }
      else{
        bState2 = 0;
      }
  }   

  if (digitalRead(Limit01)){                  // limit backward
      bLimit01 = 1;                           // send data ke nodemcu limit home ON
      ledlimit1();                            // led limit menyala
  }
  else{
      bLimit01 = 0;
  }
  
  if (digitalRead(Limit02)){                  // limit forward 
      bLimit02 = 1;                           // send data ke nodemcu limit end ON
      ledlimit2();                            // led limit menyala
  }
  else{
      bLimit02 = 0;
  }
  
  if (dvert == 0){
      bdvert = 1;                             // send data ke nodemcu panel sejajar matahari
      digitalWrite(ledfwd, HIGH);
      digitalWrite(ledback, HIGH);
  }
  else{
      bdvert = 0;
      digitalWrite(ledfwd, LOW);
      digitalWrite(ledback, LOW);      
  }

   if(buttonState4 == HIGH){
      bState4 = 1;                            // send data ke nodemcu auto ON
     // setting millis
     if(currentMillis - previousMillis >= interval)
       {
        // update previousMillis
        previousMillis = currentMillis;
        if(Amps >= 5.00){
           binv = 1;                          // send data ke nodemcu inverter ON
        }
        else{
           binv = 0;                          // send data ke nodemcu inverter OFF                                                
        }
       }                               
    }
    else{
      bState4 = 0;                            // send data ke nodemcu auto OFF
      binv = 0;                               // send data ke nodemcu inverter OFF
    }

    if(buttonState1 == HIGH && buttonState2 == HIGH){
      breset = 1;
    }
    else{
      breset = 0;
    }
  // baca permintaan dari Esp
  String minta = "";
  // baca permintaan Esp
  while(Serial.available()>0)
       {
        minta += char(Serial.read());
       }      
        //buang spasi data yang diterima
        minta.trim();
        //uji variable minta
        if(minta == "Ya")
        {
          // kirim data
          SendData();
        }
        minta = "";
        delay(1000); 
}

void voltreadmain()           // fungsi read voltase
{
  voltvalmain = analogRead(vInputmain);
  voutmain = (voltvalmain * inVolt) / 1024; // 4.55) / 1024; 
  vinmain = voutmain / (R2 / (R1 + R2));
  if (vinmain < 0.09) {
      vinmain = 0.00; 
  }
  if (vinmain <= 24.50){
     digitalWrite(ledvolt, HIGH);
  }
  else{
     digitalWrite(ledvolt, LOW);
  }
}

void AmpsRead()                               // fungsi read ampere
{
  RawValue = analogRead(ampsIn);              // Read voltage from ASC712
  Voltage = (RawValue / 1024.0) * 5000;       // Convert to mV
  Amps = ((Voltage - OffsetVoltage) / Sens);  // Convert to amps
  if(Amps <= 0.10){
  Amps     = 0.00;
  }     
}

void SendData()
{
  int lt = analogRead(ldrlt);     // top left
  int rt = analogRead(ldrrt);     // top right 
  int ld = analogRead(ldrld);     // down left
  int rd = analogRead(ldrrd);     // down right

  String kirimData = 
  String(lt)+"#"+       // nilai ldr
  String(rt)+"#"+       // nilai ldr
  String(ld)+"#"+       // nilai ldr
  String(rd)+"#"+       // nilai ldr      
  String(bLimit01)+"#"+ // limit home
  String(bLimit02)+"#"+ // limit end
  String(bState3)+"#"+  // Button Auto Direction
  String(bState4)+"#"+  // Button Auto Air    
  String(bdvert)+"#"+   // posisi panel sejajar matahari
  String(binv)+"#"+     // kondisi inverter
  String(vinmain)+"#"+  // nilai volt
  String(Amps)+"#"+     // nilai ampere
  String(watts)+"#"+    // nilai watt
  String(breset)+"#"+   // Button Clear LCD
  String(wHours)+"#"+   // nilai watt hour
  String(ampHours)+"#"; // nilai ampere hour                     

  Serial.println(kirimData);
}
