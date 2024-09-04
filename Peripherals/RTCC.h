//***********************************************************************************************************************
//                              Estante Irrigada - RTCC
//***********************************************************************************************************************
#ifndef PERIPHERALS_RTCC
#define	PERIPHERALS_RTCC

#include <xc.h>

//***********************************************************************************************************************
// Defines
//***********************************************************************************************************************
//=======================================================================================================================
// Frequências de alarme disponíveis
//=======================================================================================================================
#define ALARM_EVERY_10_SECONDS      2
#define ALARM_EVERY_MINUTE          3
#define ALARM_EVERY_10_MINUTES      4
#define ALARM_EVERY_HOUR            5
#define ALARM_ONCE_A_DAY            6

//***********************************************************************************************************************
// Tipos de variáveis relacionadas ao módulo de ADC
//***********************************************************************************************************************
// Variável de data/hora
typedef union
{
    struct
    {
        uint8_t year;
        uint8_t reserved;
        uint8_t day;
        uint8_t month;
        uint8_t hours;
        uint8_t weekday;
        uint8_t seconds;
        uint8_t minutes;
    } Time;
    uint8_t  b[8];
    uint16_t w[4];
} DateTime_t;

//***********************************************************************************************************************
// Funções públicas do módulo
//***********************************************************************************************************************
extern void initRTCC(uint8_t alarmFreq);
extern void writeDateTime(DateTime_t *value);
extern void readDateTime(DateTime_t *value);
extern void writeAlarmTime(DateTime_t *value);
extern void readAlarmTime(DateTime_t *value);
extern void setAlarmFrequency(uint8_t alarmFreq);
extern uint8_t getAlarmFrequency(void);
extern void setAlarmInterruptHandler(void (*handler)(void));
extern void loraPowerDown(void);

#endif