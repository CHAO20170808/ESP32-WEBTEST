/*
 * ESP32 電子紙顯示器 - 天氣 + HTTP API + 每日英文單字 + 中文顯示
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <SPI.h>
#include "ChineseFont.h"

// ==================== 1. 設定區域 ====================
const char* ssid = "你的個人wifi";
const char* password = "wifi密碼";

String apiKey = "你的個人key";
String city = "Taipei";
String countryCode = "TW";

#define API_MODE true
#define TIME_TO_SLEEP 1800000000ULL

// ==================== 2. 電子紙腳位與物件 ====================
#define EPD_BUSY 13
#define EPD_RST 12
#define EPD_DC 14
#define EPD_CS 27
#define EPD_SCK 18
#define EPD_MOSI 23

GxEPD2_BW<GxEPD2_290_T94, GxEPD2_290_T94::HEIGHT> display(
  GxEPD2_290_T94(/*CS=*/ 27, /*DC=*/ 14, /*RST=*/ 12, /*BUSY=*/ 13)
);

// 全域指標（供 ChineseFont.h 或內部繪圖函式存取）
//GxEPD2_GFX* displayPtr = &display;
Adafruit_GFX* displayPtr = &display; // GxEPD2 類別有繼承 Adafruit_GFX，這樣賦值就不會報錯


// ==================== 3. WebServer ====================
WebServer server(80);

// ==================== 4. 每日英文單字資料庫 ====================
struct WordEntry {
  const char* word;      // 英文單字
  const char* meaning;   // 中文意思 (UTF-8)
  const char* example;   // 英文例句
};
//---定義要顯示的中文字
WordEntry wordList[] = {
  {"Serendipity", "意外的美好發現", "Finding that book was pure serendipity."},
  {"Ephemeral", "短暫的", "Fame in social media is often ephemeral."},
  {"Eloquent", "雄辯的", "She gave an eloquent speech at the conference."},
  {"Resilient", "有適應力的", "Children are often more resilient than adults."},
  {"Ambiguous", "模糊不清的", "The contract contains ambiguous wording."},
  {"Pragmatic", "務實的", "We need a pragmatic approach to solve this."},
  {"Meticulous", "一絲不苟的", "She is meticulous about her appearance."},
  {"Tenacious", "頑強的", "A tenacious student never gives up easily."},
  {"Voracious", "貪吃的", "He is a voracious reader who finishes three books a week."},
  {"Ubiquitous", "無處不在的", "Smartphones have become ubiquitous in modern society."},
  {"Paradigm", "典範", "This discovery represents a new paradigm in physics."},
  {"Juxtapose", "並置", "The artist likes to juxtapose light and dark colors."},
  {"Lucid", "清晰的", "His explanation was clear and lucid."},
  {"Candid", "坦率的", "I appreciate your candid feedback."},
  {"Inevitable", "不可避免的", "Change is inevitable in life."},
  {"Versatile", "多才多藝的", "She is a versatile actress who can play any role."},
  {"Persist", "堅持", "If you persist, you will eventually succeed."},
  {"Derive", "源自", "Many English words derive from Latin."},
  {"Compromise", "妥協", "Both sides had to make compromises to reach an agreement."},
  {"Enhance", "增強", "The new features enhance user experience."},
  {"Abundant", "豐富的", "The region has abundant natural resources."},
  {"Maintain", "維持", "It is important to maintain a healthy lifestyle."},
  {"Obsolete", "過時的", "This technology has become obsolete."},
  {"Dilemma", "困境", "I'm in a dilemma about which job to take."},
  {"Catalyst", "催化劑", "The scandal was a catalyst for change."},
  {"Ironic", "諷刺的", "It's ironic that it rained on the day they planned a picnic."},
  {"Perceive", "感知", "How do you perceive the current situation?"},
  {"Profound", "深刻的", "The book had a profound impact on my thinking."},
  {"Subtle", "細微的", "There are subtle differences between the two versions."},
  {"Urge", "強烈欲望", "I have an urge to travel abroad."},
  {"Wander", "漫遊", "Let's wander through the old town."}
};

int wordCount = sizeof(wordList) / sizeof(wordList[0]);
int currentWordIndex = 0;

