//***********************************************************************************************************************
//                              Estante Irrigada - ADC
//***********************************************************************************************************************
#ifndef PERIPHERALS_ADC
#define	PERIPHERALS_ADC

#include <xc.h>

//=======================================================================================================================
// ID dos ADCs utilizados
//=======================================================================================================================
#define ADC_AN0       0x00
#define ADC_AN1       0x01
#define ADC_AN2       0x02
#define ADC_AN3       0x03
#define ADC_AN4       0x04
#define ADC_AN5       0x05
#define ADC_AN10      0x0A
#define ADC_AN11      0x0B
#define ADC_AN12      0x0C

#define ADC_VBG_DIV2  0x0E  // Band gap dividido por 2
#define ADC_VBG       0x0F  // Band gap
#define ADC_AVDD      0x07  // Alimentação positiva analógica
#define ADC_AVSS      0x06  // Terra analógica
#define ADC_NONE      0x0D  // Nenhum canal conectado (input flutuante)
#define ADC_ALL       0xFF

//***********************************************************************************************************************
// Outras definições
//***********************************************************************************************************************
#define PIN_DIGITAL     1
#define PIN_ANALOG      0

//***********************************************************************************************************************
// Tipos de variáveis relacionadas ao módulo de ADC
//***********************************************************************************************************************
typedef uint8_t adcChannel_t;
typedef struct
{
    adcChannel_t    channel;
    uint8_t         adstate;
} ADCSetup_t;

//***********************************************************************************************************************
// Funções públicas do módulo
//***********************************************************************************************************************
extern void initADCs(void);
extern void setupADCPinState(adcChannel_t channel, uint8_t state);
extern void setupADCPinStateList(const ADCSetup_t *list, uint8_t size);
extern uint16_t getADCSample(adcChannel_t channel);

#endif