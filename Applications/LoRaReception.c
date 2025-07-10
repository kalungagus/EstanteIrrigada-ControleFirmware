//***********************************************************************************************************************
//                                         LoRa Reception
//***********************************************************************************************************************
#include "LoRaReception.h"
#include "sensorHandling.h"
#include "../Applications/mainApplication.h"
#include "../Configuration/HardwareConfiguration.h"
#include "../Peripherals/RTCC.h"
#include "../Peripherals/LoRa.h"
#include "../Peripherals/timers.h"
#include <string.h>
#include <libpic30.h>

//***********************************************************************************************************************
// Propriedades da aplicação pai que precisam ser acessadas neste módulo
//***********************************************************************************************************************
extern controlConfig_t controlList[MAX_SENSORS];

//***********************************************************************************************************************
// Variáveis privadas do módulo
//***********************************************************************************************************************
static uint8_t receptionState = RECEPTION_STATE_IDLE, messageSize = 0, bytesReaded = 0;
static uint8_t transmissionState = TRANSMISSION_STATE_IDLE, transmissionSize = 0, ackAttempts = 0;
static uint32_t transmissionTimeOut = 0;
static unsigned char receptionBuffer[MAX_PACKET_SIZE];
static unsigned char transmissionBuffer[MAX_PACKET_SIZE];

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
// Calcula CRC8 incrementalmente.
//=======================================================================================================================
uint8_t crc8_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; ++i)
    {
        if (crc & 0x80)
            crc = (crc << 1) ^ 0x07; // Polinômio CRC-8: x^8 + x^2 + x + 1 (0x07)
        else
            crc <<= 1;
    }
    return crc;
}

uint16_t getUInt16BE(unsigned char *buffer, uint8_t index)
{
    return ((uint16_t)buffer[index] << 8) | buffer[index + 1];
}

uint16_t getUInt16LE(unsigned char *buffer, uint8_t index)
{
    return ((uint16_t)buffer[index + 1] << 8) | buffer[index];
}

