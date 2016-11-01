/* Control your coffeemaker over wifi with wemos d1 mini or other EPS8266 chips.
  
  Communication with coffeemaker is adapted from Oliver Krohns Coffeemaker-Payment-System (https://github.com/oliverk71/Coffeemaker-Payment-System)

  Pins: D2, D1 - RX, TX from coffeemaker (D2 = GPIO4 on ESP8266; D1 = GPIO5 on ESP8266)

  This software comes without any warranty "as it is".

*/

// Include Wifi stuff
#include <ESP8266WiFi.h> // wifi
#include <WiFiClient.h> // wifi
#include <ESP8266WebServer.h> // Webserver
#include <ESP8266mDNS.h> // for ota update
#include <ESP8266HTTPUpdateServer.h> // for ota update
#include <EEPROM.h> // Write and read eeprom
#include <WiFiUdp.h> // NTP server query (get time)
#include <Time.h> // Time library
#include <TimeAlarms.h> // Alarm library
#include <SoftwareSerial.h> // Include software serial to communicate with coffeemaker

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Declare variables //////////////////////////////////////

// vars for coffeemaker communications

SoftwareSerial mySerial(D2, D1); //RX TX // Change accordingly to your board
byte z0;
byte z1;
byte z2;
byte z3;
byte x0;
byte x1;
byte x2;
byte x3;
byte x4;
byte intra = 1;
byte inter = 7;
String addresses[6] = {"00", "01", "02", "03", "07", "34"}; // eeprom addresses

// vars for api
String cmStatus;

// define Wifi login credentials
// put in your own config
const char* ssid  = ""; //SSID
const char* password = ""; //pass
IPAddress gateway(192, 168, 0, 1); //gateway
IPAddress subnet(255, 255, 255, 0); // subnet
IPAddress ip(192, 168, 0, 9); //feste ip

// webserver and updater
ESP8266WebServer server(80);// starte webserver auf port 80
ESP8266HTTPUpdateServer httpUpdater;

// timeserver variables
unsigned int localPort = 8888;      // local port to listen for UDP packets
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
const int timeZone = 1; //central europe time

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;



//////////////////// Basic homepage settings ////////////////////////////////
String http_answer = ""; // set main http_answer 
String uptime() { // Betriebszeit als Stunde:Minute:Sekunde
  char stamp[10];
  int rnHrs = millis() / 3600000;
  int rnMins = millis() / 60000 - rnHrs * 60;
  int rnSecs = millis() / 1000 - rnHrs * 3600 - rnMins * 60;
  sprintf (stamp, "%03d:%02d:%02d", rnHrs, rnMins, rnSecs);
  return stamp;
}
String http_scaffold = "<html>\
  <head>\
    <title>Jura Impressa E50</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
  <h1>Hello from your Coffeemaker</h1>\
  ";
////////////////// End basic homepage settings ///////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// End declare variables //////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Functions //////////////////////////////////////

////////////////// NTP time server 
// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address){
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


//query ntp server
time_t getNtpTime() {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  
  int cb = 0;
  while (cb == 0) {
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);
    cb = udp.parsePacket();
    delay(100);
  }
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
          unsigned long secsSince1900;

    // convert four bytes starting at location 40 to a long integer
    secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
    secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
    secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
    secsSince1900 |= (unsigned long)packetBuffer[43];
    return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;    // or two words, long. First, esxtract the two words:
}

//////////////// Communication with coffeemaker ///////////
void toCoffeemaker(String outputString) {
  outputString.toUpperCase();
  outputString += "\r\n";
  for (byte a = 0; a < outputString.length(); a++) {
    byte d0 = 255;
    byte d1 = 255;
    byte d2 = 255;
    byte d3 = 255;
    bitWrite(d0, 2, bitRead(outputString.charAt(a), 0));
    bitWrite(d0, 5, bitRead(outputString.charAt(a), 1));
    bitWrite(d1, 2, bitRead(outputString.charAt(a), 2));
    bitWrite(d1, 5, bitRead(outputString.charAt(a), 3));
    bitWrite(d2, 2, bitRead(outputString.charAt(a), 4));
    bitWrite(d2, 5, bitRead(outputString.charAt(a), 5));
    bitWrite(d3, 2, bitRead(outputString.charAt(a), 6));
    bitWrite(d3, 5, bitRead(outputString.charAt(a), 7));
    mySerial.write(d0);
    delay(1);
    mySerial.write(d1);
    delay(1);
    mySerial.write(d2);
    delay(1);
    mySerial.write(d3);
    delay(7);
  }
}

