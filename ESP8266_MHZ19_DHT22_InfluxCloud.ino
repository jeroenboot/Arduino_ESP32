//#if defined(ESP32)
//#include <WiFiMulti.h>
//WiFiMulti wifiMulti;
//#define DEVICE "ESP32"
//#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
//#define DEVICE "ESP8266"
//#endif

#include <Arduino.h>
#include <MHZ19.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DEVICE "Slaapkamer"
#define WIFI_SSID "****"
#define WIFI_PASSWORD "****"
#define INFLUXDB_URL "https://westeurope-1.azure.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "FShS8yUmBlhOuQOpZVFq***EOBmUFoZLsQ1K296U55****"
#define INFLUXDB_ORG "1f*25***dd"
#define INFLUXDB_BUCKET "a0e36d0***3b**a3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

#define RX_PIN 5      //D1                                    // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 4      //D2                                    // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)

#define DHTPIN 12     //D6 // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors


// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("sensordata");

// MHZ19
MHZ19 myMHZ19;                                             // Constructor for library
SoftwareSerial mySerial(RX_PIN, TX_PIN);                   // (Uno example) create device to MH-Z19 serial
//HardwareSerial mySerial(1);                              // (ESP32 Example) create device to MH-Z19 serial

// DHT22
DHT dht(DHTPIN, DHTTYPE);



const int DELAY = 30000;
unsigned long getDataTimer = 0;

void setup()
{
  //serial
  Serial.begin(9600);                                     // Device to serial monitor feedback


  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  
  // Add tags for InfluxDB
  sensor.addTag("device", DEVICE);

  // Time sync
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  //MHZ19
  mySerial.begin(BAUDRATE);                               // (Uno example) device to MH-Z19 serial start
  //mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN); // (ESP32 Example) device to MH-Z19 serial start
  myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

  //DHT22
  dht.begin();

  //InfluxDB
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop()
{
  if (millis() - getDataTimer >= DELAY)
  {
    int CO2 = myMHZ19.getCO2();
    int8_t Temp = myMHZ19.getTemperature();
    Serial.print("MHZ19: CO2: ");
    Serial.print(CO2);
    Serial.print(" ppm \t");
    Serial.print("Temperature (C): ");
    Serial.print(Temp);
    Serial.println(" *C ");

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    Serial.print("DHT22: Hum: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.println(" *C");

    sensor.clearFields();  // Store measured value into point
    sensor.addField("ppm", CO2);  // Report CO2 ppm
    sensor.addField("temp", t);  // Report Temperature
    sensor.addField("hum", h);  // Report Humidity
    Serial.print("InfluxDB: Writing: ");
    Serial.println(sensor.toLineProtocol());
    if (!client.writePoint(sensor)) {  // Write point
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }

    getDataTimer = millis();
  }
}
