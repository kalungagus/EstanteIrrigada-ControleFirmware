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
    now.Time.seconds = intToBcd(0);
    now.Time.minutes = intToBcd(0);
    writeAlarmTime(&now);
}

//-----------------------------------------------------------------------------------------------------------------------
// Tarefa principal desta aplica��o, verificar os sensores ativos e atuar nas v�lvulas relacionadas.
//-----------------------------------------------------------------------------------------------------------------------
void taskSensorHandling(uint8_t *sendSamples, uint8_t *readSensors, uint8_t *valveActivated)
{
    if((*sendSamples != 0) || (*readSensors != 0))
    {
        writePin(ioSensorProcessing, PIN_ON);          // Sinaliza verifica��o de sensores
        readDateTime(&actualSampling.instant);         // L� data/hora para os registros

        // Leitura dos sensores. A leitura � feita para todos os sensores, antes do processamento,
        // para manter a fonte dos sensores ligada o menor tempo poss�vel.
        setSensorSourceState(1);    // Liga a fonte dos sensores
        for(int8_t index = 0; index < 6; index++)
            actualSampling.value[index] = (controlList[index].operation != CONTROL_DISABLED) ? getADCSample(controlList[index].sensorADC) : 0x0000;
        setSensorSourceState(0);    // Desliga a fonte dos sensores

        *valveActivated = 0;
        
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
                
                // Uma v�lvula foi ativada, sinaliza isto para a aplica��o.
                if(controlList[index].lastState == PIN_ON)
                {
                    *valveActivated = 1;
                }
            }
            else if(controlList[index].operation == FORCE_VALVE_ON)
            {
                setValveState(controlList[index].valvePin, PIN_ON);
                actualSampling.state[index] = PIN_ON;
                controlList[index].lastState = PIN_ON;
                *valveActivated = 1;
            }
            else
            {
                setValveState(controlList[index].valvePin, PIN_OFF);
                actualSampling.state[index] = PIN_OFF;
                controlList[index].lastState = PIN_OFF;
            }
        }

        // Envia um pacote de dados de amostras quando for requerido
        if(*sendSamples != 0)
            sendPacket(BROAD_COMMAND | CMD_SEND_SAMPLES, ((unsigned char *)&actualSampling), sizeof(Sample_t));
 
        writePin(ioSensorProcessing, PIN_OFF);   // Finaliza a verifica��o de sensores
        *sendSamples = 0;
        *readSensors = 0;
    }
}

//***********************************************************************************************************************
