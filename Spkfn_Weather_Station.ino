#include "arduino_secrets.h"
/* 
  Sketch generated by the Arduino IoT Cloud Thing "Spkfn_Weather_Station"
  https://create.arduino.cc/cloud/things/83dc2649-87db-48fa-bd56-1fd889677a3d 

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  float humidity;
  float lightningDistance;
  float pressure;
  float rain;
  float soilMoisture;
  float temperature;
  float uva;
  float uvb;
  float uvIndex;
  float windDirection;
  float windSpeed;
  bool lightningOccurred;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/

#include "thingProperties.h"

#define ARDUINO_CLOUD_PLAN_FREE   0
#define ARDUINO_CLOUD_PLAN_ENTRY  1
#define ARDUINO_CLOUD_PLAN_MAKER  2

// Set ARDUINO_CLOUD_PLAN to one of the above (ARDUINO_CLOUD_PLAN_xxxx)
#define ARDUINO_CLOUD_PLAN    ARDUINO_CLOUD_PLAN_FREE

// Set the polling period (in milliseconds)
#define POLLING_PERIOD  1000

// Libraries
#include <Wire.h>
#include <SPI.h>
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
#include "SparkFunBME280.h"
#include "SparkFun_VEML6075_Arduino_Library.h"
#include "SparkFun_AS3935.h"

// Pins
int pinWindDirection = 35;
int pinWindSpeed = 14;
int pinRainfall = 27;
int pinAS3935Int = 17; 
int pinAS3935CS = 25;
int pinSoilSignal = 34;
int pinSoilPower = 15;

// Sensors
SFEWeatherMeterKit weatherMeterKit(pinWindDirection, pinWindSpeed, pinRainfall);
BME280 myBME280;
VEML6075 myVEML6075;
SparkFun_AS3935 myAS3935;

// Flags to know whether sensors have been initialized
bool bme280Connected = false;
bool veml6075Connected = false;
bool as3935Connected = false;

/* You can decide which variables you want to use depending on your plan
 * Create the variable in the Thing and comment the declaration below */
#if ARDUINO_CLOUD_PLAN <= ARDUINO_CLOUD_PLAN_ENTRY
  // ENTRY plan

  float uva;
  float uvb;
# if ARDUINO_CLOUD_PLAN == ARDUINO_CLOUD_PLAN_FREE
  // FREE plan
  // Commented variables are the ones that should be defined in the Thing

  float humidity;
  //float pressure;
  float soilMoisture;
  //float rain;
  //float temperature;
  //float uvIndex;
  bool lightningOccurred;
  float lightningDistance;
  float windDirection;
  //float windSpeed;
# endif
#endif

unsigned long polling_period = POLLING_PERIOD;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 
  Serial.println("\r\nArduino Weather Station Demo");

  beginSensors();

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection, true);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo();
  Serial.println("Setup finished!");
}

void serial_print_data()
{
  Serial.println(String(millis()) + ":");
  Serial.println("- Temp=" + String(temperature) + ", Humidity=" + String(humidity));
  Serial.println("- Rain=" + String(rain) + ", Pressure=" + String(pressure));
  Serial.println("- Wind: Direction=" + String(windDirection) + ", Speed=" + String(windSpeed));
  Serial.println("- UVA=" + String(uva) + ", UVB=" + String(uvb) + ", UV Index=" + String(uvIndex));
  Serial.println("- Lightning: Distance=" + String(lightningDistance) + " Occurred:" + String(lightningOccurred));
  Serial.println("- SoilMoisture=" + String(soilMoisture));
}

unsigned long last_ts = 0;

void loop() {
  ArduinoCloud.update();
  // Your code here 

  unsigned long now = millis();
  if (now - last_ts > polling_period) {
    last_ts = now;
    recordSensorData();
  }
}


void beginSensors()
{
  // Set up pins
  pinMode(pinSoilSignal, INPUT);
  pinMode(pinSoilPower, OUTPUT);
  pinMode(pinAS3935Int, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinAS3935Int), lightningInterrupt, RISING);

  // Begin weather meter kit
  setWeatherMeterKitParams();
  weatherMeterKit.begin();

  // Begin communicaiton busses
  Wire.begin();
  SPI.begin(); 

  // Begin sensors on busses
  bme280Connected = myBME280.beginI2C();
  veml6075Connected = myVEML6075.begin();
  as3935Connected = myAS3935.beginSPI(pinAS3935CS);

  if(bme280Connected)
  {
    Serial.println("BME280 connected!");
  }
  else
  {
    Serial.println("BME280 not connected!");
  }

  if(veml6075Connected)
  {
    Serial.println("VEML6075 connected!");
  }
  else
  {
    Serial.println("VEML6075 not connected!");
  }

  if(as3935Connected)
  {
    Serial.println("AS3935 connected!");
  }
  else
  {
    Serial.println("AS3935 not connected!");
  }
}