// ==================== 5. 結構體 ====================
struct WeatherData {
  float temp = 0.0;
  int humidity = 0;
  String mainInfo = "N/A";
  bool success = false;
};

// ==================== 6. 天氣函式 ====================
WeatherData fetchWeather() {
  WeatherData w;
  HTTPClient http;

  String url = "http://api.openweathermap.org/data/2.5/weather?q="
               + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      w.temp = doc["main"]["temp"];
      w.humidity = doc["main"]["humidity"];
      w.mainInfo = doc["weather"][0]["main"].as<String>();
      w.success = true;
    }
  }
  http.end();
  return w;
}

void drawWeatherUI(const WeatherData& w) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(10, 20);
    display.print("Weather: ");
    display.print(city);

    display.drawFastHLine(10, 28, display.width() - 20, GxEPD_BLACK);

    if (w.success) {
      display.setFont(&FreeSansBold18pt7b);
      display.setCursor(10, 60);
      display.print(w.temp, 1);
      display.print(" C");

      display.setFont(&FreeSans9pt7b);
      display.setCursor(10, 90);
      display.print("Cond: ");
      display.print(w.mainInfo);

      display.setCursor(130, 90);
      display.print("Hum: ");
      display.print(w.humidity);
      display.print("%");
    } else {
      display.setFont(&FreeSans9pt7b);
      display.setCursor(10, 60);
      display.print("Failed to load data.");
    }
  } while (display.nextPage());
}

// ==================== 7. 顯示函式 ====================
void drawText(const char* text) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold18pt7b);
    
    int16_t x, y;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x, &y, &w, &h);
    int16_t xpos = (display.width() - w) / 2;
    if (xpos < 0) xpos = 0;
    int16_t ypos = (display.height() - h) / 2 + h;
    if (ypos < 0) ypos = 0;
    
    display.setCursor(xpos, ypos);
    display.println(text);
  } while (display.nextPage());
}

void clearScreen() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

// ==================== 8. 繪製中文字 (使用點陣字型) ====================
void drawCNChar(int x, int y, uint16_t unicode, uint16_t color = GxEPD_BLACK) {
  for (int i = 0; i < CHINESE_FONT_COUNT; i++) {
    uint16_t stored_unicode = pgm_read_word(&chinese_fonts[i].unicode);
    if (stored_unicode == unicode) {
      const uint8_t* bitmap = chinese_fonts[i].bitmap;
      
      for (int row = 0; row < 16; row++) {
        for (int byte_idx = 0; byte_idx < 2; byte_idx++) {
          uint8_t row_data = pgm_read_byte(bitmap + row * 2 + byte_idx);
          for (int bit = 0; bit < 8; bit++) {
            if (!(row_data & (1 << (7 - bit)))) {
              displayPtr->drawPixel(x + byte_idx * 8 + bit, y + row, color);
            }
          }
        }
      }
      return;
    }
  }
}

// UTF-8 字串處理：取出中文字的 Unicode
uint16_t getChineseUnicode(const char*& p) {
  uint8_t c = (uint8_t)*p;
  if (c >= 0xE4 && c <= 0xE9) {
    // UTF-8 中文字 (3 bytes)
    uint16_t unicode = ((uint16_t)(c & 0x0F) << 12) |
                       ((uint16_t)(p[1] & 0x3F) << 6) |
                       (p[2] & 0x3F);
    p += 3;
    return unicode;
  }
  p++;
  return 0;
}

// 繪製混合文字（英文數字用內建字型，中文用點陣圖）
void drawMixedText(int x, int y, const char* text) {
  int cursor_x = x;
  const char* p = text;
  
  while (*p) {
    uint8_t c = (uint8_t)*p;
    
    if (c < 0x80) {
      // ASCII - 用內建字型
      displayPtr->drawPixel(cursor_x, y + 15, GxEPD_WHITE);  // 簡單的 8x16 點陣
      cursor_x += 8;
      p++;
    } else if (c >= 0xE4 && c <= 0xE9) {
      // 中文字
      uint16_t unicode = getChineseUnicode(p);
      drawCNChar(cursor_x, y, unicode);
      cursor_x += 16;
    } else {
      p++;
    }
  }
}

