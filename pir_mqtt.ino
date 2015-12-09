#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <PubSubClient.h>

const char* MY_SSID = "SSIDHERE"; 
const char* MY_PWD = "PASSWORDHERE";

// IPAddress server(192, 168, 1, 240);  // your MQTT server

int pin = D3;

WiFiClient ethClient;
PubSubClient client(ethClient);

void connectWifi()
{
  Serial.print("Connecting to "+*MY_SSID);
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Connected");
  Serial.println("");  
}//end connect


// the setup function runs once when you press reset or power the board
void setup() {
  
  pinMode(pin, INPUT_PULLUP);
  Serial.begin(115200);

  client.setServer(server, 1883);
  // client.setCallback(callback);
  
  connectWifi();
  
}

#define SAMPLES_PER_SECOND 5
#define SECONDS_TO_SAMPLE 60

char samples[SAMPLES_PER_SECOND * SECONDS_TO_SAMPLE];
char state = 0; // starting state assume off
int  sample_index = 0;

char published_state = 0;

// the loop function runs over and over again forever
void loop() {

 if (!client.connected()) {
    reconnect();
  }

  client.loop();
  
  if (state == 0 && digitalRead(pin)) {
    Serial.println("was off now on");
    state = 1;
    
  }
  else if (state == 1 && ! digitalRead(pin)) {
    Serial.println("was on now off");
    state = 0;
    
  }

  samples[sample_index] = state;
  sample_index++;
  if (sample_index == SAMPLES_PER_SECOND * SECONDS_TO_SAMPLE) {
    sample_index = 0;
  }

  // advertise a change from "not there" to "there" as soon as it happens
  if (state == 1 && published_state == 0) {
    Serial.println("Publishing THERE");
    client.publish("espp","present");
    published_state = 1;
  }
  // otherwise publish a "not there" if no one has been there for the whole sample time
  if (state == 0 && published_state == 1) {
    char sampled_state = 0;
    for (int i = 0; i < SAMPLES_PER_SECOND * SECONDS_TO_SAMPLE ; i++) {
      if (samples[i] == 1) {
        sampled_state = 1;
      }
    }
    if (sampled_state == 0) { // not a single hit
      Serial.println("Publishing GONE");
      client.publish("espp","absent");
      published_state = 0;
    }
  }
    
  delay(1000 / SAMPLES_PER_SECOND);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