void recordSensorData()
{
  // Weather meter kit data
  windDirection = weatherMeterKit.getWindDirection();
  windSpeed = weatherMeterKit.getWindSpeed();
  rain = weatherMeterKit.getTotalRainfall();
  
  // Soil data
  digitalWrite(pinSoilPower, HIGH);
  soilMoisture = analogRead(pinSoilSignal);
  digitalWrite(pinSoilPower, LOW);

  // BME280 data
  if(bme280Connected)
  {
    humidity = myBME280.readFloatHumidity();
    pressure = myBME280.readFloatPressure() / 100.0;
    temperature = myBME280.readTempC();
  }

  // VEML6075 data
  if(veml6075Connected)
  {
    uva = myVEML6075.uva();
    uvb = myVEML6075.uvb();
    uvIndex = myVEML6075.index();
  }

  // AS3935 data
  if(as3935Connected && lightningOccurred)
  {
    lightningOccurred = false;
    // Check if this is the lightning interrupt (0x08)
    if(myAS3935.readInterruptReg() == 0x08)
    {
      lightningDistance = myAS3935.distanceToStorm();
    }
    else
    {
      lightningDistance = 0;
    }
  }
  else
  {
    lightningDistance = 0;
  }
}

void setWeatherMeterKitParams()
{
  // Here we create a struct to hold all the calibration parameters
  SFEWeatherMeterKitCalibrationParams calibrationParams = weatherMeterKit.getCalibrationParams();

  // The wind vane has 8 switces, but 2 could close at the same time, which
  // results in 16 possible positions. Each position has a resistor connected
  // to GND, so this library assumes a voltage divider is created by adding
  // another resistor to VCC. Some of the wind vane resistor values are
  // fairly close to each other, meaning an accurate ADC is required. However
  // some ADCs have a non-linear behavior that causes this measurement to be
  // inaccurate. To account for this, the vane resistor values can be manually
  // changed here to compensate for the non-linear behavior of the ADC
  calibrationParams.vaneADCValues[WMK_ANGLE_0_0] = 3118;
  calibrationParams.vaneADCValues[WMK_ANGLE_22_5] = 1526;
  calibrationParams.vaneADCValues[WMK_ANGLE_45_0] = 1761;
  calibrationParams.vaneADCValues[WMK_ANGLE_67_5] = 199;
  calibrationParams.vaneADCValues[WMK_ANGLE_90_0] = 237;
  calibrationParams.vaneADCValues[WMK_ANGLE_112_5] = 123;
  calibrationParams.vaneADCValues[WMK_ANGLE_135_0] = 613;
  calibrationParams.vaneADCValues[WMK_ANGLE_157_5] = 371;
  calibrationParams.vaneADCValues[WMK_ANGLE_180_0] = 1040;
  calibrationParams.vaneADCValues[WMK_ANGLE_202_5] = 859;
  calibrationParams.vaneADCValues[WMK_ANGLE_225_0] = 2451;
  calibrationParams.vaneADCValues[WMK_ANGLE_247_5] = 2329;
  calibrationParams.vaneADCValues[WMK_ANGLE_270_0] = 3984;
  calibrationParams.vaneADCValues[WMK_ANGLE_292_5] = 3290;
  calibrationParams.vaneADCValues[WMK_ANGLE_315_0] = 3616;
  calibrationParams.vaneADCValues[WMK_ANGLE_337_5] = 2755;

  // The rainfall detector contains a small cup that collects rain water. When
  // the cup fills, the water is dumped and the total rainfall is incremented
  // by some value. This value defaults to 0.2794mm of rain per count, as
  // specified by the datasheet
  calibrationParams.mmPerRainfallCount = 0.2794;

  // The rainfall detector switch can sometimes bounce, causing multiple extra
  // triggers. This input is debounced by ignoring extra triggers within a
  // time window, which defaults to 100ms
  calibrationParams.minMillisPerRainfall = 100;

  // The anemometer contains a switch that opens and closes as it spins. The
  // rate at which the switch closes depends on the wind speed. The datasheet
  // states that a wind of 2.4kph causes the switch to close once per second
  calibrationParams.kphPerCountPerSec = 2.4;

  // Because the anemometer generates discrete pulses as it rotates, it's not
  // possible to measure the wind speed exactly at any point in time. A filter
  // is implemented in the library that averages the wind speed over a certain
  // time period, which defaults to 1 second. Longer intervals result in more
  // accurate measurements, but cause delay in the measurement
  calibrationParams.windSpeedMeasurementPeriodMillis = 1000;

  // Now we can set all the calibration parameters at once
  weatherMeterKit.setCalibrationParams(calibrationParams);
}

void lightningInterrupt()
{
  lightningOccurred = true;
}

