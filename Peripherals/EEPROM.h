//***********************************************************************************************************************
//                              Estante Irrigada - EEPROM
//***********************************************************************************************************************
#ifndef PERIPHERALS_EEPROM
#define	PERIPHERALS_EEPROM

//=======================================================================================================================
// Funções do módulo
//=======================================================================================================================
extern uint8_t eepromBulkErase(void);
extern uint8_t eepromWriteWord(uint16_t index, uint16_t data);
extern uint16_t eepromReadWord(uint16_t index);
extern uint8_t eepromEraseWord(uint16_t index);
extern uint8_t saveToEEPROM(uint8_t *data, uint16_t address, uint16_t size);
extern uint8_t loadFromEEPROM(uint8_t *data, uint16_t address, uint16_t size);
extern void initEEPROM(uint8_t *defaultConfigData, uint16_t defaultConfigSize);
#endif