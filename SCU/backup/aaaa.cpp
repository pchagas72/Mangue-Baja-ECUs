//MODULO CONECTADO AO CARRO - PONTE RF - CAN

#include <Arduino.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

//https://nrf24.github.io/RF24/index.html
//https://maniacbug.github.io/RF24/classRF24.html

SPIClass * vspi = NULL;
SPIClass * hspi = NULL; 

RF24 radio(4, 15);               // nRF24L01 = HSPI (CSN=D15,CE=D4,MOSI=D13,MISO=D12,SCK=D14)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t other_node = 00;      // Address of the other node in Octal format

const int RF_INT_PIN = 26; //pino que gera a interrupção ao receber uma msg via RF

// The various roles supported by this sketch
typedef enum { role_sender = 1, role_receiver } role_e;                // The various roles supported by this sketch
const char* role_friendly_name[] = {"invalid", "Sender", "Receiver"};  // The debug-friendly names of those roles
role_e role;                                                           // The role of the current running sket

//https://wiki.seeedstudio.com/CAN-BUS_Shield_V2.0/#4get-can-id
#define CAN_2515

const int SPI_CS_PIN = 5; //CAN = VSPI (CSN=D5,MOSI=D23,MISO=D19,SCK=18)
const int CAN_INT_PIN = 27; //D27 = pino que gera a interrupção ao receber uma msg via CAN

#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN);

boolean flagCANInit = true; //se false indica que o modulo CAN não foi inicializado com sucesso

/****************************/

int idThisDevice = 115; //id desse dispositivo na CAN

unsigned long intervalMsg;//conta tempo entre as msgs

byte bufRF[32]; //usado para armazenar os dados recebidos via RF
byte bufCAN[32]; //usado para armazenar os dados recebidos via CAN (OBS: a posição 0 e 1 é rezervado para armazenar o id do pispositivo q enviou a msg - cada campo armazen 8 bits, ou seja, um total de 16 bits)

boolean flagRecRF = false; //flag q indica q uma msg chegou via RF
boolean flagRecCAN = false; //flag q indica q uma msg chegou via CAN

unsigned long  intervalTeste = 0;


/****************************/
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


void setup(){
  
  Serial.begin(115200);
  Serial.println("INICIANDO ECU COMMUNICATION...");

  /*
  lcd.begin (16,2); //SETA A QUANTIDADE DE COLUNAS(16) E O NÚMERO DE LINHAS(2) DO DISPLAY
  lcd.setBacklight(HIGH); //LIGA O BACKLIGHT (LUZ DE FUNDO)
  lcd.setCursor(10,0);
  lcd.print("RF ON"); //sinaliza q o RF esta desconectado
  */

  vspi = new SPIClass(VSPI);
  hspi = new SPIClass(HSPI);
  vspi->begin();
  hspi->begin();
  
  if(!radio.begin(hspi))
  {
    Serial.println(F("Radio hardware not responding!"));
  }
  network.begin(90, this_node);  //(channel, node address)
  radio.setPALevel(RF24_PA_MAX);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.setDataRate(RF24_2MBPS);



  //usado pra medir tempo nas rotinas de timeout
  unsigned long tStart = 0, timeOut = 0;

  tStart = millis();
  timeOut = 8000; //(3 segundos)
  //aguarda incializar o shield CAN
  Serial.println("Connecting CAN...");
  while((millis() - tStart) < timeOut){ //aguarda o timeout
    CAN.setSPI(vspi);
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
  
  

  /*
  while (CAN_OK != CAN.begin(CAN_500KBPS, MCP_8MHz)) {
    Serial.println("CAN init fail, retry...");
    delay(100);
  }
  Serial.println("CAN init ok!");
  */

  
  /*
  //set mask, set both the mask to 0x3ff
  // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(0, 0, 0x3ff);                         
  CAN.init_Mask(1, 0, 0x3ff);
  
  // set filter, we can receive id from 0x04 ~ 0x09
  CAN.init_Filt(0, 0, 0x04);  // there are 6 filter in mcp2515
  CAN.init_Filt(1, 0, 0x05);  // there are 6 filter in mcp2515
  CAN.init_Filt(2, 0, 0x06);  // there are 6 filter in mcp2515
  CAN.init_Filt(3, 0, 0x07);  // there are 6 filter in mcp2515
  CAN.init_Filt(4, 0, 0x08);  // there are 6 filter in mcp2515
  CAN.init_Filt(5, 0, 0x09);  // there are 6 filter in mcp2515
  */

  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MsgRecCAN, FALLING); // declara a interrupção gerada a qndo uma msg chega via CAN
  attachInterrupt(digitalPinToInterrupt(RF_INT_PIN), MsgRecRF, FALLING); // declara a interrupção gerada a qndo uma msg chega via RF

  delay(2000);

  intervalTeste = millis();

  byte msgCAN [] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //variavel não usada para nada
  CAN.sendMsgBuf(idThisDevice, 0, 8, msgCAN); //envia a msg CAN para mostrar q a ECU está funcional

  timeOutRF = millis();

  radio.startListening(); //Define o modulo como recetor (Não envia dados)

}


