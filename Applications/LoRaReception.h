//***********************************************************************************************************************
//                              Estante Irrigada - LoRa Reception
//***********************************************************************************************************************
#ifndef APPLICATION_LORA_RECEPTION
#define	APPLICATION_LORA_RECEPTION

#include <xc.h>

//=======================================================================================================================
// Comandos do m�dulo
//=======================================================================================================================
#define CMD_MESSAGE_ECHO         0x80
#define CMD_GET_DATETIME         0x81
#define CMD_SET_DATETIME         0x82
#define CMD_GET_SAMPLES          0x83
#define CMD_GET_CONTROL_CONFIG   0x84
#define CMD_SET_CONTROL_CONFIG   0x85
#define CMD_GET_ALARM_FREQUENCY  0x86
#define CMD_SET_ALARM_FREQUENCY  0x87
#define CMD_SAVE_CONFIG          0x88
#define CMD_POWER_DOWN           0x89
#define CMD_REQUEST_MESSAGE      0x90
#define CMD_HALT_TIMEOUT         0x91

//***********************************************************************************************************************
// Tipos de vari�veis relacionadas ao m�dulo de recep��o e transmiss�o LoRa
//***********************************************************************************************************************
//=======================================================================================================================
// Vari�vel de par�metros para o controle
//=======================================================================================================================
typedef struct
{
    uint8_t         index;
    uint8_t         operation;
    uint16_t        minThreshold;
    uint16_t        maxThreshold;
} CommandConfig_t;

//***********************************************************************************************************************
// Fun��es p�blicas do m�dulo
//***********************************************************************************************************************
extern void taskLoRaReception(void);
extern void sendPacket(unsigned char cmd, unsigned char *payload, uint8_t payloadSize);
extern void sendAck(unsigned char cmd);
extern void sendNack(unsigned char cmd);
extern void sendMessageRequest(void);

#endif
//***********************************************************************************************************************
