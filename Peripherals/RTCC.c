//***********************************************************************************************************************
//                                         Módulo de RTCC
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
// Variáveis privadas do módulo
//***********************************************************************************************************************
static void (*AlarmInterruptHandler)(void) = NULL;

//***********************************************************************************************************************
// Interrupções
//***********************************************************************************************************************
//-----------------------------------------------------------------------------------------------------------------------
// Interrupção do timer 1
// Descrição: Sincronização de tempo das várias funções internas.
//-----------------------------------------------------------------------------------------------------------------------
void _ISR __attribute__((no_auto_psv)) _RTCCInterrupt(void)
{
    if(AlarmInterruptHandler != NULL)
        AlarmInterruptHandler();
 
    _RTCIF = 0;   // Limpa a flag de interrupção.
}
//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicialização do módulo de RTCC
//=======================================================================================================================
void initRTCC(uint8_t alarmFreq)
{
    //_RTCCMD = 0; // Habilita o módulo de RTCC, se este foi completamente desabilitado
    unlockRTCC();
    
    // Configuração do RTCC
    RCFGCALbits.RTCOE = 0;          // Desabilita saída do RTCC
    RCFGCALbits.CAL = 0;            // Sem ajustes ao horário do RTC
    
    // Configuração dos alarmes
    ALCFGRPT = 0;                   // Desabilita tudo antes de iniciar
    ALCFGRPTbits.AMASK = alarmFreq; // A princípio, 1 alarme a cada 10 segundos
    ALCFGRPTbits.CHIME = 1;         // Habilita a repetição de alarmes
    ALCFGRPTbits.ARPT = 1;
    ALCFGRPTbits.ALRMEN = 1;        // Alarm habilitado

    IFS3bits.RTCIF = 0;
    IEC3bits.RTCIE = 1;
    IPC15bits.RTCIP = 4;            // Prioridade 4 para interrupções do RTCC
    
    RCFGCALbits.RTCEN = 1;          // Habilita o RTCC
    
    lockRTCC();  // Desabilita a escrita no RTCC
}

//=======================================================================================================================
// Escrita de Data/Hora
//=======================================================================================================================
void writeDateTime(DateTime_t *value)
{
    unlockRTCC();
    RCFGCALbits.RTCPTR = 0x03;  // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                // decrementado automaticamente.
    RTCVAL = value->w[0];       // Ano
    RTCVAL = value->w[1];       // Mês e dia
    RTCVAL = value->w[2];       // Hora e dia da semana
    RTCVAL = value->w[3];       // Minuto e segundos
    lockRTCC();
}

//=======================================================================================================================
// Leitura de Data/Hora
//=======================================================================================================================
void readDateTime(DateTime_t *value)
{
    RCFGCALbits.RTCPTR = 0x03;  // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                // decrementado automaticamente.
    value->w[0] = RTCVAL;       // Ano
    value->w[1] = RTCVAL;       // Mês e dia
    value->w[2] = RTCVAL;       // Hora e dia da semana
    value->w[3] = RTCVAL;       // Minuto e segundos
}

//=======================================================================================================================
// Escrita de Data/Hora
//=======================================================================================================================
void writeAlarmTime(DateTime_t *value)
{
    unlockRTCC();
    ALCFGRPTbits.ALRMPTR = 0x02;    // Inicia a leitura apontando para o registrador de ano. O ponteiro é
    ALCFGRPTbits.ALRMEN = 0;        // decrementado automaticamente.
    ALRMVAL = value->w[1];          // Mês e dia
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
    ALCFGRPTbits.ALRMPTR = 0x02;    // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                    // decrementado automaticamente.
    value->w[1] = ALRMVAL;          // Mês e dia
    value->w[2] = ALRMVAL;          // Hora e dia da semana
    value->w[3] = ALRMVAL;          // Minuto e segundos
}

//=======================================================================================================================
// Define a frequência do alarme
//=======================================================================================================================
void setAlarmFrequency(uint8_t alarmFreq)
{
    unlockRTCC();
    ALCFGRPTbits.AMASK = alarmFreq;
    lockRTCC();
}

//=======================================================================================================================
// Define a frequência do alarme
//=======================================================================================================================
uint8_t getAlarmFrequency(void)
{
    return ALCFGRPTbits.AMASK;
}

//=======================================================================================================================
// Definindo a função de tratamento de interrupção do RTC.
//=======================================================================================================================
void setAlarmInterruptHandler(void (*handler)(void))
{
    TurnRTCCInterruptOff();
    AlarmInterruptHandler = handler;
    TurnRTCCInterruptOn();
}

//***********************************************************************************************************************