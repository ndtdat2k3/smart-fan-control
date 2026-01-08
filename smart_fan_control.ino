#include <WiFi.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>

// Cau hinh Wifi 
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server Thingsboard 
const char* mqtt_server = "YOUR_MQTT_SERVER";
const int mqtt_port = 1883;
const char* device_token = "YOUR_ACCESS_TOKEN";

// Chân Modbus 
#define RX_PIN 17
#define TX_PIN 16
#define DE_RE_PIN 4

// Object
ModbusMaster node;
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
unsigned long lastAttrMsg = 0;

int currentFanSpeed = 1; // toc do quat  

String fanSpeed;
String savedFanID = "";
String savedIP = "";
int savedLevels[3] = {0, 0, 0};

// Điều khiển chân RE/DE 
void preTransmission(){
  digitalWrite(DE_RE_PIN, HIGH); //chế độ gửi
}

void postTransmission(){
  digitalWrite(DE_RE_PIN, LOW); //chế độ nhận
}

// Kết nối Wifi 
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Dang ket noi Wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Da ket noi Wifi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Hàm kết nối lại (Reconnect)
void reconnect() {
  while (!client.connected()) {
    Serial.print("Dang ket noi lai ThingsBoard...");

    // Thử kết nối
    if (client.connect("ESP32_Wroom_Client", device_token, NULL)) {
      Serial.println("Thanh cong");
      
      // 1. Subscribe RPC 
      client.subscribe("v1/devices/me/rpc/request/+");

      // 2. Gửi Attributes 
      String ID = "123456abcdefg";
      String payload2 = "{";  
      payload2 += "\"Fan No.\":\"" + ID + "\",";
      payload2 += "\"IP address\":\"" + WiFi.localIP().toString() + "\",";
      payload2 += "\"level\": [1, 2, 3]";
      payload2 += "}";
      
      client.subscribe("v1/devices/me/attributes/response/+");

      client.publish("v1/devices/me/attributes", payload2.c_str());
      Serial.println("Da gui Attributes");

    }
    else {
      Serial.print("That bai, rc=");
      Serial.print(client.state());
      Serial.println(" thu lai sau 5 giay");
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String topicStr = String(topic);

  StaticJsonDocument<1024> doc; 
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("Loi JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // RPC
  if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
    Serial.println("Nhan RPC: " + message); 
    const char* method = doc["method"];

    if (String(method) == "setFanSpeed" || String(method) == "setValue") {
      int params = doc["params"];

      if (params >= 1 && params <= 3) {
        currentFanSpeed = params;
        Serial.printf("Da set toc do cua quat la: %d\n", currentFanSpeed);

        // phan hoi lai cho Thingsboard
        String requestId = topicStr.substring(topicStr.lastIndexOf('/') + 1);
        String responseTopic = "v1/devices/me/rpc/response/" + requestId;
        String responsePayload = String(currentFanSpeed);
        client.publish(responseTopic.c_str(), responsePayload.c_str());
      }
    }
  }

  // Attribute response
  else if (topicStr.startsWith("v1/devices/me/attributes/response/")) {
    Serial.print("Nhan du lieu Attributes response nhu sau: ");
    Serial.println(message);

  JsonObject data;

  if (doc.containsKey("client")) {
    data = doc["client"];
  }
  else {
    data = doc.as<JsonObject>();
  }

  if (data.containsKey("Fan No.")) {
      savedFanID = data["Fan No."].as<String>();
      Serial.println("Fan No.: " + savedFanID);
    }

    if (data.containsKey("IP address")) {
      savedIP = data["IP address"].as<String>();
      Serial.println("IP Server: " + savedIP);
    }

    if (data.containsKey("level")) {
      JsonArray arr = data["level"];
      Serial.print("level: ");
      for(int i=0; i<3; i++) {
        if (i < arr.size()) { 
           savedLevels[i] = arr[i];
           Serial.print(savedLevels[i]);
           Serial.print(" ");
        }
      }
      Serial.println();
    }
  }
}

void setup() {
  // 1. Khởi động Serial Monitor
  Serial.begin(115200);
  Serial.println("\n---HE THONG KHOI DONG---");

  // 2. Cấu hình chân RS485
  pinMode(DE_RE_PIN, LOW);
  
  // 3. Khởi động Modbus
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // 4. Cấu hình thư viện Modbus
  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // 5. Kết nối mạng
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);

  client.setBufferSize(2048);
}

void loop() {
  // Kiểm tra kết nối MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc cảm biến và gửi dữ liệu mỗi 5 giây
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    // Đọc Modbus 
    uint8_t result = node.readInputRegisters(0x0001, 2);

    if (result == node.ku8MBSuccess) {
      float temp = node.getResponseBuffer(0) / 10.0;
      float humid = node.getResponseBuffer(1) / 10.0;

      Serial.printf("Gia tri thu duoc la => Temp: %.1f C, Humid: %.1f %%RH\n", temp, humid);
      
      // payload1 gửi lên telemetry
      String payload = "{";
      payload += "\"temperature\":" + String(temp) + ",";
      payload += "\"humidity\":" + String(humid); 
      payload += "}";


      //Gửi mqtt
      client.publish("v1/devices/me/telemetry", payload.c_str());
      Serial.println("=> Da gui len Thingsboard!");

    }
    else {
      Serial.print("Loi doc Modbus: ");
      Serial.println(result, HEX);
    }
  }

  // gửi lên attributes mỗi 20 giây
  if (now - lastAttrMsg >= 20000) {
    lastAttrMsg = now;
    client.publish("v1/devices/me/attributes/request/1", "{\"clientKeys\":\"Fan No.,IP address,level\"}");
  }
}
