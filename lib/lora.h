/* EMBARCATECH - INTRODUÇÃO AO PROTOCOLO LORA - CAPÍTULO 3 / PARTE 5
 * BitDogLAB - Transmissor LoRa (TX)
 * Biblioteca com endereços do registradores do SX1276 - Módulo LoRa.
 * Prof: Ricardo Prates
 */

#ifndef LORA_INCLUDED
#define LORA_INCLUDED

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdbool.h> // Para usar bool

// ============================================================================
// == Definições dos Registradores (do professor) =============================
// ============================================================================
#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08
#define REG_PA_CONFIG               0x09
#define REG_LNA                     0x0C
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_PREAMBLE_MSB            0x20
#define REG_PREAMBLE_LSB            0x21
#define REG_PAYLOAD_LENGTH          0x22
#define REG_VERSION                 0x42 // Adicionado do seu código original

// ============================================================================
// == Modos de Operação (do professor) =========================================
// ============================================================================
#define RF95_MODE_SLEEP             0x80
#define RF95_MODE_STANDBY           0x81
#define RF95_MODE_TX                0x83

#define IRQ_TX_DONE_MASK            0x08 // Adicionado do seu código original

// ============================================================================
// == Protótipos das Funções (Nossa Interface de Alto Nível) ===================
// ============================================================================

/**
 * @brief Inicializa a comunicação SPI e os pinos GPIO para o módulo LoRa.
 * @return true se a comunicação com o módulo foi bem-sucedida, false caso contrário.
 */
bool lora_setup();

/**
 * @brief Configura os parâmetros do rádio LoRa.
 * @param frequency Frequência de operação em Hz (ex: 915000000).
 * @param power Potência de saída em dBm (2 a 17).
 * @param sf Spreading Factor (7 a 12).
 * @param bw Largura de banda em Hz (ex: 125000).
 * @param cr Coding Rate (1 a 4, correspondendo a 4/5 a 4/8).
 */
void lora_init(long frequency, int8_t power, uint8_t sf, long bw, uint8_t cr);

/**
 * @brief Envia um pacote de dados (mensagem).
 * @param message A string de texto a ser enviada.
 */
void lora_send_packet(const char* message);

#endif