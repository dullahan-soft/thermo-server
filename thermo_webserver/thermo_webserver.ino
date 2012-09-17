/* 
*  Thermo Webserver
* 
*  Serves temperature readings from 4 thermocouples and one analog temperature sensor as html.
*  Temp readings are updated at least once a second.
*
*  The following routes are supported:
*    /stats     - prints the uptime
*    /thermos   - prints all temperature readings
*    /pump      - prints the current state of the pump ON/OFF
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

/* chip select pins used for thermos */
#define CS1 20
#define CS2 21
#define CS3 22
#define CS4 24

/* common clock and data pins for thermos */
#define DO  3
#define CLK 5

/* temperature threshold for the pump in farenheit */
#define PUMP_THRESHOLD 180.0

/* pin that controls the pump switch */
#define PUMP_CTRL 30

/* aref for the analog sensor */
#define AREF 3.3

/* setup thermos */
Adafruit_MAX31855 thermo1(CLK, CS1, DO);
Adafruit_MAX31855 thermo2(CLK, CS2, DO);
Adafruit_MAX31855 thermo3(CLK, CS3, DO);
Adafruit_MAX31855 thermo4(CLK, CS4, DO);

double tempReadings[4];
double roomTemp;
int    thermoErrorCodes[4];

/* timers */
float deltaReading;
float deltaPumpReading;
float d0;

char requestBuffer[1024];

boolean pumpIt;
boolean manualPumping;

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
  client.println("Access-Control-Allow-Origin: *");
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
      
      client.print("Room Temperature: ");
      client.print(roomTemp);
      client.println(" F<br>");
     
    }
    else if(command == "/thermos.json"){
      sendOKHeader(client);
      
      client.println("[");
      for(int i=0;i<4;i++){
        client.println("{\"type\": \"thermocouple\",");
        client.println("\"temp\": ");
        client.println(tempReadings[i]);
        client.println(",");
        client.println("\"error\": ");
        client.println(thermoErrorCodes[i]);
        client.println("},");
      }   
      client.println("{\"type\": \"sensor\",");
      client.println("\"temp\": ");
      client.print(roomTemp);
      client.println(",");
      client.println("\"error\": ");
      client.println(0);
      client.println("}]");
    }
    else if(command == "/pump"){
      sendOKHeader(client);
      client.print("Pump is ");
      client.println(pumpIt ? "ON" : "OFF"); 
    }
    else if(command == "/pump.json"){
      sendOKHeader(client);
      client.print("{\"state\": ");
      client.print(pumpIt ? "\"ON\"" : "\"OFF\"");
      client.print("}");
    }
    else if(command == "/stats"){
      sendOKHeader(client);
      client.print("Up for ");
      client.print((millis()-d0)/1000);
      client.println(" s");
    }
   else if(command == "/stats.json"){
      sendOKHeader(client);
      client.print("{\"time\":");
      client.print((millis()-d0)/1000);
      client.println("}");
    }
    else{
      send404(client);
    }
  }
  else if(req.startsWith("POST")){
    String command;
    for(int i=5;i<req.length();i++){
      if(req[i] == ' '){
        command = req.substring(5,i);
        break;
      }
    }
    
    if(command == "/pump/on"){
      sendOKHeader(client);
      manualPumping = true;
      pumpIt = true;
      digitalWrite(PUMP_CTRL,HIGH);
    }
    else if(command == "/pump/off"){
      sendOKHeader(client);
      pumpIt = false;
      manualPumping = false;
      digitalWrite(PUMP_CTRL,LOW); 
    }
    else{
      send404(client); 
    }
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
  manualPumping = false;
  digitalWrite(PUMP_CTRL,LOW);
  
  /* set reference for the analog sensor */
  analogReference(AREF);
  
  /* wait for MAX chips to stabilize and ethernet shield to setup */
  delay(1000);
}

float celsiusToFarenheit(float t){
  t *= 9.0;
  t /= 5.0;
  t += 32;
  return t;
}

float readRoomTemp(){
  float voltage = (analogRead(0) * AREF)/1024.0;
  return celsiusToFarenheit((voltage - 0.5) * 100);
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
    roomTemp = readRoomTemp();
    
    deltaReading = millis();
  }
  if(millis() - deltaPumpReading >= READING_PUMP_INTERVAL){
    if(!manualPumping){
      pumpIt = false;
      /* turn on pump if any thermo is greater than PUMP_THRESHOLD */
      for(int i=0; i<4; i++){
        if(tempReadings[i] >= PUMP_THRESHOLD){
          pumpIt = true;
          break;
        }
      }
      if(pumpIt){
        digitalWrite(PUMP_CTRL,HIGH);  
      }
      else{
        digitalWrite(PUMP_CTRL,LOW);
      }
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

