#include <WiFi.h>         //For ESP
#include <PubSubClient.h> //For MQTT
#include <ESPmDNS.h>      //For DNS & OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA
#include <rom/rtc.h>      //For ESP reset info

#define DEBUG_LEVEL 1     //0:No debug info, 1:Basic info, 3:Medium info, 5:All debug info

//WIFI Configuration
#define wifi_ssid "......"
#define wifi_password "......"

//MQTT Connection Configuration
#define mqtt_server "192.168.0.15"
#define mqtt_user "esp32"
#define mqtt_password "esp32password"
String mqtt_client_id="ESP32-";   //This text is concatenated with ChipId to get a unique client_id

//MQTT Topic configuration
String mqtt_base_topic="/home/room1/";
#define temperature_topic "/temperature"

//MQTT client
WiFiClient espClient;
PubSubClient mqtt_client(espClient);


//---------------------------------------------------------
// Function:  setup_wifi()
//
// Parameter:
// Returns:
// Desc: Configures WIFI connexion and connects to AP
//---------------------------------------------------------
void setup_wifi() {
  
  delay(10);
  #if (DEBUG_LEVEL>=1)
    Serial.print("   Connecting to ");
    Serial.print(wifi_ssid);
  #endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    #if (DEBUG_LEVEL>=3)
      delay(500);
      Serial.print(".");
    #endif
  }
  #if (DEBUG_LEVEL>=1)
    Serial.println("...OK");
    Serial.print("      IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

//---------------------------------------------------------
// Function:  setup()
//
// Parameter:
// Returns:
// Desc: Configures IO pins, Serial, WIFI, OTA, and MQTT
//---------------------------------------------------------
void setup() { 
  
  Serial.begin(115200);
  #if (DEBUG_LEVEL>=1)
    Serial.println("\r\nBooting...");
    Serial.print("   CPU0 reset reason: ");
    print_reset_reason(rtc_get_reset_reason(0));
    Serial.print("   CPU1 reset reason: ");
    print_reset_reason(rtc_get_reset_reason(1));
  #endif
  setup_wifi();
  uint64_t chipid;
  char HexChipId [17];
  chipid=ESP.getEfuseMac();
  sprintf (HexChipId, "%04X%08X",(uint32_t)(chipid>>32),(uint32_t)(chipid));
  HexChipId[16]='\0';
  #if (DEBUG_LEVEL>=1)
    Serial.printf("   Chip ID = %s\r\n",HexChipId);
    Serial.print("   Configuring OTA device...");
  #endif
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {Serial.println("OTA update finished!");Serial.println("Rebooting...");});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {Serial.printf("   OTA in progress: %u%%\r\n", (progress / (total / 100)));});  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  #if (DEBUG_LEVEL>=1)
    Serial.println("OK");
    Serial.println("   Configuring MQTT server:");
  #endif

  
  
  //mqtt_client_id=mqtt_client_id+(uint32_t)(chipid); 
  mqtt_client_id=mqtt_client_id+HexChipId;
  mqtt_base_topic=mqtt_base_topic+mqtt_client_id+"/data";
  mqtt_client.setServer(mqtt_server, 1883);  
  #if (DEBUG_LEVEL>=1)
    Serial.printf("      MQTT Server IP: %s\r\n",mqtt_server);  
    Serial.printf("      MQTT Username:  %s\r\n",mqtt_user);
    Serial.println("      MQTT Cliend Id: "+mqtt_client_id);  
    Serial.println("   MQTT configured!");
    Serial.println("Setup completed! Running app...");
  #endif
}


//---------------------------------------------------------
// Function:  print_reset_reason()
//
// Parameter:
//          reason: Reset Code
// Returns:
// Desc: Prints the ESP reset reason
//---------------------------------------------------------
void print_reset_reason(RESET_REASON reason){
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET");break;          /**<1,  Vbat power on reset*/
    case 3 : Serial.println ("SW_RESET");break;               /**<3,  Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET");break;             /**<4,  Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET");break;        /**<5,  Deep Sleep reset digital core*/
    case 6 : Serial.println ("SDIO_RESET");break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET");break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET");break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET");break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : Serial.println ("NO_MEAN");
  }
}


