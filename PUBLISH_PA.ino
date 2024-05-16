
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "wifi"; // isi dengan SSID jaringan WiFi Anda
const char* password = "1sampai8"; // isi dengan password jaringan WiFi Anda
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin sensor ultrasonik
#define TRIG_PIN D6
#define ECHO_PIN D7
// Define buzzer pin
#define BUZZER_PIN D0

// Variabel untuk menyimpan jarak sebelumnya
float lastDistance = 0;
// Threshold untuk deteksi pergerakan
float movementThreshold = 5.0; // 5 cm perubahan dianggap sebagai pergerakan

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message Received [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Subscribe to a topic here if needed
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  // Set up buzzer pin as output
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float currentDistance = duration * 0.034 / 2;

  if (abs(currentDistance - lastDistance) > movementThreshold) {
    char msg[50];
    snprintf(msg, 50, "Movement detected! Current Distance: %.2f cm", currentDistance);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("ultrasonic/movement", msg);

    // Activate buzzer for 1 second
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }

  lastDistance = currentDistance; // Update lastDistance to the current reading
  delay(2000);
}

