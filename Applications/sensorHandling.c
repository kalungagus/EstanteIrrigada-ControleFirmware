//***********************************************************************************************************************
//                                         Sensor Handling
//***********************************************************************************************************************
#include "sensorHandling.h"
#include "../Configuration/HardwareConfiguration.h"
#include "../Peripherals/RTCC.h"
#include "LoRaReception.h"
#include <libpic30.h>
#include <string.h>
#include <stdlib.h>

//***********************************************************************************************************************
// Definição de tipos internos ao módulo
//***********************************************************************************************************************
typedef struct 
{
    uint16_t previousValues[HISTORY_DEPTH];
    uint8_t currentIndex;
} SensorHistory_t;

//***********************************************************************************************************************
// Propriedades do módulo
//***********************************************************************************************************************
//=======================================================================================================================
// Propriedades da aplicação pai que precisam ser acessadas neste módulo
//=======================================================================================================================
extern controlConfig_t controlList[6];

//=======================================================================================================================
// Variáveis privadas do módulo
//=======================================================================================================================
static Sample_t actualSampling;
static SensorHistory_t sensorHistory[MAX_SENSORS];
static IOPort_t ioSensorProcessing = {.ID = IO_UNDEFINED}, ioSensorEn = {.ID = IO_UNDEFINED};
static uint8_t valvesState = VALVES_OFF;

//***********************************************************************************************************************
// Funções privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Define o estado da fonte de alimentação dos sensores
//=======================================================================================================================
static void setSensorSourceState(uint8_t state)
{
    if(state)
    {
        writePin(ioSensorEn, PIN_ON);
        __delay_ms(360);                   // A fonte leva 80ns para ligar. Isto dá 1,28 TCY.
                                           // No entanto, o MCP6N11 demora no máximo 360ms para ligar
                                           // sua saída, portanto é necessário aguardar este intervalo.
    }
    else
    {
        writePin(ioSensorEn, PIN_OFF);
        __delay_us(10);                   // A fonte leva 200ns para desligar. Isto dá 3,2 TCY.
                                          // No entanto, o MCP6N11 demora 10us para desligar suas saídas.
    }
}

//=======================================================================================================================
// Define o estado ligado/desligado de uma válvula
//=======================================================================================================================
static void setValveState(IOPort_t valvePin, uint8_t state)
{
    writePin(valvePin, state);
    __delay32(3);               // Com 10mA para o acionamento das válvulas, leva-se
                                // 90ns para o acionamento, que daria 1,44Tcy. Por segurança
                                // mantém-se aqui 3Tcy para o acionamento, para não haver também
                                // múltiplos acionamentos.
                                // Limite máximo de corrente para todas as portas é de 200mA.
}

//=======================================================================================================================
// Atualiza o registro de leituras dos sensores para monitoramento da derivada do sinal de entrada
//=======================================================================================================================
void updateSensorHistory(uint8_t index, uint16_t newValue) 
{
    sensorHistory[index].currentIndex = (sensorHistory[index].currentIndex + 1) % HISTORY_DEPTH;
    sensorHistory[index].previousValues[sensorHistory[index].currentIndex] = newValue;
}

//=======================================================================================================================
// Calcula a derivada do sinal lido
//=======================================================================================================================
int16_t getSensorDerivative(uint8_t index) 
{
    uint16_t prev = sensorHistory[index].previousValues[(sensorHistory[index].currentIndex + 1) % HISTORY_DEPTH];
    uint16_t curr = sensorHistory[index].previousValues[sensorHistory[index].currentIndex];
    return (int16_t)(curr - prev); // pode ser positivo ou negativo
}

//=======================================================================================================================
// Processamento de controle automático de irrigação com derivada
//=======================================================================================================================
void sensorControlsValveWithDerivative(uint8_t index)
{
    if(controlList[index].lastState == PIN_ON)
    {
        updateSensorHistory(index, actualSampling.value[index]);
        
        int16_t delta = getSensorDerivative(index);
        if(actualSampling.value[index] > controlList[index].maxThreshold || abs(delta) <= DERIVATIVE_THRESHOLD)
        {
            setValveState(controlList[index].valvePin, PIN_OFF);
            actualSampling.state[index] = PIN_OFF;
            controlList[index].lastState = PIN_OFF;
        }
    }
    if(controlList[index].lastState == PIN_OFF && actualSampling.value[index] < controlList[index].minThreshold)
    {
        setValveState(controlList[index].valvePin, PIN_ON);
        actualSampling.state[index] = PIN_ON;
        controlList[index].lastState = PIN_ON;
        
        // Inicializa os valores do histórico com o threshold máximo, já que estamos abaixo do mínimo.
        // Assim a derivada calculada quando a válvula estiver ligada vai resultar em um número
        // grande e não vai desativar a válvula antes de atuar.
        for(uint8_t clearIndex = 0; clearIndex < HISTORY_DEPTH; clearIndex++)
            sensorHistory[index].previousValues[clearIndex] = controlList[index].maxThreshold;

        sensorHistory[index].currentIndex = 0;
    }
}

