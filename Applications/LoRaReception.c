//***********************************************************************************************************************
//                                         LoRa Reception
//***********************************************************************************************************************
#include "LoRaReception.h"
#include "sensorHandling.h"
#include "../Applications/mainApplication.h"
#include "../Configuration/HardwareConfiguration.h"
#include "../Peripherals/RTCC.h"
#include "../Peripherals/LoRa.h"
#include <string.h>

//***********************************************************************************************************************
// Propriedades da aplicação pai que precisam ser acessadas neste módulo
//***********************************************************************************************************************
extern controlConfig_t controlList[6];

//***********************************************************************************************************************
// Variáveis privadas do módulo
//***********************************************************************************************************************
static uint8_t receptionState = 0, messageSize = 0, bytesReaded = 0;
static unsigned char receptionBuffer[MAX_PACKET_SIZE];

//***********************************************************************************************************************
// Macros
//***********************************************************************************************************************
#define getPacketOrigin(c)          (c & SOURCE_MASK)
#define getCmdPrefixFromOrigin(c)   ((getPacketOrigin(c) == COMMAND_SOURCE_SOFTWARE) ?  ENDPOINT_COMMAND :    \
                                                                                        ROUTER_COMMAND)

//***********************************************************************************************************************
// Funções privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Função de processamento dos pacotes recebidos.
//=======================================================================================================================
static void processReception(unsigned char *packet, uint8_t size)
{
    DateTime_t tempDateTime;
    CommandConfig_t requestedConfig, *configToSet;
    
    switch(packet[0] & COMMAND_MASK)
    {
        case CMD_MESSAGE:
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendPacket(ENDPOINT_COMMAND | CMD_MESSAGE, &packet[1], size-1);
            break;
        case CMD_GET_DATETIME:
            readDateTime(&tempDateTime);
            sendPacket(getCmdPrefixFromOrigin(packet[0]) | CMD_GET_DATETIME, ((unsigned char *)&tempDateTime), sizeof(tempDateTime));
            break;
        case CMD_SET_DATETIME:
            memcpy(&tempDateTime, &packet[1], sizeof(DateTime_t));
            writeDateTime((DateTime_t *)&packet[1]);
            // Confirmação para o software de configuração. Para o roteador, o módulo fará nova requisição se falhar.
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendAck(ENDPOINT_COMMAND | CMD_SET_DATETIME);
            
            // RTCC foi atualizado e pode perder uma amostragem, já que o alarme está configurado para 0 segundos.
            if(tempDateTime.Time.seconds < 10)
                forceTaskSetup();
            break;
        case CMD_GET_CONTROL_CONFIG:
            requestedConfig.index = packet[1];
            requestedConfig.operation = controlList[packet[1]].operation;
            requestedConfig.maxThreshold = controlList[packet[1]].maxThreshold;
            requestedConfig.minThreshold = controlList[packet[1]].minThreshold;
            sendPacket(getCmdPrefixFromOrigin(packet[0]) | CMD_GET_CONTROL_CONFIG, ((unsigned char *)&requestedConfig), sizeof(CommandConfig_t));
            break;
        case CMD_SET_CONTROL_CONFIG:
            configToSet = (CommandConfig_t *)(&packet[1]);
            controlList[packet[1]].operation = configToSet->operation;
            controlList[packet[1]].maxThreshold = configToSet->maxThreshold;
            controlList[packet[1]].minThreshold = configToSet->minThreshold;
            sendAck(getCmdPrefixFromOrigin(packet[0]) | CMD_SET_CONTROL_CONFIG);
            break;
        case CMD_SAVE_CONFIG:
            if(saveConfiguration())
                sendAck(getCmdPrefixFromOrigin(packet[0]) | CMD_SAVE_CONFIG);
            else
                sendNack(getCmdPrefixFromOrigin(packet[0]) | CMD_SAVE_CONFIG);
            break;
        case CMD_POWER_DOWN:
            // Confirmação apenas para o software exibir para o usuário
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendAck(ENDPOINT_COMMAND | CMD_POWER_DOWN);
            setTimeOutState(FORCE_TIMEOUT);
            break;
        case CMD_SET_TIMEOUT:
            setTimeOutState(packet[1]);
            sendAck(getCmdPrefixFromOrigin(packet[0]) | CMD_SET_TIMEOUT);   // Confirma para software de controle
        default:
            break;
    }
    
    resetTimeOut();        // Qualquer pacote recebido reseta o timeout da aplicação.
}

