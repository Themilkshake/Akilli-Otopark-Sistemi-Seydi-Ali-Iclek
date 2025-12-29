#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

/* WiFi */
const char* ssid = "S2233D";
const char* password = "asdf4321.";

/* Adafruit IO */
#define AIO_USERNAME "sounhatt"
#define AIO_KEY      "aio_CXVM19Qy3R313vtIJ20bdA7EAw0V"
#define FEED_NAME    "parkdurum"

/* OLED */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* Ultrasonik Pinler */
#define TRIG_PIN D1
#define ECHO_PIN D2

/* Servo & Buton Pinleri */
#define SERVO_PIN  D8
#define BUTTON_PIN D7   //

WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;

unsigned long lastSend = 0;

void connectMQTT() {
  while (!client.connected()) {
    client.connect("ESP8266", AIO_USERNAME, AIO_KEY);
    delay(500);
  }
}

float uzaklik_kismi() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  /* Servo & Buton Ayarları */
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  myServo.attach(SERVO_PIN);
  myServo.write(0);   // Servo başlangıçta 0 derece

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  client.setServer("io.adafruit.com", 1883);

  Wire.begin(D5, D6);   // SDA = D6, SCL = D5
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}
int durum = 1;

void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();

  float distance = uzaklik_kismi();
  if (distance < 0) return;

  Serial.print("Mesafe: ");
  Serial.println(distance);


  if (millis() - lastSend >= 2000) {
    durum = (distance > 20) ? 1 : 0;
    String topic = String(AIO_USERNAME) + "/feeds/" + FEED_NAME;
    client.publish(topic.c_str(), String(durum).c_str());
    lastSend = millis();
  }

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.print(durum);
  display.print(" BOS");
  display.display();

  /* ---- SERVO + BUTON KONTROLÜ + SERİ PORT ---- */
  if (digitalRead(BUTTON_PIN) == HIGH) {     // Buton basılı
    myServo.write(180);
    Serial.println("BUTON: BASILI");
  } else {
    myServo.write(0);
    Serial.println("BUTON: BIRAKILDI");
  }
  delay(500);
}
