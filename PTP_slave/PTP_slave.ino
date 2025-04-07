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

uint32_t t1 = 0, t2 = 0, t3 = 0, t4 = 0;
bool hast1 = false, hast2 = false, hast4 = false; // Flags

void IRAM_ATTR ontimer(){
  slave_count++;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //For counter
  timer = timerBegin(1000000); // Timer 1MHz
  timerAttachInterrupt(timer, &ontimer);
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
  char packetBuffer[255]; //Get message here
  int packetSize = udp.parsePacket(); // Check if there is a message
  if(packetSize) {
    udp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = '\0'; // End with NULL character
    String command = String(packetBuffer);

    if(command.startsWith("SYNC")) { // received SYNC message
      t2 = slave_count;
      hast2 = true;
      Serial.printf("Received SYNC, t2 = %u\n", t2);
    }

    else if(command.startsWith("FOLLOW_UP")) { // received FOLLOW_UP message
      udp.read((uint8_t*)&t1, sizeof(t1));
      hast1 = true;
      Serial.printf("Received FOLLOW_UP, t1 = %u\n", t1);

      if(hast1 && hast2) {
        // Send DELAY_REQ
        t3 = slave_count;
        udp.beginPacket(masterIP, port);
        udp.print("DELAY_REQ");
        udp.endPacket();
        Serial.printf("Sent DELAY_REQ, t3 = %u\n", t3); 
      }
    }

    else if(command.startsWith("DELAY_RESP")) { // received FOLLOW_UP message
      udp.read((uint8_t*)&t4, sizeof(t4));
      hast4 = true;
      Serial.printf("Received DELAY_RESP, t4 = %u\n", t4);

      if(hast1 && hast2 && hast4) {
        int32_t offset = ((int32_t)(t2 - t1) + (int32_t)(t3 - t4)) / 2;
        Serial.printf("Offset = %d\n", offset);
        slave_count -= offset; // Adjust local counter

        hast1 = hast2 = hast4 = false; // Reset flags
      }
    }
  }
  Serial.printf("Slave_counter : %u\n", slave_count);
}
