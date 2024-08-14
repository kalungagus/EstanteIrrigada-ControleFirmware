//***********************************************************************************************************************
//                              Estante Irrigada - Firmware de Controle
//***********************************************************************************************************************
#include "fuse_bits.h"
#include <xc.h>
#include "IOPinDefinitions.h"
#define FCY     16000000
#include <libpic30.h>

//=======================================================================================================================
// Vari�veis globais
//=======================================================================================================================
unsigned long int ledTimeLimit = 1000;

//=======================================================================================================================
// Interrup��es
//=======================================================================================================================
//-----------------------------------------------------------------------------------------------------------------------
// Interrup��o do timer 1
// Descri��o: Sincroniza��o de tempo das v�rias fun��es internas.
//-----------------------------------------------------------------------------------------------------------------------
void _ISR __attribute__((no_auto_psv)) _T1Interrupt(void)
{
    static unsigned long int counter = 0;
    
    counter++;
    if(counter == ledTimeLimit)
    {
        counter = 0;
        LED = ~LED;
    }
    // clear this interrupt condition
    _T1IF = 0;
}

//-----------------------------------------------------------------------------------------------------------------------
// Interrup��o do ADC
// Descri��o: Leitura de sensores
//-----------------------------------------------------------------------------------------------------------------------
void _ISR __attribute__((no_auto_psv)) _ADC1Interrupt(void)
{
    _AD1IF = 0;
}

//=======================================================================================================================
// Fun��es
//=======================================================================================================================
//-----------------------------------------------------------------------------------------------------------------------
// Inicializa��o de pinos de IO
//-----------------------------------------------------------------------------------------------------------------------
void initIOPins(void)
{
    // Nenhuma porta ser� usada como Open-drain
    ODCA = 0x0000;
    ODCB = 0x0000;
    
    // Configura��o de portas de sa�da.
    LED_TRIS = 0;
    VALVULA0_TRIS = VALVULA1_TRIS = VALVULA2_TRIS = VALVULA3_TRIS = VALVULA4_TRIS = VALVULA5_TRIS = 0;
    
    LED = 0;
    VALVULA0 = VALVULA1 = VALVULA2 = VALVULA3 = VALVULA4 = VALVULA5 = 0;

    // Configura��o de portas de entrada
    SENSOR0 = SENSOR1 = SENSOR2 = SENSOR3 = SENSOR4 = SENSOR5 = 1;
    
    // Configura��o do conversor Anal�gico-Digital
    AD1PCFG = 0xFFFF;
    SENSOR0_ADCON = SENSOR1_ADCON = SENSOR2_ADCON = SENSOR3_ADCON = SENSOR4_ADCON = SENSOR5_ADCON = 0;
}

//-----------------------------------------------------------------------------------------------------------------------
// Inicializa��o de Timers
//-----------------------------------------------------------------------------------------------------------------------
void initTimers(void)
{
    T1CON = 0x0000;
    TMR1 = 0x0000;
    PR1 = 0x07D0;       // Valor para contagem de 1ms
    T1CON = 0x8010;     // Timer 1 On, Prescaler 1:8, clock interno = 500ns de per�odo para 32MHz
    _T1IE = 1;          // Habilita interrup��o do Timer 1
    _T1IP = 0x001;      // Prioridade mais baixa para a interrup��o do Timer 1
}

//-----------------------------------------------------------------------------------------------------------------------
// Inicializa��o dos ADCs
//-----------------------------------------------------------------------------------------------------------------------
void initADC(void)
{
    AD1CON1 = 0x0000;       // SAMP bit = 0 indica fim da amostragem e in�cio da convers�o, mas aqui o m�dulo n�o 
                            // est� ligado ainda
    AD1CHS = 0x0000;        // Selecionando canal 0
    AD1CSSL = 0;            // N�o haver� varredura na leitura das portas anal�gicas.
    AD1CON3 = 0x0002;       // Manual Sample, Tad = 3Tcy
    AD1CON2 = 0;
    AD1CON1bits.ADON = 1;   // Liga o ADC
}

//-----------------------------------------------------------------------------------------------------------------------
// Obt�m uma amostra do ADC
//-----------------------------------------------------------------------------------------------------------------------
uint16_t getADCSample(void)
{
    AD1CON1bits.SAMP = 1;       // Inicia a amostragem no conversor AD
    __delay_us(1);              // Aguarda o tempo de amostragem
    AD1CON1bits.SAMP = 0;       // Termina a amostragem, entrando automaticamente no per�odo de convers�o
    while(!AD1CON1bits.DONE);   // Aguarda o fim da convers�o;
    return(ADC1BUF0);
}

int main(void) 
{
    uint16_t ADCValue;
    
    initIOPins();
    initTimers();
    initADC();
    
    for(;;) 
    {
        ADCValue = getADCSample();
        if(ADCValue > 800)
        {
            VALVULA0 = 1;
            __delay_us(1);
        }
        else
            VALVULA0 = 0;
    }
}