// ==================== 9. 每日單字顯示 (支援中文) ====================
void drawWord(const WordEntry& word) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    // ===== 第一行：英文單字（英文，用大字體） =====
    display.setFont(&FreeSansBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    
    int16_t x, y;
    uint16_t w, h;
    display.getTextBounds(word.word, 0, 0, &x, &y, &w, &h);
    int16_t xpos = (display.width() - w) / 2;
    if (xpos < 0) xpos = 0;
    
    display.setCursor(xpos, 25);
    display.println(word.word);

    // ===== 分隔線 =====
    display.drawFastHLine(10, 35, display.width() - 20, GxEPD_BLACK);

    // ===== 第二行：中文意思（用點陣字型） =====
    drawMixedText(10, 45, word.meaning);

    // ===== 第三行：英文例句（小字體） =====
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 95);
    display.print("e.g. ");
    display.print(word.example);

  } while (display.nextPage());
}

void drawNextWord() {
  currentWordIndex = (currentWordIndex + 1) % wordCount;
  drawWord(wordList[currentWordIndex]);
}

void drawCurrentWord() {
  drawWord(wordList[currentWordIndex]);
}

// ==================== 10. HTTP API ====================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32 電子紙控制</title>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#eee;padding:20px;text-align:center;}";
  html += "h1{color:#00d9ff;margin-bottom:20px;}";
  html += ".card{background:#16213e;border-radius:12px;padding:20px;max-width:500px;margin:0 auto;}";
  html += "input{width:100%;padding:12px;margin:10px 0;border:1px solid #333;border-radius:8px;background:#0f3460;color:#fff;font-size:16px;}";
  html += "button{width:100%;padding:12px;margin:5px 0;border:none;border-radius:8px;font-size:14px;cursor:pointer;}";
  html += ".btn-primary{background:linear-gradient(135deg,#00d9ff,#0099cc);color:#fff;}";
  html += ".btn-word{background:#9b59b6;color:#fff;}";
  html += ".btn-weather{background:#27ae60;color:#fff;}";
  html += ".btn-sleep{background:#e74c3c;color:#fff;}";
  html += ".status{margin-top:15px;padding:10px;background:#0f3460;border-radius:8px;font-size:14px;}";
  html += ".word-info{margin-top:15px;padding:15px;background:#2c3e50;border-radius:8px;text-align:left;}";
  html += ".word-info .word{font-size:20px;color:#00d9ff;margin-bottom:8px;}";
  html += ".word-info .meaning{color:#f39c12;margin-bottom:5px;}";
  html += ".word-info .example{color:#aaa;font-size:12px;}";
  html += "</style></head><body>";
  html += "<h1>ESP32 電子紙控制</h1>";
  html += "<div class='card'>";
  html += "<input type='text' id='textInput' placeholder='輸入要顯示的文字...'>";
  html += "<button class='btn-primary' onclick='sendText()'>顯示文字</button>";
  html += "<button class='btn-word' onclick='showWord()'>每日單字</button>";
  html += "<button class='btn-word' onclick='nextWord()'>下一個單字</button>";
  html += "<button class='btn-weather' onclick='showWeather()'>顯示天氣</button>";
  html += "<button class='btn-sleep' onclick='enterSleep()'>進入休眠</button>";
  html += "<div class='status' id='status'>Ready</div>";
  html += "<div class='word-info' id='wordInfo' style='display:none;'>";
  html += "<div class='word' id='wordWord'></div>";
  html += "<div class='meaning' id='wordMeaning'></div>";
  html += "<div class='example' id='wordExample'></div>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "let currentIdx=0;";
  html += "let wordData=[];";
  html += "async function sendText(){";
  html += "  var t=document.getElementById('textInput').value;";
  html += "  if(!t){document.getElementById('status').innerText='請輸入文字';return;}";
  html += "  var r=await fetch('/display?text='+encodeURIComponent(t));";
  html += "  var d=await r.json();";
  html += "  document.getElementById('status').innerText=d.success?'已顯示: '+t:d.message;";
  html += "}";
  html += "async function showWord(){";
  html += "  var r=await fetch('/word?action=current');";
  html += "  var d=await r.json();";
  html += "  if(d.success){";
  html += "    document.getElementById('wordWord').innerText=d.word;";
  html += "    document.getElementById('wordMeaning').innerText=d.meaning;";
  html += "    document.getElementById('wordExample').innerText='e.g. '+d.example;";
  html += "    document.getElementById('wordInfo').style.display='block';";
  html += "    document.getElementById('status').innerText='單字 #'+(d.index+1)+'/'+d.total;";
  html += "    currentIdx=d.index;";
  html += "  }";
  html += "}";
  html += "async function nextWord(){";
  html += "  var r=await fetch('/word?action=next');";
  html += "  var d=await r.json();";
  html += "  if(d.success){";
  html += "    document.getElementById('wordWord').innerText=d.word;";
  html += "    document.getElementById('wordMeaning').innerText=d.meaning;";
  html += "    document.getElementById('wordExample').innerText='e.g. '+d.example;";
  html += "    document.getElementById('wordInfo').style.display='block';";
  html += "    document.getElementById('status').innerText='單字 #'+(d.index+1)+'/'+d.total;";
  html += "    currentIdx=d.index;";
  html += "  }";
  html += "}";
  html += "async function showWeather(){";
  html += "  var r=await fetch('/weather');";
  html += "  var d=await r.json();";
  html += "  document.getElementById('status').innerText=d.success?'天氣已更新':'天氣更新失敗';";
  html += "}";
  html += "async function enterSleep(){";
  html += "  await fetch('/sleep');";
  html += "  document.getElementById('status').innerText='進入休眠模式';";
  html += "}";
  html += "window.onload=showWord;";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void handleDisplay() {
  String text = server.arg("text");
  if (text.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No text\"}");
    return;
  }
  text.replace("%20", " ");
  drawText(text.c_str());
  
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["message"] = "Displayed: " + text;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleWeather() {
  WeatherData w = fetchWeather();
  if (w.success) {
    drawWeatherUI(w);
  }
  
  DynamicJsonDocument doc(200);
  doc["success"] = w.success;
  doc["temp"] = w.temp;
  doc["humidity"] = w.humidity;
  doc["mainInfo"] = w.mainInfo;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleWord() {
  String action = server.arg("action");
  
  if (action == "next") {
    currentWordIndex = (currentWordIndex + 1) % wordCount;
  }
  
  drawWord(wordList[currentWordIndex]);
  
  DynamicJsonDocument doc(512);
  doc["success"] = true;
  doc["index"] = currentWordIndex;
  doc["total"] = wordCount;
  doc["word"] = wordList[currentWordIndex].word;
  doc["meaning"] = wordList[currentWordIndex].meaning;
  doc["example"] = wordList[currentWordIndex].example;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleClear() {
  clearScreen();
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["message"] = "Screen cleared";
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["mode"] = API_MODE ? "API" : "Weather+Sleep";
  doc["wordCount"] = wordCount;
  doc["currentWordIndex"] = currentWordIndex;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSleep() {
  display.hibernate();
  delay(1000);
  
  DynamicJsonDocument doc(200);
  doc["success"] = true;
  doc["message"] = "Entering deep sleep";
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  
  delay(1000);
  esp_deep_sleep_start();
}

void setupServer() {
  server.on("/", handleRoot);
  server.on("/display", handleDisplay);
  server.on("/weather", handleWeather);
  server.on("/word", handleWord);
  server.on("/clear", handleClear);
  server.on("/status", handleStatus);
  server.on("/sleep", handleSleep);
  server.begin();
  Serial.println("HTTP Server started");
}

// ==================== 11. 主程式 ====================
void setup() {
  Serial.begin(115200);
  delay(500);

  // 指定全域指標指向 display 物件
  displayPtr = &display;

  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(115200);
  display.setRotation(1);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed!");
  }

#if API_MODE
  setupServer();
  drawCurrentWord();
  Serial.println("API Mode Ready. Word count: " + String(wordCount));
  Serial.println("Chinese font chars: " + String(CHINESE_FONT_COUNT));
#else
  WeatherData data = fetchWeather();
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
  }
  
  if (data.success) {
    drawWeatherUI(data);
    Serial.printf("Weather OK: %.1f°C, %d%%\n", data.temp, data.humidity);
  }
  
  display.hibernate();
  delay(2000);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);
  Serial.println("Entering Deep Sleep...");
  esp_deep_sleep_start();
#endif
}

void loop() {
#if API_MODE
  server.handleClient();
#endif
}