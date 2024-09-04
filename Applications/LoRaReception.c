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
// Propriedades da aplica��o pai que precisam ser acessadas neste m�dulo
//***********************************************************************************************************************
extern controlConfig_t controlList[6];

//***********************************************************************************************************************
// Vari�veis privadas do m�dulo
//***********************************************************************************************************************
static uint8_t receptionState = 0, messageSize = 0, bytesReaded = 0;
int8_t requestMessages = 1;
static unsigned char receptionBuffer[MAX_PACKET_SIZE];

//***********************************************************************************************************************
// Fun��es privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Fun��o de processamento dos pacotes recebidos.
//=======================================================================================================================
static void processReception(unsigned char *packet, uint8_t size)
{
    DateTime_t tempDateTime;
    CommandConfig_t requestedConfig, *configToSet;
    uint8_t tmpPacket[1];
    
    switch(packet[0])
    {
        case CMD_MESSAGE_ECHO:
            sendPacket(CMD_MESSAGE_ECHO, &packet[1], size-1);
            break;
        case CMD_GET_DATETIME:
            readDateTime(&tempDateTime);
            sendPacket(CMD_GET_DATETIME, ((unsigned char *)&tempDateTime), sizeof(tempDateTime));
            break;
        case CMD_SET_DATETIME:
            memcpy(&tempDateTime, &packet[1], sizeof(DateTime_t));
            writeDateTime((DateTime_t *)&packet[1]);
            sendAck(CMD_SET_DATETIME);
            break;
        case CMD_GET_SAMPLES:
            setNewSensorReading();
            break;
        case CMD_GET_CONTROL_CONFIG:
            requestedConfig.index = packet[1];
            requestedConfig.operation = controlList[packet[1]].operation;
            requestedConfig.maxThreshold = controlList[packet[1]].maxThreshold;
            requestedConfig.minThreshold = controlList[packet[1]].minThreshold;
            sendPacket(CMD_GET_CONTROL_CONFIG, ((unsigned char *)&requestedConfig), sizeof(CommandConfig_t));
            break;
        case CMD_SET_CONTROL_CONFIG:
            configToSet = (CommandConfig_t *)(&packet[1]);
            controlList[packet[1]].operation = configToSet->operation;
            controlList[packet[1]].maxThreshold = configToSet->maxThreshold;
            controlList[packet[1]].minThreshold = configToSet->minThreshold;
            sendAck(CMD_SET_CONTROL_CONFIG);
            break;
        case CMD_GET_ALARM_FREQUENCY:
            tmpPacket[0] = getAlarmFrequency();
            sendPacket(CMD_GET_ALARM_FREQUENCY, tmpPacket, sizeof(tmpPacket));
            break;
        case CMD_SET_ALARM_FREQUENCY:
            if(packet[1] > 1 && packet[1] < 7)   // Verifica se o valor � v�lido
            {
                setAlarmFrequency(packet[1]);
                sendAck(CMD_SET_ALARM_FREQUENCY);
            }
            else
                sendNack(CMD_SET_ALARM_FREQUENCY);
            break;
        case CMD_SAVE_CONFIG:
            if(saveConfiguration())
                sendAck(CMD_SAVE_CONFIG);
            else
                sendNack(CMD_SAVE_CONFIG);
            break;
        case CMD_POWER_DOWN:
            sendAck(CMD_POWER_DOWN);
            deepSleep();   // Modo Deep Sleep. A sa�da ser� como uma reinicializa��o
            break;
        case CMD_HALT_TIMEOUT:
            setTimeOutState(TIMEOUT_DISABLE);
            sendAck(CMD_HALT_TIMEOUT);
        default:
            break;
    }
    
    resetTimeOut();        // Qualquer pacote recebido reseta o timeout da aplica��o.
}

//=======================================================================================================================
// M�quina de estados para controle e identifica��o de pacotes
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
// Fun��es p�blicas
//***********************************************************************************************************************
//=======================================================================================================================
// Monitoramento de recep��o LoRa
//=======================================================================================================================
void taskLoRaReception(void)
{
    isLoRaTransmitting();   // Faz a verifica��o para limpar uma eventual transmiss�o.
    
    // Faz pedido de mensagens ao servidor. Se houver mensagens, o servidor
    // far� v�rias requisi��es, por isto a requisi��o � feita antes do tratamento
    // de mensagens. No final, o servidor pede para entrar em modo Deep Sleep, ou o
    // timeout far� isto.
    if(requestMessages)
    {
        sendMessageRequest();
        resetTimeOut();
        requestMessages = 0;
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
// Envia uma requisi��o de mensagens ao servidor
//=======================================================================================================================
void sendMessageRequest(void)
{
    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    writeByteToLora(0x01);
    writeByteToLora(CMD_REQUEST_MESSAGE);
    endLoRaPacket();
}

//***********************************************************************************************************************
