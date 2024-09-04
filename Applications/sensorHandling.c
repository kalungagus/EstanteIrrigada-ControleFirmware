//***********************************************************************************************************************
//                                         Sensor Handling
//***********************************************************************************************************************
#include "sensorHandling.h"
#include "../Configuration/HardwareConfiguration.h"
#include "../Peripherals/RTCC.h"
#include "LoRaReception.h"
#include <libpic30.h>
#include <string.h>

//=======================================================================================================================
// Propriedades da aplica��o pai que precisam ser acessadas neste m�dulo
//=======================================================================================================================
extern controlConfig_t controlList[6];

//=======================================================================================================================
// Vari�veis privadas do m�dulo
//=======================================================================================================================
static uint8_t checkSensors = 0;
static Sample_t actualSampling;
static IOPort_t ioSensorProcessing = {.ID = IO_UNDEFINED}, ioSensorEn = {.ID = IO_UNDEFINED};

//***********************************************************************************************************************
// Fun��es privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Define o estado da fonte de alimenta��o dos sensores
//=======================================================================================================================
static void setSensorSourceState(uint8_t state)
{
    if(state)
    {
        writePin(ioSensorEn, PIN_ON);
        __delay_ms(360);                   // A fonte leva 80ns para ligar. Isto d� 1,28 TCY.
                                           // No entanto, o MCP6N11 demora no m�ximo 360ms para ligar
                                           // sua sa�da, portanto � necess�rio aguardar este intervalo.
    }
    else
    {
        writePin(ioSensorEn, PIN_OFF);
        __delay_us(10);                   // A fonte leva 200ns para desligar. Isto d� 3,2 TCY.
                                          // No entanto, o MCP6N11 demora 10us para desligar suas sa�das.
    }
}

//=======================================================================================================================
// Converte leitura do ADC para tens�o
//=======================================================================================================================
/*static float convertValueToVoltage(uint16_t value)
{
    return ((3.3f/1024) * value);
}*/

//=======================================================================================================================
// Define o estado ligado/desligado de uma v�lvula
//=======================================================================================================================
static void setValveState(IOPort_t valvePin, uint8_t state)
{
    writePin(valvePin, state);
    __delay32(3);               // Com 10mA para o acionamento das v�lvulas, leva-se
                                // 90ns para o acionamento, que daria 1,44Tcy. Por seguran�a
                                // mant�m-se aqui 3Tcy para o acionamento, para n�o haver tamb�m
                                // m�ltiplos acionamentos.
                                // Limite m�ximo de corrente para todas as portas � de 200mA.
}

//=======================================================================================================================
// Vetor de interrup��o do alarme
//=======================================================================================================================
static void alarmHandler(void)
{
    setNewSensorReading();
}

//***********************************************************************************************************************
// Fun��es p�blicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicializa��o de vari�veis relacionadas com esta Tarefa
//=======================================================================================================================
void initTaskSensorHandling(uint16_t activityPinID, uint16_t enablePinID)
{
    DateTime_t now;
    
    ioSensorProcessing.ID = activityPinID;
    ioSensorEn.ID = enablePinID;
    memset(&actualSampling, 0, sizeof(Sample_t));
    readDateTime(&now);
    writeAlarmTime(&now);
    setAlarmInterruptHandler(alarmHandler);
}

//-----------------------------------------------------------------------------------------------------------------------
// Habilita��o de leitura de sensores
//-----------------------------------------------------------------------------------------------------------------------
void setNewSensorReading(void)
{
   checkSensors = 1; 
}

//-----------------------------------------------------------------------------------------------------------------------
// Tarefa principal desta aplica��o, verificar os sensores ativos e atuar nas v�lvulas relacionadas.
//-----------------------------------------------------------------------------------------------------------------------
void taskSensorHandling(void)
{
    if(checkSensors != 0)
    {
        writePin(ioSensorProcessing, PIN_ON);          // Sinaliza verifica��o de sensores
        readDateTime(&actualSampling.instant);         // L� data/hora para os registros
        
        // Leitura dos sensores. A leitura � feita para todos os sensores, antes do processamento,
        // para manter a fonte dos sensores ligada o menor tempo poss�vel.
        setSensorSourceState(1);    // Liga a fonte dos sensores
        for(int8_t index = 0; index < 6; index++)
            actualSampling.value[index] = (controlList[index].operation != CONTROL_DISABLED) ? getADCSample(controlList[index].sensorADC) : 0x0000;
        setSensorSourceState(0);    // Desliga a fonte dos sensores
        
        // Processamento das leituras, com os sensores desligados.
        for(int8_t index = 0; index < 6; index++)
        {
            if(controlList[index].operation == SENSOR_CONTROLS_VALVE)
            {
                if(controlList[index].lastState == PIN_ON && actualSampling.value[index] > controlList[index].maxThreshold)
                {
                    setValveState(controlList[index].valvePin, PIN_OFF);
                    actualSampling.state[index] = PIN_OFF;
                    controlList[index].lastState = PIN_OFF;
                }
                if(controlList[index].lastState == PIN_OFF && actualSampling.value[index] < controlList[index].minThreshold)
                {
                    setValveState(controlList[index].valvePin, PIN_ON);
                    actualSampling.state[index] = PIN_ON;
                    controlList[index].lastState = PIN_ON;
                }
            }
            else if(controlList[index].operation == FORCE_VALVE_ON)
            {
                setValveState(controlList[index].valvePin, PIN_ON);
                actualSampling.state[index] = PIN_ON;
                controlList[index].lastState = PIN_ON;
            }
            else if(controlList[index].operation == FORCE_VALVE_OFF)
            {
                setValveState(controlList[index].valvePin, PIN_OFF);
                actualSampling.state[index] = PIN_OFF;
                controlList[index].lastState = PIN_OFF;
            }
        }
        
        // Envia um pacote de dados de amostras.
        sendPacket(CMD_GET_SAMPLES, ((unsigned char *)&actualSampling), sizeof(Sample_t));
        
        writePin(ioSensorProcessing, PIN_OFF);   // Finaliza a verifica��o de sensores
        checkSensors = 0;
    }
}

//***********************************************************************************************************************
