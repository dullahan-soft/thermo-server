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
int thermoCommonCLK = 5;
int thermoCommonCS  = 4;

/* each thermo gets its own data pin */
int thermo1D0 = 3;
int thermo2D0 = 6;
int thermo3D0 = 7;
int thermo4D0 = 8;

/* setup thermos */
Adafruit_MAX31855 thermo1(thermoCommonCLK, thermoCommonCS, thermo1D0);
Adafruit_MAX31855 thermo2(thermoCommonCLK, thermoCommonCS, thermo2D0);
Adafruit_MAX31855 thermo3(thermoCommonCLK, thermoCommonCS, thermo3D0);
Adafruit_MAX31855 thermo4(thermoCommonCLK, thermoCommonCS, thermo4D0);

double celsiusReadings[4];
int    thermoErrorCodes[4];

float deltaReading;

char requestBuffer[1024];

/* http server vars */
byte mac[]     = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[]      = { 192,168,1,20 };
byte gateway[] = { 192,168,1,1};	
byte subnet[]  = { 255, 255, 255, 0 };
Server server(80);

void send404(Client client) {
  client.println("HTTP/1.1 404 OK");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html><body>404</body></html>");
}

void sendOKHeader(Client client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
}

void handleRequest(Client client){
  String req = String(requestBuffer);
  String command;
  
  if(req.startsWith("GET")){
    String command;
    for(int i=4;i<req.length();i++){
      if(req[i] == ' '){
        command = req.substring(4,i);
        break;
      }
    }
    Serial.println(command);
    if(command == "/thermos"){
      sendOKHeader(client);     
      for(int i=0;i<4;i++){
        client.println("Thermocouple: " + String(i+1) + "<br/>");
        client.println("Temperature: ");
        client.println(celsiusReadings[i]);
        client.println(" C<br/>");
        client.println("Error Code: ");
        client.println(thermoErrorCodes[i]);
        client.println("<br/>");
      }   
    }
    else{
      send404(client);
    }
  }
  else if(req.startsWith("POST")){
    
  }
  else{
    send404(client);
  }
}

void setup(){
  Serial.begin(9600);
  /* init reading time */
  deltaReading = millis();
 
  /* start up the web server */ 
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
    int index = 0;
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        
        requestBuffer[index++] = c;

        if (c == '\n' && currentLineIsBlank) {
          handleRequest(client);
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


