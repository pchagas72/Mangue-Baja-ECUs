//MODULO CONECTADO AO CARRO - PONTE RF - CAN

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>

//https://nrf24.github.io/RF24/index.html
//https://maniacbug.github.io/RF24/classRF24.html


// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
/*uint8_t MACThisDevice[] = {0xA8, 0x03, 0x2A, 0x19, 0xB1, 0x08}; //CAR1
  uint8_t MACThisDevice[] = {0x98, 0xCD, 0xAC, 0xA9, 0x7D, 0xE4}; //CAR2
  uint8_t MACThisDevice[] = {0x98, 0xCD, 0xAC, 0xA9, 0x7D, 0xA4}; //CAR3
*/uint8_t MACCentralDevice[] = {0xA4, 0xCF, 0x12, 0x9A, 0xA9, 0xB4};
uint8_t actualAdress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
esp_now_peer_info_t peerInfo;

const int RF_INT_PIN = 26; //pino que gera a interrupção ao receber uma msg via RF

// The various roles supported by this sketch
typedef enum { role_sender = 1, role_receiver } role_e;                // The various roles supported by this sketch
const char* role_friendly_name[] = {"invalid", "Sender", "Receiver"};  // The debug-friendly names of those roles
role_e role;                                                           // The role of the current running sket

//https://wiki.seeedstudio.com/CAN-BUS_Shield_V2.0/#4get-can-id
#define CAN_2515

const int SPI_CS_PIN = 4; //CAN = VSPI (CSN=D5,MOSI=D23,MISO=D19,SCK=18)
const int CAN_INT_PIN = 22; //D27 = pino que gera a interrupção ao receber uma msg via CAN

#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN);

boolean flagCANInit = true; //se false indica que o modulo CAN não foi inicializado com sucesso

/**********************************************************************************/

int idThisDevice = 115; //id desse dispositivo na CAN

unsigned long intervalMsg;//conta tempo entre as msgs

byte bufRF[32]; //usado para armazenar os dados recebidos via RF
byte bufCAN[32]; //usado para armazenar os dados recebidos via CAN (OBS: a posição 0 e 1 é rezervado para armazenar o id do pispositivo q enviou a msg - cada campo armazen 8 bits, ou seja, um total de 16 bits)

boolean flagRecRF = false; //flag q indica q uma msg chegou via RF
boolean flagRecCAN = false; //flag q indica q uma msg chegou via CAN

unsigned long  intervalTeste = 0;


