#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <PubSubClient.h>
#include <Servo.h>

Servo servo;

//===================Wifi==================
#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "1sampai8"
//===================Wifi==================

//===================Sensor Pin==================
#define SERVO_PIN D4
//===================Sensor Pin==================


//=======================MQTT======================
const char* mqtt_server = "broker.emqx.io";  // broker gratisan
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
float motion = 0;
boolean status;

// Fungsi untuk menerima data
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '0') {
    servo.write(90);  // Untuk mengatur nilai servo
    Serial.println("pintu terbuka");
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else if ((char)payload[0] == '1') {
    servo.write(0);  // Untuk mengatur nilai servo
    Serial.println("pintu tertutup");
  } else if (String(topic) == "ultrasonic/movement") {
    motion = (float)payload[0];
    Serial.println(motion);
  }
}

// fungsi untuk mengubungkan ke broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe("servo/gerak");
      client.subscribe("ultrasonic/movement");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//=======================MQTT======================


// ==================Telegram BOT===================
#define BOT_TOKEN "6965871493:AAGFxTfgOqRF8AdTpRppgRzpvJtHSRU-B2U"

const unsigned long BOT_MTBS = 1000;  // mean time between scan messages

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;  // last time messages' scan has been done

void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    // String from_name = bot.messages[i].from_name;
    // if (from_name == "")
    //   from_name = "Guest";

    if (text == "/membuka") {
      client.publish("servo/gerak", "0");
      status = true;
      bot.sendMessage(chat_id, "Pintu Terbuka...", "");
    }
    if (text == "/menutup") {
      client.publish("servo/gerak", "1");
      status = false;
      bot.sendMessage(chat_id, "Pintu Tertutup...", "");
    }

    if (text == "/StatusPintu") {
      String message = "Status Pintu : ";
      if (status == true) {
        message += "Terbuka";
      } else {
        message += "Tertutup";
      }

      bot.sendMessage(chat_id, message, "");
    }

    if (text == "/start") {
      String welcome = "Selamat Datang \n";
      welcome += "Daftar Perintah :\n\n";
      welcome += "/membuka : Untuk Membuka Pintu\n";
      welcome += "/menutup : Untuk Menutup Pintu\n";
      welcome += "/StatusPintu : Menampilkan Status Pintu\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
    if (motion > 0 && motion < 100) {
      Serial.println("Motion Detected!!!");

      // digitalWrite(buzzerPin, HIGH);
      // delay(1000);
      // digitalWrite(buzzerPin, LOW);
      // delay(2000);

      // Notify motion detection via Telegram
      String message = "Motion Detected!!!\nSilahkan pilih aksi:\n/membuka - Buka pintu\n/menutup - Tutup pintu";
      bot.sendMessage(chat_id, message, "");
    }
  }
}
// ==================Telegram BOT===================

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Servo Pin Mode
  servo.attach(SERVO_PIN);

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");       // get UTC time via NTP untuk bot telegram
  secured_client.setTrustAnchors(&cert);  // Add root certificate for api.telegram.org
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // // Check NTP/Time, usually it is instantaneous and you can delete the code below.
  // Serial.print("Retrieving time: ");
  // time_t now = time(nullptr);
  // while (now < 24 * 3600)
  // {
  //   Serial.print(".");
  //   delay(100);
  //   now = time(nullptr);
  // }
  // Serial.println(now);

  client.setServer(mqtt_server, 1883);  // setup awal ke server mqtt
  client.setCallback(callback);
}

void loop() {
  if (status == false) {
  }
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}