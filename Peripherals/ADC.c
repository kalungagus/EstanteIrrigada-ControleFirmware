//***********************************************************************************************************************
//                                         Módulo de ADC
//***********************************************************************************************************************
#include "../Configuration/HardwareConfiguration.h"
#include <xc.h>
#include "ADC.h"
#include <libpic30.h>
#include <p24F16KA102.h>

//***********************************************************************************************************************
// Definições internas
//***********************************************************************************************************************
#define NUM_OF_ADCS     9

//***********************************************************************************************************************
// Variáveis privadas do módulo
//***********************************************************************************************************************
static const uint8_t adcList[NUM_OF_ADCS] = {ADC_0, ADC_1, ADC_2, ADC_3, ADC_4, ADC_5, ADC_10, ADC_11, ADC_12};
static uint16_t calibrationValue[NUM_OF_ADCS];

//***********************************************************************************************************************
// Interrupções
//***********************************************************************************************************************
//=======================================================================================================================
// Interrupção do ADC
// Descrição: O vetor de interrupção do ADC pode ser usado para executar algum código após a conversão
//            Ainda não está implementado o chamado a uma função de callback.
//=======================================================================================================================
void _ISR __attribute__((no_auto_psv)) _ADC1Interrupt(void)
{
    _AD1IF = 0;
}

//***********************************************************************************************************************
// Funções privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Seleciona um canal de ADC
//=======================================================================================================================
static void selectADChannel(adcChannel_t channel)
{
    if(channel != ADC_ALL)
    {
        __delay32(3);       // Espera 0.5 TAD para a troca. 
        AD1CHS = channel & 0x000F;
    }
}

//=======================================================================================================================
// Define o valor padrão de calibração de um ADC
//=======================================================================================================================
static void setADCCalibrationValue(adcChannel_t channel, uint16_t value)
{
    if(channel <= ADC_5)
        calibrationValue[channel] = value;
    else if((channel >= ADC_10) && (channel <= ADC_12))
        calibrationValue[channel-4] = value;
}

//=======================================================================================================================
// Lê o valor padrão de calibração de um ADC
//=======================================================================================================================
static uint16_t getADCCalibrationValue(adcChannel_t channel)
{
    uint16_t value = 0;
            
    if(channel <= ADC_5)
        value = calibrationValue[channel];
    else if((channel >= ADC_10) && (channel <= ADC_12))
        value = calibrationValue[channel-4];
    
    return value;
}

//=======================================================================================================================
// Calibra os ADCs
//=======================================================================================================================
static void calibrateADCs(void)
{
    AD1CON2bits.OFFCAL = 1;  // Inicializa o modo de calibração
    for(uint8_t index = 0; index <= NUM_OF_ADCS; index++)
        setADCCalibrationValue(adcList[index], getADCSample(adcList[index]));
    AD1CON2bits.OFFCAL = 0;  // Finaliza o modo de calibração
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicialização do módulo principal de ADC
//=======================================================================================================================
void initADCs(void)
{
    AD1CON1 = 0x0000;       // SAMP bit = 0 indica fim da amostragem e início da conversão, mas aqui o módulo não 
                            // está ligado ainda
    AD1CHS = 0x0000;        // Selecionando canal 0
    AD1CSSL = 0;            // Não haverá varredura na leitura das portas analógicas.
    AD1CON3 = 0x0001;       // Manual Sample, Tad = 2Tcy
    AD1CON2 = 0x0000;       // Usando as referências de tensão analógica internas. Não serão usadas interrupções aqui.
    AD1CON1bits.ADON = 1;   // Liga o ADC
    
    calibrateADCs();
}

//=======================================================================================================================
// Define um pino como analógico ou digital
//=======================================================================================================================
void setupADCPinState(adcChannel_t channel, uint8_t state)
{
    uint16_t portADValue;
    
    if(state == PIN_DIGITAL)
        portADValue = (channel == ADC_ALL) ? 0xFFFF : (AD1PCFG | (1 << channel));
    else
        portADValue = (channel == ADC_ALL) ? 0x0000 : (AD1PCFG & ~(1 << channel));
        
    AD1PCFG = portADValue;
}

//=======================================================================================================================
// Processa uma lista de configurações de ADC
//=======================================================================================================================
void setupADCPinStateList(const ADCSetup_t *list, uint8_t size)
{
    for(uint8_t index=0; index < size; index++)
        setupADCPinState(list[index].channel, list[index].adstate);
}

//=======================================================================================================================
// Obtém uma amostra do ADC
//=======================================================================================================================
uint16_t getADCSample(adcChannel_t channel)
{
    selectADChannel(channel);
    AD1CON1bits.SAMP = 1;       // Inicia a amostragem no conversor AD
    __delay32(6);               // Aguarda 3 TAD, que é o tempo máximo de amostragem segundo o datasheet
    AD1CON1bits.SAMP = 0;       // Termina a amostragem, entrando automaticamente no período de conversão
    while(!AD1CON1bits.DONE);   // Aguarda o fim da conversão;
    return(ADC1BUF0 - getADCCalibrationValue(channel));
}

//***********************************************************************************************************************