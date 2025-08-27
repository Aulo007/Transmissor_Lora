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
    char str_line1[32], str_line2[32], str_line3[32], str_line4[32];
    
    sprintf(str_line1, "Pkt:%d RSSI:%d", packet_num, rssi);
    sprintf(str_line2, "Temp: %.1f C", temp);
    sprintf(str_line3, "Umid: %.1f %%", umid);
    sprintf(str_line4, "Pres: %.1f hPa", press);

    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "LoRa Receptor", 16, 4);
    ssd1306_draw_string(ssd, str_line1, 0, 20);
    ssd1306_draw_string(ssd, str_line2, 0, 32);
    ssd1306_draw_string(ssd, str_line3, 0, 44);
    ssd1306_draw_string(ssd, str_line4, 0, 56);
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
            } else {
                 printf("Falha ao extrair dados do pacote. Itens lidos: %d\n", items_parsed);
                 ssd1306_fill(&ssd, false);
                 ssd1306_draw_string(&ssd, "Erro no Pacote", 0, 20);
                 ssd1306_draw_string(&ssd, (const char*) buffer, 0, 32);
                 ssd1306_send_data(&ssd);
            }
        }
        sleep_ms(10); // Pequena pausa para não sobrecarregar o processador
    }
    return 0;
}