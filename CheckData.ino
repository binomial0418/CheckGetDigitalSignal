//1.啟動後從nodeMCU的ROM中取出WIFI設定。
//2.若取得合理WIFI密碼，則連線WIFI。若連線失敗自動進入AP模式讓使用者連線重設帳密。
//3.若沒有合理帳密，或是連續按三下flash鍵自動進入AP模式讓使用者連線重設帳密。
//4.進入AP模式後，請連接WIFI名稱為ESP-XXXX，之後進入192.168.4.1設定WIFI。
//5.感應器資料腳接D7 (13)


#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <EasyButton.h>
#include <TridentTD_LineNotify.h>

//設定flash鍵
// Arduino pin where the button is connected to.
#define BUTTON_PIN 0
#define LINE_TOKEN "wH2keHQ0lg7GRbPJFgasjUhXjsiQ0cqOKZqhno"

// Instance of the button.
EasyButton button(BUTTON_PIN);

//WIFI相關變數
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;
const char* ap_ssid       ;
const char* ap_password   ;
//const char* machine_nam   ;
const char* WIFI_FILE     = "/wifi.ini"; //存在ROM中的 wifi設定檔名稱

//進入AP模式後，nodeMCU啟動的網頁內容
AsyncWebServer server(80);
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "password";
const char* PARAM_INPUT_3 = "machine_nam";
bool        goAPMode      = true;
String mv_Machine_nam;
int resetCount = 1440; //重啟計數器
int pushButton = 13; //感應器接D7
boolean light = false;

// HTML web page to handle 2 input fields (input1, input2)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>  
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  </head><body>
  <form action="/get">
  <div style="border:10px orange solid;padding:15px;font-size:40px">
    請設定WIFI SSID與密碼：<br>
    <table>
    <tr><td>WIFI SSID    :</td><td> <input type="text" name="ssid" style="font-size:40px;"></td></tr>
    <tr><td>WIFI Password:</td><td> <input type="text" name="password" style="font-size:40px;"></td></tr>
    <tr><td>機器說明:</td><td> <input type="text" name="machine_nam" style="font-size:40px;"></td></tr>
    <tr><td><input type="submit" value="確認" style="font-size:80px;"></td></tr>
    </table>
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  String wifiIni;
 
  String ssid        = ""  ;
  String password    = ""  ;
  String machine_nam = ""  ;
  int timeoutCount   = 30;
  boolean flash = false;
  
  Serial.begin(115200);
  pinMode(pushButton, INPUT);
  //設定flash動作
  // Initialize the button.
  button.begin();
  // Add the callback function to be called when the button is pressed.
  //button.onPressed(onPressed); 
  button.onSequence(3, 1500, sequenceEllapsed);
  if (button.supportsInterrupt())
  {
    button.enableInterrupt(buttonISR);
    Serial.println("Button will be used through interrupts");
  }

  pinMode(LED_BUILTIN, OUTPUT);
  //讀取ROM中的wifi設定
  Serial.println(" ");
  wifiIni  = readConfigFile(WIFI_FILE);
  ssid     = getSsidPwdFromJson(wifiIni,"ssid");
  password = getSsidPwdFromJson(wifiIni,"pwd");
  machine_nam = getSsidPwdFromJson(wifiIni,"machine_nam");
  mv_Machine_nam = machine_nam; 
