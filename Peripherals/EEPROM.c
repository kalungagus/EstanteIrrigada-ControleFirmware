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
uint8_t eepromBulkErase(void)
{
    NVMCONbits.WRERR = 0;       // Limpa qualquer erro anterior
    
    NVMCON = 0x4050;            // Configura NVMCOM para apagar toda a memória
    asm volatile ("disi #5");   // Desabilita interrupções por 5 instruções
    __builtin_write_NVM();      // Desbloqueia EEPROM para alterações e executa o comando
    while(_WR);                 // Aguarda o apagamento
    
    return(!NVMCONbits.WRERR);
}

uint8_t eepromWriteWord(uint16_t index, uint16_t data)
{
    uint16_t offset;
    
    NVMCONbits.WRERR = 0;                    // Limpa qualquer erro anterior
    
    if (index >= EEPROM_WORD_COUNT)
        return 0;                            // Sinaliza erro
    
    NVMCON = 0x4004;                         // Configura NVMCON para escrever uma palavra
    TBLPAG = __builtin_tblpage(&eeData);     // Inicializa TBLPAG com o endeereço superior
    offset = __builtin_tbloffset(&eeData);   // Obtém o endereço inferior base
    offset += index * sizeof(uint16_t);      // Ajusta o offset para o endereço desejado
    __builtin_tblwtl(offset, data);          // Escreve dado no latch de escrita
    asm volatile ("disi #5");                // Desabilita interrupções por 5 instruções
    __builtin_write_NVM();                   // Desbloqueia EEPROM para alterações e executa o comando
    while(_WR);                              // Aguarda a escrita do dado
    
    return(!NVMCONbits.WRERR);
}

uint16_t eepromReadWord(uint16_t index)
{
    uint16_t offset;

    if (index >= EEPROM_WORD_COUNT)
        return 0xFFFF;                       // Valor para leituras inválidas
    
    TBLPAG = __builtin_tblpage(&eeData);     // Inicializa TBLPAG com o endeereço superior
    offset = __builtin_tbloffset(&eeData);   // Obtém o endereço inferior base
    offset += index * sizeof(uint16_t);      // Ajusta o offset para o endereço desejado
    return __builtin_tblrdl(offset);         // Lê o dado no endereço desejado
}

uint8_t eepromEraseWord(uint16_t index)
{
    uint16_t offset;
    
    NVMCONbits.WRERR = 0;                    // Limpa qualquer erro anterior
    
    if (index >= EEPROM_WORD_COUNT)
        return 0; 
    
    NVMCON = 0x4058;                         // Configura NVMCON para apagar uma palavra
    TBLPAG = __builtin_tblpage(&eeData);     // Inicializa TBLPAG com o endeereço superior
    offset = __builtin_tbloffset(&eeData);   // Obtém o endereço inferior base
    offset += index * sizeof(uint16_t);      // Ajusta o offset para o endereço desejado
    
    asm volatile ("disi #5");                // Desabilita interrupções por 5 instruções
    __builtin_write_NVM();                   // Desbloqueia EEPROM para alterações e executa o comando
    while(_WR);                              // Aguarda o apagamento do dado
    
    return(!NVMCONbits.WRERR);
}

uint8_t saveToEEPROM(uint8_t *data, uint16_t address, uint16_t size)
{
    uint16_t wordToSave, addressToSave;
    uint8_t wordIndex;
    
    if((address > (EEPROM_WORD_COUNT * 2)) || (size > (EEPROM_WORD_COUNT * 2)))
        return 0;
    
    if((address + size) > (EEPROM_WORD_COUNT * 2))
        size = (EEPROM_WORD_COUNT * 2) - address;
    
    wordToSave = eepromReadWord(address >> 1);
    for(uint16_t index = 0; index < size; index++)
    {
        wordIndex = (address + index) & 0x0001;  // 0 = byte baixo, 1 = byte alto
        ((uint8_t *)(&wordToSave))[wordIndex] = data[index];
        addressToSave = (address + index) >> 1;
        
        // Escreve sempre ao final de palavra (byte alto) ou se for o último byte (independente do byte)
        if(wordIndex || (index == (size - 1)))
        {
            if(eepromReadWord(addressToSave) != wordToSave)
            {
                if(!eepromWriteWord(addressToSave, wordToSave))
                    return index;
            }

            // Atualiza wordToSave para próxima palavra, exceto se for a última iteração
            if(index != size - 1)
                wordToSave = eepromReadWord((address + index + 1) >> 1);
        }
    }
    
    return(size);
}

uint8_t loadFromEEPROM(uint8_t *data, uint16_t address, uint16_t size)
{
    uint16_t wordReaded, addressToRead;
    uint8_t wordIndex;
    
    if((address > (EEPROM_WORD_COUNT * 2)) || (size > (EEPROM_WORD_COUNT * 2)))
        return 0;
    
    if((address + size) > (EEPROM_WORD_COUNT * 2))
        size = (EEPROM_WORD_COUNT * 2) - address;
    
    for(uint16_t index = 0; index < size; index++)
    {
        addressToRead  = (address + index) >> 1;
        wordIndex = (address + index) & 0x0001;
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