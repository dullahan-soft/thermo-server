/* 
*  Thermo Webserver
* 
*  Serves temp readings from 4 thermocouples as html.
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

/* this is how often we will check if we need to change the state of the pump */
#define READING_PUMP_INTERVAL 60000

/* the error value when checking if the pump should be activated */
#define E 5

/* pin that controls the pump switch */
#define PUMP_CTRL 31

/* common clock and query pins for the thermos */
int thermoCommonCLK = 2;
int thermoCommonD0  = 5;

/* each thermo gets its own data pin */
int thermo1CS = 7;
int thermo2CS = 9;
int thermo3CS = 11;
int thermo4CS = 13;

/* setup thermos */
Adafruit_MAX31855 thermo1(thermoCommonCLK, thermo1CS, thermoCommonD0);
Adafruit_MAX31855 thermo2(thermoCommonCLK, thermo2CS, thermoCommonD0);
Adafruit_MAX31855 thermo3(thermoCommonCLK, thermo3CS, thermoCommonD0);
Adafruit_MAX31855 thermo4(thermoCommonCLK, thermo4CS, thermoCommonD0);

double tempReadings[4];
int    thermoErrorCodes[4];

/* timers */
float deltaReading;
float deltaPumpReading;
float d0;

char requestBuffer[1024];

boolean pumpIt;

/* http server vars */
byte mac[]     = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[]      = { 192,168,1,20 };
byte gateway[] = { 192,168,1,1};	
byte subnet[]  = { 255, 255, 255, 0 };
EthernetServer server(80);

void send404(EthernetClient client) {
  client.println("HTTP/1.1 404 OK");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html><body>404</body></html>");
}

void sendOKHeader(EthernetClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
}

void handleRequest(EthernetClient client){
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
    if(command == "/thermos"){
      sendOKHeader(client);     
      for(int i=0;i<4;i++){
        client.println("Thermocouple: " + String(i+1) + "<br/>");
        client.println("Temperature: ");
        client.println(tempReadings[i]);
        client.println(" F<br>");
        client.println("Error Code: ");
        client.println(thermoErrorCodes[i]);
        client.println("<br/>");
      }   
    }
    else if(command == "/pump"){
      sendOKHeader(client);
      client.print("Pump is ");
      client.println(pumpIt ? "ON" : "OFF"); 
    }
    else if(command == "/stats"){
      sendOKHeader(client);
      client.print("Up for ");
      client.print((millis()-d0)/1000);
      client.println(" s");
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
  deltaPumpReading = millis();
  d0 = millis();
  
  /* start up the web server */ 
  Ethernet.begin(mac);
  server.begin();

  /* setup pin that will be controlling the pump */
  pinMode(PUMP_CTRL,OUTPUT);
  pumpIt = false;
  digitalWrite(PUMP_CTRL,LOW);
  
  /* wait for MAX chips to stabilize and ethernet shield to setup */
  delay(1000);
}

float celsiusToFarenheit(float t){
  t *= 9.0;
  t /= 5.0;
  t += 32;
  return t;
}

void loop(){
  if(millis() - deltaReading >= READING_INTERVAL){
    tempReadings[0] = thermo1.readFarenheit();
    
    if(isnan(tempReadings[0])){
      thermoErrorCodes[0] = thermo1.readError();  
    }
    tempReadings[1] = thermo2.readFarenheit();
    if(isnan(tempReadings[1])){
      thermoErrorCodes[1] = thermo2.readError();  
    }
    tempReadings[2] = thermo3.readFarenheit();
    if(isnan(tempReadings[2])){
      thermoErrorCodes[2] = thermo3.readError();  
    }
    tempReadings[3] = thermo4.readFarenheit();
    if(isnan(tempReadings[3])){
      thermoErrorCodes[3] = thermo4.readError();  
    }
    deltaReading = millis();
  }
  if(millis() - deltaPumpReading >= READING_PUMP_INTERVAL){
    pumpIt = false;
    /* turn on pump if any thermo is greater than room temp with error E */
    for(int i=0; i<4; i++){
      if(tempReadings[i] - celsiusToFarenheit(thermo1.readInternal()) >= E){
        pumpIt = true;
        break;
      }
    }
    if(pumpIt){
      Serial.println("high");
      digitalWrite(PUMP_CTRL,HIGH);  
    }
    else{
      Serial.println("low");
      digitalWrite(PUMP_CTRL,LOW);
    }
    deltaPumpReading = millis();
  }
  
  listenForClients();  
}

void listenForClients(){
  EthernetClient client = server.available();
  
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


