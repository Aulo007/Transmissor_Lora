#include <stdio.h>
#include <string.h>
#include <math.h> // Para pow()
#include "lora.h"

// ============================================================================
// == Definições dos Pinos e Constantes Internas ==============================
// ============================================================================
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_RST  20

#define RF_CRYSTAL_FREQ_HZ 32000000

// ============================================================================
// == Funções de Baixo Nível (Privadas ao Módulo) =============================
// ============================================================================

// Essas funções são 'static' para que não possam ser chamadas de fora deste arquivo.
// É uma boa prática de encapsulamento.

static void rmf95_reset() {
    gpio_put(PIN_RST, 0); // O datasheet sugere pull-low para resetar
    sleep_ms(1);
    gpio_put(PIN_RST, 1); // Libera o pino (high-Z), resistor de pull-up externo faria o resto
    sleep_ms(10);
}

static void rmf95_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx_data[] = { reg | 0x80, value }; // Bit 7 em 1 para escrita
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, tx_data, 2);
    gpio_put(PIN_CS, 1);
}

static uint8_t rmf95_read_reg(uint8_t reg) {
    uint8_t tx_data[] = { reg & 0x7F, 0x00 }; // Bit 7 em 0 para leitura
    uint8_t rx_data[2];
    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx_data, rx_data, 2);
    gpio_put(PIN_CS, 1);
    return rx_data[1];
}

static void rmf95_write_fifo(const uint8_t* buffer, uint8_t length) {
    uint8_t tx_data = REG_FIFO | 0x80;
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &tx_data, 1); 
    spi_write_blocking(SPI_PORT, buffer, length); 
    gpio_put(PIN_CS, 1);
}

// ============================================================================
// == Implementação das Funções Públicas (declaradas em lora.h) ===============
// ============================================================================

bool lora_setup() {
    spi_init(SPI_PORT, 1000 * 1000); // 1 MHz
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    
    rmf95_reset();

    uint8_t version = rmf95_read_reg(REG_VERSION);
    printf("Versao do RFM95: 0x%02X\n", version);

    if (version != 0x12) {
        printf("Falha na comunicacao SPI. Travando. ❌\n");
        return false;
    }
    printf("Comunicacao SPI OK! ✅\n");
    return true;
}

void lora_init(long frequency, int8_t power, uint8_t sf, long bw, uint8_t cr) {
    // 1. Colocar em modo SLEEP + LoRa para configurar
    rmf95_write_reg(REG_OPMODE, RF95_MODE_SLEEP);
    sleep_ms(10);
    printf("Configurando o radio LoRa...\n");

    // 2. Configurar a frequência
    uint64_t frf = ((uint64_t)frequency << 19) / RF_CRYSTAL_FREQ_HZ;
    rmf95_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
    rmf95_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
    rmf95_write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));

    // 3. Configurar potência de saída
    if (power > 17) power = 17;
    if (power < 2) power = 2;
    rmf95_write_reg(REG_PA_CONFIG, 0x80 | (power - 2)); // 0x80 para usar PA_BOOST

    // 4. Configurar LNA (amplificador de baixo ruído) para ganho máximo
    rmf95_write_reg(REG_LNA, 0x23);

    // 5. Configurar ponteiros do FIFO
    rmf95_write_reg(REG_FIFO_TX_BASE_AD, 0x00);
    rmf95_write_reg(REG_FIFO_RX_BASE_AD, 0x80); // Não usaremos RX, mas é boa prática

    // 6. Configurar o modem (BW, CR, Header)
    // BW: 7 = 125kHz, CR: 1 = 4/5, Header: 0 = Explícito
    uint8_t modem_config_1 = 0x72; // Ex: 125kHz, 4/5, Explícito
    rmf95_write_reg(REG_MODEM_CONFIG, modem_config_1);

    // 7. Configurar o modem (SF, CRC)
    // SF: 7, CRC: 1 = On
    uint8_t modem_config_2 = (sf << 4) | 0x04;
    rmf95_write_reg(REG_MODEM_CONFIG2, modem_config_2);

    // 8. Configurar preâmbulo
    rmf95_write_reg(REG_PREAMBLE_MSB, 0x00);
    rmf95_write_reg(REG_PREAMBLE_LSB, 0x08); // 8 símbolos

    // 9. Colocar em modo STANDBY
    rmf95_write_reg(REG_OPMODE, RF95_MODE_STANDBY);
    sleep_ms(10);
    printf("RFM95 configurado para LoRa TX em %ld Hz\n", frequency);
}

void lora_send_packet(const char* message) {
    uint8_t message_len = strlen(message);
    
    // Garantir que estamos em STANDBY
    rmf95_write_reg(REG_OPMODE, RF95_MODE_STANDBY);

    // Posicionar o ponteiro do FIFO no início da área de TX
    rmf95_write_reg(REG_FIFO_ADDR_PTR, 0x00);
    
    // Escrever a mensagem no FIFO
    rmf95_write_fifo((const uint8_t*)message, message_len);
    
    // Atualizar o registrador de tamanho do payload
    rmf95_write_reg(REG_PAYLOAD_LENGTH, message_len);
    
    // Iniciar a transmissão
    rmf95_write_reg(REG_OPMODE, RF95_MODE_TX);
    
    // Aguardar o fim da transmissão (polling na flag de IRQ)
    while ((rmf95_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
        sleep_ms(10);
    }
    
    // Limpar a flag de IRQ
    rmf95_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    
    printf("Pacote enviado: '%s'\n", message);
}