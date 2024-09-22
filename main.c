//***********************************************************************************************************************
//                                 Estante Irrigada - Firmware de Controle
//
// Data:       16/08/2024
// Descrição:  Ponto de entrada do projeto
//***********************************************************************************************************************

// <editor-fold defaultstate="collapsed" desc="PIC24F16KA102 Configuration Bit Settings">
// PIC24F16KA102 Configuration Bit Settings
// 'C' source line config statements
// FBS
#pragma config BWRP = OFF               // Table Write Protect Boot (Boot segment may be written)
#pragma config BSS = OFF                // Boot segment Protect (No boot program Flash segment)

// FGS
#pragma config GWRP = OFF               // General Segment Code Flash Write Protection bit (General segment may be written)
#pragma config GCP = OFF                // General Segment Code Flash Code Protection bit (No protection)

// FOSCSEL
#pragma config FNOSC = PRIPLL           // Oscillator Select (Primary oscillator with PLL module (HS+PLL, EC+PLL))
#pragma config IESO = OFF               // Internal External Switch Over bit (Internal External Switchover mode disabled (Two-Speed Start-up disabled))

// FOSC
#pragma config POSCMOD = HS             // Primary Oscillator Configuration bits (HS oscillator mode selected)
#pragma config OSCIOFNC = ON            // CLKO Enable Configuration bit (CLKO output disabled; pin functions as port I/O)
#pragma config POSCFREQ = HS            // Primary Oscillator Frequency Range Configuration bits (Primary oscillator/external clock input frequency greater than 8 MHz)
#pragma config SOSCSEL = SOSCHP         // SOSC Power Selection Configuration bits (Secondary oscillator configured for high-power operation)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Both Clock Switching and Fail-safe Clock Monitor are disabled)

// FWDT
#pragma config WDTPS = PS32768          // Watchdog Timer Postscale Select bits (1:32,768)
#pragma config FWPSA = PR128            // WDT Prescaler (WDT prescaler ratio of 1:128)
#pragma config WINDIS = OFF             // Windowed Watchdog Timer Disable bit (Standard WDT selected; windowed WDT disabled)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))

// FPOR
#pragma config BOREN = BOR3             // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware; SBOREN bit disabled)
#pragma config PWRTEN = ON              // Power-up Timer Enable bit (PWRT enabled)
#pragma config I2C1SEL = PRI            // Alternate I2C1 Pin Mapping bit (Default location for SCL1/SDA1 pins)
#pragma config BORV = V18               // Brown-out Reset Voltage bits (Brown-out Reset set to lowest voltage (1.8V))
#pragma config MCLRE = ON               // MCLR Pin Enable bit (MCLR pin enabled; RA5 input pin disabled)

// FICD
#pragma config ICS = PGx3               // ICD Pin Placement Select bits (PGC3/PGD3 are used for programming and debugging the device)

// FDS
#pragma config DSWDTPS = DSWDTPSF       // Deep Sleep Watchdog Timer Postscale Select bits (1:2,147,483,648 (25.7 Days))
#pragma config DSWDTOSC = LPRC          // DSWDT Reference Clock Select bit (DSWDT uses LPRC as reference clock)
#pragma config RTCOSC = LPRC            // RTCC Reference Clock Select bit (RTCC uses SOSC as reference clock)
#pragma config DSBOREN = ON             // Deep Sleep Zero-Power BOR Enable bit (Deep Sleep BOR enabled in Deep Sleep)
#pragma config DSWDTEN = OFF            // Deep Sleep Watchdog Timer Enable bit (DSWDT disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
// </editor-fold>

#include <xc.h>
#include "Configuration/HardwareConfiguration.h"
#include "Peripherals/timers.h"
#include "Peripherals/ADC.h"
#include "Peripherals/EEPROM.h"
#include "Peripherals/SPI.h"
#include "Peripherals/LoRa.h"
#include "Peripherals/RTCC.h"
#include "Applications/sensorHandling.h"
#include "Applications/LoRaReception.h"
#include "Applications/mainApplication.h"

