//***********************************************************************************************************************
//                                                IO PORTS
//
// Data:         16/08/2024
// Descrição:    Definições utilizadas para modularização de Portas de IO
// Dependências: Nenhuma
//***********************************************************************************************************************
#include <xc.h>
#include "IOPorts.h"

//***********************************************************************************************************************
// Funções privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Define a direção de uma porta de IO
//=======================================================================================================================
static void setPortDirection(uint8_t ioPort, uint16_t direction)
{
    if(ioPort == IO_PORTA)
        TRISA = direction;
    else if(ioPort == IO_PORTB)
        TRISB = direction;
}

//=======================================================================================================================
// Obtém a direção de uma porta de IO
//=======================================================================================================================
static uint16_t getPortDirection(uint8_t ioPort)
{
    if(ioPort == IO_PORTA)
        return TRISA;
    else if(ioPort == IO_PORTB)
        return TRISB;
    else
        return 0xFFFF;  // Para um valor de porta indefinido, retorna-se tudo como entrada, valor default de toda porta. 
}

//=======================================================================================================================
// Define para uma porta o estado Open Drain
//=======================================================================================================================
static void setPortOpenDrainState(uint8_t ioPort, uint16_t openDrainState)
{
    if(ioPort == IO_PORTA)
        ODCA = openDrainState;
    else if(ioPort == IO_PORTB)
        ODCB = openDrainState;
}

//=======================================================================================================================
// Obtém o estado Open Drain de uma porta
//=======================================================================================================================
static uint16_t getPortOpenDrainState(uint8_t ioPort)
{
    if(ioPort == IO_PORTA)
        return ODCA;
    else if(ioPort == IO_PORTB)
        return ODCB;
    else
        return 0x0000;  // Para um valor de porta indefinido, pino normal, valor default de toda porta. 
}

//=======================================================================================================================
// Atribui um valor para uma porta
//=======================================================================================================================
static void setPort(uint8_t ioPort, uint16_t value)
{
    if(ioPort == IO_PORTA)
        LATA = value;
    else if(ioPort == IO_PORTB)
        LATB = value;
}

//=======================================================================================================================
// Lê o valor de uma porta
//=======================================================================================================================
static uint16_t getPort(uint8_t ioPort)
{
    if(ioPort == IO_PORTA)
        return PORTA;
    else if(ioPort == IO_PORTB)
        return PORTB;
    else
        return 0x0000;  // Para um valor de porta indefinido, retorna-se tudo 0. 
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Define a direção de um pino de IO
//=======================================================================================================================
void setPinDirection(IOPort_t ioPin, uint8_t direction)
{
    uint16_t portValue;
    
    if(direction == IO_OUTPUT)
        portValue = (ioPin.Pin == ALL_PINS) ? 0x0000 : getPortDirection(ioPin.Port) & ~(1 << ioPin.Pin);
    else
        portValue = (ioPin.Pin == ALL_PINS) ? 0xFFFF : getPortDirection(ioPin.Port) | (1 << ioPin.Pin);
    
    setPortDirection(ioPin.Port, portValue);
}

//=======================================================================================================================
// Obtém direção de um pino de IO
//=======================================================================================================================
uint16_t getPinDirection(IOPort_t ioPin)
{
    if(ioPin.Pin == ALL_PINS)
        return(getPortDirection(ioPin.Port));
    else
        return((getPortDirection(ioPin.Port) & (1 << ioPin.Pin)) ? IO_INPUT : IO_OUTPUT);
}

//=======================================================================================================================
// Define se um pino é coletor aberto
//=======================================================================================================================
void setPinOpenDrainState(IOPort_t ioPin, uint8_t openDrainState)
{
    uint16_t portOpenDrainValue;
    
    if(openDrainState == IO_NORMAL_OUTPUT)
        portOpenDrainValue = (ioPin.Pin == ALL_PINS) ? 0x0000 : getPortOpenDrainState(ioPin.Port) & ~(1 << ioPin.Pin);
    else
        portOpenDrainValue = (ioPin.Pin == ALL_PINS) ? 0xFFFF : getPortOpenDrainState(ioPin.Port) | (1 << ioPin.Pin);
    
    setPortOpenDrainState(ioPin.Port, portOpenDrainValue);
}

//=======================================================================================================================
// Escreve um valor em um pino
//=======================================================================================================================
void writePin(IOPort_t ioPin, uint8_t value)
{
    uint16_t portValue;
    
    if(ioPin.ID != IO_UNDEFINED)
    {
        if(value)
            portValue = (ioPin.Pin == ALL_PINS) ? 0xFFFF : getPort(ioPin.Port) | (1 << ioPin.Pin);
        else
            portValue = (ioPin.Pin == ALL_PINS) ? 0x0000 : getPort(ioPin.Port) & ~(1 << ioPin.Pin);

        setPort(ioPin.Port, portValue);
    }
}

//=======================================================================================================================
// Inverte o valor de um pino
//=======================================================================================================================
void invertPin(IOPort_t ioPin)
{
    uint16_t portValue;
    
    if(ioPin.ID != IO_UNDEFINED)
    {
        portValue = getPort(ioPin.Port) ^ ((ioPin.Pin == ALL_PINS) ? 0xFFFF : (1 << ioPin.Pin));
        setPort(ioPin.Port, portValue);
    }
}

//=======================================================================================================================
// Lê o valor de um pino
//=======================================================================================================================
uint16_t readPin(IOPort_t ioPin)
{
    if(ioPin.ID != IO_UNDEFINED)
    {
        if(ioPin.Pin == ALL_PINS)
            return(getPort(ioPin.Port));
        else
            return((getPort(ioPin.Port) & (1 << ioPin.Pin)) != 0);
    }
    else
        return 0x0000;
}

//=======================================================================================================================
// Através de uma lista de configurações de pinos, atribui todas as configurações aos pinos selecionados
//=======================================================================================================================
void setupPinList(const IOPortSetup_t *list, uint8_t size)
{
    for(uint8_t index=0; index < size; index++)
    {
        setPinDirection(list[index].ioPin, list[index].direction);
        setPinOpenDrainState(list[index].ioPin, list[index].openDrain);
        writePin(list[index].ioPin, list[index].initialState);
    }
}

//***********************************************************************************************************************