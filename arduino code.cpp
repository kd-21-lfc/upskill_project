#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com"; // Free public broker
const char* mqtt_topic = "airquality/sensor1";
const char* mqtt_client_id = "ESP8266_AirQuality";

// MQ135 Sensor pin
const int MQ135_PIN = A0;

// Variables
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int sensorValue = 0;
float airQualityPPM = 0;
String airQualityStatus = "";

void setup() {
  Serial.begin(115200);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Air Quality Monitor");
  display.println("Initializing...");
  display.display();
  delay(2000);
  
  // Connect to WiFi
  setup_wifi();
  
  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  
  Serial.println("System Ready!");
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long now = millis();
  if (now - lastMsg > 2000) { // Update every 2 seconds
    lastMsg = now;
    
    // Read sensor
    sensorValue = analogRead(MQ135_PIN);
    
    // Convert to PPM (approximate calculation)
    // MQ135: 0-1024 analog value
    airQualityPPM = map(sensorValue, 0, 1024, 0, 500);
    
    // Determine air quality status
    if (airQualityPPM < 50) {
      airQualityStatus = "Excellent";
    } else if (airQualityPPM < 100) {
      airQualityStatus = "Good";
    } else if (airQualityPPM < 150) {
      airQualityStatus = "Moderate";
    } else if (airQualityPPM < 200) {
      airQualityStatus = "Poor";
    } else {
      airQualityStatus = "Hazardous";
    }
    
    // Display on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Air Quality Monitor");
    display.println("------------------");
    display.print("Raw Value: ");
    display.println(sensorValue);
    display.print("PPM: ");
    display.println(airQualityPPM);
    display.println();
    display.setTextSize(2);
    display.print(airQualityStatus);
    display.display();
    
    // Publish to MQTT
    String payload = "{\"sensor\":\"MQ135\",\"raw\":" + String(sensorValue) + 
                     ",\"ppm\":" + String(airQualityPPM) + 
                     ",\"status\":\"" + airQualityStatus + "\"}";
    
    client.publish(mqtt_topic, payload.c_str());
    
    // Serial output
    Serial.print("Raw: ");
    Serial.print(sensorValue);
    Serial.print(" | PPM: ");
    Serial.print(airQualityPPM);
    Serial.print(" | Status: ");
    Serial.println(airQualityStatus);
  }
}