//***********************************************************************************************************************
//                                         Módulo de SPI
//***********************************************************************************************************************
#include "../Configuration/HardwareConfiguration.h"
#include <xc.h>

//***********************************************************************************************************************
// Funções públicas do módulo
//***********************************************************************************************************************
//=======================================================================================================================
// Transmissão SPI
//=======================================================================================================================
uint8_t SPITransfer(uint8_t data)
{
    SPI1BUF = data;
    while(SPI1STATbits.SPITBF);   // Aguarda Transmissão
    while(!SPI1STATbits.SPIRBF);  // Aguarda Recepção
    return(SPI1BUF);
}

//=======================================================================================================================
// Comunicação com o módulo LoRa
//=======================================================================================================================
void initSPI(void)
{
    SPI1CON1bits.CKP = 0;           // O estado Idle do Clock é nível 0
    SPI1CON1bits.CKE = 1;           // Dado é colocado no barramento na borda de subida
    SPI1CON1bits.SMP = 0;           // Dado de entrada é amostrado no meio do tempo de transmissão
    SPI1CON1bits.SPRE = 6;          // A velocidade máxima de transmissão SPI do LoRa é 10MHz
    SPI1CON1bits.PPRE = 3;          // Definindo então 2:1 e 1:1, para uma velocidade de 8MHz
    SPI1CON1bits.MSTEN = 1;         // Master mode
    SPI1CON2 = 0x0000;
    SPI1STATbits.SPIROV = 0;        // Sem Overflow
    SPI1STATbits.SPIEN = 1;         // Habilita SPI
}

//***********************************************************************************************************************