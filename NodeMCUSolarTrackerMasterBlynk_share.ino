#include <SoftwareSerial.h>         // khusus esp32, esp8266 tidak perlu softaware serial. konfigurasi dibawah tidak bisa digunakan untuk esp32
SoftwareSerial DataSerial(12, 13);  // DataSerial(tx, rx)
#include <NTPClient.h>              // NTP server untuk get world clock
#include <WiFiUdp.h>                // library wifi udp untuk NTP
#include <ESP8266WiFi.h>            // library wifi untuk esp8266
#include <Wire.h>                   // library wire untuk menggunakan pin lain sebagai tx dan rx
#include <LiquidCrystal_I2C.h>      // library untuk lcd 
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

// Thinger.io config
#include <ThingerESP8266.h>
#define USERNAME "user_name_akun_thinger.io_agan"
#define DEVICE_ID "device_id_agan"
#define DEVICE_CREDENTIAL "credential_thinger_agan"
ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Blynk config
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

BlynkTimer timer;
char ssid[] = "ssid_wifi_agan";       // isi ssid wifi agan
char pass[] = "password_wifi_agan";   // isi password wifi agan
char auth[] = "auth_blynk_agan";      // auth pada saat create di blynk dikirim ke email agan

int lt;
int rt;
int ld;
int rd;
int batt;
int pvamps;
int watt;
int wHour;
int AHour;
int lcdrst;
int power;
int inv;
char Buffer1[16];
char Buffer2[16];
char Buffer3[16];
char Buffer4[16];
char Buffer5[16];

float Bbatt;
float Bpvamps;
float Bwatt;
float BwHour;
float BAHour;

WidgetLED led1(V5);
// End Thinger.io config 

// Define NTP Client to get time
WiFiUDP ntpUDP; 
NTPClient timeClient(ntpUDP, "pool.ntp.org");   // server untuk get world clock
WiFiClient client;

//Week Days
String weekDays[7]={"Min", "Sen", "Sel", "Rab", "Kam", "Jum", "Sab"};

//Month names
String months[12]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Des"};

// millis
unsigned long previousMillis = 0;
const long interval = 2000;       // interval 2 detik untuk minta data ke arduino

// array parsing data
String arrData[17];               // jumlah data dikirim (sengaja dilebihkan satu karena simbol ikut terbaca jika dibuat 16

byte battery[8] = {0b01110,  0b11011,  0b10001,  0b10001,  0b11111,  0b11111,  0b11111,  0b11111};      // icon baterai
byte energi[8] = {0b00010,  0b00100,  0b01000,  0b11111,  0b00010,  0b00100,  0b01000,  0b00000};       // icon petir
byte air[8] = {0b00100,  0b01110,  0b01010,  0b10001,  0b10001,  0b11101,  0b11111,  0b01110};          // icon air
byte listrik[8] = {0b01010,  0b11111,  0b10001,  0b10001,  0b01110,  0b00100,  0b00100,  0b00100};      // icon steker
byte solar[8] = {0b11111, 0b10101, 0b11111, 0b10101, 0b11111, 0b10101, 0b11111, 0b00000};               // icon solar
byte dir[8] = {0b10000,  0b11000,  0b11100,  0b11001,  0b10011,  0b00111,  0b00011,  0b00001};          // icon direction
byte istop[8] = {0b00000, 0b00000, 0b01110, 0b11111, 0b11111, 0b11111, 0b01110, 0b00000};               // icon wifi
byte istart[8] = {0b00000, 0b01000, 0b01100, 0b01110, 0b01111, 0b01110, 0b01100, 0b01000};              // icon wifi

#define ledinv 0         // led info inverter auto / manual (auto = mesin air hidup pada jam 9 - 14 jika ampere > 5A inverter ON, manual = mesin air hidup dengan PLN saja)
#define leddir 2         // led info direction auto / manual
#define radar 16         // input radar toren air
#define relay1 14        // relay Inverter
#define relay2 15        // relay PLN
int hourandminute = 0;   // konfigurasi untuk menit relay inverter
int inputStart = 9;      // konfigurasi untuk start relay inverter jam 9 pagi
int inputStop  = 14;     // konfigurasi untuk stop relay inverter jam 14 siang

