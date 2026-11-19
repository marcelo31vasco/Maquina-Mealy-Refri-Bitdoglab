#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "ws2812.pio.h"

// Definição de Pinos da BitDogLab
#define I2C_PORT i2c1
#define PIN_SDA 14
#define PIN_SCL 15
#define PINO_LED_WS2812 7
#define BOTAO_A 5       // Botão A: 25 centavos
#define BOTAO_B 6       // Botão B: 50 centavos
#define BUZZER_PIN 21

// Variáveis de Estado do Transdutor (em centavos: 0, 25, 50, 75...)
int estado_saldo = 0;
int total_latas_liberadas = 0;

void tocar_beep(bool sucesso) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 25000);
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_init(slice_num, &config, false);

    pwm_set_chan_level(slice_num, channel, sucesso ? 18000 : 8000);
    pwm_set_enabled(slice_num, true);
    sleep_ms(sucesso ? 250 : 100);
    pwm_set_enabled(slice_num, false);
}

// Os dados sao enviados em GRB, na ordem exigida pela WS2812.
void acender_matriz(PIO pio, uint sm, bool liberado) {
    uint32_t cor = liberado ? 0x0F000000 : 0x000F0000;

    for (int i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio, sm, cor);
    }
    sleep_ms(250);

    for (int i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio, sm, 0x00000000);
    }
}

void atualizar_display(uint8_t *ssd, struct render_area *frame_area, int ultima_moeda, int saida_atual) {
    char linha[32];

    memset(ssd, 0, ssd1306_buffer_length);

    // Mostra os valores de moeda da questão e atalhos nos botões
    ssd1306_draw_string(ssd, 0, 0,  "25 50 100 LATA");
    ssd1306_draw_string(ssd, 0, 8,  "A:25 B:50");
    ssd1306_draw_line(ssd, 0, 19, 127, 19, true);

    snprintf(linha, sizeof(linha), "SALDO: %d", estado_saldo);
    ssd1306_draw_string(ssd, 0, 24, linha);

    if (ultima_moeda > 0) {
        snprintf(linha, sizeof(linha), "MOEDA: %d", ultima_moeda);
    } else {
        snprintf(linha, sizeof(linha), "MOEDA: NENHUMA");
    }
    ssd1306_draw_string(ssd, 0, 40, linha);

    snprintf(linha, sizeof(linha), "SAIDA: %d", saida_atual);
    ssd1306_draw_string(ssd, 0, 52, linha);

    render_on_display(ssd, frame_area);
}

int main() {
    stdio_init_all();

    // Inicializa i2c1 nos pinos 14 (SDA) e 15 (SCL) antes de ssd1306_init()
    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);
    sleep_ms(50);

    ssd1306_init();

    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, PINO_LED_WS2812, 800000, false);

    int ultima_moeda_inserida = 0;
    int saida = 0;

    atualizar_display(ssd, &frame_area, ultima_moeda_inserida, saida);

    while (true) {
        int moeda_inserida = 0;

        if (!gpio_get(BOTAO_A)) {
            sleep_ms(30);
            if (!gpio_get(BOTAO_A)) {
                moeda_inserida = 25;
                while (!gpio_get(BOTAO_A)) {
                    sleep_ms(10);
                }
            }
        } else if (!gpio_get(BOTAO_B)) {
            sleep_ms(30);
            if (!gpio_get(BOTAO_B)) {
                moeda_inserida = 50;
                while (!gpio_get(BOTAO_B)) {
                    sleep_ms(10);
                }
            }
        }

        if (moeda_inserida > 0) {
            ultima_moeda_inserida = moeda_inserida;
            estado_saldo += moeda_inserida;

            if (estado_saldo >= 100) {
                saida = 1;
                total_latas_liberadas++;
                estado_saldo -= 100;
                tocar_beep(true);
                acender_matriz(pio, sm, true);
            } else {
                saida = 0;
                tocar_beep(false);
                acender_matriz(pio, sm, false);
            }

            atualizar_display(ssd, &frame_area, ultima_moeda_inserida, saida);
            printf("Moeda: %d | Saldo: %d | Saida: %d\n", moeda_inserida, estado_saldo, saida);
        }

        sleep_ms(50);
    }

    return 0;
}