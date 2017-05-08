/*
  Author:chenzhuo
  DateTime:5/2 2017
  Second Commit:5/4 2017
 */
#include <aJSON.h>
#include <Wire.h> 
#include <dht11.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
dht11 DHT11;
#define DHT11PIN 2
int sensorPin = A0;
int sensorValue = 0;
int trshidu = A1;
int trshiduValue = 0;
int yuyin;
int LED = 8;
int PUMP = 7;
int HEAT = 4;

String DEVICEID="xxx"; // device ID=======
String APIKEY="xxx"; // password of device==
//port ID==============
String INPUTID1="xxx";
String INPUTID2="xxx";
String INPUTID3="xxx";
String INPUTID4="xxx";
//=======================================
unsigned long lastCheckInTime = 0;
unsigned long lastUpdateTime = 0;
const unsigned long postingInterval = 40000; 
const unsigned long updateInterval = 5000; 
String inputString = "";
boolean stringComplete = false;
boolean CONNECT = true; 
boolean isCheckIn = false; 
char* parseJson(char *jsonString);

void setup()
{
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.print("Waiting...");
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  mySerial.begin(115200);
  delay(8000);
  Serial.print("@TextToSpeech#听从吩咐$");
}

void loop()
{
  yuyin =  Serial.read();
  speech(yuyin);
  if(millis() - lastCheckInTime > postingInterval || lastCheckInTime==0) {
    checkIn();
  }
  int chk = DHT11.read(DHT11PIN); 
  int valt = DHT11.temperature;
  int valh = DHT11.humidity;
  sensorValue = analogRead(sensorPin);
  trshiduValue = analogRead(trshidu);
  switch (chk){
    case DHTLIB_OK:
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(valt);
      lcd.write(0xDF);
      lcd.print("C");
      lcd.print(",L:");
      lcd.print(sensorValue);
      lcd.print("cd");
      
      lcd.setCursor(0, 1);
      lcd.print("H:");
      lcd.print(valh);
      lcd.print("%");
      lcd.print(",TH:");
      lcd.print(trshiduValue);
      lcd.print("%");
      break;

    case DHTLIB_ERROR_CHECKSUM: 
      lcd.clear();
      lcd.print("Checksum error"); 
      delay(1000);  
      lcd.clear();
      break;
    case DHTLIB_ERROR_TIMEOUT: 
      lcd.clear();
      lcd.print("Time out error"); 
      delay(1000);             
      lcd.clear();
      break;
    default: 
      lcd.clear();
      lcd.print("Unknown error"); 
      delay(1000);                
      lcd.clear();
      break;
  }
  if((millis() - lastUpdateTime > updateInterval)) {
    updateToNet(DEVICEID,INPUTID1,valt,INPUTID2,valh,INPUTID3,(int)sensorValue,INPUTID4,(int)trshiduValue);
  }
  serialEvent();
  if (stringComplete) {
    inputString.trim();
    if(inputString=="CLOSED"){
    mySerial.println("connect closed!");
    CONNECT=false;   
    isCheckIn=false;     
    }else{
      //Serial.println(inputString);
      int len = inputString.length()+1;    
      if(inputString.startsWith("{") ){//&& inputString.endsWith("}")){
        char jsonString[len];
        inputString.toCharArray(jsonString,len);
        aJsonObject *msg = aJson.parse(jsonString);
       //Serial.println(&msg);
        processMessage(msg);
        aJson.deleteItem(msg);          
      }
    }      
    inputString = "";
    stringComplete = false;
  }
  delay(5000);
}

void speech(signed int val){
  if (-1 != val)
  {
    if (16 == val)
    {
      digitalWrite(LED, HIGH);
    }else if(17 == val)
    {
      digitalWrite(LED, LOW);
    }else if(22 == val){
      digitalWrite(PUMP, HIGH);
    }else if(23 == val){
		digitalWrite(PUMP, LOW);
	}else if(24 == val){
		digitalWrite(HEAT, HIGH);
	}else if(25 == val){
		digitalWrite(HEAT, LOW);
	}
  }
}

void checkIn() {
  if (!CONNECT) {
    mySerial.print("+++");
    delay(500);  
    mySerial.print("\r\n"); 
    delay(1000);
    mySerial.print("AT+RST\r\n"); 
    delay(6000);
    CONNECT=true;
    lastCheckInTime==0;
  }
  else{
    mySerial.print("{\"M\":\"checkin\",\"ID\":\"");
    mySerial.print(DEVICEID);
    mySerial.print("\",\"K\":\"");
    mySerial.print(APIKEY);
    mySerial.print("\"}\r\n");
    lastCheckInTime = millis();
  }
}

void updateToNet(String did, String inputid1, int value1, String inputid2, int value2,String inputid3, int value3,String inputid4, int value4){
  mySerial.print("{\"M\":\"update\",\"ID\":\"");
  mySerial.print(did);
  mySerial.print("\",\"V\":{\"");
  mySerial.print(inputid1);
  mySerial.print("\":\"");
  mySerial.print(value1);
  mySerial.print("\",\"");
  mySerial.print(inputid2);
  mySerial.print("\":\"");
  mySerial.print(value2);
  mySerial.print("\",\"");
  mySerial.print(inputid3);
  mySerial.print("\":\"");
  mySerial.print(value3);
  mySerial.print("\",\"");
  mySerial.print(inputid4);
  mySerial.print("\":\"");
  mySerial.print(value4);
  mySerial.println("\"}}");
  lastCheckInTime = millis();
  lastUpdateTime= millis(); 
}

void serialEvent() {
  while(mySerial.available()) {
    char inChar = (char)mySerial.read();
    inputString += inChar;
    //if (inChar == '\n') {
      //stringComplete = true;
    //}
  }
  stringComplete = true;
}

void processMessage(aJsonObject *msg){
  aJsonObject* method = aJson.getObjectItem(msg, "M");
  aJsonObject* content = aJson.getObjectItem(msg, "C");     
  aJsonObject* client_id = aJson.getObjectItem(msg, "ID");  
  //char* st = aJson.print(msg);
  if (!method) {
    return;
  }
    //Serial.println(st); 
    //free(st);
    String M=method->valuestring;
    if(M=="checkinok"){
      isCheckIn = true;
    }
}
