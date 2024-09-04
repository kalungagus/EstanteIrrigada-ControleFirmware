//***********************************************************************************************************************
//                                         Módulo de EEPROM
//***********************************************************************************************************************
#include "../Configuration/HardwareConfiguration.h"
#include <xc.h>

//***********************************************************************************************************************
// Definições internas
//***********************************************************************************************************************
#define CONFIG_SAVED_ID           0x4353

//***********************************************************************************************************************
// Variáveis privadas do módulo
//***********************************************************************************************************************
static int __attribute__ ((space(eedata))) eeData = 0x1234;;
uint8_t teste[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t teste2[5];

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Apaga toda a EEPROM
//=======================================================================================================================
void eepromBulkErase(void)
{
    NVMCON = 0x4050;            // Configura NVMCOM para apagar toda a memória
    asm volatile ("disi #5");   // Desabilita interrupções por 5 instruções
    __builtin_write_NVM();      // Desbloqueia EEPROM para alterações e executa o comando
    while(_WR);                 // Aguarda o apagamento
}

void eepromWriteWord(uint16_t index, uint16_t data)
{
    uint16_t offset;
    
    NVMCON = 0x4004;                         // Configura NVMCON para escrever uma palavra
    TBLPAG = __builtin_tblpage(&eeData);     // Inicializa TBLPAG com o endeereço superior
    offset = __builtin_tbloffset(&eeData);   // Obtém o endereço inferior base
    offset += index * sizeof(uint16_t);      // Ajusta o offset para o endereço desejado
    __builtin_tblwtl(offset, data);          // Escreve dado no latch de escrita
    asm volatile ("disi #5");                // Desabilita interrupções por 5 instruções
    __builtin_write_NVM();                   // Desbloqueia EEPROM para alterações e executa o comando
    while(_WR);                              // Aguarda a escrita do dado
}

uint16_t eepromReadWord(uint16_t index)
{
    uint16_t offset;

    TBLPAG = __builtin_tblpage(&eeData);     // Inicializa TBLPAG com o endeereço superior
    offset = __builtin_tbloffset(&eeData);   // Obtém o endereço inferior base
    offset += index * sizeof(uint16_t);      // Ajusta o offset para o endereço desejado
    return __builtin_tblrdl(offset);         // Lê o dado no endereço desejado
}

void eepromEraseWord(uint16_t index)
{
    uint16_t offset;
    
    NVMCON = 0x4058;                         // Configura NVMCON para apagar uma palavra
    TBLPAG = __builtin_tblpage(&eeData);     // Inicializa TBLPAG com o endeereço superior
    offset = __builtin_tbloffset(&eeData);   // Obtém o endereço inferior base
    offset += index * sizeof(uint16_t);      // Ajusta o offset para o endereço desejado
    __builtin_tblwtl(offset, 0);             // Escreve 0 no latch de escrita para apagar
    asm volatile ("disi #5");                // Desabilita interrupções por 5 instruções
    __builtin_write_NVM();                   // Desbloqueia EEPROM para alterações e executa o comando
    while(_WR);                              // Aguarda o apagamento do dado
}

uint8_t saveToEEPROM(uint8_t *data, uint16_t address, uint16_t size)
{
    uint16_t wordToSave, addressToSave;
    uint8_t wordIndex;
    
    if(address > 256 || size > 256)
        return 0;
    
    if(address + size > 256)
        size = 256 - address;
    
    wordToSave = eepromReadWord(address >> 1);
    for(uint16_t index = 0; index < size; index++)
    {
        wordIndex = (address + index) & 0x01;
        ((uint8_t *)(&wordToSave))[wordIndex] = data[index];
        addressToSave = (address + index) >> 1;
        if(wordIndex)
        {
            eepromWriteWord(addressToSave, wordToSave);
            wordToSave = eepromReadWord((address + index + 1) >> 1);
        }
    }
    
    return(size);
}

uint8_t loadFromEEPROM(uint8_t *data, uint16_t address, uint16_t size)
{
    int16_t wordReaded, addressToRead;
    uint8_t wordIndex;
    
    if(address > 256 || size > 256)
        return 0;
    
    if(address + size > 256)
        size = 256 - address;
    
    for(uint16_t index = 0; index < size; index++)
    {
        addressToRead  = (address + index) >> 1;
        wordIndex = (address + index) & 0x01;
        wordReaded = eepromReadWord(addressToRead);
        data[index] = ((uint8_t *)&wordReaded)[wordIndex];
    }
    
    return(size);
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//-----------------------------------------------------------------------------------------------------------------------
// Salva configurações do sistema
//-----------------------------------------------------------------------------------------------------------------------
void initEEPROM(uint8_t *defaultConfigData, uint16_t defaultConfigSize)
{
    if(eepromReadWord(0) != CONFIG_SAVED_ID)
    {
        saveToEEPROM(defaultConfigData, 2, defaultConfigSize);        
        eepromWriteWord(0, CONFIG_SAVED_ID);
    }
}

//***********************************************************************************************************************