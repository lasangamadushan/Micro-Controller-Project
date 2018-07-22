//Library files
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

//Definitons
#define RELAY 2
const char*   LINK_SSID = "Link";
const char*   LINK_PASSWORD = "abc123abc";
const char*   ID = "shock 1";
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

  //initialize the state of the shock as ON
  state = ON;
  digitalWrite(RELAY, ON);
  
  // To Avoid Broadcasting An SSID
  WiFi.mode(WIFI_STA);
  
  // The SSID That We Want To Connect To
  WiFi.begin(LINK_SSID, LINK_PASSWORD);
  Serial.println("!--- Connecting To " + WiFi.SSID() + " ---!");

  // Checking For Connection
  check_connectivity();
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

}

void loop()
{
  if(Serial.available())
  {
      value = Serial.readStringUntil('\n');
      out = convert_to_json(ID, value);
      response = send_data(out);
      Serial.println(response);
      Serial.println(String(response==ON));

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
    connect_to_link();
  }
  shock.println(out);
  //Serial.println(shock.readStringUntil('\n'));
  SHOCK_STATE_t resp = (SHOCK_STATE_t)(shock.readStringUntil('\n').toInt());
  //Serial.println(resp);
  return resp;
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
void check_connectivity()
{
  while(WiFi.status() != WL_CONNECTED)
  {
    for(int i=0; i < 10; i++)
    {
      Serial.print(".");
    }
    Serial.println("");
    delay(1000);
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
    Serial.println("<CONNECTED>");
    //shock.println ("<CONNECTED>");
  }
}

//====================================================================================
