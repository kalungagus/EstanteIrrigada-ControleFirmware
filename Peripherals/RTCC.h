//***********************************************************************************************************************
//                              Estante Irrigada - RTCC
//***********************************************************************************************************************
#ifndef PERIPHERALS_RTCC
#define	PERIPHERALS_RTCC

#include <xc.h>

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
extern void initRTCC(void);
extern void writeDateTime(DateTime_t *value);
extern void readDateTime(DateTime_t *value);
extern void writeAlarmTime(DateTime_t *value);
extern void readAlarmTime(DateTime_t *value);
extern void setAlarmInterruptHandler(void (*handler)(void));
extern void loraPowerDown(void);
extern uint8_t isRTCCUpdated(void);
extern uint16_t bcdToInt(uint8_t data);
extern uint8_t intToBcd(uint16_t data);

#endif