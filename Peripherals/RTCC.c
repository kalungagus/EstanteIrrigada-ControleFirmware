//***********************************************************************************************************************
//                                         M�dulo de RTCC
//***********************************************************************************************************************
#include "../Configuration/HardwareConfiguration.h"
#include "IOPorts.h"
#include "RTCC.h"
#include <xc.h>
#include <stddef.h>

//***********************************************************************************************************************
// Macros
//***********************************************************************************************************************
#define unlockRTCC()                {                               \
                                        NVMKEY = 0x55;              \
                                        NVMKEY = 0xAA;              \
                                        RCFGCALbits.RTCWREN = 1;    \
                                    }
#define lockRTCC()                  RCFGCALbits.RTCWREN = 0
#define TurnRTCCInterruptOff()      _RTCIE = 0;
#define TurnRTCCInterruptOn()       _RTCIE = 1;

//***********************************************************************************************************************
// Vari�veis privadas do m�dulo
//***********************************************************************************************************************
static void (*AlarmInterruptHandler)(void) = NULL;

//***********************************************************************************************************************
// Interrup��es
//***********************************************************************************************************************
//-----------------------------------------------------------------------------------------------------------------------
// Interrup��o do timer 1
// Descri��o: Sincroniza��o de tempo das v�rias fun��es internas.
//-----------------------------------------------------------------------------------------------------------------------
void _ISR __attribute__((no_auto_psv)) _RTCCInterrupt(void)
{
    if(AlarmInterruptHandler != NULL)
        AlarmInterruptHandler();
 
    _RTCIF = 0;   // Limpa a flag de interrup��o.
}
//***********************************************************************************************************************
// Fun��es p�blicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicializa��o do m�dulo de RTCC
//=======================================================================================================================
void initRTCC(void)
{
    //_RTCCMD = 0; // Habilita o m�dulo de RTCC, se este foi completamente desabilitado
    unlockRTCC();
    
    // Configura��o do RTCC
    RCFGCALbits.RTCOE = 0;          // Desabilita sa�da do RTCC
    RCFGCALbits.CAL = 0;            // Sem ajustes ao hor�rio do RTC
    
    // Configura��o dos alarmes
    ALCFGRPT = 0;                   // Desabilita tudo antes de iniciar
    ALCFGRPTbits.AMASK = 2;         // 1 alarme a cada 10 segundos
    ALCFGRPTbits.CHIME = 1;         // Habilita a repeti��o de alarmes
    ALCFGRPTbits.ARPT = 1;
    ALCFGRPTbits.ALRMEN = 1;        // Alarm habilitado

    IFS3bits.RTCIF = 0;
    IEC3bits.RTCIE = 1;
    IPC15bits.RTCIP = 4;            // Prioridade 4 para interrup��es do RTCC
    
    RCFGCALbits.RTCEN = 1;          // Habilita o RTCC
    
    lockRTCC();  // Desabilita a escrita no RTCC
}

//=======================================================================================================================
// Escrita de Data/Hora
//=======================================================================================================================
void writeDateTime(DateTime_t *value)
{
    unlockRTCC();
    RCFGCALbits.RTCPTR = 0x03;  // Inicia a leitura apontando para o registrador de ano. O ponteiro �
                                // decrementado automaticamente.
    RTCVAL = value->w[0];       // Ano
    RTCVAL = value->w[1];       // M�s e dia
    RTCVAL = value->w[2];       // Hora e dia da semana
    RTCVAL = value->w[3];       // Minuto e segundos
    lockRTCC();
}

//=======================================================================================================================
// Leitura de Data/Hora
//=======================================================================================================================
void readDateTime(DateTime_t *value)
{
    RCFGCALbits.RTCPTR = 0x03;  // Inicia a leitura apontando para o registrador de ano. O ponteiro �
                                // decrementado automaticamente.
    value->w[0] = RTCVAL;       // Ano
    value->w[1] = RTCVAL;       // M�s e dia
    value->w[2] = RTCVAL;       // Hora e dia da semana
    value->w[3] = RTCVAL;       // Minuto e segundos
}

//=======================================================================================================================
// Escrita de Data/Hora
//=======================================================================================================================
void writeAlarmTime(DateTime_t *value)
{
    unlockRTCC();
    ALCFGRPTbits.ALRMPTR = 0x02;    // Inicia a leitura apontando para o registrador de ano. O ponteiro �
    ALCFGRPTbits.ALRMEN = 0;        // decrementado automaticamente.
    ALRMVAL = value->w[1];          // M�s e dia
    ALRMVAL = value->w[2];          // Hora e dia da semana
    ALRMVAL = value->w[3];          // Minuto e segundos
    ALCFGRPTbits.ALRMEN = 1;
    lockRTCC();
}

//=======================================================================================================================
// Leitura de Data/Hora
//=======================================================================================================================
void readAlarmTime(DateTime_t *value)
{
    ALCFGRPTbits.ALRMPTR = 0x02;    // Inicia a leitura apontando para o registrador de ano. O ponteiro �
                                    // decrementado automaticamente.
    value->w[1] = ALRMVAL;          // M�s e dia
    value->w[2] = ALRMVAL;          // Hora e dia da semana
    value->w[3] = ALRMVAL;          // Minuto e segundos
}

//=======================================================================================================================
// Definindo a fun��o de tratamento de interrup��o do RTC.
//=======================================================================================================================
void setAlarmInterruptHandler(void (*handler)(void))
{
    TurnRTCCInterruptOff();
    AlarmInterruptHandler = handler;
    TurnRTCCInterruptOn();
}

//=======================================================================================================================
// Define se o RTC est� atualizado, baseando-se no ano do desenvolvimento do projeto. Se o ano do RTC for menor que
// o ano de desenvolvimento do projeto, define-se que este est� desatualizado
//=======================================================================================================================
uint8_t isRTCCUpdated(void)
{
    DateTime_t now;
    
    readDateTime(&now);
    if(bcdToInt(now.Time.year) < 24)
        return 0;
    else
        return 1;
}

//=======================================================================================================================
// Convers�o de bcd para n�mero inteiro
//=======================================================================================================================
uint16_t bcdToInt(uint8_t data)
{
    return ((((data & 0xF0) >> 4) * 10) + (data & 0x0F));
}

//=======================================================================================================================
// Convers�o de n�mero inteiro para bcd
//=======================================================================================================================
uint8_t intToBcd(uint16_t data)
{
    uint8_t value = ((((data & 0x00FF) / 10) << 4) + (data & 0x00FF) % 10);
    return value;
}

//***********************************************************************************************************************