//---------------------------------------------------------
// Function:  mqtt_reconnect()
//
// Parameter:
// Returns:
//          0: Connection is estabilished
//          -1: Connection error
//
// Desc: Reconects to MQTT Server if disconnected
//---------------------------------------------------------
int mqtt_reconnect() {
  if (!mqtt_client.connected()) {
    // If you do not want to use a username and password, change next line to
    // if (client.connect(mqtt_user)) {    
    if (mqtt_client.connect(mqtt_client_id.c_str(), mqtt_user, mqtt_password)) {
      #if (DEBUG_LEVEL>=1)
        Serial.println("Connected to MQTT Server!");
      #endif
    } else {
      #if (DEBUG_LEVEL>=5)
        Serial.print("Attempting MQTT connection... failed, rc=");
        Serial.print(mqtt_client.state());
        Serial.println(" try again in 5 seconds");
      #endif
      return(-1);
    }
    return(0);
  }else{
    return(0);
  }
}


//---------------------------------------------------------
// Function:  send_mqtt_message()
//
// Parameter:
// Returns:
// Desc: Creates and sends the MQTT messages
//---------------------------------------------------------
void send_mqtt_message(){
  float readTemp = temperatureRead();
  readTemp=readTemp-45.3; //sw calibration
  #if (DEBUG_LEVEL>=1)
    Serial.print("   MQTT Sent: ");
    Serial.print(mqtt_base_topic+temperature_topic"  Value: ");
    Serial.println(String(readTemp).c_str());
  #endif
  mqtt_client.publish((mqtt_base_topic+temperature_topic).c_str(), String(readTemp).c_str(), true);
}


unsigned long int now=0;
bool mqtt_next_data_ready = false;
unsigned long int last_millis_send_new_data=0;
unsigned long int last_millis_send_last_data=0;
unsigned long int last_millis_mqtt_connection=0;
int mqtt_send_message_errors=0;
int mqtt_connection_errors=0;


//---------------------------------------------------------
// Function:  loop()
//
// Parameter:
// Returns:
// Desc: Main code
//---------------------------------------------------------
void loop() {
  
  //Keeps other threads running 
  delay(0);
  ArduinoOTA.handle();

  //Timming for non critical actions
  now=millis();

  //Check if there is new data to send
  if ((last_millis_send_new_data==0) || ((now-last_millis_send_new_data)>10000)){  //15 seconds
    last_millis_send_new_data=now;
    #if (DEBUG_LEVEL>=3)
      Serial.println("MQTT: New data ready to send!");
    #endif
    mqtt_next_data_ready=true;
    last_millis_mqtt_connection=0;
  }
  
  //Keep connected to mqtt server (1 try each 3s)
  if ((last_millis_mqtt_connection==0) || ((now-last_millis_mqtt_connection)>3000)){
    last_millis_mqtt_connection=now;
    if (mqtt_reconnect()==0){
      mqtt_client.loop();
      #if (DEBUG_LEVEL>=5)
        Serial.println("MQTT: Execute mqtt loop");
      #endif
    }else{
      mqtt_connection_errors++;
      if (mqtt_connection_errors>3){
        mqtt_connection_errors=0;
        #if (DEBUG_LEVEL>=1)
          Serial.println("   MQTT: Could not connect to MQTT server!");
        #endif
      }
    }
  }

  //Send or retry to send a message (1 try each 3 seccond)
  if ((mqtt_next_data_ready==true) && ((last_millis_mqtt_connection==0) || ((now-last_millis_send_last_data)>3000))){
    last_millis_send_last_data=now;
    if (mqtt_client.connected()) {
      send_mqtt_message();
      mqtt_next_data_ready=false;
    }else{
      mqtt_send_message_errors++;
      if(mqtt_send_message_errors>3) {
        mqtt_send_message_errors=0;
        mqtt_next_data_ready=false;
        #if (DEBUG_LEVEL>=1)
          Serial.println("MQTT: Message cound not be sent!");
        #endif
      }
    }
  }

}
