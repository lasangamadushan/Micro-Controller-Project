#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

char*       ssid = "Link";              // Link ssid
char*       password = "abc123abc";          // Link password
#define     MAXSC     10           // maximum number of shocks
WiFiServer  server(9001);      // Link server
WiFiClient  shocks[MAXSC];     // shocks array

ESP8266WebServer webServer(80);

typedef enum
{
  OFF = 0,
  ON,
}SHOCK_STATE_t;

SHOCK_STATE_t state = OFF;

String makepage()
{
  String s = R"=====(
  <!DOCTYPE html><html>
  <head></head>
  <body>
  <ol>
  )=====";
  for(int i=0;i<MAXSC;i++)
  {
    String ip = shocks[i].remoteIP ().toString();
    if(ip != "0.0.0.0")
    {
      s += "<a href=\"http://"+ip+"\"><li>" + ip + "</li></a><br>";
    }
  }

  s += R"=====(
  </ol>
  </body>
  </html>
  )=====";
  return s;
}

void handleRoot() {
  String s = makepage();
  Serial.println(s);
  webServer.send(200, "text/html", s);
}

void setup()
{
  Serial.begin(115200);
  SetWifi();
  webServer.on("/", handleRoot);
  webServer.begin();
}

void loop()
{
  if(Serial.available())
  {
    state = (SHOCK_STATE_t)Serial.readStringUntil('\n').toInt();
  }
  IsClients();
  webServer.handleClient();
  
}
void SetWifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  Serial.println("WIFI Mode : AccessPoint Station");
  
  WiFi.softAP(ssid, password);
  Serial.println("WIFI < " + String(ssid) + " > ... Started");
  
  delay(1000);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AccessPoint IP : ");
  Serial.println(IP);

  // Starting Server
  server.begin();
  Serial.println("Server Started");
}



void IsClients()
{
  if (server.hasClient())
  {
    Serial.println("");
    for(int i = 0; i < MAXSC; i++)
    {
      //find free/disconnected spot
      if (!shocks[i] || !shocks[i].connected())
      {
        if(shocks[i]) shocks[i].stop();
        shocks[i] = server.available();
        Serial.print("New Client : "); Serial.print(String(i+1) + " - ");
        continue;
      }
    }
    // no free / disconnected spot so reject
    WiFiClient shocks = server.available();
    shocks.stop();
  }
  
  //check clients for data -------------------------------------------------------
  
  for(int i = 0; i < MAXSC; i++)
  {
    if (shocks[i] && shocks[i].connected())
    {
      if(shocks[i].available())
      {
        // If Any Data Was Available We Read IT
        while(shocks[i].available()) 
        {
          // Read From Client
          String data = shocks[i].readStringUntil('\n');
          DynamicJsonBuffer jBuffer;
          JsonObject& jObject = jBuffer.parseObject(data);
          String id = jObject["id"];
          String value = jObject["value"];
          Serial.print(id);
          Serial.print(",");
          Serial.println(value);
          //state = (SHOCK_STATE_t)Serial.readStringUntil('\n').toInt();
          // Reply To Client
          shocks[i].println(state);
          Serial.println(state);
          
        }
      }
    }
  }
}

//====================================================================================
