#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

const char* ssid = "ali";
const char* password = "123456789";
const char* mqtt_server = "192.168.137.96";

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C OLED ekran tanımı: Adres 0x3C, width, height, &Wire
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi ve MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Pin tanımlamaları
#define TRIG_PIN D1
#define ECHO_PIN D2
#define BUTTON_PIN D3
#define SERVO_PIN D4

// Nesneler
Servo Servo;

// Mesafe ölçüm eşiği (cm cinsinden)
const int distanceThreshold = 15;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("WiFi bağlantısı kuruluyor: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi bağlı!");
  Serial.print("IP adresi: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT bağlanıyor...");
    if (client.connect("OtoparkESP")) {
      Serial.println("bağlandı!");
    } else {
      Serial.print("bağlantı başarısız, rc=");
      Serial.print(client.state());
      Serial.println(" 5 saniye içinde tekrar denenecek...");
      delay(5000);
    }
  }
}

void setup() {

  Wire.begin(D5, D6);  // ESP8266 için SDA = D2, SCL = D1
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED başlatılamadı"));
    for (;;);  // Sonsuz döngü
  }
  Servo.write(5);

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SEYDI ALI ICLEK");
  display.display();
  delay(1000);


  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);  // Buton D3'e bağlı ve aktif LOW
  Servo.attach(SERVO_PIN);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  
}

// Ultrasonik mesafe ölçüm fonksiyonu
long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

// Servo bariyeri kaldır
void openBarrier() {
  Servo.write(360); // Aç
  delay(3000);
  Servo.write(90);  // Kapat
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Butona basıldıysa bariyeri aç
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Butona basıldı, bariyer açılıyor...");
    openBarrier();
    delay(500); // Buton bouncing önleme
  }

  // Mesafe ölç ve MQTT ile gönder
  long distance = getDistanceCM();
  Serial.print("Mesafe: ");
  Serial.print(distance);
  Serial.println(" cm");

  int parkDurum = (distance < distanceThreshold) ? 1 : 0;
  display.clearDisplay();
  display.setTextSize(6);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 5);
  display.println(parkDurum);
  display.display();

  char msg[2];
  snprintf(msg, 2, "%d", parkDurum);
  client.publish("parkDurum", msg); 

  delay(1000); // Her saniyede bir gönderim
}
