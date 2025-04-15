//Developer : Anvith M
//Date : 03-04-2025
//Function : PTP Master
#include <WiFi.h>
#include <WiFiUdp.h>

//For filehandling and logging
#include <FS.h>

const char* apSSID = "ESP32-Master"; //AP-Name
const char* apPassword = "12345678"; //AP Password (minimum 8 characters)
WiFiUDP udp;

bool drift = false;
uint32_t last_logged_counter = 0; 

const int port = 1234;

hw_timer_t *timer = NULL;
volatile uint32_t master_counter = 0;

void IRAM_ATTR onTimer() {
    master_counter++;
}

void setup() {
    Serial.begin(115200);

    drift = false;
    
    //For counter
    timer = timerBegin(1000000); // Timer 80/xMHz
    timerAttachInterrupt(timer, &onTimer); // Attach function onTimer() to timer
    timerAlarm(timer, 10, true, 0); // Set alarm to call every second(value in us)

    // Start ESP32 as an Access Point(AP)
    WiFi.softAP(apSSID, apPassword); 
    IPAddress masterIP = WiFi.softAPIP(); // Get IP address of AP
    Serial.print("Master AP IP : ");
    Serial.print(masterIP);
    Serial.println(" ");

    //Start UDP communication
    udp.begin(port);
}

void loop() {
    
    //Drift mode
    if(drift){
      if(master_counter - last_logged_counter >= 10 || master_counter - last_logged_counter < 0){
      udp.beginPacket(IPAddress(192, 168, 4, 3), port);
      udp.printf("MASTER,%lu,%u\n",millis(),master_counter);
      udp.endPacket();
      Serial.printf("MASTER,%lu,%u\n", millis(), master_counter);
      last_logged_counter = master_counter;
      }
      return;
    }

    uint32_t T1, T4; // T1 and T4
    
    // Send SYNC message
    udp.beginPacket(IPAddress(192,168,4,255), port); // Broadcast to all devices
    udp.print("SYNC"); // Send SYNC message
    T1 = master_counter; // Save timestamp when SYNC is sent
    udp.endPacket();
    Serial.printf("Sent SYNC message @ T1 = %u\n", T1);
    

    // Send FOLLOW_UP message with T1
    udp.beginPacket(IPAddress(192,168,4,255), port);
    udp.print("FOLLOW_UP"); // Send FOLLOW_UP Message
    udp.write((uint8_t*)&T1, sizeof(T1)); // Send T1 value
    udp.endPacket();    
    Serial.printf("Sent FOLLOW_UP message @ Time = %u\n", master_counter);
    ;

    char packetBuffer[255]; // To recieve message from udp
    int packetSize = udp.parsePacket();
    if(packetSize){ // A message is recieved
      T4 = master_counter;
      udp.read(packetBuffer, packetSize); // Read Message
      packetBuffer[packetSize] = '\0';
      String command = String(packetBuffer);

      if(command == "DELAY_REQ"){        
        Serial.printf("Received DELAY_REQ @ time = %u\n", master_counter);
        udp.beginPacket(udp.remoteIP(), udp.remotePort()); // Prepare response to sending device
        udp.print("DELAY_RESP"); // Delay Response
        udp.write((uint8_t*)&T4, sizeof(T4)); // Send T4 value
        udp.endPacket(); 
        Serial.printf("Sent DELAY t4 message @ Time = %u\n", T4);
      }

      else if(command == "DRIFT_READY"){
        Serial.println("Received DRIFT_READY, Entering drift mode...");
        drift = true;
      }
    }
    Serial.printf("Master counter : %u\n", master_counter);
}