void setup() {
  Serial.begin(9600);
  DataSerial.begin(9600);
  Wire.begin(D2, D1);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(radar, INPUT);
  pinMode(ledinv, OUTPUT);
  pinMode(leddir, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  
  // Connect to Wi-Fi
  //  Serial.print("Connecting to ");
  //  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
  }
    Serial.print("connected");
    digitalWrite(LED_BUILTIN, HIGH);
    
    thing.add_wifi(ssid, pass);
    thing["koindata"] >> [](pson& out)
    {
    out["lt"]     = arrData[0];
    out["rt"]     = arrData[1];
    out["ld"]     = arrData[2];
    out["rd"]     = arrData[3];
    out["batt"]   = arrData[10];
    out["amps"]   = arrData[11];
    out["watt"]   = arrData[12];
    out["wHour"]  = arrData[14];
    out["AH"]     = arrData[15];
    out["Air"]    = radar;
    };
    
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(25200);   // set 25200 untuk GMT 07, 28800 untuk GMT 08, GMT 09 cek di google
  
  Blynk.begin(auth, ssid, pass);

  // Setup a function to be called every second
  timer.setInterval(10000L, sendSensor);        // set 10 detik untuk send ke server blynk biar esp8266 gk sering restart
  Blynk.connect();
  
   lcd.begin(20, 4);
   lcd.home();
   lcd.createChar(5, listrik);    // icon listrik
   lcd.createChar(1, istop);      // icon stop
   lcd.createChar(2, battery);    // icon batteray
   lcd.createChar(3, energi);     // icon energi
   lcd.createChar(4, solar);      // icon panel surya
   lcd.createChar(6, dir);        // icon panah direction
   lcd.createChar(0, air);        // icon air
   lcd.createChar(7, istart);     // icon play / start
   lcd.clear();
}

void sendSensor()
{
  // You can send any value at any time. Please don't send more that 10 values per second.

  dtostrf(Bbatt, 1, 2, Buffer1);            // untuk mengconvert decimal menjadi 2 digit saja
  Blynk.virtualWrite(V0, Buffer1);          // data yang akan dikirim ke server blynk
  dtostrf(Bpvamps, 1, 2, Buffer2);
  Blynk.virtualWrite(V1, Buffer2);
  dtostrf(Bwatt, 1, 2, Buffer3);
  Blynk.virtualWrite(V2, Buffer3);
  dtostrf(BwHour, 1, 2, Buffer4);
  Blynk.virtualWrite(V3, Buffer4);
  dtostrf(BAHour, 1, 2, Buffer5);
  Blynk.virtualWrite(V4, Buffer5);
  Blynk.virtualWrite(V5, led1.getValue());  // indikator led di server blynk
   
}

