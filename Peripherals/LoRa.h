//***********************************************************************************************************************
//                              Estante Irrigada - LoRa
//***********************************************************************************************************************
#ifndef PERIPHERALS_LORA
#define	PERIPHERALS_LORA

//=======================================================================================================================
// Definições
//=======================================================================================================================
#define EXPLICIT_MODE              0x00
#define IMPLICIT_MODE              0x01

//=======================================================================================================================
// Funções públicas do módulo
//=======================================================================================================================
extern uint8_t initLoRa(uint16_t resetPinID, uint16_t NSSPinID);
extern uint8_t isLoRaTransmitting(void);
extern uint8_t beginLoRaPacket(uint8_t implicitHeader);
extern uint8_t loadBufferToLoRa(uint8_t *buffer, uint8_t size);
extern uint8_t writeByteToLora(uint8_t byte);
extern void    endLoRaPacket(void);
extern uint8_t checkLoRaReception(void);
extern uint8_t LoRaBytesAvailable(void);
extern uint8_t readByteFromLoRa(void);

#endif