//***********************************************************************************************************************
//                                                IO PORTS
//
// Data:         16/08/2024
// Descrição:    Definições utilizadas para modularização de Portas de IO
// Dependências: Nenhuma
//***********************************************************************************************************************
#ifndef _IO_PORTS_
#define	_IO_PORTS_

#include <xc.h>

//***********************************************************************************************************************
// IDs
//***********************************************************************************************************************
//=======================================================================================================================
// ID das Portas
//=======================================================================================================================
#define IO_PORTA                0x00
#define IO_PORTB                0x01

//=======================================================================================================================
// ID dos Pinos
//=======================================================================================================================
#define IO_A0                   0x0000
#define IO_A1                   0x0001
#define IO_A2                   0x0002
#define IO_A3                   0x0003
#define IO_A4                   0x0004
#define IO_A5                   0x0005
#define IO_A6                   0x0006
#define IO_A7                   0x0007
#define IO_A8                   0x0008
#define IO_A9                   0x0009
#define IO_A10                  0x000A
#define IO_A11                  0x000B
#define IO_A12                  0x000C
#define IO_A13                  0x000D
#define IO_A14                  0x000E
#define IO_A15                  0x000F

#define IO_B0                   0x0100
#define IO_B1                   0x0101
#define IO_B2                   0x0102
#define IO_B3                   0x0103
#define IO_B4                   0x0104
#define IO_B5                   0x0105
#define IO_B6                   0x0106
#define IO_B7                   0x0107
#define IO_B8                   0x0108
#define IO_B9                   0x0109
#define IO_B10                  0x010A
#define IO_B11                  0x010B
#define IO_B12                  0x010C
#define IO_B13                  0x010D
#define IO_B14                  0x010E
#define IO_B15                  0x010F

#define IO_A_ALL                0x00FF
#define IO_B_ALL                0x01FF
#define ALL_PINS                0xFF

#define IO_UNDEFINED            0xFFFF

//***********************************************************************************************************************
// Outras definições
//***********************************************************************************************************************
#define IO_INPUT                1
#define IO_OUTPUT               0

#define IO_OPEN_DRAIN           1
#define IO_NORMAL_OUTPUT        0

#define PIN_OFF                 0
#define PIN_ON                  1

//***********************************************************************************************************************
// Tipos de variáveis relacionadas ao módulo de pinos de IO
//***********************************************************************************************************************
// Define um pino de IO
typedef union
{
    uint16_t ID;
    struct
    {
        uint8_t Pin;
        uint8_t Port;
    };
} IOPort_t;

// Define a configuração de um pino de IO
typedef struct
{
    IOPort_t    ioPin;
    uint8_t     direction;
    uint8_t     openDrain;
    uint8_t     initialState;
} IOPortSetup_t;

//***********************************************************************************************************************
// Funções públicas do módulo
//***********************************************************************************************************************
extern void setPinDirection(IOPort_t ioPin, uint8_t direction);
extern uint16_t getPinDirection(IOPort_t ioPin);
extern void setPinOpenDrainState(IOPort_t ioPin, uint8_t openDrainState);
extern void writePin(IOPort_t ioPin, uint8_t state);
extern void invertPin(IOPort_t ioPin);
extern uint16_t readPin(IOPort_t ioPin);
extern void setupPinList(const IOPortSetup_t *list, uint8_t size);

#endif /* I_IO_PORTS_ */
//***********************************************************************************************************************