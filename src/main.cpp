#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include "SparkFun_SGP30_Arduino_Library.h"
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h"
#include <SoftwareSerial.h> // Include software serial library, ESP8266 library dependency
#include <SparkFunESP8266WiFi.h> // Include the ESP8266 AT library

const char SSID[] = "HolePatrol";
const char PSK[] = "Maggerdochez";

uint8_t loopCnt = 0;

Adafruit_Si7021 sensor;
SGP30 sgp30;
SFE_PARTICLE_SENSOR particleAirSensor;

boolean foundSi7021 = false;
boolean foundSgp30 = false;
boolean foundSFEParticleAirSensor = false;

// errorLoop prints an error code, then loops forever.
void errorLoop(int error)
{
  Serial.print(F("Error: ")); Serial.println(error);
  Serial.println(F("Looping forever."));
  for (;;)
    ;
}

// serialTrigger prints a message, then waits for something
// to come in from the serial port.
void serialTrigger(String message)
{
  Serial.println();
  Serial.println(message);
  Serial.println();
  while (!Serial.available())
    ;
  while (Serial.available())
    Serial.read();
}

void initializeESP8266()
{
  // esp8266.begin() verifies that the ESP8266 is operational
  // and sets it up for the rest of the sketch.
  // It returns either true or false -- indicating whether
  // communication was successful or not.
  // true
  int test = esp8266.begin();
  if (test != true)
  {
    Serial.println(F("Error talking to ESP8266."));
    errorLoop(test);
  }
  Serial.println(F("ESP8266 Shield Present"));
}

void connectESP8266()
{
  // The ESP8266 can be set to one of three modes:
  //  1 - ESP8266_MODE_STA - Station only
  //  2 - ESP8266_MODE_AP - Access point only
  //  3 - ESP8266_MODE_STAAP - Station/AP combo
  // Use esp8266.getMode() to check which mode it's in:
  int retVal = esp8266.getMode();
  if (retVal != ESP8266_MODE_STA)
  { // If it's not in station mode.
    // Use esp8266.setMode([mode]) to set it to a specified
    // mode.
    retVal = esp8266.setMode(ESP8266_MODE_STA);
    if (retVal < 0)
    {
      Serial.println(F("Error setting mode."));
      errorLoop(retVal);
    }
  }
  Serial.println(F("Mode set to station"));

  // esp8266.status() indicates the ESP8266's WiFi connect
  // status.
  // A return value of 1 indicates the device is already
  // connected. 0 indicates disconnected. (Negative values
  // equate to communication errors.)
  retVal = esp8266.status();
  if (retVal <= 0)
  {
    Serial.print(F("Connecting to "));
    Serial.println(SSID);
    // esp8266.connect([ssid], [psk]) connects the ESP8266
    // to a network.
    // On success the connect function returns a value >0
    // On fail, the function will either return:
    //  -1: TIMEOUT - The library has a set 30s timeout
    //  -3: FAIL - Couldn't connect to network.
    retVal = esp8266.connect(SSID, PSK);
    if (retVal < 0)
    {
      Serial.println(F("Error connecting"));
      errorLoop(retVal);
    }
  }
}

void displayConnectInfo()
{
  char connectedSSID[24];
  memset(connectedSSID, 0, 24);
  // esp8266.getAP() can be used to check which AP the
  // ESP8266 is connected to. It returns an error code.
  // The connected AP is returned by reference as a parameter.
  int retVal = esp8266.getAP(connectedSSID);
  if (retVal > 0)
  {
    Serial.print(F("Connected to: "));
    Serial.println(connectedSSID);
  }

  // esp8266.localIP returns an IPAddress variable with the
  // ESP8266's current local IP address.
  IPAddress myIP = esp8266.localIP();
  Serial.print(F("Assigned IP: "));
  Serial.println(myIP);
}

