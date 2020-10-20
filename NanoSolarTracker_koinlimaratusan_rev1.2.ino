#include <Wire.h>
#define LHigh 6             // Pin 6 connected to Limit switch home/low
#define LLow 5              // Pin 5 connected to Limit switch end/high
#define ledfwd 12           // led forward
#define ledback 10          // led backward
#define ledcenter 11        // led center/auto-manual

// L298N
int enA   = 7;              // the PWM pin the LED is attached to /enable
int IN1   = 8;              // pin in1
int IN2   = 9;              // pin in2

// LDR config
int ldrlt = A1;             // ldr left top
int ldrrt = A0;             // ldr right top
int ldrld = A2;             // ldr left down
int ldrrd = A3;             // ldr right down

// Tombol
int buttonState1 = 0;
int buttonState2 = 0;
int buttonState3 = 0;
const int btnfwd  = 3;      // tombol forward
const int btnrwd  = 2;      // tombol backward
const int btnauto = 4;      // tombol Auto direction

//####################################################### SILAKAN SESUAIKAN CONFIG ########################################################
// DelayMotor | nilai 1 detik disesuaikan dengan actuator parabola, untuk actuator/motor lain silakan setting sendiri.                    #
int Mdelay = 1000;//                                                                                                                      #
// Toleransi LDR, jika nilai LDR melewati angka tol atau -tol maka motor akan bergerak susai nilai dvert.                                 #
int tol = 15;//                                                                                                                           #
// LDR Correction | board LDR ada miss dengan desain yg sebelumnya, samakan nilai sensor LDR sebelum dipasang disisi panel.               #
int ltcor = 74;             // nilai 74 sudah dicek dan sesuai dengan board ldr, semakin tinggi nilai LDR akan berkurang                  #
int rtcor = 0;              // semakin tinggi nilai LDR akan berkurang                                                                    #
int ldcor = 0;              // semakin tinggi nilai LDR akan berkurang                                                                    #
int rdcor = 0;              // semakin tinggi nilai LDR akan berkurang                                                                    #  
//####################################################### SILAKAN SESUAIKAN CONFIG ########################################################                                                                                                                                            

void forward() {                  // function motor forward, karena motor dc actuator tdk ada encoder terpaksa runningnya di jeda 1 detik :))
    analogWrite(enA, 255);
    digitalWrite (IN1, HIGH);
    digitalWrite (IN2, LOW);
    delay(Mdelay);            
    analogWrite(enA, 0);
}

void backward() {                 // function motor backward
    analogWrite(enA, 255); 
    digitalWrite (IN1, LOW);
    digitalWrite (IN2, HIGH);
    delay(Mdelay);
    analogWrite(enA, 0);
}

void ledbackward(){               // indikator led backward
  digitalWrite(ledback, HIGH);
  delay(100);
  digitalWrite(ledback, LOW);
}

void ledforward(){                // indikator led forward
  digitalWrite(ledfwd, HIGH);
  delay(100);
  digitalWrite(ledfwd, LOW);
}

void ledlimit1() {                // indikator led limit home
  digitalWrite(ledback, HIGH);
  delay(100);
  digitalWrite(ledback, LOW);
}

void ledlimit2() {                // indikator led limit end
  digitalWrite(ledfwd, HIGH);
  delay(100);
  digitalWrite(ledfwd, LOW);
}

void setup() {
   pinMode(enA, OUTPUT);
   pinMode(ldrlt, INPUT);
   pinMode(ldrrt, INPUT);
   pinMode(ldrld, INPUT);
   pinMode(ldrrd, INPUT);
   pinMode(LHigh, INPUT);
   pinMode(LLow, INPUT);
   pinMode(btnfwd, INPUT);
   pinMode(btnrwd, INPUT);
   pinMode(btnauto, INPUT);
   pinMode(IN1, OUTPUT);
   pinMode(IN2, OUTPUT);
   pinMode(ledfwd, OUTPUT);
   pinMode(ledback, OUTPUT);
   pinMode(ledcenter, OUTPUT);
   Serial.begin(9600);
}

void loop() {
  
  int lt = (1023-ltcor)-analogRead(ldrlt);    // top left
  int rt = (1023-rtcor)-analogRead(ldrrt);    // top right 
  int ld = (1023-ldcor)-analogRead(ldrld);    // down left
  int rd = (1023-rdcor)-analogRead(ldrrd);    // down right   
  int avt = (lt + rt) / 2;                    // average top value
  int avd = (ld + rd) / 2;                    // average down value
  int dvert = avt - avd;                      // check the top/down diference
  buttonState1 = digitalRead(btnfwd);         // tombol forward
  buttonState2 = digitalRead(btnrwd);         // tombol backward
  buttonState3 = digitalRead(btnauto);        // tombol auto/manual

      Serial.print(lt); 
      Serial.print("+");
      Serial.print(rt);
      Serial.print("/2=");
      Serial.print(avt);
      Serial.print("\t");
      Serial.print(ld);
      Serial.print("+");
      Serial.print(rd);
      Serial.print("/2=");
      Serial.print(avd);
      Serial.print("\t""\t");
      Serial.print("dvert="); 
      Serial.println(dvert);
      
  if (buttonState3 == HIGH) {
      digitalWrite(ledcenter, LOW);           // led center OFF
      if(avt <= 900 || avd <= 900){           // jika nilai ldr lebih besar dari 900 kurleb pukul (17:50WIB) RTH
         if(-1*tol > dvert || dvert > tol){
            if(avt > avd){
              if(digitalRead(LLow)) {}
                else{
                    digitalWrite(ledcenter, LOW);
                    ledforward();
                    forward();                // step motor forward
                    Serial.print("dvert="); 
                    Serial.print(dvert);
                    Serial.print("\t");
                    Serial.println("Forward");
                }
            }
        }
      }
      else{
           if(digitalRead(LHigh)) {}
              else{
                  digitalWrite(ledcenter, LOW);
                  ledbackward();
                  backward();
                  Serial.print("dvert="); 
                  Serial.print(dvert);
                  Serial.print("\t");
                  Serial.println("Backward");
              }
      }
      if(dvert >= -3 && dvert <= 3){          // led center ON, panel sejajar matahari
        digitalWrite(ledcenter, HIGH);
      }
      else{
          digitalWrite(ledcenter, LOW);    
      }
  }
  else{
      digitalWrite(ledcenter, HIGH);          // led center ON
      if(buttonState1 == HIGH){               // fungsi tombol forward
        if(digitalRead(LLow)) {}
          else{
              ledforward();
              forward();
          }
      }
      if(buttonState2 == HIGH){               // fungsi tombol backward
        if(digitalRead(LHigh)) {}
          else{
              ledbackward();    
              backward();
         }
      }
  }   

  if(digitalRead(LHigh)){                     // limit high/top
    ledlimit1();                              // led limit home menyala
  }
  
  if(digitalRead(LLow)){                      // limit low/home 
    ledlimit2();                              // led limit end menyala
  }
}