//***********************************************************************************************************************
// Variáveis globais
//***********************************************************************************************************************
// <editor-fold defaultstate="collapsed" desc="Project pin configuration table">
const IOPortSetup_t ioSetup[] =
{
    {.ioPin.ID = LED, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = LORA_RST, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = LORA_NSS, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_ON},
    {.ioPin.ID = SENSOR_EN, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = VALVULA0, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = VALVULA1, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = VALVULA2, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = VALVULA3, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = VALVULA4, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = VALVULA5, .direction = IO_OUTPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = SENSOR0, .direction = IO_INPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = SENSOR1, .direction = IO_INPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = SENSOR2, .direction = IO_INPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = SENSOR3, .direction = IO_INPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = SENSOR4, .direction = IO_INPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF},
    {.ioPin.ID = SENSOR5, .direction = IO_INPUT, .openDrain = IO_NORMAL_OUTPUT, .initialState = PIN_OFF}
};
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Project ADC Configuration table">
const ADCSetup_t adSetup[] =
{
    {.channel = SENSOR0_ADC, .adstate = PIN_ANALOG},
    {.channel = SENSOR1_ADC, .adstate = PIN_ANALOG},
    {.channel = SENSOR2_ADC, .adstate = PIN_ANALOG},
    {.channel = SENSOR3_ADC, .adstate = PIN_ANALOG},
    {.channel = SENSOR4_ADC, .adstate = PIN_ANALOG},
    {.channel = SENSOR5_ADC, .adstate = PIN_ANALOG}
};
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Sensor Configuration table">
controlConfig_t controlList[MAX_SENSORS] =
{
    {.operation = SENSOR_ENABLED, .sensorADC = ADC_0, .valvePin.ID = VALVULA0, .lastState = PIN_OFF, .minThreshold = 620, .maxThreshold = 860},
    {.operation = SENSOR_ENABLED, .sensorADC = ADC_1, .valvePin.ID = VALVULA1, .lastState = PIN_OFF, .minThreshold = 620, .maxThreshold = 860},
    {.operation = SENSOR_ENABLED, .sensorADC = ADC_2, .valvePin.ID = VALVULA2, .lastState = PIN_OFF, .minThreshold = 620, .maxThreshold = 860},
    {.operation = SENSOR_ENABLED, .sensorADC = ADC_3, .valvePin.ID = VALVULA3, .lastState = PIN_OFF, .minThreshold = 620, .maxThreshold = 860},
    {.operation = SENSOR_ENABLED, .sensorADC = ADC_4, .valvePin.ID = VALVULA4, .lastState = PIN_OFF, .minThreshold = 620, .maxThreshold = 860},
    {.operation = SENSOR_ENABLED, .sensorADC = ADC_5, .valvePin.ID = VALVULA5, .lastState = PIN_OFF, .minThreshold = 620, .maxThreshold = 860 }
};
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Non-volatile Configurations for saving">
typedef struct
{
    uint16_t        operation[MAX_SENSORS];
    uint16_t        minThreshold[MAX_SENSORS];
    uint16_t        maxThreshold[MAX_SENSORS];
} nonVolatileConfig_t;

nonVolatileConfig_t nonVolatileConfig = 
{
    .operation = {0, 0, 0, 0, 0, 0},
    .minThreshold = {620, 620, 620, 620, 620, 620},
    .maxThreshold = {860, 860, 860, 860, 860, 860}
};
// </editor-fold>

uint32_t applicationTimeOut = 0;
uint8_t setupTaks = 1;
uint8_t timeOutState = TIME_OUT_ENABLED;
uint8_t valveActivated = 0, readSensors = 0, requestCalendar = 0;
uint8_t requestMessages = 0, sendSamples = 0;

//***********************************************************************************************************************
// Funções privadas que não podem ser acessadas por aplicações-filho
//***********************************************************************************************************************
//=======================================================================================================================
// Configura a execução das tasks
//=======================================================================================================================
static void setupForTaskExecution(void)
{
    if(setupTaks)
    {
        DateTime_t now;
        readDateTime(&now);

        requestCalendar = !isRTCCUpdated();
        readSensors = valveActivated;
        if(!requestCalendar)
        {
            requestMessages = 1;
            
            if(bcdToInt(now.Time.seconds) < 10)
                sendSamples = 1;
        }
        
        setupTaks = 0;
    }    
}

//=======================================================================================================================
// Vetor de interrupção do alarme
//=======================================================================================================================
static void alarmHandler(void)
{
    setupTaks = 1;
}

//=======================================================================================================================
// Inicialização de pinos de IO
//=======================================================================================================================
void initIOPins(void)
{
    if(RCONbits.DPSLP)               // Setado se foi acordado de um Deep Sleep.
    {
        RCONbits.DPSLP = 0;
        DSCONbits.RELEASE = 0;       // Libera os pinos para seu estado anterior ao Deep Sleep
    }

    // Desliga todos os módulos para reduzir o consumo
    // Depois, liga-se apenas os necessários.
    PMD1 = 0xFFFF;
    PMD2 = 0xFFFF;
    PMD3 = 0xFFFF;
    PMD4 = 0xFFFF;
    
    PMD1bits.SPI1MD = 0;         // Habilita o módulo SPI para comunicação LoRa
    PMD1bits.ADC1MD = 0;         // Habilita o ADC
    PMD1bits.T1MD = 0;           // Habilita o Timer 1
    PMD3bits.RTCCMD = 0;         // Habilita o RTCC
    PMD4bits.EEMD = 0;           // Habilita a EEPROM
    
    // Configuração dos pinos usados no projeto
    setupPinList(ioSetup, sizeof(ioSetup)/sizeof(IOPortSetup_t));
    // Configuração de conversores analógico-digital
    setupADCPinState(ADC_ALL, PIN_DIGITAL);
    setupADCPinStateList(adSetup, sizeof(adSetup)/sizeof(ADCSetup_t));
}

