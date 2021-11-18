/////////////////// Importing Libraries ///////////////////

///////// OTA 
//#ifdef ESP32
//  #include <WiFi.h>
//#else
//  #include <ESP8266WiFi.h>
//#endif
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

///////// Telegram Bot 
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

///////// Login Credentials
const char* ssid = "YOUR WIFI HERE";
const char* password = "YOUR PASSWORD HERE";

//1.0///////////////// Initialize Telegram BOT ///////////////////

#define BOTtoken "YOUR BOT TOKEN HERE"   // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "YOUR CHAT ID HERE"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//1.1///////////////// Init Pump Timer ///////////////////

// Pump Test
//const long pumpInterval = 10000;
//const long pumpRuntime = 2000;

const long pumpInterval = 3600000;
const long pumpRuntime = 300000;

unsigned long lastTimePumpStart = 0;

int Relay = 13;

int statusIndicatorLED = 27; // Status LED


//1.2///////////////// Init DHT11 Temp Sensor ///////////////////

#include "DHT.h"
#define DHTPIN 12   // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);


//1.3///////////////// Define Readings ///////////////////

// Get DHT11 sensor readings and return them as a String variable
String getReadings(){
  float temperature, humidity;
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  String message = "It is currently " + String(temperature) + " ºC. \n";
  message += "The Humidity is " + String (humidity) + " % \n";

  return message;
}

void pumpUpdateBot() {
  bot.sendMessage(CHAT_ID, "Hey, I just switched on the pump." , "");
  bot.sendMessage(CHAT_ID, getReadings(), "");
  }

//1.4///////////////// Incoming Messages ///////////////////

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to talk with your Spirulina.\n\n";
      welcome += "/hi to say hello \n";
      welcome += "/temp to request current Temperature \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/hi") {
      bot.sendMessage(chat_id, "Hello :)", "");
    }
    
    if (text == "/temp") {
      String readings = getReadings();
      bot.sendMessage(chat_id, readings, "");
         
        
    }
  }
}

void setup() {

  dht.begin(); // Initiate DHT11 Temp Sensor

  pinMode(Relay,OUTPUT); // Set Relay Output
  pinMode(statusIndicatorLED,OUTPUT); // Set Indicator LED Output

  digitalWrite(statusIndicatorLED, HIGH); // Indicating Setup Start

  /////////////////// Setup WIFI ///////////////////
  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  /////////////////// Setup Telegram ///////////////////

  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif

  /////////////////// Setup OTA ///////////////////

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(statusIndicatorLED, LOW); // Indicating Setup Successful
  
}

void loop() {
//OTA handler
  ArduinoOTA.handle(); 
  
//Telegram Bot
  if (millis() > lastTimeBotRan + botRequestDelay)  {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while(numNewMessages) {
    Serial.println("got response");
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
  lastTimeBotRan = millis();
  }

//Pump Control
  if (millis() - lastTimePumpStart >= pumpInterval) {
  lastTimePumpStart = millis();
  digitalWrite(Relay,HIGH);
  pumpUpdateBot();
  }

  if (millis() - lastTimePumpStart >= pumpRuntime) {
  digitalWrite(Relay,LOW);
  }

}