void loop(){
    
  if((millis() - timeOutRF) > 1000){ //se não houver requisição o sistema em 1s envia os dados mesmo assim
   Serial.println("Auto update RF"); 
   requestData();
  }
  

  if(flagRecCAN == true){
    Serial.println("Received CAN");
    flagRecCAN = false;  // limpa a flag
    trataMsgRecCAN();
    attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MsgRecCAN, FALLING);
  }
  

  if(flagRecRF == true){
    Serial.println("Received RF");
    flagRecRF = false; //limpa o flag
    trataMsgRecRF();
    attachInterrupt(digitalPinToInterrupt(RF_INT_PIN), MsgRecRF, FALLING);    
  }
  
    
}


//interrupção gerada qndo o modulo CAN recebe uma msg via CAN
void MsgRecCAN() {
    detachInterrupt(digitalPinToInterrupt(CAN_INT_PIN)); // Caso essa boa prática não seja implementada, há problema de interrupt timetout
    flagRecCAN = true; //seta o flag q indica que há uma msg CAN no buffer
} 


//rotina que trata quando há msg CAN no controlador
void trataMsgRecCAN(){

  byte bufCANAux[8]; //recebe a msg vinda via CAN
  
  //Serial.println("Message CAN recived...");

  uint32_t idRecCAN = 0;
  unsigned char len = 0;


  while (CAN_MSGAVAIL == CAN.checkReceive()) {

      // read data,  len: data length, buf: data buf
      CAN.readMsgBuf(&len, bufCANAux);
      idRecCAN = CAN.getCanId(); 

      uint8_t type = (CAN.isExtendedFrame() << 0) | (CAN.isRemoteRequest() << 1); // Se 0: ext, Se 1: rtr

      /**************/
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

      /**************/
      //Serial.print("Receive by CAN:\tid ");
      //Serial.print(idRecCAN); 
      //Serial.print("\t");
      
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


        if(idRecCAN == 100){ //se msg vem da ZED/RASP 0x64 (savla a msg da ecu num buffer)
          flagSonarArduino = true;
          bufSonarArduino[i + 2] = bufCANAux[i]; 
        }

          bufCAN[i+2] = bufCANAux[i];//copia os elementos de bufCANAux (msg CAN) para bufCAN (vetor com o id do dispositivo que enviou a msg)
        
          //Serial.print(bufCANAux[i], HEX); 
          //Serial.print("\t");
          
      }
      
      //Serial.println();
      
     
  }


}



//trata a interrupção gerada pelo modulo RF (pino IRQ)
void MsgRecRF(){

  //verifica oq houve (motivo pq a rotina foi gerada - pq o pino IRQ foi pra nivel alto)
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);

  /*
  // Have we successfully transmitted?
  if (tx){
   if (role == role_sender)
     //printf("Send:OK\n\r");
   if (role == role_receive)
      //printf("Ack Payload:Sent\n\r");
  }
  
  // Have we failed to transmit?
  if (fail){
    if (role == role_sender )
      Serial.printf("Send:Failed\n\r");
    if (role == role_receiver)
      Serial.printf("Ack Payload:Failed\n\r");
  }
  */

  // Did we receive a message?
  if (rx){
    detachInterrupt(digitalPinToInterrupt(RF_INT_PIN)); // Caso essa boa prática não seja implementada, há problema de interrupt timetout
    //Serial.println("Receive by RF");
    flagRecRF = true; //seta o flag q indica que há uma msg RF no buffer

    /*
    // If we're the sender, we've received an ack payload
    if ( role == role_sender )
    {
      //radio.read(&message_count,sizeof(message_count));
      //printf("Ack:%lu\n\r",message_count);
    }
    // If we're the receiver, we've received a time message
    if ( role == role_receiver )
    {
      // Get this payload and dump it
      static unsigned long got_time;
      radio.read( &got_time, sizeof(got_time) );
      printf("Got payload %lu\n\r",got_time);
      // Add an ack packet for the next time around.  This is a simple
      // packet counter
      radio.writeAckPayload( 1, &message_count, sizeof(message_count) );
      ++message_count;
    }
    */
     
  }
  
}



