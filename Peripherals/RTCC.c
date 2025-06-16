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
#define unlockRTCC()                do {                            \
                                        NVMKEY = 0x55;              \
                                        NVMKEY = 0xAA;              \
                                        RCFGCALbits.RTCWREN = 1;    \
                                    } while(0)
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
void initRTCC(void)
{
    //_RTCCMD = 0; // Habilita o módulo de RTCC, se este foi completamente desabilitado
    unlockRTCC();
    
    // Configuração do RTCC
    RCFGCALbits.RTCOE = 0;          // Desabilita saída do RTCC
    RCFGCALbits.CAL = 0;            // Sem ajustes ao horário do RTC
    
    // Configuração dos alarmes
    ALCFGRPT = 0;                   // Desabilita tudo antes de iniciar
    ALCFGRPTbits.AMASK = 2;         // 1 alarme a cada 10 segundos
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
// Alterado para tornar uma função atômica
//=======================================================================================================================
void writeDateTime(DateTime_t *value)
{
    __builtin_disi(0x3FFF);     // Desativa interrupções
    unlockRTCC();
    
    RCFGCALbits.RTCEN = 0;      // Desativa o RTC para evitar corrupção
    RCFGCALbits.RTCPTR = 0x03;  // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                // decrementado automaticamente.
    RTCVAL = value->w[0];       // Ano
    RTCVAL = value->w[1];       // Mês e dia
    RTCVAL = value->w[2];       // Hora e dia da semana
    RTCVAL = value->w[3];       // Minuto e segundos
    RCFGCALbits.RTCEN = 1;      // Religa o RTC
    lockRTCC();
    __builtin_disi(0x0000);     // Reativa interrupções
}

//=======================================================================================================================
// Leitura de Data/Hora
// Alterado para tornar uma função atômica
//=======================================================================================================================
void readDateTime(DateTime_t *value)
{
    __builtin_disi(0x3FFF);       // Desativa interrupções
    // Aguarda sincronização com o clock do RTC
    while (RCFGCALbits.RTCSYNC);  // Espera se estiver sincronizando
    
    RCFGCALbits.RTCPTR = 0x03;    // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                  // decrementado automaticamente.
    value->w[0] = RTCVAL;         // Ano
    value->w[1] = RTCVAL;         // Mês e dia
    value->w[2] = RTCVAL;         // Hora e dia da semana
    value->w[3] = RTCVAL;         // Minuto e segundos
    
    __builtin_disi(0x0000);       // Reativa interrupções
}

//=======================================================================================================================
// Escrita de Data/Hora
// Alterado para tornar a função atômica
//=======================================================================================================================
void writeAlarmTime(DateTime_t *value)
{
    __builtin_disi(0x3FFF);         // Desabilita interrupções
    unlockRTCC();

    ALCFGRPTbits.ALRMEN = 0;        // Desabilita o alarme       
    ALCFGRPTbits.ALRMPTR = 0x02;    // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                    // decrementado automaticamente.
    ALRMVAL = value->w[1];          // Mês e dia
    ALRMVAL = value->w[2];          // Hora e dia da semana
    ALRMVAL = value->w[3];          // Minuto e segundos
    ALCFGRPTbits.ALRMEN = 1;        // Reabilita o alarme
    lockRTCC();
    __builtin_disi(0x0000);         // Reabilita interrupções
}

//=======================================================================================================================
// Leitura de Data/Hora
// Alterado para tornar a função atômica
//=======================================================================================================================
void readAlarmTime(DateTime_t *value)
{
    __builtin_disi(0x3FFF);         // Desabilita interrupções
    ALCFGRPTbits.ALRMPTR = 0x02;    // Inicia a leitura apontando para o registrador de ano. O ponteiro é
                                    // decrementado automaticamente.
    value->w[1] = ALRMVAL;          // Mês e dia
    value->w[2] = ALRMVAL;          // Hora e dia da semana
    value->w[3] = ALRMVAL;          // Minuto e segundos
    __builtin_disi(0x0000);         // Reabilita interrupções
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

//=======================================================================================================================
// Define se o RTC está atualizado, baseando-se no ano do desenvolvimento do projeto. Se o ano do RTC for menor que
// o ano de desenvolvimento do projeto, define-se que este está desatualizado
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
// Conversão de bcd para número inteiro
//=======================================================================================================================
uint16_t bcdToInt(uint8_t data)
{
    return ((((data & 0xF0) >> 4) * 10) + (data & 0x0F));
}

//=======================================================================================================================
// Testa a validade da data e hora fornecida
//=======================================================================================================================
uint8_t isTimeDateValid(DateTime_t *value)
{
    uint8_t isValid = 1;
    
    isValid &= (uint8_t)(bcdToInt(value->Time.day) < 32);
    isValid &= (uint8_t)(bcdToInt(value->Time.hours) < 24);
    isValid &= (uint8_t)(bcdToInt(value->Time.minutes) < 60);
    isValid &= (uint8_t)(bcdToInt(value->Time.month) < 13);
    isValid &= (uint8_t)(bcdToInt(value->Time.seconds) < 60);
    isValid &= (uint8_t)(bcdToInt(value->Time.weekday) < 7);
    
    return isValid;
}

//=======================================================================================================================
// Conversão de número inteiro para bcd
//=======================================================================================================================
uint8_t intToBcd(uint16_t data)
{
    uint8_t value = ((((data & 0x00FF) / 10) << 4) + (data & 0x00FF) % 10);
    return value;
}

//***********************************************************************************************************************