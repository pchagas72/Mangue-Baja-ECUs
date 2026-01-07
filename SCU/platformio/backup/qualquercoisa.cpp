#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "mcp2515_can.h"


unsigned long period = 5;  //modifique aqui o intervalo entre medidas, valor em ms
unsigned long time_now = 0;
int pulse_counter = 0;
int num_files = 0;

char name_file[20];
File root;

const byte ledPin = 2;
const byte interruptPin = 27;
const byte SD_CS_PIN = 5;
const byte CAN_CS_PIN = 15;
const int CAN_INT_PIN = 26; 

mcp2515_can CAN(CAN_CS_PIN);

void can_ISR();
void pulse_counter_ISR();
int countFiles(File dir);

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  unsigned long tStart = millis();
  unsigned long timeOut = 3000; //(3 segundos)
  //aguarda incializar o shield CAN
  Serial.println("Connecting CAN...");
  while((millis() - tStart) < timeOut){ //aguarda o timeout
    if(CAN_OK == CAN.begin(CAN_500KBPS, MCP_8MHz)){
      Serial.println("CAN init ok!!!");  
      break; //sai do laço 
    }   
  }

  root = SD.open("/");
  int num_files = countFiles(root);
  sprintf(name_file, "/%s%d.csv", "data", num_files+1);

  String dataString = "";
      dataString += "time_now";
      dataString += ",";
      dataString += "pulse_counter";
      dataString += ",";

  File dataFile = SD.open(name_file, FILE_APPEND);  
      if (dataFile)
      {
        dataFile.println(dataString);
        dataFile.close();
      }

  attachInterrupt(digitalPinToInterrupt(interruptPin), pulse_counter_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), can_ISR, FALLING);
}

void loop() {
  
  if (millis() >= time_now + period)
  {
    time_now += period;
        
    String dataString = "";
      dataString += String(time_now);
      dataString += ",";
      dataString += String(pulse_counter);
      dataString += ",";
           
      File dataFile = SD.open(name_file, FILE_APPEND);
       
      if (dataFile)
      {
        dataFile.println(dataString);
        dataFile.close();
      }
          
      else
      {
        Serial.println(F("ERRO"));
      }
      pulse_counter = 0;
  }
}

void can_ISR() {

}

void pulse_counter_ISR() {
  pulse_counter++;
}

int countFiles(File dir) {
  int fileCountOnSD = 0; // for counting files
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    // for each file count it
    fileCountOnSD++;
    entry.close();
  }
  return fileCountOnSD - 1;
}