void setup() {
  Serial.begin(9600);

  // wait for serial port to open
  while (!Serial) {
    delay(10);
  }

  // initializeESP8266() verifies communication with the WiFi
  // shield, and sets it up.
  initializeESP8266();

  // connectESP8266() connects to the defined WiFi network.
  connectESP8266();

  // displayConnectInfo prints the Shield's local IP
  // and the network it's connected to.
  displayConnectInfo();

  Wire.begin();
  Wire.setClock(400000);

  Serial.println("Looking for Si7021 sensor.");

  if (sensor.begin()) {
    Serial.println("Found Si7021 sensor!");
    foundSi7021 = true;
    Serial.print("Found model ");
    switch(sensor.getModel()) {
      case SI_Engineering_Samples:
        Serial.print("SI engineering samples"); break;
      case SI_7013:
        Serial.print("Si7013"); break;
      case SI_7020:
        Serial.print("Si7020"); break;
      case SI_7021:
        Serial.print("Si7021"); break;
      case SI_UNKNOWN:
      default:
        Serial.print("Unknown");
    }
    Serial.print(" Rev(");

    Serial.print(sensor.getRevision());
    Serial.print(")");
    Serial.print(" Serial #"); Serial.print(sensor.sernum_a, HEX); Serial.println(sensor.sernum_b, HEX);
  }
  else {
    Serial.println("Did not find Si7021 sensor!");
  }

  if (sgp30.begin()) {
    Serial.println("Found SGP30.");
    foundSgp30 = true;
    //Initializes sensor for air quality readings
    //measureAirQuality should be called in one second increments after a call to initAirQuality
    sgp30.initAirQuality();
  }
  else {
    Serial.println("No SGP30 Detected. Check connections.");
  }

  if (particleAirSensor.begin()) {
    Serial.println("Particle sensor found.");
    foundSFEParticleAirSensor = true;
  }
  else {
    Serial.println("The particle sensor did not respond. Please check wiring.");
  }
  delay(1000);
}

void loop() {
  if (foundSi7021) {
    Serial.print("Humidity:    ");
    Serial.print(sensor.readHumidity(), 2);
    Serial.print("\tTemperature: ");
    Serial.print(sensor.readTemperature(), 2);
  }
  if (foundSgp30) {
    SGP30ERR error = sgp30.measureAirQuality();
    if (error == SUCCESS) {
      Serial.print("\tCO2: ");
      Serial.print(sgp30.CO2);
      Serial.print(" ppm\tTVOC: ");
      Serial.print(sgp30.TVOC);
      Serial.print(" ppb\tH2: ");
      Serial.print(sgp30.H2);
    } else if (error == ERR_BAD_CRC) {
      Serial.print("CRC Failed");
    } else if (error == ERR_I2C_TIMEOUT) {
      Serial.print("I2C Timed out");
    }
  }
  Serial.println();

  if (foundSFEParticleAirSensor) {
    float pm1_0 = particleAirSensor.getPM1_0();
    Serial.print("PM 1.0: ");
    Serial.print(pm1_0, 2); //Print float with 2 decimals
    Serial.print("\tPM 2.5:");
    float pm2_5 = particleAirSensor.getPM2_5();
    Serial.print(pm2_5, 2);
    Serial.print("\tPM 10: ");
    float pm10 = particleAirSensor.getPM10();
    Serial.print(pm10, 2);
    Serial.print("\tPC 0.5: ");
    unsigned int pc0_5 = particleAirSensor.getPC0_5();
    Serial.print(pc0_5);
    Serial.print("\tPC 1: ");
    unsigned int pc1_0 = particleAirSensor.getPC1_0();
    Serial.print(pc1_0);
    Serial.print("\tPC 2.5: ");
    unsigned int pc2_5 = particleAirSensor.getPC2_5();
    Serial.print(pc2_5);
    Serial.print("\tPC 5: ");
    unsigned int pc5_0 = particleAirSensor.getPC5_0();
    Serial.print(pc5_0);
    Serial.print("\tPC 7.5: ");
    unsigned int pc7_5 = particleAirSensor.getPC7_5();
    Serial.print(pc7_5);
    Serial.print("\tPC 10: ");
    unsigned int pc10 = particleAirSensor.getPC10();
    Serial.print(pc10);
    Serial.println();
  }
  delay(1000);
}