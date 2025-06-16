//***********************************************************************************************************************
//                              Estante Irrigada - LoRa Reception
//***********************************************************************************************************************
#ifndef APPLICATION_LORA_RECEPTION
#define	APPLICATION_LORA_RECEPTION

#include <xc.h>

//=======================================================================================================================
// Máscaras de comandos
//=======================================================================================================================
#define BROAD_COMMAND            0xC0
#define ENDPOINT_COMMAND         0x80
#define ROUTER_COMMAND           0x40
#define COMMAND_SOURCE_MODULE    0x20
#define COMMAND_SOURCE_SOFTWARE  0x10
#define COMMAND_SOURCE_ROUTER    0x00
#define COMMAND_MASK             0x0F
#define SOURCE_MASK              0x30

//=======================================================================================================================
// Comandos do módulo
//=======================================================================================================================
#define CMD_MESSAGE              0x00
#define CMD_GET_DATETIME         0x01
#define CMD_SET_DATETIME         0x02
#define CMD_SEND_SAMPLES         0x03
#define CMD_GET_CONTROL_CONFIG   0x04
#define CMD_SET_CONTROL_CONFIG   0x05
#define CMD_SAVE_CONFIG          0x06
#define CMD_POWER_DOWN           0x07
#define CMD_REQUEST_ACTION       0x08
#define CMD_SET_TIMEOUT          0x09
#define CMD_UNKNOWN              0xFF

//=======================================================================================================================
// Flags da task Communication
//=======================================================================================================================
#define FLAGS_NO_TRANSMITION     0x00
#define FLAGS_REQUEST_CALENDAR   0x01
#define FLAGS_REQUEST_MESSAGES   0x02

//***********************************************************************************************************************
// Tipos de variáveis relacionadas ao módulo de recepção e transmissão LoRa
//***********************************************************************************************************************
//=======================================================================================================================
// Variável de parâmetros para o controle
//=======================================================================================================================
typedef struct
{
    uint8_t         index;
    uint8_t         operation;
    uint16_t        minThreshold;
    uint16_t        maxThreshold;
} CommandConfig_t;

//***********************************************************************************************************************
// Funções públicas do módulo
//***********************************************************************************************************************
extern void taskCommunication(uint8_t flags);
extern void sendPacket(unsigned char cmd, unsigned char *payload, uint8_t payloadSize);
extern void sendAck(unsigned char cmd);
extern void sendNack(unsigned char cmd);
extern void sendMessageRequest(void);
extern void sendDateTimeRequest(void);

#endif
//***********************************************************************************************************************