void loop() {
     
     timeClient.update();
     waktu();
     symbol();
     hourandminute = timeClient.getHours() + (timeClient.getMinutes() / 60);  // untuk mendapatkan nilai akumulasi jam awal dan jam akhir start inverter mesin air

     if(timeClient.getHours() == 6 && timeClient.getMinutes() == 0){          // untuk reset lcd pada pukul 06:00 nilai A, W, WH, AH menjadi 0
      arrData[11] = "";
      arrData[12] = "";
      arrData[14] = "";
      arrData[15] = "";
      lcd.setCursor(0, 0);
      lcd.print("    ");
      lcd.setCursor(0, 1);
      lcd.print("    ");
      lcd.setCursor(0, 2);
      lcd.print("    ");
      lcd.setCursor(0, 3);
      lcd.print("    ");
      lcd.setCursor(5, 1);
      lcd.print("    ");
      lcd.setCursor(13, 0);
      lcd.print("    ");
      lcd.setCursor(18, 0);
      lcd.print("    ");
      lcd.setCursor(5, 0);
      lcd.print("    ");
      lcd.setCursor(5, 2);
      lcd.print("    ");
      lcd.setCursor(13, 1);
      lcd.print("    ");
      lcd.clear();
    }
    
     if(power == 1){                        // tombol Auto Air
        lcd.setCursor(17, 2);
        lcd.print("A");
        digitalWrite(ledinv, HIGH);
        if(digitalRead(radar) == HIGH){
           lcd.setCursor(18, 2);
           lcd.write(byte(7));              // 
           led1.on();                       // icon led di interface blynk
           if (inputStop > inputStart) { 
             if(inv == 1 && hourandminute >= inputStart && hourandminute <= inputStop){
               lcd.setCursor(19, 2);
               lcd.write(byte(5));          // symbol Inverter ON                            
               digitalWrite(relay1, LOW);   // Relay INV ON
               delay(1000);
               digitalWrite(relay2, LOW);   // Relay PLN ON
             }
             else{
               lcd.setCursor(19, 2);
               lcd.write(byte(3));          // symbol PLN ON
               digitalWrite(relay1, HIGH);  // Relay INV OFF
               delay(1000);
               digitalWrite(relay2, LOW);   // Relay PLN ON
             }
           }
           else{
             lcd.setCursor(19, 2);
             lcd.write(byte(3));            // symbol PLN ON
             digitalWrite(relay1, HIGH);    // Relay INV OFF
             delay(1000);
             digitalWrite(relay2, LOW);     // Relay PLN ON
           }
        }
        else{
           lcd.setCursor(18, 2);
           lcd.write(byte(1));              // symbol stop
           led1.off();                      // icon led di interface blynk
           digitalWrite(relay1, HIGH);      // Relay INV OFF
           digitalWrite(relay2, HIGH);      // Relay PLN OFF
        }
     }
     else {        
        digitalWrite(ledinv, LOW);
        lcd.setCursor(17, 2);
        lcd.print("M");
        lcd.setCursor(19, 2);
        lcd.write(byte(3));
        if(digitalRead(radar) == HIGH){
           lcd.setCursor(18, 2);
           lcd.write(byte(7));
           led1.on();
           digitalWrite(relay1, HIGH);       // Relay INV
           delay(1000);
           digitalWrite(relay2, LOW);        // Relay PLN
        }
        else{
          lcd.setCursor(18, 2);
          lcd.write(byte(1));
          led1.off();
          digitalWrite(relay1, HIGH);        // Relay INV
          digitalWrite(relay2, HIGH);        // Relay PLN
        }
     }
            
     // setting millis
     unsigned long currentMillis = millis();
     if(currentMillis - previousMillis >= interval)
       {
        // update previousMillis
        previousMillis = currentMillis;

        // prioritaskan pembacaan data dari Mega baca data serial
        String bacaData = "";
        while(DataSerial.available()>0)
             {
             bacaData += char(DataSerial.read());
             }
        // buang spasi data
        bacaData.trim();
        
        // uji data
        if(bacaData != "")
          {
          // parsing data
           int index = 0;
           for(int i=0; i<= bacaData.length(); i++)
              {
              char delimiter = '#';
              if(bacaData[i] != delimiter)
                 arrData[index] += bacaData[i];
              else                 
                 index++;  // variable index bertambah 1            
              }
        // pastikan bahwa data yang dikirim lengkap
        if(index == 16)
        {
           // tampilkan nilai sensor ke serial monitor

            if(arrData[0] < "1000"){          // print lt value
                lcd.setCursor(0, 0);
                lcd.print(arrData[0]);  
                lcd.print(" ");
              }
              else if(arrData[0] < "100"){
                lcd.setCursor(0, 0);
                lcd.print(arrData[0]);  
                lcd.print("  ");
              }
              else if(arrData[0] < "10"){
                lcd.setCursor(0, 0);
                lcd.print(arrData[0]);  
                lcd.print("   ");
              }
              else{
                lcd.setCursor(0, 0);
                lcd.print(arrData[0]);
              }

              if(arrData[1] < "1000"){        // print rt value
                lcd.setCursor(0, 1);
                lcd.print(arrData[1]);  
                lcd.print(" ");
              }
              else if(arrData[1] < "100"){
                lcd.setCursor(0, 1);
                lcd.print(arrData[1]);  
                lcd.print("  ");
              }
              else if(arrData[1] < "10"){
                lcd.setCursor(0, 1);
                lcd.print(arrData[0]);  
                lcd.print("   ");
              }
              else{
                lcd.setCursor(0, 1);
                lcd.print(arrData[1]);
              }
              
            if(String(arrData[2]) < "1000"){  // print ld value
                lcd.setCursor(0, 2);
                lcd.print(arrData[2]);  
                lcd.print(" ");
              }
              else if(arrData[2] < "100"){
                lcd.setCursor(0, 2);
                lcd.print(arrData[2]);  
                lcd.print("  ");
              }
              else if(arrData[2] < "10"){
                lcd.setCursor(0, 2);
                lcd.print(arrData[2]);  
                lcd.print("   ");
              }
              else{
                lcd.setCursor(0, 2);
                lcd.print(arrData[2]);
              }
              
            if(arrData[3] < "1000"){        // print rd value
                lcd.setCursor(0, 3);
                lcd.print(arrData[3]);  
                lcd.print(" ");
              }
              else if(arrData[3] < "100"){
                lcd.setCursor(0, 3);
                lcd.print(arrData[3]);  
                lcd.print("  ");
              }
              else if(arrData[3] < "10"){
                lcd.setCursor(0, 3);
                lcd.print(arrData[3]);  
                lcd.print("   ");
              }
              else{
                lcd.setCursor(0, 3);
                lcd.print(arrData[3]);
              }
              
            if(arrData[4] == "1" || arrData[5] == "1"){
               lcd.setCursor(19, 3);
               lcd.print("X");             // icon limit switch aktif
            }
            else if(arrData[8] == "1"){
               lcd.setCursor(19, 3);
               lcd.print("=");             // icon panel sejajar matahari
            }
            else{
              lcd.setCursor(19, 3);
              lcd.print(" ");
            }
            
            lcd.setCursor(15, 2);
            if(arrData[6] == "1"){          // icon Auto Direction
               lcd.print("A");
               digitalWrite(leddir, HIGH);
            }
            else {
               lcd.print("M");              // icon Manual Direction
               digitalWrite(leddir, LOW);
            } 
                       
            lcd.setCursor(5, 1);
            lcd.print(arrData[12]);         // watt value
            lcd.setCursor(11, 1);
            lcd.print("W ");
            lcd.setCursor(14, 0);
            lcd.print(arrData[10]);         // Voltage Batterai
            lcd.setCursor(19, 0);
            lcd.print("V");
            lcd.setCursor(5, 0);
            lcd.print(arrData[11]);         // Ampere Solar
            lcd.setCursor(11, 0);     
            lcd.print("A");  
            lcd.setCursor(5, 2);
            lcd.print(arrData[14]);         // Watt Hours
            lcd.setCursor(11, 2);
            lcd.print("WH");
            lcd.setCursor(14, 1);
            lcd.print(arrData[15]);         // Ampere Hours
            lcd.setCursor(19, 1);
            lcd.print("A");                    
            }
            
               // variable thinger.io send
               lt     = arrData[0].toInt();   
               rt     = arrData[1].toInt();
               ld     = arrData[2].toInt();
               rd     = arrData[3].toInt();
               power  = arrData[7].toInt();
               inv    = arrData[9].toInt();
               lcdrst = arrData[13].toInt();
               batt   = arrData[10].toInt();
               pvamps = arrData[11].toInt();
               watt   = arrData[12].toInt();
               wHour  = arrData[14].toInt();
               AHour  = arrData[15].toInt();

               // variable blynk
               Bbatt   = arrData[10].toFloat();
               Bpvamps = arrData[11].toFloat();
               Bwatt   = arrData[12].toFloat();
               BwHour  = arrData[14].toFloat();
               BAHour  = arrData[15].toFloat();
               thing.handle();
                Blynk.run();
                timer.run();
  
                arrData[0] = "";      // clear semua data setelah kirim
                arrData[1] = "";
                arrData[2] = "";
                arrData[3] = "";
                arrData[4] = "";
                arrData[5] = "";
                arrData[6] = "";
                arrData[7] = "";
                arrData[8] = "";
                arrData[9] = "";
                arrData[10] = "";
                arrData[11] = "";
                arrData[12] = "";
                arrData[13] = "";
                arrData[14] = "";
                arrData[15] = "";
                arrData[16] = "";
           }
        // minta data ke arduino nano
        DataSerial.println("Ya");
        }
 }

