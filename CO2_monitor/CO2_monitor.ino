/******************************************************************
 * CO2 control project
 * 
 * Features:
 *    Co2 sensor
 *    User set desired co2 level
 *    Autonomously controlled solenoid
 *    Four fans to mix air
 *    User set number of fans running
 *    Two lcd screens to display information
 *    Button to turn on/off screen backlights
 *    Write to SD card and google docs
 *    
 * Written by:
 *    Zac Carico and James Thomas
 *****************************************************************/

 /*
  * Possible scheduler: protothread library
  */

/******************************************************************
 *  Arduino Setup:
 *    Digital Pins:
 *      Wifi:                   0(Rx), 1(Tx)
 *      Fans:                   2, 3, 4, 5 
 *      Solenoid:               7
 *      CO2 sensor:             12(Rx), 13(Tx)
 *      **Free:                 6, 8, 9, 10, 11
 * 
 *    Analog Pins:
 *      Fan Pot:                0 
 *      CO2 Pots:               1, 2
 *      I2c:                    4(SDA), 5(SCL) // SD card, 2 screens
 *      **Free:                 3
 *****************************************************************/

//Arduino Libraries
#include <Arduino_FreeRTOS.h>
//#include <semphr.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

//Project Libraries
#include "fan.h"
#include "CO2_Sensor.h"

//SEMAPHORES
//SemaphoreHandle_t xLevelMutex;
//SemaphoreHandle_t xTargetMutex;
// QueueHandle_t xDisplayQueue;

//GLOBAL VARIABLES
volatile unsigned long CO2_target = 100;
volatile unsigned long CO2_level = 100;
LiquidCrystal_I2C lcd2(0x27,2,1,0,4,5,6,7);
LiquidCrystal_I2C lcd1(0x20,2,1,0,4,5,6,7);
SoftwareSerial sensorSerial(12, 13);        // RX, TX pins on Ardunio
CO2_Sensor mySensor(&sensorSerial);         // Set up sensor with software serial object.
uint8_t numFans = 0;
Fans fans = Fans(2, 3, 4, 5);
bool refreshDisplay = true;
uint8_t solenoidPin = 7;

//Prototypes
//void writeToCloud(void *pvParameters);
void readCO2_sensor(void *pvParameters);
void manageCO2_levels(void *pvParameters);
void setUpHardware();

void setup() 
{
  Serial.begin(9600);
  setUpHardware();

  //define Semaphores
//  if(xLevelMutex == NULL)
//  {
//    xLevelMutex = xSemaphoreCreateMutex();
//    if( (xLevelMutex) != NULL)
//    {
//      xSemaphoreGive((xLevelMutex));
//    }
//  }
//
//  if(xTargetMutex == NULL)
//  {
//    xTargetMutex = xSemaphoreCreateMutex();
//    if( (xTargetMutex) != NULL)
//    {
//      xSemaphoreGive((xTargetMutex));
//    }
//  }

  xTaskCreate(manageCO2_levels, "mngCO2", 
              100, NULL, 3, NULL);
  xTaskCreate(readCO2_sensor, "rdCO2", 
              100, NULL, 3, NULL);
//   xTaskCreate(writeToCloud, "wrt2cld", 
//               100, NULL, 1, NULL);
}

void loop() 
{
  //Tasks during down time, or delays.
  uint8_t tempFans = numFans;
  numFans = map(analogRead(A0), 0, 1024, 0, 5);

  if (numFans != tempFans)
  {
    fans.on(numFans);
    refreshDisplay = true;
  }
  if(refreshDisplay)
  {
    // Reset screen
    lcd2.setCursor(4, 0);
    lcd2.print("          ");
    lcd2.setCursor(4, 1);
    lcd2.print("          ");

    lcd2.setCursor(4, 0);
    lcd2.print(CO2_level);
  
    lcd2.setCursor(4, 1);
    lcd2.print(CO2_target);
  
    lcd1.setCursor(6, 1);
    lcd1.print(numFans);

    refreshDisplay = false;
  }
}

void setUpHardware()
{
  // LCD 1 (left side)
  lcd1.begin(16,2);
  lcd1.clear();
  lcd1.home();
  
  lcd1.print("Control Station");
  lcd1.setCursor(0, 1);
  lcd1.print(F("FANS: "));
  
  // LCD 2 (right side)
  lcd2.begin(16,2);
  lcd2.clear();
  lcd2.home();
  
  lcd2.print(F("ACT: "));
  lcd2.setCursor(0,1);
  lcd2.print(F("SET: "));

  pinMode(A0, INPUT);                   // fan control (left)
  pinMode(A1, INPUT);                   // CO2 1 million (middle)
  pinMode(A2, INPUT);                   // CO2 10 thousand (right)
  pinMode(solenoidPin, OUTPUT);
}

/**
 * Writes information to the cloud (Should make this a time interupt)
 */
//void writeToCloud(void *pvParameters)
//{  
//  //runs when function is called for the first time.
//
//  //runs forever
//  for(;;)
//  {
//    //TODO: Write to cloud here... 
//  }
////  Serial.println("written to cloud");
//}

/**
 * Reads CO2 sensor
 */
void readCO2_sensor(void *pvParameters)
{
  //runs when function is called for the first time.
  unsigned long tempLevel = 0;
  unsigned long tempTarget = 0;
  //runs forever
  for(;;)
  {
    // Read sensor here...
    mySensor.read();
    tempLevel = mySensor.getCO2();

    // READ USER INPUT
    tempTarget = (map(analogRead(A2), 0, 1023, 0, 100) * 100) + 
                 (map(analogRead(A1), 0, 1023, 1, 100) * 10000);
    if (tempTarget > 1000000)
    {
      tempTarget = 1000000;
    }
    
//    if(xSemaphoreTake(xLevelMutex, 1000))
//    {
      if(CO2_level != tempLevel)
        refreshDisplay = true;
        
      CO2_level = tempLevel;
//      xSemaphoreGive(xLevelMutex);
//    }
//    if(xSemaphoreTake(xTargetMutex, 1000))
//    {
      if(CO2_target != tempTarget)
        refreshDisplay = true;
        
      CO2_target = tempTarget;
//      xSemaphoreGive(xTargetMutex);
//    }

    vTaskDelay(1000/*in ms*/ / portTICK_PERIOD_MS);
  }
}

/**
 * Controls how much CO2 is in the tank, and tries to 
 * match it based on the CO2 Sensor reading
 */
void manageCO2_levels(void *pvParameters)
{
  //runs forever.
  for(;;)
  {
    //TODO: Turn on solenoid here...
    if (CO2_target <= CO2_level)
    {
      digitalWrite(solenoidPin, LOW);
    }
    else
    {
      digitalWrite(solenoidPin, HIGH);
    }
    vTaskDelay(1000/*in ms*/ / portTICK_PERIOD_MS);
  }
}