//  Serial.println("Saved wifi data:" + String(ssid) + String(password) );
  Serial.println("Saved ssid:" + String(ssid));
  Serial.println("Saved pwd:" + String(password) );
  Serial.println("Saved machine_nam:" + String(machine_nam));
  
  //有讀取到合理的wifi設定就開始連接WIFI，若無法成功連線，則自動進入AP模式。
  if (ssid.length() > 0 && password.length() > 0 && ssid != "0" && password != "0"){  
    Serial.println("WIFI MODE ");
    Serial.print(F("Wait for WiFi"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    while (WiFi.status() != WL_CONNECTED && timeoutCount > 0){
      flash = !flash;
      if (flash ==true){
        digitalWrite(LED_BUILTIN, HIGH);  
      }else{
        digitalWrite(LED_BUILTIN, LOW);  
      }
//      pinMode(LED_BUILTIN, flash);
      Serial.print(".");
      delay(1000);
      timeoutCount--;
    }
    goAPMode = timeoutCount == 0;
   
    if (goAPMode == true) {
      Serial.println("WiFi Failed!");
    }else{
      Serial.println();
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());   
      digitalWrite(LED_BUILTIN, LOW);  
      delay(300); 
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);  
      delay(300); 
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);  
      delay(300); 
      digitalWrite(LED_BUILTIN, HIGH); 
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);  
      delay(300); 
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);  
      delay(300); 
      digitalWrite(LED_BUILTIN, HIGH);
    }    
  }
  //wifi連線失敗，進入AP模式
  if (goAPMode == true){
    auto chipID = ESP.getChipId();
    pinMode(LED_BUILTIN, true);   
    Serial.println("AP MODE ");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(String("ESP-") + String(chipID, HEX)); 
    
    //以下處理網頁動作及取得內容
    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
              request->send_P(200, "text/html", index_html); 
    });
   //Send a GET request to <ESP_IP>/get?input1=<inputMessage>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String ssid;
    String pwd;
    String nam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      ssid = request->getParam(PARAM_INPUT_1)->value();
      pwd  = request->getParam(PARAM_INPUT_2)->value();
      nam  = request->getParam(PARAM_INPUT_3)->value();
      writeWifiConfigFile(WIFI_FILE,ssid,pwd,nam);
      ESP.restart();
    }
    //轉換網頁，按下http get後網頁跳轉動作
    request->send(200, "text/html", "<a href=\"/\">Return to Home Page</a>");                                     
  });
  server.onNotFound(notFound);
  server.begin();
  }
}

void loop()
{
  int buttonState;
  
  if (WiFi.status() == WL_CONNECTED && goAPMode == false){
    digitalWrite(LED_BUILTIN, LOW);  
    delay(500);
    //讀取感應器狀態
    buttonState = digitalRead(pushButton);
    if (buttonState ==1){
      Serial.println("open"); 
//      LINE.setToken(LINE_TOKEN);
//      LINE.notify("\n" + String(mv_Machine_nam) + "開啟");
    }
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
    resetCount--;
  }
  if (resetCount == 0){
    Serial.println("Reset..");
    ESP.restart();
    }
}
void onPressed() {
    Serial.println("Button has been pressed!");
}
//利用點三下flash按鈕重設wifi資訊
void sequenceEllapsed()
{
  Serial.println("reset wifi data");
  writeWifiConfigFile(WIFI_FILE,"0","0","0");
  ESP.restart();
}
void buttonISR()
{
  /*
    When button is being used through external interrupts, 
    parameter INTERRUPT must be passed to read() function
   */
  button.read();
}
//用從ROM中讀取檔案
String readConfigFile(String typ){
  String s;
  SPIFFS.begin();
  File data = SPIFFS.open(typ, "r");   
  
  size_t size = data.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  // Read and store file contents in buf
  data.readBytes(buf.get(), size-2);
  // Closing file
  data.close();
  //Serial.println(String(typ) + ":" + String(buf.get()));
  s = String(buf.get());
  return s;
}

//將WIFI資訊組成JSON寫入ROM中
void writeWifiConfigFile(String typ,String ssid,String pwd,String nam){
  String str;
  StaticJsonDocument<200> json_doc;
  char json_output[100];
  json_doc["ssid"] = ssid + "#";
  json_doc["pwd"]  = pwd + "#";
  json_doc["machine_nam"]  = nam + "#";
  serializeJson(json_doc, json_output);
 
  SPIFFS.begin();
  File f = SPIFFS.open(typ, "w");  
  if (!f) {
    Serial.println("Failed to open config file for writing");
  }
  f.println(json_output);
  f.close();
}

//從JSON字串中取出WIFI資訊
String getSsidPwdFromJson(String val,String field){
  StaticJsonDocument<200> json_doc;
  DeserializationError json_error;
  //const char* ssid;
  //const char* pwd;
  int i;
  
  json_error = deserializeJson(json_doc, val);
  if (!json_error) {
    String s    = String(json_doc[field]);
    i = s.indexOf('#');
    s = s.substring(0,i);
    //Serial.println(field + ":" + s);
    
    return s;
  }
}
