//Library files
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

//for smart config
const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "S0001";
boolean settingMode;
String ssidList;

extern String css;
extern String maincss;
extern String utilcss;
extern String first_half;
extern String second_half;


DNSServer dnsServer;
ESP8266WebServer webServer(80);

//Definitons
#define RELAY 2
#define INDICATOR 0
String   LINK_SSID;
String   LINK_PASSWORD;
const char*   ID = "S0001";
IPAddress     link(192,168,4,1);
WiFiClient    shock;

//Statas of the shock ON/OFF
typedef enum
{
  OFF = 0,
  ON,
}SHOCK_STATE_t;


// local variables
String value;
String out;
SHOCK_STATE_t response;
SHOCK_STATE_t state;


void setup() 
{
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  pinMode(INDICATOR, OUTPUT);

  //initialize the state of the shock as ON
  state = ON;
  digitalWrite(RELAY, state);

  //initialize idicator as not connected to the link
  digitalWrite(INDICATOR, HIGH);

  //for smart config
  EEPROM.begin(512);
  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();

}

void loop()
{
  if (settingMode) {
    dnsServer.processNextRequest();
  }
  else
  {
    if(Serial.available())
    {
        value = Serial.readStringUntil('\n');
        out = convert_to_json(ID, value);
        response = send_data(out);
        Serial.println(response);
  
        switch(state)
        {
          case ON:
            if(response==OFF){
              //relay off
              digitalWrite(RELAY, OFF);
              Serial.println("relay off");
              state = OFF;
            }
            else{Serial.println("no action");}
            break;
          case OFF:
            if(response==ON){
              //relay on
              digitalWrite(RELAY, ON);
              Serial.println("relay on");
              state = ON;
            }
            else{Serial.println("no action");}
            break;
          default:
            Serial.println("invalid response");
            break;
        }
     }
  }
  webServer.handleClient();

}


/* @brief
 *  convert data into json format
 *  
 * @param
 * id - id of the shock device
 * value - power readings data
 * 
 * @return
 * String json formatted 
 */
String convert_to_json(String id, String value)
{
  DynamicJsonBuffer jBuffer;
  JsonObject& root = jBuffer.createObject();
  root["id"]= id;
  root["value"]= value;
  String out;
  root.printTo(out);
  return out;
}


/* @brief
 * send data to the link
 *  
 * @param
 * out - data which should be sent to the link
 * 
 * @return
 * respons from the link
 */
SHOCK_STATE_t send_data(String out)
{
  Serial.println(out);
  if(!shock.connected())
  {
    digitalWrite(INDICATOR, HIGH); //indicate as disconnected
    checkConnection();
  }
  shock.println(out);
  //Serial.println(shock.readStringUntil('\n'));
  SHOCK_STATE_t resp = (SHOCK_STATE_t)(shock.readStringUntil('\n').toInt());
  //Serial.println(resp);
  return resp;
}



boolean restoreConfig() {
  Serial.println("Reading EEPROM...");
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      LINK_SSID += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(LINK_SSID);
    for (int i = 32; i < 96; ++i) {
      LINK_PASSWORD += char(EEPROM.read(i));
    }
    // To Avoid Broadcasting An SSID
    WiFi.mode(WIFI_STA);
    
    // The SSID That We Want To Connect To
    WiFi.begin(LINK_SSID.c_str(), LINK_PASSWORD.c_str());
    
    return true;
  }
  else {
    Serial.println("Config not found.");
    return false;
  }
}


/* @brief
 * check the connectivity with the link
 *  
 * @param
 * None
 * 
 * @return
 * NONE
 */
void connect_to_link()
{
  shock.stop();
  if(shock.connect(link, 9001))
  {
    digitalWrite(INDICATOR, LOW);
    Serial.println("<CONNECTED>");
    //shock.println ("<CONNECTED>");
  }
}


/* @brief
 * check the connectivity with the link
 *  
 * @param
 * None
 * 
 * @return
 * NONE
 */

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  Serial.println("!--- Connecting To " + WiFi.SSID() + " ---!");

  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("!-- shock Device Connected --!");
  
      // Printing IP Address
      Serial.println("Connected To      : " + String(WiFi.SSID()));
      Serial.println("Signal Strenght   : " + String(WiFi.RSSI()) + " dBm");
      Serial.print  ("link IP Address : ");
      Serial.println(link);
      Serial.print  ("Device IP Address : ");
      Serial.println(WiFi.localIP());
      
      // Conecting To The Link
      connect_to_link();
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

void startWebServer() {
  if (settingMode) {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s = first_half;
      s += ssidList;
      s += second_half;
      makePage("Wi-Fi Settings", s);
    });
    webServer.on("/setap", []() {
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      Serial.println(pass);
      Serial.println("Writing SSID to EEPROM...");
      for (int i = 0; i < ssid.length(); ++i) {
        EEPROM.write(i, ssid[i]);
      }
      Serial.println("Writing Password to EEPROM...");
      for (int i = 0; i < pass.length(); ++i) {
        EEPROM.write(32 + i, pass[i]);
      }
      EEPROM.commit();
      Serial.println("Write EEPROM done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      makePage("Wi-Fi Settings", s);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      makePage("AP mode", s);
    });
  }
  else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      makePage("STA mode", s);
    });
    webServer.on("/reset", []() {
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      makePage("Reset Wi-Fi Settings", s);
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(53, "*", apIP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

void makePage(String title, String contents) {
  String s1 = "<!DOCTYPE html><html><head>";
  s1 += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s1 += "<title>";
  s1 += title;
  s1 += "</title>";
  String s2 = maincss;
  s2 += utilcss;
  String s3 = css;
  s3 += "</head><body>";
  String s4 = contents;
  s4 = "</body></html>";
  webServer.setContentLength(s1.length()+s2.length()+s3.length()+s4.length());
  webServer.send(200, "text/html", s1);
  webServer.sendContent(s2);
  webServer.sendContent(s3);
  webServer.sendContent(s4);
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}


//====================================================================================
