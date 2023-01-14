/*
  Author: Werner Paulin
  Purpose: Template for WIFI and MQTT
  Date created: 01.03.2019
*/

#include <Arduino.h>
#include <ArduinoJson.h>    //need to be added via library manager
#include <PubSubClient.h>   //need to be added via library manager
#include <WiFi.h>

/* GLOBAL CONSTANTS */
#define CYCLE_TIME 1000     //cycle time or loop() function in ms

/* GLOBAL VARIABLES */
unsigned long gLastStartTime;
unsigned long gSettingParameter1;
unsigned long gMonitorValue1;
boolean gCommandCmd1;

/* WIFI */
const char* WIFI_NETWORK_SSID = "AndromedaGuest";
const char* WIFI_NETWORK_PASSWORD = "pice7-firebugs";
WiFiClient wiFiClient;

/* MQTT */
#define MQTT_CLIENT_ID "MqttDemo"
#define MQTT_TOPIC_COMMANDS "/demo_scope/commands"         //read from the UI
#define MQTT_TOPIC_MONITOR "/demo_scope/monitor"          //sent to UI
#define MQTT_TOPIC_SETTINGS "/demo_scope/settings"        //read from the UI
#define MQTT_TOPIC_ON_CONNECT "/demo_scope/onconnect"     //typically settings which will be transfered to initialize UI


//Note: update function "mqttPublishTopicsCyclic()" accordingly

const char* MQTT_BROKER_IP = "192.168.0.10";  //IP address of raspberry PI
#define MQTT_PORT       1883
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""
PubSubClient mqttClient(wiFiClient);

//allocate memory for MQTT JSON communication
//ATTENTION with long JSON key names: MQTT message length is 128 byte!!! (see h-fie)
DynamicJsonDocument gPublishDataCyclic(1024);
DynamicJsonDocument gPublishDataOnConnect(1024);
DynamicJsonDocument gSubscribeDataCyclic(1024);

void initSerialPort()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(10);
  Serial.println("I'm awake.");
}

void initIOs()
{
  //pinMode(#pin_number#, OUTPUT);   //IO pin of pump
  return;
}


void readIOs()
{
  //variable = analogRead(#pin_number#);
  return;
}

void writeIOs()
{
  //digitalWrite(#pin_number#, variable);
  return;
}


void wifiConnect(const char* network_ssid, const char* network_password)
{
  Serial.print("Connecting to ");
  Serial.println(network_ssid);

  WiFi.begin(network_ssid, network_password);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      return;
    }

    // Only try for 15 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}


void mqttPublishTopicsCyclic()
{
  char jsonString[300];

  gPublishDataCyclic["gMonitorValue1"] = gMonitorValue1;
  
  serializeJson(gPublishDataCyclic, jsonString);

  mqttClient.publish(MQTT_TOPIC_MONITOR, jsonString);
}

void mqttPublishTopicsOnConnect()
{
  char jsonString[1024];
  gPublishDataOnConnect["gSettingParameter1"]  = gSettingParameter1;

  serializeJson(gPublishDataOnConnect, jsonString);
  mqttClient.publish(MQTT_TOPIC_ON_CONNECT, jsonString);
}

void mqttSubscribeToTopics()
{
  mqttClient.subscribe(MQTT_TOPIC_COMMANDS);
  mqttClient.subscribe(MQTT_TOPIC_SETTINGS);
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = MQTT_CLIENT_ID;
    // Create a random client ID
    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqttPublishTopicsOnConnect();
      mqttSubscribeToTopics();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void mqttGetSubscriptions()
{
  //process incoming messages and maintain its connection to the server
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
}




/*
   Callback function that gets called when subscribed topic gets
   a message published to it.  Can handle multiple topics with
   added conditionals.

   @topic the topic that published the message
   @payload the message
   @length the length of the message
*/
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  //print recevied messages on the serial console

    Serial.println("-------new message from broker-----");
    Serial.print("topic:");
    Serial.println(topic);
    Serial.print("payload:");
    Serial.write(payload, length);
    Serial.println();

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(gSubscribeDataCyclic, payload);
  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  else
  {
    //data interpretation depends on data
    if (String(topic) == MQTT_TOPIC_COMMANDS)
    {
      Serial.println("Command received");

      //ArduinoJson implements the Null Object Pattern, it is always safe to read the object. If the key doesn’t exist, it will return an empty value.
      if (gSubscribeDataCyclic.containsKey("gCommandCmd1") == true)
      {
        const char* cmd1 = gSubscribeDataCyclic["gCommandCmd1"];
        if (String(cmd1) == "on") {
          gCommandCmd1 = true;
          Serial.println("gCommandCmd1 on");
        }
        else if (String(cmd1) == "off") {
          gCommandCmd1 = false;
          Serial.println("gCommandCmd1 off");
        }
      }
    }
    else if (String(topic) == MQTT_TOPIC_SETTINGS)
    {
      if (gSubscribeDataCyclic.containsKey("gSettingParameter1") == true)
      {
        Serial.println("Valid gSettingParameter1 change");

        long setting1 = gSubscribeDataCyclic["gSettingParameter1"];
        Serial.println(setting1);
        gSettingParameter1 = setting1;
      }
    }
  }



}


void mqttConnect(const char* broker_ip, int broker_port)
{
  mqttClient.setServer(broker_ip, broker_port);
  mqttClient.setCallback(mqttCallback);
  mqttReconnect();

}



void setup() {
  //initialize serial communication for serial monitor (debugging)
  initSerialPort();

  // initialize I/O pins
  initIOs();

  //connect to WIFI networt
  wifiConnect(WIFI_NETWORK_SSID, WIFI_NETWORK_PASSWORD);

  //connect to MQTT broker
  mqttConnect(MQTT_BROKER_IP, MQTT_PORT);

  //initialize cycle time calculation
  gLastStartTime = millis();

  /**************** START OF PROGRAM INITIALIZSATION ****************/
  //TODO: add your code here
  gSettingParameter1 = 1;

  /**************** END OF PROGRAM INITIALIZSATION ****************/
}


void cyclicProgram(unsigned long cycleTime)
{
  //runtime measurement
  unsigned long now = millis();

  //check how much time is elapsed since the last call and skip main execution until cycle time has been reached
  if ((now - gLastStartTime) < cycleTime)
  {
    return;
  }
  gLastStartTime = now;

  /**************** START OF PROGRAM EXECUTION ****************/
  //get I/O image (inputs and current outputs states)
  readIOs();

  //TODO: add your code here
  gMonitorValue1 += 1;                    //demo code
  gMonitorValue1 *= gSettingParameter1;   //demo code

  if (gCommandCmd1 == true)
  {
    gCommandCmd1 = false;
    gMonitorValue1 = 0;
  }

  //publish all data
  mqttPublishTopicsCyclic();

  //write I/O image
  writeIOs();
  /**************** END OF PROGRAM EXECUTION ****************/
}

void loop() {
  mqttGetSubscriptions();
  cyclicProgram(CYCLE_TIME);
}








