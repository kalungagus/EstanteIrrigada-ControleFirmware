//***********************************************************************************************************************
//                                         M�dulo de SPI
//***********************************************************************************************************************
#include "../Configuration/HardwareConfiguration.h"
#include <xc.h>

//***********************************************************************************************************************
// Fun��es p�blicas do m�dulo
//***********************************************************************************************************************
//=======================================================================================================================
// Transmiss�o SPI
//=======================================================================================================================
uint8_t SPITransfer(uint8_t data)
{
    SPI1BUF = data;
    while(SPI1STATbits.SPITBF);   // Aguarda Transmiss�o
    while(!SPI1STATbits.SPIRBF);  // Aguarda Recep��o
    return(SPI1BUF);
}

//=======================================================================================================================
// Comunica��o com o m�dulo LoRa
//=======================================================================================================================
void initSPI(void)
{
    SPI1CON1bits.CKP = 0;           // O estado Idle do Clock � n�vel 0
    SPI1CON1bits.CKE = 1;           // Dado � colocado no barramento na borda de subida
    SPI1CON1bits.SMP = 0;           // Dado de entrada � amostrado no meio do tempo de transmiss�o
    SPI1CON1bits.SPRE = 6;          // A velocidade m�xima de transmiss�o SPI do LoRa � 10MHz
    SPI1CON1bits.PPRE = 3;          // Definindo ent�o 2:1 e 1:1, para uma velocidade de 8MHz
    SPI1CON1bits.MSTEN = 1;         // Master mode
    SPI1CON2 = 0x0000;
    SPI1STATbits.SPIROV = 0;        // Sem Overflow
    SPI1STATbits.SPIEN = 1;         // Habilita SPI
}

//***********************************************************************************************************************