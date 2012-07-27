/* 
*  Thermo Webserver
* 
*  Serves temp readings from 4 thermocouples as html or json.
*  Temp readings are updated at least once a second.
*
*  created July 27, 2012
*  by Chris Cacciatore (chris.cacciatore bat-'b' dullahansoft.com)
*/

#include <SPI.h>
#include <Ethernet.h>
#include "Adafruit_MAX31855.h"

/* this is how often we will query the thermos for readings */
#define READING_INTERVAL 1000

/* common clock and query pins for the thermos */
int thermoCommonCLK = 3;
int thermoCommonCS  = 4;

/* each thermo gets its own data pin */
int thermo1D0 = 5;
int thermo2D0 = 6;
int thermo3D0 = 7;
int thermo4D0 = 8;

Adafruit_MAX31855 thermo1(thermoCommonCLK, thermoCommonCS, thermo1D0);
Adafruit_MAX31855 thermo2(thermoCommonCLK, thermoCommonCS, thermo2D0);
Adafruit_MAX31855 thermo3(thermoCommonCLK, thermoCommonCS, thermo3D0);
Adafruit_MAX31855 thermo4(thermoCommonCLK, thermoCommonCS, thermo4D0);

double celsiusReadings[4];
String thermoErrorCodes[4];

float deltaReading;

/* http server vars */
byte mac[]     = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[]      = { 192,168,1,20 };
byte gateway[] = { 192,168,1,1};	
byte subnet[]  = { 255, 255, 255, 0 };

Server server(80);

void setup(){
  
  /* init reading time */
  deltaReading = millis();
  
  Ethernet.begin(mac, ip);
  server.begin();
  
  /* wait for MAX chips to stabilize and ethernet shield to setup */
  delay(1000);
}

void loop(){
  if(millis() - deltaReading >= READING_INTERVAL){
    celsiusReadings[0] = thermo1.readCelsius();
    if(isnan(celsiusReadings[0])){
      thermoErrorCodes[0] = thermo1.readError();  
    }
    celsiusReadings[1] = thermo2.readCelsius();
    if(isnan(celsiusReadings[1])){
      thermoErrorCodes[1] = thermo2.readError();  
    }
    celsiusReadings[2] = thermo3.readCelsius();
    if(isnan(celsiusReadings[2])){
      thermoErrorCodes[2] = thermo3.readError();  
    }
    celsiusReadings[3] = thermo4.readCelsius();
    if(isnan(celsiusReadings[3])){
      thermoErrorCodes[3] = thermo4.readError();  
    }
    deltaReading = millis();
  }
  
   listenForClients();  
}

void listenForClients(){
  Client client = server.available();
  
  if(client){
    //an http request ends with a blank line
    boolean currentLineIsBlank = true;
    
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          for(int i=0;i<4;i++){
            client.print("Thermocouple " + String(i) + ":<br/>");
            client.print("Temperature: ");
            client.print(celsiusReadings[i]);
            client.print(" degrees C <br/>");
            client.print("Error: ");
            client.print(thermoErrorCodes[i]);
            client.print("<br/>");
          }
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  } 
}
