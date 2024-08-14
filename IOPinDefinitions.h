//***********************************************************************************************************************
//                              Estante Irrigada - Definição de pinos
//***********************************************************************************************************************
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h>
//=======================================================================================================================
// Definições de pinos gerais
//=======================================================================================================================
#define LED             PORTBbits.RB4
#define LED_TRIS        TRISBbits.TRISB4
#define SENSOR_EN       PORTAbits.RA6
#define SENSOR_EN_TRIS  TRISAbits.TRISA6
#define LORA_RST        PORTAbits.RA4
#define LORA_RST_TRIS   TRISAbits.TRISA4

//=======================================================================================================================
// Definições de pinos de controle das válvulas
//=======================================================================================================================
#define VALVULA0        PORTBbits.RB14
#define VALVULA0_TRIS   TRISBbits.TRISB14
#define VALVULA1        PORTBbits.RB12
#define VALVULA1_TRIS   TRISBbits.TRISB12
#define VALVULA2        PORTAbits.RA7
#define VALVULA2_TRIS   TRISAbits.TRISA7
#define VALVULA3        PORTBbits.RB9
#define VALVULA3_TRIS   TRISBbits.TRISB9
#define VALVULA4        PORTBbits.RB8
#define VALVULA4_TRIS   TRISBbits.TRISB8
#define VALVULA5        PORTBbits.RB7
#define VALVULA5_TRIS   TRISBbits.TRISB7

//=======================================================================================================================
// Definições de pinos leitura dos sensores
//=======================================================================================================================
#define SENSOR0         PORTAbits.RA0
#define SENSOR0_TRIS    TRISAbits.TRISA0
#define SENSOR0_ADCON   AD1PCFGbits.PCFG0
#define SENSOR1         PORTAbits.RA1
#define SENSOR1_TRIS    TRISAbits.TRISA1
#define SENSOR1_ADCON   AD1PCFGbits.PCFG1
#define SENSOR2         PORTBbits.RB0
#define SENSOR2_TRIS    TRISBbits.TRISB0
#define SENSOR2_ADCON   AD1PCFGbits.PCFG2
#define SENSOR3         PORTBbits.RB1
#define SENSOR3_TRIS    TRISBbits.TRISB1
#define SENSOR3_ADCON   AD1PCFGbits.PCFG3
#define SENSOR4         PORTBbits.RB2
#define SENSOR4_TRIS    TRISBbits.TRISB2
#define SENSOR4_ADCON   AD1PCFGbits.PCFG4
#define SENSOR5         PORTBbits.RB3
#define SENSOR5_TRIS    TRISBbits.TRISB3
#define SENSOR5_ADCON   AD1PCFGbits.PCFG5

#endif	/* XC_HEADER_TEMPLATE_H */

