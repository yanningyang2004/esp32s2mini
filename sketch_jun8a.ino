#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <WiFi.h>
#include <WebServer.h>

// 定义引脚
#define BUZZER_PIN 18       // 蜂鸣器引脚
#define ONE_WIRE_BUS 34     // DS18B20 数据引脚

// WiFi配置
const char* ssid = "nova";
const char* password = "66666666";

// 初始化传感器
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
BH1750 lightSensor;
WebServer server(80);

// OLED 显示屏初始化
#define SCREEN_WIDTH 128 // OLED 显示屏宽度，单位像素
#define SCREEN_HEIGHT 64 // OLED 显示屏高度，单位像素
#define OLED_RESET     -1 // 重置引脚
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 阈值设置
const float TEMP_THRESHOLD = 30.0; // 温度阈值，单位摄氏度
const float LIGHT_THRESHOLD = 500.0; // 光照阈值，单位 lux

// 全局变量存储传感器数据
float temperature = 0;
uint16_t lightLevel = 0;

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32-S2 环境监测</h1>";
  html += "<p>温度: " + String(temperature, 1) + " °C</p>";
  html += "<p>光照: " + String(lightLevel) + " lux</p>";
  html += "<p>更新时间: " + String(millis() / 1000) + "秒前</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // 确保蜂鸣器初始关闭

  // 初始化I2C总线
  Wire.begin(); // 使用默认SDA=21, SCL=22 (ESP32)
  Wire.setClock(400000); // 设置I2C速度为400kHz

  // 初始化 OLED 显示屏
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.display();
  delay(1000);

  // 显示启动信息
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // 初始化 DS18B20 传感器
  sensors.begin();
  Serial.println("DS18B20 initialized");

  // 初始化 BH1750 光照传感器
  if (!lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("Error initializing BH1750"));
    display.setCursor(0, 20);
    display.println("BH1750 init failed!");
    display.display();
    while (1);
  }
  Serial.println("BH1750 initialized");

  // 连接WiFi
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting to WiFi");
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }

  // 显示系统信息
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Ready");
  display.setCursor(0, 10);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  
  Serial.println("");
  Serial.println("WiFi连接成功");
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());

  // 启动Web服务器
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void readSensors() {
  // 读取温度
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  
  if (temperature == DEVICE_DISCONNECTED_C) {
    Serial.println(F("DS18B20 read error"));
    temperature = -99.9; // 错误值
  }

  // 读取光照
  lightLevel = lightSensor.readLightLevel();
  
  if (lightLevel == 65535) { // BH1750错误值
    Serial.println(F("BH1750 read error"));
    lightLevel = 0;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // 显示温度
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");
  
  // 显示光照
  display.setCursor(0, 10);
  display.print("Light: ");
  display.print(lightLevel);
  display.println(" lux");
  
  // 显示IP地址
  display.setCursor(0, 20);
  display.print("IP: ");
  display.println(WiFi.localIP());
  
  // 显示阈值状态
  display.setCursor(0, 30);
  if (temperature > TEMP_THRESHOLD || lightLevel > LIGHT_THRESHOLD) {
    display.println("ALARM! Threshold Exceeded");
  } else {
    display.println("Normal");
  }
  
  display.display();
}

void checkThresholds() {
  if (temperature > TEMP_THRESHOLD || lightLevel > LIGHT_THRESHOLD) {
    Serial.println("Threshold exceeded!");
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
    }
  }
}

void loop() {
  // 读取传感器数据
  readSensors();
  
  // 更新显示
  updateDisplay();
  
  // 检查阈值
  checkThresholds();
  
  // 处理Web请求
  server.handleClient();
  
  delay(1000); // 每秒更新一次
}