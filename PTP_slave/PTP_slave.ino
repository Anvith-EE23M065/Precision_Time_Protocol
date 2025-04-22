// ================================ SLAVE CODE ================================
// Developer : Anvith M
// Date : 03-04-2025
// Function : PTP Slave

#include <WiFi.h>
#include <WiFiUdp.h>
#include <FS.h> // For file handling and logging if needed

const char* apSSID = "ESP32-Master";     // AP SSID
const char* apPassword = "12345678";     // AP Password
const char* masterIP = "192.168.4.1";    // Master IP
const int port = 1234;                   // UDP port

WiFiUDP udp;

hw_timer_t* timer = NULL;
volatile uint32_t slave_counter = 10;

uint32_t t1 = 0, t2 = 0, t3 = 0, t4 = 0;
bool hast1 = false, hast2 = false, hast4 = false, sync1 = false;

bool drift = false;
uint32_t last_logged_counter = 0;
const int time_step = 100;

void IRAM_ATTR ontimer() {
  slave_counter++;
}

void setup() {
  Serial.begin(115200);

  // Start timer
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &ontimer);
  timerAlarm(timer, 100, true, 0);

  // Connect to Master AP
  WiFi.begin(apSSID, apPassword);
  Serial.print("Connecting to Master");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected to ESP32 Master!");
    Serial.print("Slave IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Failed to connect to ESP32 Master.");
  }

  // Start UDP
  if (udp.begin(port)) {
    Serial.println("✅ UDP started on port 1234");
  } else {
    Serial.println("❌ UDP failed to start");
  }
}

void loop() {
  if (drift) {
    if (slave_counter - last_logged_counter >= time_step || slave_counter < last_logged_counter) {
      udp.beginPacket(IPAddress(192, 168, 4, 3), port);
      udp.printf("SLAVE,%lu,%u\n", millis(), slave_counter);
      udp.endPacket();
      Serial.printf("SLAVE,%lu,%u\n", millis(), slave_counter);
      last_logged_counter = slave_counter;
    }
    return;
  }

  char packetBuffer[255];
  int packetSize = udp.parsePacket();

  if (packetSize) {
    udp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = '\0';

    String full_command = String(packetBuffer);
    int cmdend = 0;
    while (isAlpha(packetBuffer[cmdend]) || packetBuffer[cmdend] == '_') {
      cmdend++;
    }
    String command = full_command.substring(0, cmdend);

    Serial.printf("Slave counter: %u\n", slave_counter);

    if (command.startsWith("SYNC") && !sync1) {
      t2 = slave_counter;
      hast2 = true;
      Serial.printf("SYNC received: t2 = %u\n", t2);
    }

    else if (command.startsWith("FOLLOW_UP") && !sync1) {
      memcpy(&t1, packetBuffer + cmdend, sizeof(t1));
      hast1 = true;
      Serial.printf("FOLLOW_UP received: t1 = %u\n", t1);

      if (hast1 && hast2) {
        t3 = slave_counter;
        udp.beginPacket(masterIP, port);
        udp.print("DELAY_REQ");
        udp.endPacket();
        Serial.printf("DELAY_REQ sent: t3 = %u\n", t3);
        sync1 = true;
      }
    }

    else if (command.startsWith("DELAY_RESP")) {
      memcpy(&t4, packetBuffer + cmdend, sizeof(t4));
      hast4 = true;
      Serial.printf("DELAY_RESP received: t4 = %u\n", t4);

      if (hast1 && hast2 && hast4) {
        int32_t offset = ((int32_t)(t2 - t1) + (int32_t)(t3 - t4)) / 2;
        Serial.printf("Offset = %d\n", offset);
        slave_counter -= offset;
        hast1 = hast2 = hast4 = sync1 = false;

        if (offset != 0) {
          Serial.printf("Slave counter adjusted: %u\n", slave_counter);
        } else {
          Serial.println("Synchronization Achieved");
          udp.beginPacket(masterIP, port);
          udp.print("DRIFT_READY");
          udp.endPacket();
          Serial.println("DRIFT_READY sent");
          drift = true;
        }
      }
    }
  }
}
