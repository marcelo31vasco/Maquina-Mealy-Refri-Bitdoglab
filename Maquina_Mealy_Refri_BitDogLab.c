#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "ws2812.pio.h"

#define portaI2c i2c1
#define pinoSda 14
#define pinoScl 15
#define pinoLedWs2812 7
#define botaoA 5
#define botaoB 6
#define pinoBuzzer 21

int saldoAtual = 0;
int totalLatasLiberadas = 0;

void tocarBeep(bool sucesso) {
    gpio_set_function(pinoBuzzer, GPIO_FUNC_PWM);
    uint fatiaPwm = pwm_gpio_to_slice_num(pinoBuzzer);
    uint canalPwm = pwm_gpio_to_channel(pinoBuzzer);
    pwm_config configuracaoPwm = pwm_get_default_config();
    pwm_config_set_wrap(&configuracaoPwm, 25000);
    pwm_config_set_clkdiv(&configuracaoPwm, 1.0f);
    pwm_init(fatiaPwm, &configuracaoPwm, false);

    pwm_set_chan_level(fatiaPwm, canalPwm, sucesso ? 18000 : 8000);
    pwm_set_enabled(fatiaPwm, true);
    sleep_ms(sucesso ? 250 : 100);
    pwm_set_enabled(fatiaPwm, false);
}

void acenderMatriz(PIO pio, uint maquinaEstado, bool liberado) {
    uint32_t cor = liberado ? 0x0F000000 : 0x000F0000;

    for (int indice = 0; indice < 25; indice++) {
        pio_sm_put_blocking(pio, maquinaEstado, cor);
    }
    sleep_ms(250);

    for (int indice = 0; indice < 25; indice++) {
        pio_sm_put_blocking(pio, maquinaEstado, 0x00000000);
    }
}

void atualizarDisplay(uint8_t *bufferDisplay, struct render_area *areaRenderizacao, int ultimaMoeda, int saidaAtual) {
    char textoLinha[32];

    memset(bufferDisplay, 0, ssd1306_buffer_length);

    ssd1306_draw_string(bufferDisplay, 0, 0, "25 50 100 LATA");
    ssd1306_draw_string(bufferDisplay, 0, 8, "A:25 B:50");
    ssd1306_draw_line(bufferDisplay, 0, 19, 127, 19, true);

    snprintf(textoLinha, sizeof(textoLinha), "SALDO: %d", saldoAtual);
    ssd1306_draw_string(bufferDisplay, 0, 24, textoLinha);

    if (ultimaMoeda > 0) {
        snprintf(textoLinha, sizeof(textoLinha), "MOEDA: %d", ultimaMoeda);
    } else {
        snprintf(textoLinha, sizeof(textoLinha), "MOEDA: NENHUMA");
    }
    ssd1306_draw_string(bufferDisplay, 0, 40, textoLinha);

    snprintf(textoLinha, sizeof(textoLinha), "SAIDA: %d", saidaAtual);
    ssd1306_draw_string(bufferDisplay, 0, 52, textoLinha);

    render_on_display(bufferDisplay, areaRenderizacao);
}

int main() {
    stdio_init_all();

    i2c_init(portaI2c, ssd1306_i2c_clock * 1000);
    gpio_set_function(pinoSda, GPIO_FUNC_I2C);
    gpio_set_function(pinoScl, GPIO_FUNC_I2C);
    gpio_pull_up(pinoSda);
    gpio_pull_up(pinoScl);
    sleep_ms(50);

    ssd1306_init();

    struct render_area areaRenderizacao = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&areaRenderizacao);

    uint8_t bufferDisplay[ssd1306_buffer_length];
    memset(bufferDisplay, 0, ssd1306_buffer_length);
    render_on_display(bufferDisplay, &areaRenderizacao);

    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);

    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);

    PIO pio = pio0;
    uint maquinaEstado = 0;
    uint deslocamentoPrograma = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, maquinaEstado, deslocamentoPrograma, pinoLedWs2812, 800000, false);

    int ultimaMoedaInserida = 0;
    int saida = 0;

    atualizarDisplay(bufferDisplay, &areaRenderizacao, ultimaMoedaInserida, saida);

    while (true) {
        int moedaInserida = 0;

        if (!gpio_get(botaoA)) {
            sleep_ms(30);
            if (!gpio_get(botaoA)) {
                moedaInserida = 25;
                while (!gpio_get(botaoA)) {
                    sleep_ms(10);
                }
            }
        } else if (!gpio_get(botaoB)) {
            sleep_ms(30);
            if (!gpio_get(botaoB)) {
                moedaInserida = 50;
                while (!gpio_get(botaoB)) {
                    sleep_ms(10);
                }
            }
        }

        if (moedaInserida > 0) {
            ultimaMoedaInserida = moedaInserida;
            saldoAtual += moedaInserida;

            if (saldoAtual >= 100) {
                saida = 1;
                totalLatasLiberadas++;
                saldoAtual -= 100;
                tocarBeep(true);
                acenderMatriz(pio, maquinaEstado, true);
            } else {
                saida = 0;
                tocarBeep(false);
                acenderMatriz(pio, maquinaEstado, false);
            }

            atualizarDisplay(bufferDisplay, &areaRenderizacao, ultimaMoedaInserida, saida);
            printf("Moeda: %d | Saldo: %d | Saida: %d\n", moedaInserida, saldoAtual, saida);
        }

        sleep_ms(50);
    }

    return 0;
}