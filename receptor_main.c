// receptor_main.c

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// ========================================
// NOSSAS BIBLIOTECAS
// ========================================
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/lora.h"

// ========================================
// CONFIGURAÇÕES DO RÁDIO LORA (DEVEM SER IGUAIS ÀS DO TRANSMISSOR!)
// ========================================
#define LORA_FREQUENCY 915000000 // 915 MHz
#define LORA_POWER_DBM 17        // Potência não é usada no RX, mas a função init precisa
#define LORA_SPREADING_FACTOR 7  // SF7
#define LORA_BANDWIDTH 125000    // 125 kHz
#define LORA_CODING_RATE 1       // 4/5

// ========================================
// CONFIGURAÇÃO DOS PINOS
// ========================================
// Pinos I2C para o display
#define I2C_PORT_DISPLAY i2c1
#define I2C_SDA_DISPLAY 14
#define I2C_SCL_DISPLAY 15
#define DISPLAY_ENDERECO 0x3C

// ========================================
// FUNÇÃO PARA ATUALIZAR O DISPLAY
// ========================================
void update_display(ssd1306_t *ssd, int packet_num, float temp, float umid, float press, int rssi) {
    char str_tmp_bmp[10], str_press[10], str_tmp_aht[10], str_umi[10];
            sprintf(str_tmp_bmp, "%.1fC", temp);
            sprintf(str_press, "%.0fhPa", press);
            sprintf(str_tmp_aht, "%.1fC", temp);
            sprintf(str_umi, "%.1f%%", umid);
            ssd1306_fill(ssd, false);
            ssd1306_draw_string(ssd, "Lora Receptor", 12, 4);
            ssd1306_draw_string(ssd, "BMP280", 12, 22);
            ssd1306_draw_string(ssd, "AHT20", 76, 22);
            ssd1306_line(ssd, 63, 18, 63, 61, true);
            ssd1306_draw_string(ssd, str_tmp_bmp, 12, 36);
            ssd1306_draw_string(ssd, str_press, 12, 48);
            ssd1306_draw_string(ssd, str_tmp_aht, 76, 36);
            ssd1306_draw_string(ssd, str_umi, 76, 48);
            ssd1306_send_data(ssd);
}


// ========================================
// FUNÇÃO PRINCIPAL
// ========================================
int main() {
    stdio_init_all();
    sleep_ms(3000);
    printf("Receptor LoRa - Estacao Meteorologica\n");

    // --- Inicialização do Rádio LoRa ---
    if (!lora_setup()) {
        printf("Falha ao iniciar o radio LoRa. Travando.\n");
        while (1);
    }
    lora_init(LORA_FREQUENCY, LORA_POWER_DBM, LORA_SPREADING_FACTOR, LORA_BANDWIDTH, LORA_CODING_RATE);

    // --- Inicialização do Display SSD1306 ---
    i2c_init(I2C_PORT_DISPLAY, 400 * 1000);
    gpio_set_function(I2C_SDA_DISPLAY, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISPLAY, GPIO_FUNC_I2C);
    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, DISPLAY_ENDERECO, I2C_PORT_DISPLAY);
    ssd1306_config(&ssd);
    ssd1306_draw_string(&ssd, "Aguardando...", 10, 30);
    ssd1306_send_data(&ssd);

    // Coloca o rádio em modo de recepção
    lora_enter_receive_mode();

    uint8_t buffer[256];
    
    // Variáveis para armazenar os dados recebidos
    int pkt_id = 0;
    float temp_rx = 0.0, umid_rx = 0.0, press_rx = 0.0;
    int rssi = 0;

    // Loop principal
    while (true) {
        int packet_size = lora_check_packet();
        if (packet_size > 0) {
            printf("Pacote recebido! Tamanho: %d bytes\n", packet_size);
            
            lora_read_packet(buffer, sizeof(buffer));
            rssi = lora_get_rssi();

            printf("Dados: '%s' | RSSI: %d dBm\n", buffer, rssi);
            
            // Tenta extrair os dados do pacote usando o formato do seu transmissor
            int items_parsed = sscanf((const char*)buffer, "ID:Node1,Pkt:%d,T:%f,U:%f,P:%f", 
                                       &pkt_id, &temp_rx, &umid_rx, &press_rx);

            if (items_parsed == 4) {
                printf("Dados extraidos com sucesso -> Pkt: %d, T: %.1f, U: %.1f, P: %.1f\n",
                       pkt_id, temp_rx, umid_rx, press_rx);
                
                update_display(&ssd, pkt_id, temp_rx, umid_rx, press_rx, rssi);
            }
        }
        sleep_ms(10); // Pequena pausa para não sobrecarregar o processador
    }
    return 0;
}