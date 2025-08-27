// lora.h

#ifndef LORA_H
#define LORA_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// ============================================================================
// == Definições de Registradores do RFM95 (conforme datasheet) ===============
// ============================================================================

// --- Registradores Comuns ---
#define REG_FIFO 0x00
#define REG_OPMODE 0x01
#define REG_FRF_MSB 0x06
#define REG_FRF_MID 0x07
#define REG_FRF_LSB 0x08
#define REG_PA_CONFIG 0x09
#define REG_LNA 0x0C
#define REG_FIFO_ADDR_PTR 0x0D
#define REG_FIFO_TX_BASE_AD 0x0E
#define REG_FIFO_RX_BASE_AD 0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS 0x12
#define REG_RX_NB_BYTES 0x13
#define REG_PKT_SNR_VALUE 0x19
#define REG_PKT_RSSI_VALUE 0x1A
#define REG_MODEM_CONFIG 0x1D
#define REG_MODEM_CONFIG2 0x1E
#define REG_PREAMBLE_MSB 0x20
#define REG_PREAMBLE_LSB 0x21
#define REG_PAYLOAD_LENGTH 0x22
#define REG_MODEM_CONFIG3 0x26
#define REG_VERSION 0x42

// --- Máscaras de Flags de Interrupção (IRQ) ---
#define IRQ_TX_DONE_MASK 0x08
#define IRQ_RX_DONE_MASK 0x40
#define IRQ_PAYLOAD_CRC_ERR_MASK 0x20

// --- Modos de Operação ---
#define RF95_MODE_SLEEP 0x80         // Modo LoRa + Sleep
#define RF95_MODE_STANDBY 0x81       // Modo LoRa + Standby
#define RF95_MODE_TX 0x83            // Modo LoRa + Transmissão
#define RF95_MODE_RX_CONTINUOUS 0x85 // Modo LoRa + Recepção Contínua

// ============================================================================
// == Funções Públicas da Biblioteca ==========================================
// ============================================================================

/**
 * @brief Configura os pinos GPIO e a interface SPI. Deve ser chamada primeiro.
 * @return true se a comunicação com o rádio for bem-sucedida, false caso contrário.
 */
bool lora_setup();

/**
 * @brief Inicializa e configura o rádio LoRa com os parâmetros fornecidos.
 * @param frequency Frequência de operação em Hz (ex: 915000000).
 * @param power Potência de transmissão em dBm (2 a 17).
 * @param sf Spreading Factor (6 a 12).
 * @param bw Largura de banda em Hz (ex: 125000).
 * @param cr Coding Rate (1 a 4, representando 4/5 a 4/8).
 */
void lora_init(long frequency, int8_t power, uint8_t sf, long bw, uint8_t cr);

/**
 * @brief Envia uma mensagem de texto via LoRa.
 * @param message A string a ser enviada.
 */
void lora_send_packet(const char *message);

/**
 * @brief Coloca o rádio em modo de recepção contínua.
 */
void lora_enter_receive_mode();

/**
 * @brief Verifica se um novo pacote foi recebido. Função não bloqueante.
 * @return O tamanho do pacote recebido (em bytes), ou 0 se nenhum pacote chegou.
 */
int lora_check_packet();

/**
 * @brief Lê o último pacote recebido do buffer FIFO.
 * @param buffer Ponteiro para o buffer onde os dados serão armazenados.
 * @param max_len O tamanho máximo do buffer.
 * @return O número de bytes lidos.
 */
int lora_read_packet(uint8_t *buffer, int max_len);

/**
 * @brief Obtém o RSSI (Received Signal Strength Indicator) do último pacote.
 * @return O valor do RSSI em dBm.
 */
int lora_get_rssi();

#endif // LORA_H