//=======================================================================================================================
// Máquina de estados para controle e identificação de pacotes
//=======================================================================================================================
static void processCharReception(unsigned char data)
{
    switch(receptionState)
    {
        case 0:
            if(data == 0xAA) 
                receptionState++;
            break;
        case 1:
            if(data == 0x55)
                receptionState++;
            else
                receptionState = 0;
            break;
        case 2:
            messageSize = (data > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : data;
            bytesReaded = 0;
            receptionState++;
            break;
        default:
            if(bytesReaded < messageSize)
            {
                receptionBuffer[bytesReaded++] = data;
                if(bytesReaded >= messageSize)
                {
                    processReception(receptionBuffer, messageSize);
                    receptionState = 0;
                }
            }
            else
                receptionState = 0;
      break;
  }
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Monitoramento de recepção LoRa
//=======================================================================================================================
void taskLoRaReception(uint8_t *requestCalendar, uint8_t *requestMessages)
{
    while(isLoRaTransmitting());   // Aguarda quaisquer transmissões pendentes.
    
    // Faz pedido de mensagens ao servidor. Se houver mensagens, o servidor
    // fará várias requisições, por isto a requisição é feita antes do tratamento
    // de mensagens. No final, o servidor pede para entrar em modo Deep Sleep, ou o
    // timeout fará isto.
    if(*requestCalendar)
    {
        sendDateTimeRequest();
        *requestCalendar = 0;
    }
    else if(*requestMessages)
    {
        sendMessageRequest();
        resetTimeOut();
        *requestMessages = 0;
    }
    
    if(checkLoRaReception())
    {
        while(LoRaBytesAvailable())
            processCharReception(readByteFromLoRa());
    }
}

//=======================================================================================================================
// Envia um pacote LoRa
//=======================================================================================================================
void sendPacket(unsigned char cmd, unsigned char *payload, uint8_t payloadSize)
{
    // Define que todo comando enviado é de origem do módulo
    cmd &= ~SOURCE_MASK;
    cmd |= COMMAND_SOURCE_MODULE;

    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    writeByteToLora(payloadSize+1);
    writeByteToLora(cmd);
    loadBufferToLoRa(payload, payloadSize);
    endLoRaPacket();
}

//=======================================================================================================================
// Envia um pacote LoRa de resposta ACK
//=======================================================================================================================
void sendAck(unsigned char cmd)
{
    // Define que todo comando enviado é de origem do módulo
    cmd &= ~SOURCE_MASK;
    cmd |= COMMAND_SOURCE_MODULE;

    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    writeByteToLora(0x02);
    writeByteToLora(cmd);
    writeByteToLora(0x06);
    endLoRaPacket();
}

//=======================================================================================================================
// Envia um pacote LoRa de resposta ACK
//=======================================================================================================================
void sendNack(unsigned char cmd)
{
    // Define que todo comando enviado é de origem do módulo
    cmd &= ~SOURCE_MASK;
    cmd |= COMMAND_SOURCE_MODULE;
    
    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    writeByteToLora(0x02);
    writeByteToLora(cmd);
    writeByteToLora(0x15);
    endLoRaPacket();
}

//=======================================================================================================================
// Envia uma requisição de mensagens ao servidor
//=======================================================================================================================
void sendMessageRequest(void)
{
    DateTime_t tempDateTime;
    
    readDateTime(&tempDateTime);
    sendPacket(ROUTER_COMMAND | COMMAND_SOURCE_MODULE | CMD_REQUEST_ACTION, ((unsigned char *)&tempDateTime), sizeof(tempDateTime));
}

//=======================================================================================================================
// Envia uma requisição de data/hora ao servidor
//=======================================================================================================================
void sendDateTimeRequest(void)
{
    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    writeByteToLora(0x01);
    writeByteToLora(ROUTER_COMMAND | COMMAND_SOURCE_MODULE | CMD_GET_DATETIME);
    endLoRaPacket();
}

//***********************************************************************************************************************