String fromCoffeemaker() {
  String inputString = "";
  char d4 = 255;
  while (mySerial.available()) {   // if data is available to read
    byte d0 = mySerial.read();
    delay (1);
    byte d1 = mySerial.read();
    delay (1);
    byte d2 = mySerial.read();
    delay (1);
    byte d3 = mySerial.read();
    delay (7);

    bitWrite(d4, 0, bitRead(d0, 2));
    bitWrite(d4, 1, bitRead(d0, 5));
    bitWrite(d4, 2, bitRead(d1, 2));
    bitWrite(d4, 3, bitRead(d1, 5));
    bitWrite(d4, 4, bitRead(d2, 2));
    bitWrite(d4, 5, bitRead(d2, 5));
    bitWrite(d4, 6, bitRead(d3, 2));
    bitWrite(d4, 7, bitRead(d3, 5));

    if (d4 != 10) {
      inputString += d4;
    }
    else {
      inputString += d4;
      return (inputString);
    }
  }
}

//////////////////////// Coffeemaker functions ////////////////////////////

// get status from coffeemaker
String getStatus() {
  toCoffeemaker("RR:20");
  delay(50);  // wait for answer?
  String inputString;
  while (inputString.length() < 10) {
    inputString = fromCoffeemaker();
  }
  inputString = inputString.substring(3, 11);
  String poi = inputString.substring(0, 2) + inputString.substring(6, 8);
  String answer;
  if (poi == "0000") {
    answer = "off";
  }
  else if (poi == "0100") {
    answer = "busy";
  }
  else if (poi == "0101") {
    answer = "ready";
  }
  else if (poi == "1111") {
    answer = "clean";
  }
  else if (poi == "4000") {
    answer = "flushing";
  }
  else if (poi == "8180") {
    answer = "need_water";
  }
  else if (poi == "9190") {
    answer = "need_water";
  }
  else if (poi == "4040") {
    answer = "need_flushing";
  }
  else if (poi == "0505") {
    answer = "powder";
  }
  else if (poi == "2120") {
    answer = "full";
  }
  else if (poi == "3130") {
    answer = "full";
  }
  else {
    answer = "unknown";
    answer += poi;
  }
  return answer;
}

// Read single coffeemaker EEPROM addresses
String readEEPROMAddress(String address){
    String outputString = "RE:"+address;
    String inputString;
    while (inputString.length() < 9) {
      toCoffeemaker(outputString);
      delay(50);  // wait for answer?
      inputString = fromCoffeemaker();
    }
    inputString.remove(0, 3);
    inputString.remove(4);
    
    return inputString;
}

// Convert cm answer to Int
long stringToInt(String cString, int len){
  len++;
  char intin[len];
  cString.toCharArray(intin, len);
  long byteval = strtol(intin, NULL, 16);
  return byteval;
  
}



void command() {
  String outputString = server.arg(0);

  toCoffeemaker(outputString);
  delay(50);  // wait for answer?
  String inputString = fromCoffeemaker();

  delay(100);

  server.send(200, "text/plain", inputString);//a html_message);
}

//////////////////// CHIP EEPROM ///////////////////////

// Read chips EEPROM
long getEEPROMval(int i) {
    int a = ((i+1)*2)-2; //eeprom address first byte
    String hex = String(EEPROM.read(a), HEX);
    if (hex.length() < 2) {
      hex = "0"+hex;
    }
    a++;
    String hex2 = String(EEPROM.read(a), HEX);
    if (hex2.length() < 2){
      hex2 = "0"+hex2;
    }
    return stringToInt(hex+hex2, 4);
}
// update Chips EEPROM
void updateEEPROM(int addressOffset){
   // bit 0 to 9 are for day
  for (int i; i<5; i++){

    // get current values
    String inputString = readEEPROMAddress(addresses[i]);
    
    String Hex = String(inputString.charAt(0))+String(inputString.charAt(1));
    long byteval = stringToInt(Hex, 2);
    int a = ((i+addressOffset+1)*2)-2; //eeprom address first byte
    EEPROM.write(a, byteval);
    a++;
    Hex = String(inputString.charAt(2))+String(inputString.charAt(3));
    byteval = stringToInt(Hex, 2);    
    EEPROM.write(a, byteval);
    
    EEPROM.commit();
    
  }
}

// update function each day
void dailyUpdate(){
  updateEEPROM(0);
}

// update function each week
void weeklyUpdate(){
  updateEEPROM(10);
}
//////////////////// End CHIP EEPROM ///////////////////////



