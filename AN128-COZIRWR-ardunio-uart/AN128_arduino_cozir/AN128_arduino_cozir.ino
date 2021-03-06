/*
AN128_ardunio_cozir CO2 Demonstration code 11/29/2017 Runs on Ardunio UNO, MEGA or MEGA2560
  Written by: Marv Kausch 11/22/2016 or Co2meter.com
  Revised 11/29/17 by John Houck
   
  This sketch connects a COZIR sensor and reports readings back to the host computer over USB.
  The value is stored in a global variable 'co2' and can be used for any number of applications.
   
  pin connections:
   
  Arduino________COZIR Sensor
   GND ------------------ 1 (gnd)
   3.3v------------------- 3 (Vcc)  
    13 -------------------- 5 (Rx)
    12 -------------------- 7 (Tx)
    
*/
#include <SoftwareSerial.h>

SoftwareSerial mySerial(12, 13); // RX, TX pins on Ardunio

int co2 = 0;
double multiplier = 100;// 1 for 2% =20000 PPM, 10 for 20% = 200,000 PPM
uint8_t buffer[25];
uint8_t ind = 0;
uint8_t index = 0;
#define bufferMax 7

int fill_buffer();  // function prototypes here
int format_output();

void setup() {
  Serial.begin(9600);
  Serial.print("\n\n");
  Serial.println("             AN128 Ardunio to Cozir CO2 Sensor - Demonstration code 11/29/2017\n\n"); 
  mySerial.begin(9600); // Start serial communications with sensor
  mySerial.println("K 0");  // Set Command mode
  mySerial.println("Z");// send Mode for H, T, and Z outputs
  // "H xxxxx T xxxxx Z xxxxx" (humidity, temperature, and filtered CO2 level)
  mySerial.println("K 2");  // set polling mode
  delay(1000);
//  mySerial.println("X 20");
}

void loop() {
  for(int i = 0; i < bufferMax; i++)
  {
    buffer[i] = ' ';
  }
  mySerial.println("Z");// send Mode Z outputs
  // "H xxxxx T xxxxx Z xxxxx" (humidity, temperature, and filtered CO2 level)
  fill_buffer();  // function call that reads CO2 sensor and fills buffer
 
  Serial.print("Buffer contains: ");
  for(int j=0; j<ind; j++)
  {
    Serial.print((char)buffer[j]);
  }
  Serial.print("\n");

  index = 0;
  Serial.print("\tFiltered PPM");
  format_output_co2();

  Serial.println("\n\n");
  delay(5000);
}

int fill_buffer(void)
{        
  // Fill buffer with sensor ascii data
  ind = 0;
  while(buffer[ind-1] != 0x0A)
  {  // Read sensor and fill buffer up to 0XA = CR
    if(mySerial.available())
    {
      buffer[ind] = mySerial.read();
      ind++;   
    } 
  }        
  // buffer() now filled with sensor ascii data
  //   ind contains the number of characters loaded into buffer up to 0xA =  CR
  ind = ind - 2; // decrement buffer to exactly match last numerical character
}

int format_output_co2(void){ // read buffer, extract 6 ASCII chars, convert to PPM and print
  co2 = buffer[bufferMax-index++]-0x30;
  co2 += (buffer[bufferMax-index++]-0x30)*10;
  co2 += (buffer[bufferMax-index++]-0x30)*100;
  co2 += (buffer[bufferMax-index++]-0x30)*1000;
  co2 += (buffer[bufferMax-index++]-0x30)*10000;
  Serial.print("\n CO2 = ");
  Serial.print(co2*multiplier, 0);
  Serial.print(" PPM");
  Serial.print("\n");
 }
