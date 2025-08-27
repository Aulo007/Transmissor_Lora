#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ========================================
// NOSSAS BIBLIOTECAS
// ========================================
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/aht20.h"
#include "lib/bmp280.h"
#include "lib/lora.h"

// ========================================
// CONFIGURAÇÕES DO RÁDIO LORA
// ========================================
#define LORA_FREQUENCY 915000000 // 915 MHz
#define LORA_POWER_DBM 17        // 17 dBm
#define LORA_SPREADING_FACTOR 7  // SF7
#define LORA_BANDWIDTH 125000    // 125 kHz
#define LORA_CODING_RATE 1       // 4/5

// ========================================
// CONFIGURAÇÃO DOS PINOS
// ========================================
#define BOTAO_A 5
#define BOTAO_B 6

// Pinos I2C para os sensores
#define I2C_PORT_SENSORES i2c0
#define I2C_SDA_SENSORES 0
#define I2C_SCL_SENSORES 1

// Pinos I2C para o display
#define I2C_PORT_DISPLAY i2c1
#define I2C_SDA_DISPLAY 14
#define I2C_SCL_DISPLAY 15
#define DISPLAY_ENDERECO 0x3C

// ========================================
// VARIÁVEIS GLOBAIS DE CONTROLE E DADOS
// ========================================
volatile bool g_enviar_dados_lora = true;
volatile int g_tela_display = 0;
volatile uint32_t g_last_interrupt_time = 0;

float g_temp_bmp = 0.0f;
float g_pressao_kpa = 0.0f;
float g_temp_aht = 0.0f;
float g_umidade_aht = 0.0f;
float g_temp_media = 0.0f;

// ========================================
// ROTINA DE INTERRUPÇÃO PARA OS BOTÕES
// ========================================
void gpio_callback(uint gpio, uint32_t events)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - g_last_interrupt_time < 200)
    {
        return;
    }
    g_last_interrupt_time = now;

    if (gpio == BOTAO_A)
    {
        g_enviar_dados_lora = !g_enviar_dados_lora;
        printf("Botao A pressionado! Envio LoRa: %s\n", g_enviar_dados_lora ? "ATIVO" : "PAUSADO");
    }
    else if (gpio == BOTAO_B)
    {
        g_tela_display = 1 - g_tela_display;
        printf("Botao B pressionado! Trocando para tela: %d\n", g_tela_display);
    }
}

// ========================================
// FUNÇÃO PRINCIPAL
// ========================================
int main()
{
    stdio_init_all();
    sleep_ms(3000);
    printf("Estacao Meteorologica - MODO TESTE DE SENSORES\n");

    // --- ATENÇÃO: INICIALIZAÇÃO DO LORA ESTÁ COMENTADA PARA TESTES ---
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

    // --- Inicialização dos Sensores (BMP280 e AHT20) ---
    i2c_init(I2C_PORT_SENSORES, 400 * 1000);
    gpio_set_function(I2C_SDA_SENSORES, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_SENSORES, GPIO_FUNC_I2C);
    bmp280_init(I2C_PORT_SENSORES);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT_SENSORES, &params);
    aht20_init(I2C_PORT_SENSORES);

    // --- CORREÇÃO: Configuração dos Botões e Interrupções ---
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    // Registra o callback UMA VEZ para todos os pinos
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    printf("Sistema pronto! Pressione os botoes A e B para testar.\n");
    int packet_counter = 0;

    // Loop principal
    while (true)
    {
        // --- Leitura dos Sensores ---
        int32_t raw_temp_bmp, raw_pressure_pa_int;
        bmp280_read_raw(I2C_PORT_SENSORES, &raw_temp_bmp, &raw_pressure_pa_int);
        g_temp_bmp = bmp280_convert_temp(raw_temp_bmp, &params) / 100.0;
        g_pressao_kpa = bmp280_convert_pressure(raw_pressure_pa_int, raw_temp_bmp, &params) / 1000.0;

        // DEBUG: Imprime os valores lidos no monitor serial
        printf("BMP280 -> Temp: %.2f C, Pressao: %.2f kPa\n", g_temp_bmp, g_pressao_kpa);

        AHT20_Data data_aht;
        if (aht20_read(I2C_PORT_SENSORES, &data_aht))
        {
            g_temp_aht = data_aht.temperature;
            g_umidade_aht = data_aht.humidity;
        }
        g_temp_media = (g_temp_bmp + g_temp_aht) / 2.0f;

        // --- Atualização do Display ---
        ssd1306_fill(&ssd, false);
        char lora_status_str[16];
        sprintf(lora_status_str, "LoRa: %s", g_enviar_dados_lora ? "ON" : "OFF");
        ssd1306_draw_string(&ssd, lora_status_str, 28, 4);

        if (g_tela_display == 0)
        { // Tela de dados dos sensores
            char str_tmp_bmp[10], str_press[10], str_tmp_aht[10], str_umi[10];
            sprintf(str_tmp_bmp, "%.1fC", g_temp_bmp);
            sprintf(str_press, "%.0fhPa", g_pressao_kpa * 10.0);
            sprintf(str_tmp_aht, "%.1fC", g_temp_aht);
            sprintf(str_umi, "%.1f%%", g_umidade_aht);

            ssd1306_draw_string(&ssd, "BMP280", 12, 22);
            ssd1306_draw_string(&ssd, "AHT20", 76, 22);
            ssd1306_line(&ssd, 63, 18, 63, 61, true);
            ssd1306_draw_string(&ssd, str_tmp_bmp, 12, 36);
            ssd1306_draw_string(&ssd, str_press, 12, 48);
            ssd1306_draw_string(&ssd, str_tmp_aht, 76, 36);
            ssd1306_draw_string(&ssd, str_umi, 76, 48);
        }
        else
        { // Tela de parâmetros LoRa
            char str_freq[20], str_sf_bw[20], str_pwr_cr[20];
            sprintf(str_freq, "F:%.1fMHz", LORA_FREQUENCY / 1000000.0);
            sprintf(str_sf_bw, "SF%d BW%ldk", LORA_SPREADING_FACTOR, LORA_BANDWIDTH / 1000);
            sprintf(str_pwr_cr, "P:%ddBm CR:4/%d", LORA_POWER_DBM, LORA_CODING_RATE + 4);

            ssd1306_draw_string(&ssd, "LoRa Params", 16, 16);
            ssd1306_draw_string(&ssd, str_freq, 4, 30);
            ssd1306_draw_string(&ssd, str_sf_bw, 4, 42);
            ssd1306_draw_string(&ssd, str_pwr_cr, 4, 54);
        }
        ssd1306_send_data(&ssd);

        // Linha comentada para testes
        if (g_enviar_dados_lora) {
            char pacote_lora[100];
            snprintf(pacote_lora, sizeof(pacote_lora), "ID:Node1,Pkt:%d,T:%.1f,U:%.1f,P:%.1f",
                packet_counter++, g_temp_media, g_umidade_aht, g_pressao_kpa * 10);
        
            lora_send_packet(pacote_lora);
        }

        sleep_ms(2000);
    }
    return 0; 
}