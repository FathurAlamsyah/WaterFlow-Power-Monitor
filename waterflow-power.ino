#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_INA219 ina1(0x40);

#define LED 2
#define flowSens 23

unsigned long cM = 0;
unsigned long pM = 0;
int intval = 1000;
bool ledSdate = LOW;
float calibrateFactor = 7.5;
volatile unsigned long pulsCount = 0;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowML;
unsigned long totalML;

float volt, curr;
float liter;

int interval = 30000;
long prevMilMon, currMilMon;

String server = "https://trainer-plta.proaction-palu.com";

void IRAM_ATTR pulsCounter(){
  pulsCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED,OUTPUT);
  pinMode(flowSens, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("TRAINER PLTA");
  lcd.setCursor(2,1);
  lcd.print("GET  STARTED");
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
//  wm.resetSettings();
  bool res;
  Serial.print("RES: ");Serial.println(res);
  if(res == 0) {Serial.println("GAGAL CONNECT");} 
  else {Serial.println("WIFI TERKONEKSI");}
  res = wm.autoConnect("~TRAINER PLTA~","pass1234");
  Serial.print("RES: ");Serial.println(res);

  Wire.begin();
  while(!ina1.begin()){
    Serial.println("INA219 1 is Not Connected!");
    delay(1000);
    ina1.begin();
  }
  ina1.setCalibration_16V_400mA();
  Serial.println("INA219 1 CONNECTED");

  pulsCount = 0;
  flowRate = 0.0;
  flowML = 0;
  totalML = 0;
  pM = 0;
  
  attachInterrupt(digitalPinToInterrupt(flowSens), pulsCounter, RISING);
  currMilMon = millis();

  lcd.clear();
}

void loop() {
  lcd.clear();
  currMilMon = millis();
  cM = millis();

  getINA();
  getWater();
  
  if(currMilMon - prevMilMon > interval){
    sendData();
    prevMilMon = currMilMon;
  }
  delay(500);
}

void getINA(){
  volt = ina1.getBusVoltage_V();
  curr = ina1.getCurrent_mA();
  
  Serial.print("Volt : ");Serial.print(volt);Serial.println(" V");
  Serial.print("Curr : ");Serial.print(curr,4);Serial.println(" mA");

  lcd.setCursor(0,0);
  lcd.print(volt); lcd.print("V, "); lcd.print(curr);lcd.print("mA");
}

void getWater(){
  if(cM - pM > intval){
    noInterrupts();
    unsigned long pulses = pulsCount;
    pulsCount = 0;
    interrupts();

     // Debit air (L/min)
    flowRate = pulses / calibrateFactor;

    // Volume air (mL)
    flowML = (flowRate / 60.0) * 1000.0;
    totalML += flowML;
    
    Serial.print("Flow rate: ");
    Serial.print(flowRate);  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space

    Serial.print("Total: ");
    Serial.print(totalML);
    Serial.print("mL / ");
    Serial.print(float(totalML / 1000));
    Serial.println("L");

    lcd.setCursor(0,1);
    lcd.print(flowRate); lcd.print(" L/min ");
    lcd.print(totalML); lcd.print(" mL");
    
    pM = cM;
  }
}

void sendData(){
  if(WiFi.status() == WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;

    String httpReqData = server + "/InsertDB.php?volt=" + String(volt) + "&curr=" + String(curr) + 
                        + "&water=" + String(flowRate);

    http.begin(httpReqData.c_str());
    
//    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    Serial.print("HTTP REQ Data: "); Serial.println(httpReqData);
    
    int httpResponCode = http.GET();
    String payload = http.getString();
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(httpResponCode);
    
     if(httpResponCode > 0){
      Serial.print("HTTP Response Code: ");Serial.println(httpResponCode);
      Serial.print("Payload           : ");Serial.println(payload);
      Serial.println("DATA TERKIRIM");
      
      lcd.setCursor(0,1);
      lcd.print("DATA TERKIRIM");
    }
    else{
      Serial.print("ERROR CODE  :");Serial.println(httpResponCode);
      Serial.print("Payload     : ");Serial.println(payload);
      Serial.println("GAGAL TERKIRIM");
      
      lcd.setCursor(0,1);
      lcd.print("GAGAL TERKIRIM");
    }
    http.end();
  }
  delay(500);
}