//=======================================================================================================================
// Recupera as configurações do módulo
//=======================================================================================================================
void loadModuleConfiguration(void)
{
    loadFromEEPROM((uint8_t *)&nonVolatileConfig, 2, sizeof(nonVolatileConfig));
    for(uint8_t index = 0; index < MAX_SENSORS; index++)
    {
        controlList[index].operation = (uint8_t)nonVolatileConfig.operation[index];
        controlList[index].minThreshold = nonVolatileConfig.minThreshold[index];
        controlList[index].maxThreshold = nonVolatileConfig.maxThreshold[index];
        controlList[index].lastState = readPin(controlList[index].valvePin);
    }
}

//***********************************************************************************************************************
// Funções públicas que podem ser acessadas por aplicações-filho
//***********************************************************************************************************************
//=======================================================================================================================
// Salva as configurações do projeto
//=======================================================================================================================
uint8_t saveConfiguration(void)
{
    for(uint8_t index = 0; index < MAX_SENSORS; index++)
    {
        nonVolatileConfig.operation[index] = controlList[index].operation;
        nonVolatileConfig.minThreshold[index] = controlList[index].minThreshold;
        nonVolatileConfig.maxThreshold[index] = controlList[index].maxThreshold;
    }
    
    if(saveToEEPROM((uint8_t *)&nonVolatileConfig, 2, sizeof(nonVolatileConfig)) == sizeof(nonVolatileConfig))
        return 1;
    else
        return 0;
}

//=======================================================================================================================
// Coloca o dispositivo em modo Deep Sleep para consumo mínimo de energia
//=======================================================================================================================
void deepSleep(void)
{
    loraPowerDown();
    DSCONbits.DSEN = 1; // Define o modo Deep Sleep
    Sleep();
}

//=======================================================================================================================
// Força o setup das tarefas, para o caso de atualização de RTCC
//=======================================================================================================================
void forceTaskSetup(void)
{
    setupTaks = 1;
}

//=======================================================================================================================
// Reseta o timeout da aplicação. Quando chega ao final do timeout, a aplicação entra em modo Deep Sleep
//=======================================================================================================================
void resetTimeOut(void)
{
    applicationTimeOut = getTimerInterruptCount();
}

//=======================================================================================================================
// Reseta o timeout da aplicação. Quando chega ao final do timeout, a aplicação entra em modo Deep Sleep
//=======================================================================================================================
void setTimeOutState(uint8_t state)
{
    timeOutState = state;
}

//***********************************************************************************************************************
// Função principal
//***********************************************************************************************************************
int main(void) 
{
    uint8_t valveActivationLastState;
    
    // Inicialização do sistema
    initIOPins();
    initTimers();
    initADCs();
    
    initEEPROM((uint8_t *)&nonVolatileConfig, sizeof(nonVolatileConfig));
    loadModuleConfiguration();
    
    setAlarmInterruptHandler(alarmHandler);
    initRTCC();
    
    initSPI();
    initLoRa(LORA_RST, LORA_NSS);

    initTaskSensorHandling(LED, SENSOR_EN);
    
    setTimerState(TIMER_ON);
    
    // Loop Principal do sistema
    for(;;) 
    {
        valveActivationLastState = valveActivated;
        
        setupForTaskExecution();
        taskSensorHandling(&sendSamples, &readSensors, &valveActivated);
        taskLoRaReception(&requestCalendar, &requestMessages);
        
        // Sem válvulas ativas, o sistema pode operar no modo de power-down
        if(valveActivated == 0)
        {
            // Inicia o timeout assim que desativar todas as válvulas
            if(valveActivationLastState == 1)
                resetTimeOut();
        
            // O firmware deve entrar em modo Deep Sleep quando não houver válvulas ligadas. Isto porque
            // no Deep Sleep as portas do microcontrolador são desligadas.
            if(timeOutState == FORCE_TIMEOUT)
                deepSleep();
            else if(timeOutState == TIME_OUT_ENABLED)
            {
                if((getTimerInterruptCount() - applicationTimeOut) >= APPLICATION_TIME_OUT)
                    deepSleep();
            }
        }
    }
}

//***********************************************************************************************************************