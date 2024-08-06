
#include <WiFi.h>
#include <ModbusIP_ESP8266.h>

const char* ssid = "stargate";
const char* password = "picot001";

IPAddress MBremote(192, 168, 0, 105);  // Address of Modbus Slave device
const int START_REG = 12696;           // Starting holding register
const int NUM_REGS = 1;           // Number of holding registers to read
const int INTERVAL = 1000;         // Interval between reads (in milliseconds)

WiFiClient client;
ModbusIP mb;

uint16_t res[NUM_REGS];
uint8_t show = NUM_REGS;  // Counter for displaying values
uint32_t LastModbusRequest = 0;  // Variable to track the last Modbus request time

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize Modbus client
  mb.client();
}

void loop() {
  if (millis() - LastModbusRequest >= INTERVAL) {
    LastModbusRequest = millis();

    if (mb.isConnected(MBremote)) {  
      // Read holding registers from Modbus Slave
      uint8_t transaction = mb.readHreg(MBremote, START_REG, res, 10, nullptr, 1);
      
      // Wait for the transaction to complete
      while(mb.isTransaction(transaction)) {
        mb.task();
        delay(10);
      }

      // Print holding register values
      Serial.println("Holding Register Values:");
      for (int i = 0; i < NUM_REGS; i++) {
        Serial.print("Register ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(res[i]);
      }
    } else {
      // If not connected, try to connect
      mb.connect(MBremote);
    }
  }
}