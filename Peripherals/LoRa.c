//***********************************************************************************************************************
//                                         Módulo LoRa
//***********************************************************************************************************************
#include "../Configuration/HardwareConfiguration.h"
#include "SPI.h"
#include "LoRa.h"
#include <xc.h>
#include <libpic30.h>

//***********************************************************************************************************************
// Definições internas
//***********************************************************************************************************************
//=======================================================================================================================
// Registradores do módulo LoRa
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0B
#define REG_LNA                  0x0C
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1A
#define REG_MODEM_CONFIG_1       0x1D
#define REG_MODEM_CONFIG_2       0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2A
#define REG_RSSI_WIDEBAND        0x2C
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3B
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4D

// Modos de operação
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06

// Máscaras de interrupção
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40

// PA config
#define PA_BOOST                 0x80
#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1

#define MAX_PKT_LENGTH           255

//***********************************************************************************************************************
// Variáveis privadas do módulo
//***********************************************************************************************************************
static IOPort_t ioLoRaReset = {.ID = IO_UNDEFINED}, ioLoRaNSS = {.ID = IO_UNDEFINED};

//***********************************************************************************************************************
// Funções privadas
//***********************************************************************************************************************
//=======================================================================================================================
// Faz a leitura/escrita de um registrador LoRa através da SPI
//=======================================================================================================================
static uint8_t LoRaTransfer(uint8_t address, uint8_t value)
{
    uint8_t response;
    
    writePin(ioLoRaNSS, PIN_OFF);
    SPITransfer(address);
    response = SPITransfer(value);
    writePin(ioLoRaNSS, PIN_ON);
    
    return(response);
}

//=======================================================================================================================
// Escreve em um registrador
//=======================================================================================================================
static void writeLoRaRegister(uint8_t address, uint8_t data)
{
    LoRaTransfer(address | 0x80, data);
}

//=======================================================================================================================
// Lê um registrador
//=======================================================================================================================
static uint8_t readLoRaRegister(uint8_t address)
{
    return LoRaTransfer(address & 0x7F, 0x00);
}

//=======================================================================================================================
// Define a frequência de operação
//=======================================================================================================================
static void setLoRaFrequency(long frequency)
{
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
    
    writeLoRaRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
    writeLoRaRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
    writeLoRaRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

//=======================================================================================================================
// Define o modo de operação do módulo LoRa
//=======================================================================================================================
static void setLoRaOpMode(uint8_t mode)
{
  writeLoRaRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | mode);
}
//=======================================================================================================================
// Lê um registrador
//=======================================================================================================================
static void setOCP(uint8_t mA)
{
    uint8_t ocpTrim = 27;

    if (mA <= 120) 
    {
        ocpTrim = (mA - 45) / 5;
    } 
    else if (mA <=240)
    {
        ocpTrim = (mA + 30) / 10;
    }

    writeLoRaRegister(REG_OCP, 0x20 | (0x1F & ocpTrim));
}

//=======================================================================================================================
// Define a potência dos amplificadores de transmissão
//=======================================================================================================================
static void setLoRaTxPower(int8_t level, uint8_t outputPin)
{
    if (PA_OUTPUT_RFO_PIN == outputPin)  // RFO
    {
        if (level < 0)
            level = 0;
        else if (level > 14)
            level = 14;

        writeLoRaRegister(REG_PA_CONFIG, 0x70 | level);
    }
    else // PA BOOST
    {
        if (level > 17) 
        {
            if (level > 20)
                level = 20;

            // Diminui 3 do nível para que a diferença 18 - 20 seja mapeada para 15 - 17
            level -= 3;

            // High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
            writeLoRaRegister(REG_PA_DAC, 0x87);
            setOCP(140);
        }
        else 
        {
            if (level < 2) 
                level = 2;

            // Valor padrão PA_HF/LF or +17dBm
            writeLoRaRegister(REG_PA_DAC, 0x84);
            setOCP(100);
        }

        writeLoRaRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
    }
}

//=======================================================================================================================
// Define o modo de transmissão de pacotes do módulo LoRa
//=======================================================================================================================
static void setLoRaPacketMode(uint8_t mode)
{
    if(mode == EXPLICIT_MODE)
        writeLoRaRegister(REG_MODEM_CONFIG_1, readLoRaRegister(REG_MODEM_CONFIG_1) & 0xFE);
    else
        writeLoRaRegister(REG_MODEM_CONFIG_1, readLoRaRegister(REG_MODEM_CONFIG_1) | 0x01);
}

//***********************************************************************************************************************
// Funções públicas
//***********************************************************************************************************************
//=======================================================================================================================
// Inicializa módulo LoRa
//=======================================================================================================================
uint8_t initLoRa(uint16_t resetPinID, uint16_t NSSPinID)
{
    uint8_t version;
    
    ioLoRaReset.ID = resetPinID;
    ioLoRaNSS.ID = NSSPinID;
    
    if(ioLoRaReset.ID == IO_UNDEFINED || ioLoRaNSS.ID == IO_UNDEFINED)
        return 0;
    
    writePin(ioLoRaReset, PIN_OFF);  // Reseta o módulo LoRa
    __delay_ms(10);
    writePin(ioLoRaReset, PIN_ON);  // Liga o módulo LoRa
    __delay_ms(10);                 // Tempo necessário para o módulo entrar em funcionamento.
    
    version = readLoRaRegister(REG_VERSION);
    if(version != 0x12)
        return 0;

    // O modo LoRa só pode ser setado quando o chip está em modo Sleep.
    // O modo Sleep também limpa todo o conteúdo dos buffers
    setLoRaOpMode(MODE_SLEEP);
    
    setLoRaFrequency(433E6);
    // Define os endereços base para os buffers
    writeLoRaRegister(REG_FIFO_TX_BASE_ADDR, 0);
    writeLoRaRegister(REG_FIFO_RX_BASE_ADDR, 0);

    // Liga o LNA boost
    writeLoRaRegister(REG_LNA, readLoRaRegister(REG_LNA) | 0x03);
  
    // Liga o controle automático de ganho
    writeLoRaRegister(REG_MODEM_CONFIG_3, 0x04);
    
    setLoRaTxPower(17, PA_OUTPUT_PA_BOOST_PIN);
    
    // Inicia o modo padrão de operação do módulo.
    setLoRaOpMode(MODE_STDBY);
    
    return 1;
}

