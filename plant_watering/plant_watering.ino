#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPtimeESP.h>

#define DEBUG_ON

//wifi credentials
#include "credentials.h"

NTPtime NTPch("de.pool.ntp.org");   // Choose server pool as required
strDateTime dateTime;


// Connect to the WiFi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER_IP;
const char* user= MQTT_USER;
const char* passw= MQTT_PASSWORD;

String receivedString;
WiFiClient espClient;
PubSubClient client(espClient);


//push-button for manual usage
const int button = 5;
int buttonState = 0;


//motor-ic
const int motor_A = 12;
const int motor_B = 13;

//humidity-sensor
const int hsensor = 0;
int humidity = 0;

long sensorTimer = 0;


//watering
long wateringTime = 5000;




void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 if (strcmp(topic,"home/balcony/watering")==0){
 for (int i=0;i<length;i++) {
  receivedString += (char)payload[i];
  
  if (receivedString == "water"){
    Serial.println("MQTT");
    mqtt_watering();
    
    String cTime = (String)dateTime.hour + ":" + dateTime.minute + ":" + dateTime.second;
    Serial.println(cTime);
    char cTimeC[20];
    cTime.toCharArray(cTimeC, 20);
    client.publish("home/balcony/waterTime", cTimeC);
    
  }
  }
 //Serial.println();
 receivedString = "";
 
 }
}

void mqtt_watering(){
  Serial.println("automatic watering");
    digitalWrite(motor_B, HIGH);
    digitalWrite(motor_A, LOW);
    
    delay(wateringTime);

    digitalWrite(motor_B, LOW);
    digitalWrite(motor_A, LOW);
}


void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Attempt to connect
 if (client.connect("ESP8266 Client", user, passw)) {
  Serial.println("connected");
  // ... and subscribe to topic
  client.subscribe("home/balcony/watering");
  
 } else {
  Serial.print("failed, rc=");
  Serial.print(client.state());
  Serial.println(" try again in 5 seconds");
  // Wait 5 seconds before retrying
  delay(5000);
  }
 }
}





void setup() {

  pinMode(button, INPUT);
  pinMode(motor_A, OUTPUT);
  pinMode(motor_B, OUTPUT);
  pinMode(hsensor, INPUT);
  
  Serial.begin(9600);
  
  WiFi.hostname("WateringESP");

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  
 
 client.setServer(mqtt_server, 1883);
 client.setCallback(callback);
}

void loop() {
  buttonState = digitalRead(button);
  humidity = analogRead(hsensor);

  dateTime = NTPch.getNTPtime(2.0, 0);

  if(dateTime.valid){
    NTPch.printDateTime(dateTime);

    byte actualHour = dateTime.hour;
    byte actualMinute = dateTime.minute;
    byte actualsecond = dateTime.second;
    int actualyear = dateTime.year;
    byte actualMonth = dateTime.month;
    byte actualday =dateTime.day;
    byte actualdayofWeek = dateTime.dayofWeek;
  }
  
//manual watering
  if(buttonState == HIGH){
    Serial.println("motor on");
    digitalWrite(motor_B, HIGH);
    digitalWrite(motor_A, LOW);
    
    delay(1000);

    digitalWrite(motor_A, LOW);
    digitalWrite(motor_B, LOW);
    
    String cTime = (String)dateTime.hour + ":" + dateTime.minute + ":" + dateTime.second;
    Serial.println(cTime);
    char cTimeC[20];
    cTime.toCharArray(cTimeC, 20);
    client.publish("home/balcony/waterTime", cTimeC);
    
  }

  
//sensor-controlled watering
  if(humidity > 400){
    Serial.println("automatic watering");
    digitalWrite(motor_B, HIGH);
    digitalWrite(motor_A, LOW);
    
    delay(wateringTime);

    digitalWrite(motor_A, LOW);
    digitalWrite(motor_B, LOW);

    String cTime = (String)dateTime.hour + ":" + dateTime.minute + ":" + dateTime.second;
    Serial.println(cTime);
    char cTimeC[20];
    cTime.toCharArray(cTimeC, 20);
    client.publish("home/balcony/waterTime", cTimeC);
  }

  Serial.println("Feuchtigkeit: " + String(humidity));

  
  
  if(millis()-sensorTimer > 10000){
    sensorTimer = millis();
    char sensorVal[10];
    int percentage = (230-(humidity-250))/(250/100) ;
    if(percentage > 100){
      percentage = 100;
    }
    if(percentage < 0){
      percentage = 0;
    }
    //String helpVal = String(percentage);
    //helpVal.toCharArray(sensorVal, 10);
    String helpVal = String(humidity);
    helpVal.toCharArray(sensorVal, 10);
    client.publish("home/balcony/humidity", sensorVal);
  }
  

  if (!client.connected()) {
  reconnect();
 }
 client.loop();

}