void getStats() {
  String totals[5];
  String trester;
  String todays[5];
  String weeks[5];
  for (int i = 0; i < 6; i++) {
    String value = readEEPROMAddress(addresses[i]);
    long totalVal = stringToInt(value, 4);
    if (i == 5) {
      trester = totalVal;
      continue;
    }
    
    totals[i] = totalVal;
    long todayVal = getEEPROMval(i);
    long weekVal = getEEPROMval(i+10);

    weeks[i] = totalVal - weekVal;
    todays[i] = totalVal - todayVal;
    delay(50);
  }
  String json = "{";
  String total = "\"total\":{";
  total = total + "\"onecup\":"+ totals[0] +",";
  total = total + "\"strong\":"+ totals[1] +",";
  total = total + "\"xstrong\":"+ totals[2] +",";
  total = total + "\"double\":"+ totals[3] +",";
  total = total + "\"flushs\":"+ totals[4];
  total += "},";
  String today = "\"today\":{";
  today = today + "\"onecup\":"+ todays[0] +",";
  today = today + "\"strong\":"+ todays[1] +",";
  today = today + "\"xstrong\":"+ todays[2] +",";
  today = today + "\"double\":"+ todays[3] +",";
  today = today + "\"flushs\":"+ todays[4];
  today += "},";
  String week = "\"week\":{";
  week = week + "\"onecup\":"+ weeks[0] +",";
  week = week + "\"strong\":"+ weeks[1] +",";
  week = week + "\"xstrong\":"+ weeks[2] +",";
  week = week + "\"double\":"+ weeks[3] +",";
  week = week + "\"flushs\":"+ weeks[4];
  week += "},";
  json = json + total + today + week + "\"trester\":" + trester+"}";
  
  server.send(200, "application/json", json);
}


String apiOn() {
  toCoffeemaker("AN:01"); 
  delay(50); //wait for answer
  return fromCoffeemaker();
}
String apiFlush() {
  toCoffeemaker("FA:02");
  delay(50);  // wait for answer?
  return fromCoffeemaker();
}
String apiCoffee() {
  toCoffeemaker("FA:06");
  delay(50); 
  return fromCoffeemaker();
}

String apiOff() {
  toCoffeemaker("AN:02");
  delay(50);  // wait for answer?
  return fromCoffeemaker();
}
String apiTwoCups() {
  toCoffeemaker("FA:07");
  delay(50); // wait for answer?
  return fromCoffeemaker();
}