//rotina que trata quando há msg RF no controlador
void trataMsgRecRF(){

  bool flagRecRFAux = false; //flag q indica q uma msg chegou via RF e não um ACK (evita o bug no recebimento do ACK) Ao receber o ACK o sistema gera outra interrupção como se fosse recebimento de msg


  network.update();
  //===== RECEBE OS DADOS  =====/
  RF24NetworkHeader header;     
  if(network.available()){//verifica se recebeu algum dado via RF
    //RF24NetworkHeader header;     
    network.read(header, &bufRF, sizeof(bufRF)); // Read the incoming data

    //Serial.println(header.from_node);


     flagRecRFAux = true;
  
    //Serial.println("*** Recebido ***");//teste
  }

  
  //intervalMsg = millis(); //usado para mudar a msg para RF OFF no display
  
  //lcd.setCursor(10,0);
  //lcd.print("RF ON "); //sinaliza q o RF esta conectado


  if(flagRecRFAux == true){   

    
    //Serial.println("#####################################");
    //Serial.print("Receive by RF:\t");
    //Serial.print(header.from_node >> 8, HEX); 
    //Serial.print(header.from_node & 0xff, HEX); 
    
    //Serial.print("\t");

    // print the data
    //for (int i = 0; i < sizeof(bufRF); i++){
      
        //Serial.print(bufRF[i], HEX); 
        //Serial.print("\t");
    //}
    
    //Serial.println();


     
    if(bufRF[2] == 0x88){ //0x88 indica request de todas as mensgens do buffer (se não houver requisição o sistema envia os dados)
      //Serial.println("MSG REQUEST");
      requestData();//evina os dados armazenados nos buffers
      
    }else{
      //Serial.println("MSG COMUM");
      enviaMsgCAN();//pega a msg recebia via RF e envia pela CAN (apenas os 8 primeiros bytes)  
    }    
    

  }
    
  
}  



//qndo chamada envia os dados dos buffers das ECUs
void requestData(){

  timeOutRF = millis();
  
  //Envia o dado recebido via CAN atraves do modulo RF
  radio.stopListening(); //Define o modulo como trasmissor (Não recebe dados)
  
  if(flagECUPowertrain == true){
    
    //envia a msg vinda via CAN atraves do modulo RF
    RF24NetworkHeader header(other_node);     // (Address where the data is going)
    bool ok = network.write(header, &bufECUPowertrain, sizeof(bufECUPowertrain)); // Send the data
    
    if(ok){ //testa se a msg foi entregue
      flagECUPowertrain = false;
    }
    
  }
  
  
  if(flagECUDirecao == true){
  
    //envia a msg vinda via CAN atraves do modulo RF
    RF24NetworkHeader header(other_node);     // (Address where the data is going)
    bool ok = network.write(header, &bufECUDirecao, sizeof(bufECUDirecao)); // Send the data
              
    if(ok){ //testa se a msg foi entregue
      flagECUDirecao = false;
    }
    
  }
  
  if(flagbufECUZED == true){
  
    //envia a msg vinda via CAN atraves do modulo RF
    RF24NetworkHeader header(other_node);     // (Address where the data is going)
    bool ok = network.write(header, &bufECUZED, sizeof(bufECUZED)); // Send the data
  
    if(ok){ //testa se a msg foi entregue
      flagbufECUZED = false;
    }

  } 


   if(flagSonarArduino == true){
  
      //envia a msg vinda via CAN atraves do modulo RF
      RF24NetworkHeader header(other_node);     // (Address where the data is going)
      bool ok = network.write(header, &bufSonarArduino, sizeof(bufSonarArduino)); // Send the data
    
      if(ok){ //testa se a msg foi entregue
        flagSonarArduino = false;
      }
  
   }

  
  radio.startListening(); //Define o modulo como recetor (Não envia dados)
  
  /*
  if(ok){
  Serial.println("msg RF entregue"); //indica q o receptor recebeu a msg
  }else{
  Serial.println("msg RF NÃO entregue"); //indica q a msg não chegou ao receptor
  }
      */

}



//rotina que envia msg via CAN
void enviaMsgCAN(){
   
  //Serial.print("Send by CAN:\t");

  // print the data
  //for (int i = 0; i < 8; i++){
  
    //Serial.print(bufRF[i], HEX); 
    //Serial.print("\t");
 // }
  
  //Serial.println();
  
  CAN.sendMsgBuf(idThisDevice, 0, 8, bufRF); //envia do dado recebido via CAN]

   
}