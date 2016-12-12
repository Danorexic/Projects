
#include <ESP8266WiFi.h>
#include <DHT.h>;
#include <Wire.h>

#define wifi_ssid "The Cosmos"
#define wifi_password "jeepcherokee"

#define temp_topic "sensors/temp"
#define hum_topic "sensors/hum"

WiFiClient espClient;


//Constants
#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino


int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value
float temp_f; //Float for Fahrenheit

String temp_str; //see last code block below use these to convert the float that you get back from DHT to a string =str
String hum_str;
char tempchar[50];
char humchar[50];

// ThingSpeak Settings
const char* server = "api.thingspeak.com";
String apiKey = "32UYT53ND3QM3LC3";   
int sent = 0;

// Variable Setup
long lastConnectionTime = 0;
boolean lastConnected = false;


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

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup() {
  Serial.begin(115200);
  setup_wifi();
  Serial.print("Wifi Setup");
  dht.begin();
  delay(1500);
}



void sendTeperatureTS(float tempf, float hum)
{  
   WiFiClient clientwifi;
  
   if (clientwifi.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
   Serial.println("WiFi Client connected ");
   
   String postStr = apiKey;
   postStr += "&field3="; //1+2 are Room. 3+4 are Kitchen
   postStr += String(tempf);
   postStr += "&field4=";
   postStr += String(hum);
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
   Serial.println("Data published to Thingspeak");
   delay(1000);
   
   }//end if
   sent++;
 clientwifi.stop();
}//end send

void publish_temp() {
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    temp_f = (temp * 9.0)/5.0 + 32.0;

    temp_str = String(temp_f); //converting ftemp (the float variable above) to a string 
    temp_str.toCharArray(tempchar, temp_str.length() + 1); //packaging up the data to publish to mqtt whoa...

    hum_str = String(hum); //converting Humidity (the float variable above) to a string
    hum_str.toCharArray(humchar, hum_str.length() + 1); //packaging up the data to publish to mqtt whoa...

    sendTeperatureTS(temp_f, hum);
    delay(60000);
}
void loop() {
  publish_temp();
  }

