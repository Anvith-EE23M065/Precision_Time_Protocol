// ================================ SLAVE CODE ================================
// Developer : Anvith M
// Date : 03-04-2025
// Function : PTP Slave
#include <WiFi.h>
#include <WiFiUdp.h>

// For file handling and logging 
#include <FS.h>


const char* apSSID = "ESP32-Master"; // AP-Name
const char* apPassword = "12345678"; // AP Password
WiFiUDP udp;
const char* masterIP = "192.168.4.1";
const int port = 1234;

hw_timer_t *timer = NULL;
volatile uint32_t slave_counter = 10;

uint32_t t1 = 0, t2 = 0, t3 = 0, t4 = 0;
bool hast1 = false, hast2 = false, hast4 = false, sync1 = false; // Flags

bool drift = false; //For drift mode
uint32_t last_logged_counter = 0;

void IRAM_ATTR ontimer(){
  slave_counter++;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  drift = false;
 
  //For counter
  timer = timerBegin(1000000); // Timer 80/xMHz
  timerAttachInterrupt(timer, &ontimer);
  timerAlarm(timer, 10, true, 0);

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
  // drift code
  if(drift){
    if(slave_counter - last_logged_counter >= 10 || slave_counter - last_logged_counter < 0){
    udp.beginPacket(IPAddress(192, 168, 4, 3), port);
    udp.printf("SLAVE,%lu,%u\n",millis(),slave_counter);
    udp.endPacket();
    Serial.printf("SLAVE,%lu,%u\n", millis(), slave_counter);
    last_logged_counter = slave_counter;
    }
    return;
  }

  //Wifi comm code
  char packetBuffer[255]; //Get message here
  int packetSize = udp.parsePacket(); // Check if there is a message
  if(packetSize) {
    udp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = '\0'; // End with NULL character
    String full_command = String(packetBuffer);

    int cmdend = 0;
    while(isAlpha(packetBuffer[cmdend]) || packetBuffer[cmdend] == '_'){
      cmdend++;
    }

    String command = full_command.substring(0, cmdend);
    
    Serial.printf("Slave counter : %u\n", slave_counter);

    if(command.startsWith("SYNC") && !sync1) { // received SYNC message
      t2 = slave_counter;
      hast2 = true;
      Serial.printf("Received SYNC, t2 = %u\n", t2);
      Serial.printf("%s\n",command);
    }

    else if(command.startsWith("FOLLOW_UP") && !sync1) { // received FOLLOW_UP message
      memcpy(&t1, packetBuffer + cmdend, sizeof(t1));
      hast1 = true;
      Serial.printf("Received FOLLOW_UP, t1 = %u\n", t1);
      Serial.printf("%s\n",command);

      if(hast1 && hast2) {
        // Send DELAY_REQ
        t3 = slave_counter;
        udp.beginPacket(masterIP, port);
        udp.print("DELAY_REQ");
        udp.endPacket();
        Serial.printf("Sent DELAY_REQ, t3 = %u\n", t3); 
        sync1 = true;
      }
    }
    else if(command.startsWith("DELAY_RESP")) { // received FOLLOW_UP message
      memcpy(&t4, packetBuffer + cmdend, sizeof(t4));
      hast4 = true;
      Serial.printf("Received DELAY_RESP, t4 = %u\n", t4);
      Serial.printf("%s\n",command);

      if(hast1 && hast2 && hast4) {
        int32_t offset = ((int32_t)(t2 - t1) + (int32_t)(t3 - t4)) / 2;
        Serial.printf("Offset = %d\n", offset);
        slave_counter -= offset; // Adjust local counter
        hast1 = hast2 = hast4 = sync1 = false; // Reset flags
        
        if(offset)
          Serial.printf("Slave counter after offset: %u\n", slave_counter);
        else {
          // Send Master signal saying drift is achieved
          Serial.println("Synchronization Achieved");
          udp.beginPacket(masterIP, port);
          udp.print("DRIFT_READY");
          udp.endPacket();
          Serial.println("sent DRIFT_READY");

          drift = true;
        }  
      }
    }
  }
  // Serial.printf("Slave_counter : %u\n", slave_counter);
}
