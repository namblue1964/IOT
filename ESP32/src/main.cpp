  #include <WiFi.h>
  #include "DHT.h"
  #include "Adafruit_MQTT.h"
  #include "Adafruit_MQTT_Client.h"

  // =================== CẤU HÌNH WIFI ===================
  #define WIFI_SSID "Wokwi-GUEST"
  #define WIFI_PASS ""


  // =================== CẤU HÌNH ADAFRUIT IO ===================
  #define AIO_SERVER      "io.adafruit.com"
  #define AIO_SERVERPORT  1883
  #define AIO_USERNAME    "thanhnam"       // <-- ĐIỀN username Adafruit IO
  #define AIO_KEY         "aio_jfAD253kYzcDSXFJEIV3oddnj4Fr"               // <-- ĐIỀN AIO Key

  // =================== CHÂN CẢM BIẾN & RELAY ===================
  #define DHTPIN 13         // DHT22 DATA
  #define DHTTYPE DHT22
  #define RELAY1_PIN 15     // Relay 1 - LED 1
  #define RELAY2_PIN 4      // Relay 2 - LED 2

  // =================== KHAI BÁO BIẾN & ĐỐI TƯỢNG ===================
  DHT dht(DHTPIN, DHTTYPE);
  WiFiClient client;
  Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

  // Feed gửi dữ liệu
  Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
  Adafruit_MQTT_Publish humidityFeed    = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

  // Feed nhận điều khiển relay
  Adafruit_MQTT_Subscribe lightbulb1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lightbulb1");
  Adafruit_MQTT_Subscribe lightbulb2 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lightbulb2");

  // =================== KẾT NỐI MQTT ===================
  void MQTT_connect() {
    int8_t ret;
    if (mqtt.connected()) return;

    Serial.print(" - Kết nối MQTT...");
    while ((ret = mqtt.connect()) != 0) {
      Serial.println(mqtt.connectErrorString(ret));
      Serial.println(" - Thử lại sau 5s...");
      mqtt.disconnect();
      delay(5000);
    }
    Serial.println(" - MQTT Connected!");
  }

  // =================== SETUP ===================
  void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);

    Serial.println(" - Đang kết nối WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n - WiFi Connected!");

    mqtt.subscribe(&lightbulb1);
    mqtt.subscribe(&lightbulb2);
  }

  // =================== LOOP ===================
  void loop() {
    MQTT_connect();

    // Nhận lệnh điều khiển từ dashboard
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(10))) {
      if (subscription == &lightbulb1) {
        String cmd = (char *)lightbulb1.lastread;
        Serial.println(" - Lightbulb 1: " + cmd);
        if (cmd == "On")  digitalWrite(RELAY1_PIN, HIGH);
        else              digitalWrite(RELAY1_PIN, LOW);
      }
      if (subscription == &lightbulb2) {
        String cmd = (char *)lightbulb2.lastread;
        Serial.println(" - Lightbulb 2: " + cmd);
        if (cmd == "On")  digitalWrite(RELAY2_PIN, HIGH);
        else              digitalWrite(RELAY2_PIN, LOW);
      }
    }

    // Đọc dữ liệu cảm biến DHT22
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("Errol: Lỗi đọc DHT22 !!!");
      return;
    }

    Serial.printf("- Temp: %.2f °C | - Humidity: %.2f %%\n", t, h);
     // ====== 💡 THÊM ĐIỀU KIỆN ĐIỀU KHIỂN THEO NHIỆT ĐỘ ======
  if (t > 50.0) {
    digitalWrite(RELAY1_PIN, HIGH);   // Bật đèn (relay1)
    Serial.println("⚠️ Nhiệt độ cao > 50°C → Bật đèn cảnh báo!");
  } else {
    digitalWrite(RELAY1_PIN, LOW);    // Tắt đèn
    Serial.println("✅ Nhiệt độ bình thường → Tắt đèn cảnh báo.");
  }

    // Gửi dữ liệu lên Adafruit IO
    if (temperatureFeed.publish(t)) Serial.println("✅Đã gửi nhiệt độ");
    else Serial.println("Errol: Lỗi gửi nhiệt độ !!!");

    if (humidityFeed.publish(h)) Serial.println("✅Đã gửi độ ẩm");
    else Serial.println("Errol: Lỗi gửi độ ẩm !!!");

    delay(5000); // gửi mỗi 5s
  }