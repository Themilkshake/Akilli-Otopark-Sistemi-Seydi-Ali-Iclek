#include <ESP8266WiFi.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

/* ========== WiFi Bilgileri ========== */
const char* ssid = "S2233D";
const char* password = "asdf4321.";

/* ========== Adafruit IO Bilgileri ========== */
#define AIO_USERNAME "sounhatt"
#define AIO_KEY      "aio_CXVM19Qy3R313vtIJ20bdA7EAw0V"
#define FEED_NAME    "parkdurum"


/* ========== OLED Tanımı ========== */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ========== Pin Tanımları ========== */
#define TRIG_PIN   D1
#define ECHO_PIN   D2
#define BUTTON_PIN D3
#define SERVO_PIN  D4

Servo Servo;

/* ========== Mesafe Eşiği (cm) ========== */
const int distanceThreshold = 15;

/* ========== WiFi Bağlantısı ========== */
void setup_wifi() {
  Serial.print("WiFi baglaniyor: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi baglandi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

/* ========== Adafruit IO Veri Gönderme ========== */
void sendToAdafruit(int value) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url = "https://io.adafruit.com/api/v2/";
  url += AIO_USERNAME;
  url += "/feeds/";
  url += FEED_NAME;
  url += "/data";

  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");
  https.addHeader("X-AIO-Key", AIO_KEY);

  String json = "{\"value\":\"" + String(value) + "\"}";
  int httpCode = https.POST(json);

  Serial.print("Adafruit IO HTTP Kod: ");
  Serial.println(httpCode);

  https.end();
}

/* ========== Ultrasonik Mesafe Ölçüm ========== */
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

/* ========== Servo Bariyer ========== */
void openBarrier() {
  Servo.write(360);
  delay(3000);
  Servo.write(90);
}

/* ========== SETUP ========== */
void setup() {
  Serial.begin(115200);

  Wire.begin(D5, D6);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED baslatilamadi");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SEYDI ALI");
  display.println("ICLEK");
  display.display();
  delay(1000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  Servo.attach(SERVO_PIN);
  Servo.write(90);

  setup_wifi();
}

/* ========== LOOP ========== */
void loop() {

  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Buton basildi");
    openBarrier();
    delay(500);
  }

  long distance = getDistanceCM();
  Serial.print("Mesafe: ");
  Serial.print(distance);
  Serial.println(" cm");

  int parkDurum = (distance < distanceThreshold) ? 1 : 0;

  display.clearDisplay();
  display.setTextSize(6);
  display.setCursor(10, 10);
  display.println(parkDurum);
  display.display();

  sendToAdafruit(parkDurum);

  delay(1000);
}
