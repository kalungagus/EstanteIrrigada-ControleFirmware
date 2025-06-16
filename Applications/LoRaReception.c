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
extern controlConfig_t controlList[MAX_SENSORS];

//***********************************************************************************************************************
// Variáveis privadas do módulo
//***********************************************************************************************************************
static uint8_t receptionState = 0, messageSize = 0, bytesReaded = 0;
static unsigned char receptionBuffer[MAX_PACKET_SIZE];

//***********************************************************************************************************************
// Macros
//***********************************************************************************************************************
#define getPacketOrigin(c)              (c & SOURCE_MASK)
#define getResponsePrefixForOrigin(c)   ((getPacketOrigin(c) == COMMAND_SOURCE_SOFTWARE) ?  ENDPOINT_COMMAND :    \
                                                                                            ROUTER_COMMAND)

//***********************************************************************************************************************
// Funções privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Função de processamento dos pacotes recebidos.
//=======================================================================================================================
static void processReception(unsigned char *packet, uint8_t size)
{
    DateTime_t tempDateTime, systemDateTime;
    CommandConfig_t requestedConfig, *configToSet;
    
    switch(packet[0] & COMMAND_MASK)
    {
        case CMD_MESSAGE:
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendPacket(ENDPOINT_COMMAND | CMD_MESSAGE, &packet[1], size-1);
            break;
            
        case CMD_GET_DATETIME:
            readDateTime(&tempDateTime);
            sendPacket(getResponsePrefixForOrigin(packet[0]) | CMD_GET_DATETIME, ((unsigned char *)&tempDateTime), sizeof(tempDateTime));
            break;
            
        case CMD_SET_DATETIME:
            if (size < (1 + sizeof(DateTime_t))) break;   // Verifica se todos os dados chegaram antes da atualização.
            
            memcpy(&tempDateTime, &packet[1], sizeof(DateTime_t));
            readDateTime(&systemDateTime);
            writeDateTime((DateTime_t *)&packet[1]);
            // Confirmação para o software de configuração. Para o roteador, o módulo fará nova requisição se falhar.
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendAck(ENDPOINT_COMMAND | CMD_SET_DATETIME);
            
            // RTCC foi atualizado e pode perder uma amostragem, já que o alarme está configurado para 0 segundos.
            if((bcdToInt(systemDateTime.Time.seconds) > 50) && (bcdToInt(tempDateTime.Time.seconds) < 10))
                callTaskScheduler();
            break;
            
        case CMD_GET_CONTROL_CONFIG:
            // Índice apontando para fora das configurações de sensor.
            if(size < 2 || packet[1] >= MAX_SENSORS) break;
            
            requestedConfig.index = packet[1];
            requestedConfig.operation = controlList[packet[1]].operation;
            requestedConfig.maxThreshold = controlList[packet[1]].maxThreshold;
            requestedConfig.minThreshold = controlList[packet[1]].minThreshold;
            sendPacket(getResponsePrefixForOrigin(packet[0]) | CMD_GET_CONTROL_CONFIG, ((unsigned char *)&requestedConfig), sizeof(CommandConfig_t));
            break;
            
        case CMD_SET_CONTROL_CONFIG:
            // Índice apontando para fora das configurações de sensor.
            if(size < (1 + sizeof(CommandConfig_t)) || packet[1] >= MAX_SENSORS) break;
            
            configToSet = (CommandConfig_t *)(&packet[1]);
            controlList[packet[1]].operation = configToSet->operation;
            controlList[packet[1]].maxThreshold = configToSet->maxThreshold;
            controlList[packet[1]].minThreshold = configToSet->minThreshold;
            sendAck(getResponsePrefixForOrigin(packet[0]) | CMD_SET_CONTROL_CONFIG);
            break;
            
        case CMD_SAVE_CONFIG:
            if(saveConfiguration())
                sendAck(getResponsePrefixForOrigin(packet[0]) | CMD_SAVE_CONFIG);
            else
                sendNack(getResponsePrefixForOrigin(packet[0]) | CMD_SAVE_CONFIG);
            break;
            
        case CMD_POWER_DOWN:
            // Confirmação apenas para o software exibir para o usuário
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendAck(ENDPOINT_COMMAND | CMD_POWER_DOWN);
            setDeepSleepTimeOutState(FORCE_TIMEOUT);
            break;
            
        case CMD_SET_TIMEOUT:
            if (size < 2) break;
            
            setDeepSleepTimeOutState(packet[1]);
            sendAck(getResponsePrefixForOrigin(packet[0]) | CMD_SET_TIMEOUT);   // Confirma para software de controle
            break;
            
        default:
            sendNack(getResponsePrefixForOrigin(packet[0]) | CMD_UNKNOWN);
            break;
    }
    
    resetDeepSleepTimeOut();        // Qualquer pacote recebido reseta o timeout da aplicação.
}

//=======================================================================================================================
// Máquina de estados para controle e identificação de pacotes
//=======================================================================================================================
static void processCharReception(unsigned char data)
{
    switch(receptionState)
    {
        case 0: // Aguardando byte 0xAA
            if(data == 0xAA) 
                receptionState = 1;
            break;
            
        case 1: // Aguardando byte 0x55
            if(data == 0x55)
                receptionState = 2;
            else
                receptionState = 0; // Reseta se não for o byte esperado
            break;
            
        case 2: // Recebe tamanho da mensagem
            messageSize = (data > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : data;
            bytesReaded = 0;
            receptionState = 3;
            break;
            
        case 3:
            receptionBuffer[bytesReaded++] = data;
            if(bytesReaded >= messageSize)
            {
                processReception(receptionBuffer, messageSize);
                receptionState = 0; // Reseta para próxima mensagem
            }
            break;
            
        default:
            receptionState = 0; // Em caso de estado inválido, reseta
            break;
  }
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Monitoramento de recepção LoRa
//=======================================================================================================================
void taskCommunication(uint8_t flags)
{
    if (isLoRaTransmitting())   // Transmissão em andamento, retorne e tente novamente depois
        return;
    
    // Faz pedido de mensagens ao servidor. Se houver mensagens, o servidor
    // fará várias requisições, por isto a requisição é feita antes do tratamento
    // de mensagens. No final, o servidor pede para entrar em modo Deep Sleep, ou o
    // timeout fará isto.
    if(flags & FLAGS_REQUEST_CALENDAR)
    {
        sendDateTimeRequest();
    }
    else if(flags & FLAGS_REQUEST_MESSAGES)
    {
        sendMessageRequest();
        resetDeepSleepTimeOut();
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
