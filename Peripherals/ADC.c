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
static const uint8_t adcList[NUM_OF_ADCS] = {ADC_AN0, ADC_AN1, ADC_AN2, ADC_AN3, ADC_AN4, ADC_AN5, ADC_AN10, ADC_AN11, ADC_AN12};
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
static void selectADChannel(uint8_t channel)
{
    if(channel != ADC_ALL)
    {
        __delay32(6);              // Aguarda 1 TAD para troca segura do mux

        AD1CHSbits.CH0NA = 0;      // Usa VR- (normalmente AVSS) como referência negativa

        // Ajusta canal positivo
        AD1CHSbits.CH0SA = channel & 0x0F;
    }
}

//=======================================================================================================================
// Obtém o índice do canal de ADC
//=======================================================================================================================
static int8_t getCalibrationIndex(adcChannel_t channel)
{
    if(channel <= ADC_AN5)
        return channel;
    else if(channel >= ADC_AN10 && channel <= ADC_AN12)
        return channel - 4;
    else
        return -1; // Canal inválido
}

//=======================================================================================================================
// Define o valor padrão de calibração de um ADC
//=======================================================================================================================
static void setADCCalibrationValue(adcChannel_t channel, uint16_t value)
{
    int8_t index = getCalibrationIndex(channel);
    if(index >= 0)
        calibrationValue[index] = value;
}

//=======================================================================================================================
// Lê o valor padrão de calibração de um ADC
//=======================================================================================================================
static uint16_t getADCCalibrationValue(adcChannel_t channel)
{
    int8_t index = getCalibrationIndex(channel);
    return (index >= 0) ? calibrationValue[index] : 0;
}

//=======================================================================================================================
// Calibra os ADCs
//=======================================================================================================================
static void calibrateADCs(void)
{
    AD1CON2bits.OFFCAL = 1;  // Inicializa o modo de calibração
    
    for(uint8_t index = 0; index < NUM_OF_ADCS; index++)
    {
        uint8_t channel = adcList[index];
        uint16_t sample = getADCSample(channel);
        setADCCalibrationValue(channel, sample);
    }
    
    AD1CON2bits.OFFCAL = 0;  // Finaliza o modo de calibração
}

//=======================================================================================================================
// Leitura de valores de um canal qualquer do módulo
//=======================================================================================================================
static uint16_t getADCSampleRaw(adcChannel_t channel)
{
    if(channel == ADC_ALL)
        return 0;

    selectADChannel(channel);
    AD1CON1bits.SAMP = 1;      
    __delay32(6);              // espera ~3 TAD
    AD1CON1bits.SAMP = 0;      
    while(!AD1CON1bits.DONE);
    return ADC1BUF0 - getADCCalibrationValue(channel);
}

//=======================================================================================================================
// Leitura do valor de referência
//=======================================================================================================================
static uint16_t readAVssOffset(void)
{
    return getADCSampleRaw(ADC_AVSS);  // Canal AVss definido como 0x06
}


//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicialização do módulo principal de ADC
//=======================================================================================================================
void initADCs(void)
{
    // --- Configuração dos registradores de controle ---
    AD1CON1 = 0x0000;       // Modo manual
    AD1CON2 = 0x0000;       // Referências internas padrão, sem scan
    AD1CON3 = 0x0001;       // TAD = 2 × Tcy

    // --- Seleção inicial de canal e varredura ---
    AD1CHS  = 0x0000;       // Começa com AN0
    AD1CSSL = 0x0000;       // Sem CSSL (scan)

    // --- Habilita o ADC ---
    AD1CON1bits.ADON = 1;

    // --- Calibração dos canais ---
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
// Faz uma média das leituras, uma vez que todo ADC SAR é susceptível a ruídos e precisa que sua leitura seja
// filtrada.
//=======================================================================================================================
uint16_t getADCSample(adcChannel_t channel)
{
    if (channel == ADC_ALL)
        return 0;

    // Calcula o erro de offset, para o caso da referência de tensão sofrer variações na leitura
    // Considera também que estas variações sejam lentas o suficiente para não ser necessário
    // ler o valor da referência junto com toda leitura.
    uint16_t offsetError = readAVssOffset() + getADCCalibrationValue(channel);

    uint32_t readingTotal = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        uint16_t sample = getADCSampleRaw(channel);
        readingTotal += (sample > offsetError) ? sample - offsetError : 0;
    }

    return (readingTotal >> 3);
}

//***********************************************************************************************************************