// Api for the app via HTTP get
void api() {
  String answer;

  String command = server.arg(0);
  if (command == "on") {
    answer = apiOn(); 
  } else if (command == "off") {
    answer = apiOff();
  } else if (command == "onecup") {
    answer = apiCoffee();
  } else if (command == "twocups") {
    answer = apiTwoCups();
  } else if (command == "flush") {
      answer = apiFlush();
  } else if (command == "status") {
    answer = getStatus();
  }
    
  server.sendHeader("Access-Control-Request-Method", "POST,GET,PUT,DELETE,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "accept,Content-Type");
  server.send(200, "text/plain", answer);
}

//////////////////////////////////// BASIC HTML COMMANDS ///////////////////////////////

// Main page
void rootOverview() { // bei Aufruf des Root-Verzeichnisses
  http_answer = http_scaffold;
  http_answer = http_answer + "<h2>Status: " + getStatus() + "</h2>";
  http_answer = http_answer + "<p>Uptime: " + uptime() + " (Std:Min:Sek)</p><br>";
  http_answer = http_answer + "Connected to: " + ssid + "<br>";
  int rssi = WiFi.RSSI();
  http_answer = http_answer + "Signal strnength: " + String(rssi) + " dBm<br></p>";
  http_answer = http_answer + "Time " + String(hour()) + ":" + String(minute()) + "<br>";
  http_answer = http_answer + "<p>Commands:";
  http_answer = http_answer + "<ul><li><a href='./on'>Turn me on!</a></li>";
  http_answer = http_answer + "<li><a href='./off'>Turn me off!</a></li>";
  http_answer = http_answer + "<li><a href='./reboot'>Reboot</a></li>";
  http_answer = http_answer + "<li><a href='./flush'>Flush!</a></li>";
  http_answer = http_answer + "<li><a href='./coffee'>Make a Coffee!</a></li>";
  http_answer = http_answer + "<li><a href='./update'>Update me!</a></li>";
  http_answer = http_answer + "<li><a href='./read'>Read EEPROM!</a></li>";
  http_answer = http_answer + "</ul></body></html>";
  server.send(300, "text/html", http_answer);
  delay(150);
}


// power on coffeemaker
void turnOn() {
  String html_message = http_scaffold;
  if (getStatus() != "off") {
    html_message += "<p>Coffeemaker already on</p><a href='../'>BACK</a></body></html>";
    server.send(200, "text/html", html_message);

    return;
  }
  toCoffeemaker("AN:01");
  delay(100);
  String cm_status = getStatus();
  if (cm_status == "busy") {
    html_message = http_scaffold + "<p>Coffeemaker is heating up!</p> <a href='../'>BACK</a></body></html>";
    server.send(200, "text/html", html_message);
    delay(100);
    while (cm_status == "busy") {
      cm_status = getStatus();
      delay(100);
    }
  }

  if (cm_status == "ready")  {
    html_message = http_scaffold + "<p>Coffeemaker is ready for Coffee</p><a href='../'>BACK</a></body></html>";
    server.send(200, "text/html", html_message);
    return;
  }
  else if (cm_status == "need_flushing") {
    html_message = http_scaffold + "<p>Coffeemaker is ready, but needs flushing!</p> <a href='../'>BACK</a></body></html>";
    server.send(200, "text/html", html_message);
    return;
  }
  else if (cm_status == "need_water") {
    html_message = http_scaffold + "<p>Coffeemaker is ready, but needs water!</p> <a href='../'>BACK</a></body></html>";
    server.send(200, "text/html", html_message);
    return;
  }
  server.send(200, "text/html", html_message);

}

// power off coffeemaker
void turnOff() {
  toCoffeemaker("AN:02");
  String html_message = http_scaffold;
  html_message = html_message + "Coffeemaker is shutting down!!<p><a href='../'>BACK</a></body></html>";

  server.send(200, "text/html", html_message);
  delay(100);
}
// Flushing
void flushing() {
  toCoffeemaker("FA:02");
  String html_message = http_scaffold;
  //html_message = html_message + "Coffeemaker is flushing! No Mug right now!!<p><a href='../'>BACK</a></body></html>";

  server.send(200, "text/html", html_message);
  delay(100);
}
// Make one cup of delicious coffee
void makeCoffee() {

  unsigned long startedAt = millis();
  while((millis()-startedAt) < 5000) {
    toCoffeemaker("FA:06");
  }
  String html_message = http_scaffold;
  html_message = html_message + "I am making a delicious cup of coffee! Hope you put a mug under me!!<p><a href='../'>BACK</a></body></html>";

  server.send(200, "text/html", html_message);
  delay(100);

  Serial.println(uptime() + " one coffee via http");
}
// Reboot the unit
void reboot() {
  ESP.restart();
}

// Output EEPROM of coffeemaker
void readEEPROM() {
  String message;
  // query all addresses from 00 to 0x7F
  for (int i = 0; i <= 0x7F; i++ ) {
    String outputString = "RE:";
    if (i <= 0xF) {
      outputString += "0";
    }

    outputString += String(i, HEX);
    String inputString;
    while (inputString.length() < 9) {
      //message = message + "redooooooooo";
      toCoffeemaker(outputString);
      delay(50);  // wait for answer?
      inputString = fromCoffeemaker();
    }
    inputString.remove(0, 3);
    inputString.remove(4);

    char intin[5];

    inputString.toCharArray(intin, 5);
    long test = strtol(intin, NULL, 16);
    message = message + test + "\n";
    delay(100);
  }
  server.send(200, "text/plain", message);
}

//////////////////////////////////// END BASIC HTML COMMANDS ///////////////////////////////


void setup() {
  // serial for cm communication
  mySerial.begin(9600);

  // WLAN-Verbindung herstellen
  WiFi.config(ip, gateway, subnet); // auskommentieren, falls eine dynamische IP bezogen werden soll
  WiFi.begin(ssid, password);

  // Verbindungsaufbau abwarten
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  ////// Time Server
  // Start UDP
  udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(1000000); // now every 1000 seconds
  Alarm.alarmRepeat(04,30,0,dailyUpdate);  // 4:30 every day 
  Alarm.alarmRepeat(dowMonday,4,00,30,weeklyUpdate);  // 4:00 every Monday 

  // Reboot every Night to prevent crash
  Alarm.alarmRepeat(03,0,0,reboot);
  
  // Update chip via web
  MDNS.begin(ssid);
  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);

  // Start EEPROM
  EEPROM.begin(512);
 

  // HTTP-Anfragen bearbeiten
  server.on("/", rootOverview);
  server.on("/on", turnOn);
  server.on("/off", turnOff);
  server.on("/flush", flushing);
  server.on("/coffee", makeCoffee);
  server.on("/read", readEEPROM);
  server.on("/status", getStatus);
  server.on("/command", command);
  server.on("/api", api);
  server.on("/reboot", reboot);
  
  server.on("/stats", getStats);
  server.on("/daily", dailyUpdate);
  server.on("/weekly", weeklyUpdate);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient(); // auf HTTP-Anfragen warten
  Alarm.delay(0);
  
}
