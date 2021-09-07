/*
  Web client

 This sketch connects to the Thingspeak website using SSL through a MKR GSM 1400 board. Specifically,
 this example performs a write operation to the thingspeak channel specified in the
 arduino_secrets.h file.
 
 It only performs a write GPRS connection through to Thingspeak when the button is depressed.
 
 You can Change the fields it writes to on Thingspeak as you wish This sketch also prints output to the Serial monitor
 to let you know where it is up to at any given time, lagrely for debugging.

 Other Stuff:
 optional stuff added this sketch currently is the ability to turn an LED on, based on the results of reading a Thingspeak field value.
 A light sensor so that we can read the light value and add that to a Thingspeak field in near realtime and track the lgith throughout the day.

 If the code and the comments disagree, then both are probably wrong. -- Norm Schryer

  Changes:
  04092018 - Added inital code and used the ArduinoJSON libraries
  05092018 - updated serial speed from Max of 115200 to something more stable, 57600
  07092018 - heavily updated JSON formatting code
  17092018 - Added watchdog function in case of HTTP post failure
  18092018 - Tested working with writing Lat and Long through to ThingSpeak 
           - Added functions to print a better status output to Thingspeak status field
           - Made the secret channel id go through properly, pulls in directly from the arduino_secrets.h file where it is defined...
           

 Circuit:
 * MKR GSM 1400 board
 * Antenna
 * SIM card with a data plan
 * Push Button
 * LED
 * Resistor
 * TEMT 600 Light Sensor
 * Breadboard

 created 05 Feb 2018
 by SMGS
*/

#include <MKRGSM.h>
#include "arduino_secrets_voda.h" 
#include <ArduinoJson.h>
#include <Adafruit_SleepyDog.h>

// Please enter your sensitive data in the Secret tab or arduino_secrets.h
// PIN Number
const char PINNUMBER[]     = SECRET_PINNUMBER;
// APN data
const char GPRS_APN[]      = SECRET_GPRS_APN;
const char GPRS_LOGIN[]    = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

// Thingspeak Secret Settings
//char writeAPIKey[]      = secretWriteAPIKey;
//char readAPIKey[]       = secretReadAPIKey;
//char channelID[]        = secretChannelID;

// initialize the library instance
GSMSSLClient client;
GSM gsmAccess;
GPRS gprs;
GSMLocation location;
GSMScanner scanner;

// Domain name and port that we will connect too (for example: api.thingspeak.com)
char server[] = "api.thingspeak.com";
int port = 443; // port 443 is the default for HTTPS

int lightLevel; //setup the Light Level field for us to store the current result in from the TEMT6000 Sensor 
float t_latitude;
float t_longitude;
long t_altitude;
long t_accuracy;


String stat;        // Used to capture the Previous runs response code
String cResp = "First run";
long buttonIterations = 0;    //Track the number of runs since last bootup.


// Memory pool for JSON object tree.
//
// Inside the brackets, 200 is the size of the pool in bytes.
// Don't forget to change this value to match your JSON document.
// Use arduinojson.org/assistant to compute the capacity.
StaticJsonDocument<200> jsonDocument;

void setup() {
  
  

  // initialize serial communications and wait for port to open:
  Serial.begin(57600); //Using a conservative Baud rate (max is 115200)
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  

  Serial.println("Starting Arduino web client.");

  // connection state

  bool connected = false;

  // After starting the modem with GSM.begin()

  // attach the shield to the GPRS network with the APN, login and password

  while (!connected) {

    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&

        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {

      connected = true;

    } else {

      Serial.println("Not connected");

      delay(1000);

    }

  }

  Serial.println("connecting...");




 
  Watchdog.enable(10000); //Start a 10 second watchdog timer
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if (buttonIterations > 0)
  {
    Serial.println(F("End"));
    for (;;);
    }
  
  
    // Reset the watchdog with every loop to make sure the sketch keeps running.
    // If you comment out this call watch what happens after about 4 iterations!
    Watchdog.reset();
    
    stat = ("Arduino Test: ");    
    buttonIterations++; //Add one button press for the next Iteration.
    String sigStrength = scanner.getSignalStrength();

    
    Serial.println(F("Connecting to HTTP Server..."));

    // Connect to HTTP server
    if (!client.connect(server, port)) {
      Serial.println(F("Connection failed"));
      return;
    }
    Serial.println(F("Connected!"));
    
    // Create the JSON Array and the root of the object tree.
    //
    // It's a reference to the JsonObject, the actual bytes are inside the
    // JsonBuffer with all the other nodes of the object tree.
    // Memory is freed when jsonBuffer goes out of scope.
    const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(10);
    DynamicJsonDocument  jsonDocument(bufferSize);
    
    jsonDocument["api_key"] = secretWriteAPIKey;    
    jsonDocument["field1"] = "32";        
    jsonDocument["latitude"] = ""; // Location Latitude
    jsonDocument["longitude"] = ""; // Location Longitude    
    jsonDocument["status"] = stat; // Total Button Presses/Runs + The previous response code
    
    //root.printTo(Serial);
    //Serial.println();
    //root.prettyPrintTo(Serial);

    Serial.println("connected to: ");
    Serial.println(server);
    Serial.println(port);
    Serial.println("Beginnning JSON Post Sequence");
    Serial.println();
    client.print("POST ");    
    client.println("/update.json HTTP/1.1");
    Serial.print("POST ");    
    Serial.println("/update.json HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    Serial.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino MKR GSM 1400)");
    Serial.println("User-Agent: mw.doc.bulk-update (Arduino MKR GSM 1400)");
    client.println("Connection: close");
    Serial.println("Connection: close");
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    client.print("Content-Length: ");
    Serial.print("Content-Length: ");
    client.println(measureJsonPretty(jsonDocument));
    Serial.println(measureJsonPretty(jsonDocument));
    
    // Terminate headers
    client.println();
    Serial.println();

    // Send body
    serializeJson(jsonDocument,Serial);
    serializeJson(jsonDocument,client);
    
        
    delay(350); //Wait to receive the response
    client.parseFloat();
    cResp = String(client.parseInt());
    Serial.println();
    Serial.println("Response code: "+cResp); // Print the response code. 202 indicates that the server has accepted the response
    stat = ("Arduino MKR GSM 1400 - run number: ");
    stat += (buttonIterations);
    stat += (" Response code: ");
    stat += (cResp); //Store the response to send through as the status on the next run
    
    // Disconnect
    client.stop();


  
  
  // Reset the watchdog with every loop to make sure the sketch keeps running.
  Watchdog.reset();
    
} //End of Loop