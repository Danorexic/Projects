
#include <ESP8266WiFi.h>
#include <DHT.h>;
#include <SFE_BMP180.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
 
#define wifi_ssid "The Cosmos"
#define wifi_password "jeepcherokee"

WiFiClient espClient;

//Constants
#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
float val = 0;                    // variable for reading the pin status

int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value
float temp_f; //Float for Fahrenheit
float pressure_TS; //Float for BMP180 val

String temp_str; //see last code block below use these to convert the float that you get back from DHT to a string =str
String hum_str;
String pressure_str; // String for BMP180
char tempchar[50];
char humchar[50];

// ThingSpeak Settings
const char* server = "api.thingspeak.com";
String apiKey = "3JC0MFQ581UYRKUD";   
int sent = 0;

// Variable Setup
long lastConnectionTime = 0;
boolean lastConnected = false;

//Weather Underground Setup
char WUNDERGROUND[] = "rtupdate.wunderground.com"; 
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
char ID [] = "KNCMONRO19";
char PASSWORD [] = "ej4td49o";

//BMP180 Setup
SFE_BMP180 pressure;
#define ALTITUDE 179.0 //Altitude for my house
double T,P,p0,a;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
 //OTA update setup 
 ArduinoOTA.setHostname("esp8266-outside-weather");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup() {

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  setup_wifi();
  dht.begin();
  delay(1500);
  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
}

void sendTeperatureTS(float tempf, float hum, float pressure_TS)
{  
   WiFiClient clientwifi;
  
   if (clientwifi.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
   Serial.println("WiFi Client connected ");
   
   String postStr = apiKey;
   postStr += "&field1=";
   postStr += String(tempf);
   postStr += "&field2=";
   postStr += String(hum);
   postStr += "\r\n\r\n";
   postStr += "&field3=";
   postStr += String(pressure_TS);
   postStr += "\r\n\r\n";
   
   clientwifi.print("POST /update HTTP/1.1\n");
   clientwifi.print("Host: api.thingspeak.com\n");
   clientwifi.print("Connection: close\n");
   clientwifi.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
   clientwifi.print("Content-Type: application/x-www-form-urlencoded\n");
   clientwifi.print("Content-Length: ");
   clientwifi.print(postStr.length());
   clientwifi.print("\n\n");
   clientwifi.print(postStr);
   delay(1000);
   
   }//end if
   sent++;
 clientwifi.stop();
}//end send

void publish_WU()
{
  WiFiClient clientwifi;
  
  if (clientwifi.connect(WUNDERGROUND, 80)) { 
    Serial.println("WiFi Client Connected - Wunderground ");
    // Ship it!
    clientwifi.print(WEBPAGE); 
    clientwifi.print("ID=");
    clientwifi.print(ID);
    clientwifi.print("&PASSWORD=");
    clientwifi.print(PASSWORD);
    clientwifi.print("&dateutc=");
    clientwifi.print("now");    //can use instead of RTC if sending in real time
    clientwifi.print("&tempf=");
    clientwifi.print(temp_f);
    clientwifi.print("&baromin=");
    clientwifi.print(pressure_TS);
    clientwifi.print("&humidity=");
    clientwifi.print(hum);
    //client.print("&action=updateraw");//Standard update
    clientwifi.print("&softwaretype=Arduino%20UNO%20version1&action=updateraw&realtime=1&rtfreq=2.5");//Rapid Fire
    clientwifi.println();
}
}

void publish_temp() {
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    temp_f = (temp * 9.0)/5.0 + 32.0;
    pressure_TS = (p0*0.0295333727);
    publish_WU();
    sendTeperatureTS(temp_f, hum, pressure_TS);
    delay(60000);
}
void loop() {
  ArduinoOTA.handle();
  // Begin BMP180 stuff
  char status;
  // double T,P,p0,a; moved to top of file

  // Loop here getting pressure readings every 10 seconds.

  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:
  
  Serial.println();
  Serial.print("provided altitude: ");
  Serial.print(ALTITUDE,0);
  Serial.print(" meters, ");
  Serial.print(ALTITUDE*3.28084,0);
  Serial.println(" feet");
  
  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.

  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.print(" deg C, ");
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(" deg F");
      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.print("absolute pressure: ");
          Serial.print(P,2);
          Serial.print(" mb, ");
          Serial.print(P*0.0295333727,2);
          Serial.println(" inHg");

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");

          // On the other hand, if you want to determine your altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).
          // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
          // Result: a = altitude in m.

          a = pressure.altitude(P,p0);
          Serial.print("computed altitude: ");
          Serial.print(a,0);
          Serial.print(" meters, ");
          Serial.print(a*3.28084,0);
          Serial.println(" feet");
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");

  publish_temp();

  }