//=======================================================================================================================
// Verifica se o módulo LoRa está transmitindo dados
//=======================================================================================================================
uint8_t isLoRaTransmitting(void)
{
  // Verifica se ainda está em modo de transmissão 
  if ((readLoRaRegister(REG_OP_MODE) & MODE_TX) == MODE_TX) 
    return 1;

  // Limpa o flag de interrupção, caso esteja setado.
  if (readLoRaRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) 
    writeLoRaRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

  return 0;
}

//=======================================================================================================================
// Inicializa o modo de transmissão LoRa, para que pacotes sejam carregados em sua memória.
//=======================================================================================================================
uint8_t beginLoRaPacket(uint8_t implicitHeader)
{
    if (isLoRaTransmitting())
        return 0;

    // Coloca o módulo em modo Standby para transmissão.
    setLoRaOpMode(MODE_STDBY);

    setLoRaPacketMode(implicitHeader);

    // Reseta os endereços de escrita e tamanho do payload
    writeLoRaRegister(REG_FIFO_ADDR_PTR, 0);
    writeLoRaRegister(REG_PAYLOAD_LENGTH, 0);

    return 1;
}

//=======================================================================================================================
// Carrega um buffer para a memória de transmissão do módulo LoRa
//=======================================================================================================================
uint8_t loadBufferToLoRa(uint8_t *buffer, uint8_t size)
{
    uint8_t currentLength = readLoRaRegister(REG_PAYLOAD_LENGTH);

    // Garante que o pacote atual não vai ser escrito além dos limites do buffer do módulo LoRa
    if ((currentLength + size) > MAX_PKT_LENGTH)
        size = MAX_PKT_LENGTH - currentLength;

    // Escreve os dados
    for(uint8_t index = 0; index < size; index++)
        writeLoRaRegister(REG_FIFO, buffer[index]);

    // Atualiza tamanho do Payload
    writeLoRaRegister(REG_PAYLOAD_LENGTH, currentLength + size);

    return(size);
}

//=======================================================================================================================
// Escreve um byte na memória de transmissão do módulo LoRa
//=======================================================================================================================
uint8_t writeByteToLora(uint8_t byte)
{
    return loadBufferToLoRa(&byte, sizeof(byte));
}

//=======================================================================================================================
// Finaliza o carregamento de dados para o buffer de transmissão e inicia o envio
//=======================================================================================================================
void endLoRaPacket(void)
{
    // Coloca o módulo em modo de transmissão
    setLoRaOpMode(MODE_TX);
    
    while ((readLoRaRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0);
    writeLoRaRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
}

//=======================================================================================================================
// Verifica se houve uma recepção
//=======================================================================================================================
uint8_t checkLoRaReception(void)
{
    uint8_t irqFlags = readLoRaRegister(REG_IRQ_FLAGS);
    uint8_t packetLength = 0;
    
    // Limpa os flags de interrupção
    writeLoRaRegister(REG_IRQ_FLAGS, irqFlags);
    
    // Pacote recebido
    if((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)
    {
        packetLength = readLoRaRegister(REG_RX_NB_BYTES);
        
        // Define o endereço de leitura para o endereço atual
        writeLoRaRegister(REG_FIFO_ADDR_PTR, readLoRaRegister(REG_FIFO_RX_CURRENT_ADDR));
        
        // Coloca o módulo em modo Standby para transmissão.
        setLoRaOpMode(MODE_STDBY);
    }
    else if(readLoRaRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE))  // Não está em modo de recepção
    {
        // Reseta o endereço da FIFO
        writeLoRaRegister(REG_FIFO_ADDR_PTR, 0);
        setLoRaOpMode(MODE_RX_SINGLE);
    }
    
    return(packetLength);
}

//=======================================================================================================================
// Retorna quantos bytes não foram lidos do buffer de recepção
//=======================================================================================================================
uint8_t LoRaBytesAvailable(void)
{
    uint8_t baseAddres = readLoRaRegister(REG_FIFO_RX_BASE_ADDR);
    uint8_t currentAddress = readLoRaRegister(REG_FIFO_ADDR_PTR);
    uint8_t quantBytes = readLoRaRegister(REG_RX_NB_BYTES);
    
    if(quantBytes > (currentAddress - baseAddres))
        return (quantBytes - (currentAddress - baseAddres));
    else
        return(0);
}

//=======================================================================================================================
// Lê um byte do módulo
//=======================================================================================================================
uint8_t readByteFromLoRa(void)
{
    if(!LoRaBytesAvailable())
        return(0xFF);
    return readLoRaRegister(REG_FIFO);
}

//=======================================================================================================================
// Lê um byte do módulo
//=======================================================================================================================
void loraPowerDown(void)
{
    // Coloca o módulo em modo Sleep para reduzir consumo.
    setLoRaOpMode(MODE_SLEEP);
}

//***********************************************************************************************************************