# Mangue Baja - ECU Firmware (Temporada 2026)

Este repositório contém o firmware embarcado para as Unidades de Controle Eletrônico (ECUs) do veículo off-road da equipe Mangue Baja.

## Arquitetura do Sistema

A eletrônica do veículo é distribuída por múltiplos módulos principais interconectados via **Barramento CAN (CAN Bus)**:

### ECUs de comunicação

#### 1. MPU (Mapping and Positioning Unit)
Responsável pela localização do veículo e telemetria via rádio.
* **Funções:** Aquisição de dados de GPS e comunicação LoRa.
* **Status:** A espera das PCBs serem montadas para programação do firmware final.
    * **CAN:** Migração para o driver nativo **TWAI (Two-Wire Automotive Interface)** para temporização precisa e tratamento de erros.
    * **LoRa:** Implementação de um driver UART customizado com **DMA (Direct Memory Access)** em substituição às chamadas bloqueantes, garantindo que o loop principal permaneça livre durante a transmissão.

#### 2. SCU (Storage Control Unit)
É o node central de aquisição de dados, datalogger e telemetria em nuvem.
* **Funções:** Gravação local de dados em Cartão SD e telemetria MQTT via rede GSM.
* **Status:** A espera das PCBs serem montadas para programação do firmware final.
    * **Armazenamento:** Implementação de **Virtual File System (VFS)** com barramento SPI para robustez de arquivos.
    * **GSM:** Implementa um GSM A7670 e implementa telemetria via protocolo MQTT pela rede celular.

#### 3. Steering Wheel (Volante)
Responsável por apresentar informações críticas ao piloto em tempo real.
* **Funções:** Painel de instrumentos integrando displays OLED (SSD1309) via I2C/SPI e leitura de inputs.

### ECUs de aquisição

#### 4. VMU Center (Vehicle Management Unit - Center)
Unidade central focada no sensoriamento inercial do chassi.
* **Funções:** Leitura de IMU (LSM6DS3 para acelerômetro/giroscópio), cálculos de RPM do eixo, e implementação de **Filtro de Kalman** para atenuação de ruídos vibracionais.
* **Framework:** Desenvolvido nativamente em C no ecossistema STM32 (HAL/LL).

#### 5. VMU Rear (Vehicle Management Unit - Rear)
Responsável pela aquisição dos sensores localizados após a parede corta-fogo (firewall).
* **Funções:** Processamento da temperatura da CVT, temperatura do motor, leitura de velocidade (Speed) nos eixos e monitoramento ativo de tensão/corrente da bateria via ADC.
* **Framework:** Desenvolvido nativamente em C no ecossistema STM32 (HAL/LL).

## Principais Funcionalidades

* **Atualizações OTA:** Suporte a atualizações de firmware Over-The-Air nativas usando conectividade Wi-Fi.
    * *Access Point Padrão:* `<NOME_DA_PLACA> Mangue_Baja`
    * *Gateway de Update:* `192.168.34.1:1880`
* **Protocolo Automotivo:** Utiliza comunicação CAN padrão (série de IDs `0x500`) para troca inter-modular robusta.
* **Data Logging Local:** Persistência em cartões SD para análise profunda de engenharia pós-corrida.
* **Redundância de Telemetria:** Combinação de rede rádio local e rede celular, garantindo a disponibilidade dos dados no box em regiões remotas.

## Ferramentas Extras (`/Extra-Tools`)

Este repositório possui uma série de sub-projetos e utilitários úteis durante o desenvolvimento em bancada:
* **CAN Analytics & CAN Reader:** Ferramentas para varredura, monitoramento e log de frames que transitam no barramento CAN.
* **CAN Simulator:** Um nó simulador de pacotes automotivos (mock-up). Essencial para testar o comportamento das outras ECUs sem a necessidade de ligá-las ao chicote elétrico do carro.
* **I2C Scanner:** Script de diagnóstico rápido para mapear os endereços dos escravos no barramento I2C.
* **Blinker Tests (`blinker-test-esp` e `blinker-test-stm`):** Os clássicos "Hello World" nativos, úteis para validar de imediato os toolchains e a pinagem básica das placas antes de embarcar lógicas pesadas.
* **Temperatura Habitáculo:** Um módulo experimental/teste focado unicamente no monitoramento ambiental do piloto e indicação via hardware de LEDs.
* **STM Tests:** Projetos paralelos e testes em STM32, com destaque para prototipagem de rotinas de navegação (`ECU_Navegacao_prototipagem`) e testes fechados de CAN (`loopback-test`).

*Mangue Baja** | *Pernambuco, Brazil*