//=======================================================================================================================
// Função de processamento dos pacotes recebidos.
//=======================================================================================================================
static void processReception(unsigned char *packet, uint8_t size)
{
    DateTime_t tempDateTime, systemDateTime;
    CommandConfig_t requestedConfig;
    uint8_t operationResult;
    
    switch(packet[0] & COMMAND_MASK)
    {
        case CMD_MESSAGE:
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
                sendPacket(ENDPOINT_COMMAND | CMD_MESSAGE, &packet[1], size-1);
            break;
            
        case CMD_GET_DATETIME:
            operationResult = readDateTime(&tempDateTime);
            if(operationResult)
                sendPacket(getResponsePrefixForOrigin(packet[0]) | CMD_GET_DATETIME, ((unsigned char *)&tempDateTime), sizeof(tempDateTime));
            else
                sendNack(getResponsePrefixForOrigin(packet[0]) | CMD_GET_DATETIME);
            break;
            
        case CMD_SET_DATETIME:
            if (size < (1 + sizeof(DateTime_t))) break;   // Verifica se todos os dados chegaram antes da atualização.
            
            memcpy(&tempDateTime, &packet[1], sizeof(DateTime_t));
            readDateTime(&systemDateTime);
            operationResult = writeDateTime(&tempDateTime);
            // Confirmação para o software de configuração. Para o roteador, o módulo fará nova requisição se falhar.
            if(getPacketOrigin(packet[0]) == COMMAND_SOURCE_SOFTWARE)
            {
                if(operationResult)
                    sendAck(ENDPOINT_COMMAND | CMD_SET_DATETIME);
                else
                    sendNack(ENDPOINT_COMMAND | CMD_SET_DATETIME);
            }
            
            // RTCC foi atualizado e pode perder uma amostragem, já que o alarme está configurado para 0 segundos.
            if((bcdToInt(systemDateTime.Time.seconds) > 50) && (bcdToInt(tempDateTime.Time.seconds) < 10))
                callTaskScheduler();
            break;
            
        case CMD_GET_CONTROL_CONFIG:
            if(size < 2) break;
            
            if(packet[1] == 0xFF)
            {
                CommandConfig_t allConfigs[MAX_SENSORS];
                for (uint8_t i = 0; i < MAX_SENSORS; i++)
                {
                    allConfigs[i].index = i;
                    allConfigs[i].operation = controlList[i].operation;
                    allConfigs[i].maxThreshold = controlList[i].maxThreshold;
                    allConfigs[i].minThreshold = controlList[i].minThreshold;
                }
                sendPacket(getResponsePrefixForOrigin(packet[0]) | CMD_GET_CONTROL_CONFIG, ((unsigned char *)&allConfigs), sizeof(allConfigs));
            }
            else if(packet[1] < MAX_SENSORS)
            {
                requestedConfig.index = packet[1];
                requestedConfig.operation = controlList[packet[1]].operation;
                requestedConfig.maxThreshold = controlList[packet[1]].maxThreshold;
                requestedConfig.minThreshold = controlList[packet[1]].minThreshold;
                sendPacket(getResponsePrefixForOrigin(packet[0]) | CMD_GET_CONTROL_CONFIG, ((unsigned char *)&requestedConfig), sizeof(CommandConfig_t));
            }
            break;
            
        case CMD_SET_CONTROL_CONFIG:
            if(size < 2) break;
            
            for(uint8_t i = 1; i < size; i+= sizeof(CommandConfig_t))
            {
                uint8_t index = packet[i];
                controlList[index].operation = packet[i+1];
                controlList[index].minThreshold = getUInt16LE(packet, i+2);
                controlList[index].maxThreshold = getUInt16LE(packet, i+4);
            }
            
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
            
        case CMD_SEND_SAMPLES:
            if (size != 2) break;
            
            if(packet[1] == 0x05)          // Envio de ENQ é uma confirmação com pedido de parada de timer.
            {
                setDeepSleepTimeOutState(packet[1]);
                transmissionState = TRANSMISSION_STATE_IDLE;
                sendAck(getResponsePrefixForOrigin(packet[0]) | CMD_SET_TIMEOUT);
            }
            else if(packet[1] == 0x06)     // Envio de ACK é apenas uma confirmação de recepção
                transmissionState = TRANSMISSION_STATE_IDLE;
            
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
    static uint8_t crc;
    
    switch(receptionState)
    {
        case RECEPTION_STATE_IDLE: // Aguardando byte 0xAA
            if(data == 0xAA) 
                receptionState = RECEPTION_STATE_HEADER;
            break;
            
        case RECEPTION_STATE_HEADER: // Aguardando byte 0x55
            if(data == 0x55)
                receptionState = RECEPTION_STATE_SIZE;
            else
                receptionState = RECEPTION_STATE_IDLE; // Reseta se não for o byte esperado
            break;
            
        case RECEPTION_STATE_SIZE: // Recebe tamanho da mensagem
            if(data < 2 || data > MAX_PACKET_SIZE) 
            {
                receptionState = RECEPTION_STATE_IDLE;  // Tamanho inválido
                break;
            }
            
            messageSize = data;
            bytesReaded = 0;
            crc = crc8_update(0x00, data);  // Inclui tamanho no CRC
            receptionState = RECEPTION_STATE_PAYLOAD;
            break;
            
        case RECEPTION_STATE_PAYLOAD: // Recebendo payload (inclui comando + payload + CRC)
            receptionBuffer[bytesReaded++] = data;
            crc = crc8_update(crc, data);
            
            if (bytesReaded >= messageSize - 1)
                receptionState = RECEPTION_STATE_CRC;
            break;
            
        case RECEPTION_STATE_CRC:
            if(crc == data)
                processReception(receptionBuffer, messageSize-1);

            receptionState = RECEPTION_STATE_IDLE; // Reseta para próxima mensagem
            break;
            
        default:
            receptionState = RECEPTION_STATE_IDLE; // Em caso de estado inválido, reseta
            break;
  }
}

//=======================================================================================================================
// Máquina de estados para transmissão e retransmissão de amostras
//=======================================================================================================================
void processTransmission(void)
{
    switch(transmissionState)
    {
        case TRANSMISSION_STATE_IDLE:
            break;
            
        case TRANSMISSION_STATE_SEND:
            beginLoRaPacket(EXPLICIT_MODE);
            loadBufferToLoRa(transmissionBuffer, transmissionSize);
            endLoRaPacket();

            transmissionTimeOut = getTimerInterruptCount();
            transmissionState = TRANSMISSION_STATE_WAIT_ACK;
            break;
            
        case TRANSMISSION_STATE_WAIT_ACK:
            if(getElapsedTimeSince(transmissionTimeOut) >= TRANSMISSION_TIME_OUT)
            {
                if(--ackAttempts > 0)
                    transmissionState = TRANSMISSION_STATE_SEND;
                else
                    transmissionState = TRANSMISSION_STATE_IDLE;
            }
            break;
            
        default:
            transmissionState = TRANSMISSION_STATE_IDLE;
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

    if(transmissionState != TRANSMISSION_STATE_IDLE)
        processTransmission();
}

//=======================================================================================================================
// Envia um pacote LoRa
//=======================================================================================================================
void sendPacket(unsigned char cmd, unsigned char *payload, uint8_t payloadSize)
{
    uint8_t crc = 0x00;
    
    // Define que todo comando enviado é de origem do módulo
    cmd &= ~SOURCE_MASK;
    cmd |= COMMAND_SOURCE_MODULE;

    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);

    writeByteToLora(0xAA);
    writeByteToLora(0x55);

    crc = crc8_update(crc, payloadSize+2);
    writeByteToLora(payloadSize+2);

    crc = crc8_update(crc, cmd);
    writeByteToLora(cmd);

    for (uint8_t i = 0; i < payloadSize; i++)  // 'length' é o número de bytes do payload
    {
        crc = crc8_update(crc, payload[i]);
        writeByteToLora(payload[i]);
    }
    
    writeByteToLora(crc);
    
    endLoRaPacket();
}

//=======================================================================================================================
// Inicia a transmissão de um pacote com retransmissão
//=======================================================================================================================
void startTransmission(unsigned char cmd, unsigned char *payload, uint8_t payloadSize)
{
    if(transmissionState == TRANSMISSION_STATE_IDLE && (payloadSize + 5 < MAX_PACKET_SIZE))
    {
        uint8_t crc = 0x00;

        // Define que todo comando enviado é de origem do módulo
        cmd &= ~SOURCE_MASK;
        cmd |= COMMAND_SOURCE_MODULE;

        transmissionBuffer[0] = 0xAA;
        transmissionBuffer[1] = 0x55;

        crc = crc8_update(crc, payloadSize+2);
        transmissionBuffer[2] = payloadSize+2;

        crc = crc8_update(crc, cmd);
        transmissionBuffer[3] = cmd;

        for (uint8_t i = 0; i < payloadSize; i++)
        {
            crc = crc8_update(crc, payload[i]);
            transmissionBuffer[4+i] = payload[i];
        }

        transmissionBuffer[4+payloadSize] = crc;

        ackAttempts = 3;
        transmissionSize = payloadSize + 5;
        transmissionState = TRANSMISSION_STATE_SEND;
    }
}

//=======================================================================================================================
// Envia um pacote LoRa de resposta ACK
//=======================================================================================================================
void sendAck(unsigned char cmd)
{
    uint8_t crc = 0x00;
    
    // Define que todo comando enviado é de origem do módulo
    cmd &= ~SOURCE_MASK;
    cmd |= COMMAND_SOURCE_MODULE;

    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    
    crc = crc8_update(crc, 0x03);
    writeByteToLora(0x03);
    
    crc = crc8_update(crc, cmd);
    writeByteToLora(cmd);
    
    crc = crc8_update(crc, 0x06);
    writeByteToLora(0x06);
    
    writeByteToLora(crc);
    
    endLoRaPacket();
}

//=======================================================================================================================
// Envia um pacote LoRa de resposta ACK
//=======================================================================================================================
void sendNack(unsigned char cmd)
{
    uint8_t crc = 0x00;
    
    // Define que todo comando enviado é de origem do módulo
    cmd &= ~SOURCE_MASK;
    cmd |= COMMAND_SOURCE_MODULE;
    
    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    
    crc = crc8_update(crc, 0x03);
    writeByteToLora(0x03);
    
    crc = crc8_update(crc, cmd);
    writeByteToLora(cmd);
    
    crc = crc8_update(crc, 0x15);
    writeByteToLora(0x15);
    
    writeByteToLora(crc);
    
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
    uint8_t crc = 0x00;
    
    while(isLoRaTransmitting());
    beginLoRaPacket(EXPLICIT_MODE);
    
    writeByteToLora(0xAA);
    writeByteToLora(0x55);
    
    crc = crc8_update(crc, 0x02);
    writeByteToLora(0x02);
    
    crc = crc8_update(crc, ROUTER_COMMAND | COMMAND_SOURCE_MODULE | CMD_GET_DATETIME);
    writeByteToLora(ROUTER_COMMAND | COMMAND_SOURCE_MODULE | CMD_GET_DATETIME);
    
    writeByteToLora(crc);
    
    endLoRaPacket();
}

//=======================================================================================================================
// Verifica se os canais de comunicação estão ocupados
//=======================================================================================================================
uint8_t isCommunicationFree(void)
{
    uint8_t result = ((transmissionState == TRANSMISSION_STATE_IDLE) &&
                      (receptionState    == RECEPTION_STATE_IDLE) &&
                      !isLoRaTransmitting());
    return result;
}
//***********************************************************************************************************************