//=======================================================================================================================
// Processamento de controle automático de irrigação
//=======================================================================================================================
void sensorControlsValve(uint8_t index)
{
    if((controlList[index].lastState == PIN_ON) && actualSampling.value[index] > controlList[index].maxThreshold)
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
        
        // Mesmo que não seja usada a derivada aqui, estamos inicializando a estrutura para o caso
        // do usuário mudar de um para outro modo de controle.
        // Inicializa os valores do histórico com o threshold máximo, já que estamos abaixo do mínimo.
        // Assim a derivada calculada quando a válvula estiver ligada vai resultar em um número
        // grande e não vai desativar a válvula antes de atuar.
        for(uint8_t clearIndex = 0; clearIndex < HISTORY_DEPTH; clearIndex++)
            sensorHistory[index].previousValues[clearIndex] = controlList[index].maxThreshold;

        sensorHistory[index].currentIndex = 0;
    }
}
//=======================================================================================================================
// Válvula é forçada a ficar sempre ligada
//=======================================================================================================================
void forceValveOn(uint8_t index)
{
    setValveState(controlList[index].valvePin, PIN_ON);
    actualSampling.state[index] = PIN_ON;
    controlList[index].lastState = PIN_ON;
}

//=======================================================================================================================
// Válvula é forçada a ficar sempre desligada
//=======================================================================================================================
void forceValveOff(uint8_t index)
{
    setValveState(controlList[index].valvePin, PIN_OFF);
    actualSampling.state[index] = PIN_OFF;
    controlList[index].lastState = PIN_OFF;
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicialização de variáveis relacionadas com esta Tarefa
//=======================================================================================================================
void initTaskSensorHandling(uint16_t activityPinID, uint16_t enablePinID)
{
    DateTime_t now;
    
    ioSensorProcessing.ID = activityPinID;
    ioSensorEn.ID = enablePinID;
    memset(&actualSampling, 0, sizeof(Sample_t));
    memset(&sensorHistory, 0, sizeof(sensorHistory));
    now.Time.seconds = intToBcd(0);
    now.Time.minutes = intToBcd(0);
    writeAlarmTime(&now);
}

//-----------------------------------------------------------------------------------------------------------------------
// Verifica se há alguma válvula ligada
//-----------------------------------------------------------------------------------------------------------------------
uint8_t isAnyValveOn(void)
{
    return valvesState;
}

//-----------------------------------------------------------------------------------------------------------------------
// Tarefa principal desta aplicação, verificar os sensores ativos e atuar nas válvulas relacionadas.
//-----------------------------------------------------------------------------------------------------------------------
void taskSensorHandling(uint8_t sendSamples)
{
    writePin(ioSensorProcessing, PIN_ON);          // Sinaliza verificação de sensores
    readDateTime(&actualSampling.instant);         // Lê data/hora para os registros
    
    // Leitura dos sensores. A leitura é feita para todos os sensores, antes do processamento,
    // para manter a fonte dos sensores ligada o menor tempo possível.
    setSensorSourceState(1);    // Liga a fonte dos sensores
    actualSampling.vss = readAVssOffset();
    actualSampling.vdd = readAVddOffset();
    actualSampling.vbg = readAVbgOffset();
    for(int8_t index = 0; index < MAX_SENSORS; index++)
        actualSampling.value[index] = (controlList[index].operation != CONTROL_DISABLED) ? getADCSample(controlList[index].sensorADC) : 0x0000;
    setSensorSourceState(0);    // Desliga a fonte dos sensores

    valvesState = VALVES_OFF;

    // Processamento das leituras, com os sensores desligados.
    for(int8_t index = 0; index < MAX_SENSORS; index++)
    {
        switch(controlList[index].operation)
        {
            case SENSOR_CONTROLS_VALVE:
                sensorControlsValve(index);
                break;
            case FORCE_VALVE_ON:
                forceValveOn(index);
                break;
            case SENSOR_CONTROLS_DERIVATIVE:
                sensorControlsValveWithDerivative(index);
                break;
            default:
                forceValveOff(index);
                break;
        }
        
        // Uma válvula foi ativada, sinaliza isto para a aplicação.
        if(controlList[index].lastState == PIN_ON)
            valvesState = VALVES_ON;
    }

    // Envia um pacote de dados de amostras quando for requerido
    if(sendSamples != 0)
        startTransmission(BROAD_COMMAND | CMD_SEND_SAMPLES, ((unsigned char *)&actualSampling), sizeof(Sample_t));

    writePin(ioSensorProcessing, PIN_OFF);   // Finaliza a verificação de sensores
}

//***********************************************************************************************************************
