// Developer : Anvith M
// Date : 03-04-2025
// Function : PTP Slave
#include <WiFi.h>
#include <WiFiUdp.h>


const char* apSSID = "ESP32-Master"; // AP-Name
const char* apPassword = "12345678"; // AP Password
WiFiUDP udp;
const char* masterIP = "192.168.4.1";
const int port = 1234;

hw_timer_t *timer = NULL;
volatile uint32_t slave_count = 10;

void IRAM_ATTR ontimer(){
  slace_count++;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //For counter
  timer = timerBegin(1000000); // Timer 1MHz
  timerAttachInterupt(timer, &ontimer);
  timerAlarm(timer, 1000000, true, 0);

  // Connect to ESP32 Master
  WiFi.begin(apSSID, apPassword);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to ESP32 Master.");
  
  //Start UDP communication
  udp.begin(port);
}

void loop() {
  // put your main code here, to run repeatedly:

}