/**********************************************************************************/
/*     buffer com info das das ECUS       */
byte bufECUDirecao[10] = {0x00, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //usado para armazenar os dados recebidos via CAN  - OBS:o primeiro campo armazena o enredeço da ECU
byte bufECUPowertrain[10] = {0x00, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //usado para armazenar os dados recebidos via CAN - OBS:o primeiro campo armazena o enredeço da ECU
byte bufECUZED[10] = {0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //usado para armazenar os dados recebidos via CAN - OBS:o primeiro campo armazena o enredeço da ECU
byte bufSonarArduino[10] = {0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //usado para armazenar os dados recebidos via CAN - OBS:o primeiro campo armazena o enredeço da ECU
boolean flagECUDirecao = false, flagECUPowertrain = false, flagbufECUZED = false, flagSonarArduino = false; //se true indica q uma msg via CAN foi recebida

unsigned long timeOutRF = 0; //varivel q controla o tempo de envio dos buffers via RF (caso não haja requisição via RF)

void MsgRecCAN();
void trataMsgRecCAN();
void MsgRecRF();
void trataMsgRecRF();
void requestData();
void enviaMsgCAN();
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("\r\nLast Packet Send Status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  memcpy(actualAdress, mac, sizeof(actualAdress));

  if ((actualAdress[0]==MACCentralDevice[0])&&(actualAdress[1]==MACCentralDevice[1])&&(actualAdress[2]==MACCentralDevice[2])&&
      (actualAdress[3]==MACCentralDevice[3])&&(actualAdress[4]==MACCentralDevice[4])&&(actualAdress[5]==MACCentralDevice[5])){
    flagRecRF = true;
    memcpy(bufRF, incomingData, len);
  }
}


void setup(){
  
  Serial.begin(115200);
  Serial.println("INICIANDO ECU COMMUNICATION...");

  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  //usado pra medir tempo nas rotinas de timeout
  unsigned long tStart = 0, timeOut = 0;

  tStart = millis();
  timeOut = 8000; //(3 segundos)
  //aguarda incializar o shield CAN
  Serial.println("Connecting CAN...");
  while((millis() - tStart) < timeOut){ //aguarda o timeout
    if(CAN_OK == CAN.begin(CAN_500KBPS, MCP_8MHz)){
      Serial.println("CAN init ok!!!");
      flagCANInit = true; //marca a flag q indica q inicialização correta da CAN  
      break; //sai do laço 
    }

    flagCANInit = false; //marca a flag q indica q houve problema na inicialização da CAN
        
  }

  //se houve erro na CAN mostra 
  if(flagCANInit == false){
    Serial.println("CAN error!!!");
  }

  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MsgRecCAN, FALLING); // declara a interrupção gerada a qndo uma msg chega via CAN

  intervalTeste = millis();

  byte msgCAN [] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //variavel não usada para nada
  CAN.sendMsgBuf(idThisDevice, 0, 8, msgCAN); //envia a msg CAN para mostrar q a ECU está funcional

  timeOutRF = millis();
}


void loop(){
    
  if((millis() - timeOutRF) > 1000){ //se não houver requisição o sistema em 1s envia os dados mesmo assim
   Serial.println("Auto update RF"); 
   requestData();
  }
  

  if(flagRecCAN == true){
    trataMsgRecCAN();
  }
  

  if(flagRecRF == true){
    trataMsgRecRF();
  }
}


//interrupção gerada qndo o modulo CAN recebe uma msg via CAN
void MsgRecCAN() {
    detachInterrupt(digitalPinToInterrupt(CAN_INT_PIN)); // Caso essa boa prática não seja implementada, há problema de interrupt timetout
    flagRecCAN = true; //seta o flag q indica que há uma msg CAN no buffer
} 


//rotina que trata quando há msg CAN no controlador
void trataMsgRecCAN(){
  
  flagRecCAN = false;  // limpa a flag
  byte bufCANAux[8]; //recebe a msg vinda via CAN
  
  //Serial.println("Message CAN recived...");

  uint32_t idRecCAN = 0;
  unsigned char len = 0;


  while (CAN_MSGAVAIL == CAN.checkReceive()) {

      // read data,  len: data length, buf: data buf
      CAN.readMsgBuf(&len, bufCANAux);
      idRecCAN = CAN.getCanId(); 

      uint8_t type = (CAN.isExtendedFrame() << 0) | (CAN.isRemoteRequest() << 1); // Se 0: ext, Se 1: rtr

      /****************************************/
      // ESSAS LINHAS CONVERTEM O ID DO DISPOSITIVO SENDER PARA 32 BITS (id DE QUEM ENVIOU A MSG PELA CAN) (SÓ SERA USADO 16 BITS MENOS SIGIFICATIVOS, OU SEJA, PERMITE ENDEREÇOS ATE 65535)
  
      uint8_t idFromSender[sizeof(int32_t)];
      idFromSender[0] = (uint8_t)(idRecCAN >> 24); //8 bits mais siginifativos do id
      idFromSender[1] = (uint8_t)(idRecCAN >> 16);
      idFromSender[2] = (uint8_t)(idRecCAN >> 8);
      idFromSender[3] = (uint8_t)idRecCAN; //8 bits menos siginifativos do id

      bufCAN[0] = idFromSender[2]; //o id do device é colocado no incio do pacote enviado via RF
      bufCAN[1] = idFromSender[3]; //o id do device é colocado no incio do pacote enviado via RF
  
      //Serial.println(idFromSender[0]);
      //Serial.println(idFromSender[1]);
      //Serial.println(idFromSender[2]);
      //Serial.println(idFromSender[3]);

      /****************************************/
      Serial.print("Receive by CAN:\tid ");
      Serial.print(idRecCAN); 
      Serial.print("\t");
      
      // print the data
      //for (int i = 1; i < sizeof(bufCAN); i++){
      for (int i = 0; i < len; i++){

        if(idRecCAN == 105){ //se msg vem da ECU de Powertrain 0x69 (savla a msg da ecu num buffer)
          flagECUPowertrain = true;
          bufECUPowertrain[i + 2] = bufCANAux[i];
        }
  
        if(idRecCAN == 110){ //se msg vem da ECU de Direcao 0x6E (savla a msg da ecu num buffer)
          flagECUDirecao = true;
          bufECUDirecao[i + 2] = bufCANAux[i];
        }
  
        if(idRecCAN == 100){ //se msg vem da ZED/RASP 0x64 (savla a msg da ecu num buffer)
          flagbufECUZED = true;
          bufECUZED[i + 2] = bufCANAux[i]; 
        }


        if(idRecCAN == 120){ //se msg vem do sonar 0x78 (savla a msg da ecu num buffer)
          flagSonarArduino = true;
          bufSonarArduino[i + 2] = bufCANAux[i]; 
        }

          bufCAN[i+2] = bufCANAux[i];//copia os elementos de bufCANAux (msg CAN) para bufCAN (vetor com o id do dispositivo que enviou a msg)
        
          Serial.print(bufCANAux[i], HEX); 
          Serial.print("\t");
          
      }
      
      Serial.println();
  }
  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MsgRecCAN, FALLING);
}


//rotina que trata quando há msg RF no controlador
void trataMsgRecRF(){

  flagRecRF = false;

  for (int i = 0; i < sizeof(bufRF); i++){
    Serial.print(bufRF[i], HEX); 
    Serial.print("\t");
  }

  Serial.println();

  if(bufRF[2] == 0x88){ //0x88 indica request de todas as mensgens do buffer (se não houver requisição o sistema envia os dados)
    Serial.println("MSG REQUEST");
    requestData();//evina os dados armazenados nos buffers
      
  }else{
    Serial.println("MSG COMUM");
    enviaMsgCAN();//pega a msg recebia via RF e envia pela CAN (apenas os 8 primeiros bytes)  
  }
  Serial.println();      
}  



//qndo chamada envia os dados dos buffers das ECUs
void requestData(){

  timeOutRF = millis();
  
  if(flagECUDirecao == true){
  
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &bufECUDirecao, sizeof(bufECUDirecao));
              
    if(result == ESP_OK){ //testa se a msg foi entregue
      flagECUDirecao = false;
    }
    
  }
  delay(1);
  
  if(flagECUPowertrain == true){
    
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &bufECUPowertrain, sizeof(bufECUPowertrain));
    
    if(result == ESP_OK){ //testa se a msg foi entregue
      flagECUPowertrain = false;
    }
    
  }

  delay(1);
  
  if(flagbufECUZED == true){
  
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &bufECUZED, sizeof(bufECUZED));
  
    if(result == ESP_OK){ //testa se a msg foi entregue
      flagbufECUZED = false;
    }

  } 

  delay(1);

   if(flagSonarArduino == true){
  
      // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &bufSonarArduino, sizeof(bufSonarArduino));
    
      if(result == ESP_OK){ //testa se a msg foi entregue
        flagSonarArduino = false;
      }
  
   }
}


//rotina que envia msg via CAN
void enviaMsgCAN(){
   
  Serial.print("Send by CAN:\t");

  // print the data
  for (int i = 0; i < 8; i++){
  
    Serial.print(bufRF[i], HEX); 
    Serial.print("\t");
  }
  
  Serial.println();
  
  CAN.sendMsgBuf(idThisDevice, 0, 8, bufRF); //envia do dado recebido via CAN]
}