void symbol(){
    lcd.setCursor(4, 0);
    lcd.write(byte(4));     // icon solar amps
    lcd.setCursor(4, 1); 
    lcd.write(byte(3));     // icon energy watt
    lcd.setCursor(4, 2); 
    lcd.write(byte(3));     // icon energy watt hour
    lcd.setCursor(13, 0); 
    lcd.write(byte(2));     // icon batterai
    lcd.setCursor(13, 1); 
    lcd.write(byte(2));     // icon batterai
    lcd.setCursor(16, 2);   
    lcd.write(byte(0));     // icon air
    lcd.setCursor(14, 2); 
    lcd.write(byte(6));     // icon direction
}

void waktu(){               // function untuk get waktu dari NTP server
    unsigned long epochTime = timeClient.getEpochTime();
    String weekDay = weekDays[timeClient.getDay()];
    struct tm *ptm = gmtime ((time_t *)&epochTime); 
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    String currentMonthName = months[currentMonth-1];
    int currentYear = ptm->tm_year+1900;
    
    lcd.setCursor(4, 3);
    lcd.print(weekDay);    
    lcd.print(",");
    if(monthDay < 10){
      lcd.print("0");
      lcd.print(monthDay);
    }
    else{
    lcd.print(monthDay); 
    }
    lcd.print(currentMonthName);
    lcd.print(" ");
    if(currentHour < 10){
      lcd.print("0");
      lcd.print(currentHour);
    }
    else{
    lcd.print(currentHour); 
    }
    lcd.print(":");
    if(currentMinute < 10){
      lcd.print("0");
      lcd.print(currentMinute);
    }
    else{
    lcd.print(currentMinute); 
    }
}
