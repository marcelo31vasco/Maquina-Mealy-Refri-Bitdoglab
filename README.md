# Máquina de Mealy — Dispensador de Refrigerante (BitDogLab)

Este repositório contém a implementação em linguagem C do transdutor de estados finitos (**Máquina de Mealy**) para um dispensador de refrigerante utilizando o **Raspberry Pi Pico / BitDogLab** com o **Pico SDK**, no contexto da **Tarefa 6** do **Programa de Residência EmbarcaTech** (Trilha de Software Embarcado).

---

## Enunciado da Atividade

Implementar um transdutor de estados finitos (**Máquina de Mealy**) na placa **BitDogLab** que modele um dispensador de refrigerante com entradas de moedas de **25** e **50** centavos e liberação de lata ao atingir **100** centavos, indicando o estado do saldo, a última moeda inserida e a saída (`1` para lata liberada e `0` caso contrário) por meio dos recursos de hardware da placa.

> *Observação: Caso deseje transcrever literalmente o texto integral ou algum detalhe específico do enunciado original da sua lista da Tarefa 6, basta substituir ou complementar o bloco acima antes de publicar.*

---

## Como a Atividade Foi Resolvida

O sistema foi modelado como uma **Máquina de Mealy**, na qual a saída (`saida`) e a transição de saldo dependem tanto do **saldo acumulado atual** quanto da **moeda inserida** no momento:

1. **Entradas de moedas via botões físicos:**
   - **Botão A (`pino 5`):** simula a inserção de **25** centavos.
   - **Botão B (`pino 6`):** simula a inserção de **50** centavos.
   - Ambos os pinos são configurados como entrada com resistor de *pull-up* interno (`gpio_pull_up`), com *debounce* de `30 ms` e espera pela soltura do botão para evitar leituras múltiplas acidentais.

2. **Lógica de transição de estados e saída (Mealy):**
   - A cada moeda válida (`moedaInserida`), `ultimaMoedaInserida` é atualizada e somada a `saldoAtual`.
   - Se `saldoAtual` for maior ou igual a `100` centavos:
     - A saída assume `saida = 1` (lata liberada).
     - `totalLatasLiberadas` é incrementado.
     - O valor da lata (`100`) é subtraído de `saldoAtual`, preservando eventual excedente/troco no saldo acumulado.
     - O *buzzer* emite o sinal sonoro de sucesso (`tocarBeep(true)`) e a matriz WS2812 indica liberação (`acenderMatriz(pio, maquinaEstado, true)`).
   - Caso `saldoAtual` seja menor que `100` centavos:
     - A saída assume `saida = 0`.
     - O sistema emite o *feedback* correspondente de saldo insuficiente (`tocarBeep(false)` e `acenderMatriz(pio, maquinaEstado, false)`).

3. **Feedback visual, sonoro e serial:**
   - **Display OLED SSD1306 (`i2c1`, SDA no pino `14`, SCL no pino `15`):** exibe o cabeçalho (`25 50 100 LATA` e `A:25 B:50`), o `SALDO` atual, a última `MOEDA` inserida (`25`, `50` ou `NENHUMA`) e o valor de `SAIDA` (`0` ou `1`).
   - **Matriz de LEDs WS2812 (`pino 7`):** acionada via PIO (`pio0`, máquina de estados `0`) por `250 ms` e apagada em seguida.
   - **Buzzer (`pino 21`):** acionado via PWM (`slice` e `channel` configurados com *wrap* de `25000` e *clkdiv* de `1.0f`).
   - **Saída serial USB (`printf`):** imprime no monitor serial o resumo a cada transição (`Moeda: ... | Saldo: ... | Saida: ...`).

4. **Padrão de código adotado:**
   - Código principal escrito em C puro com variáveis, constantes, macros e funções nomeadas em português no padrão *camelCase* (`saldoAtual`, `totalLatasLiberadas`, `tocarBeep`, `acenderMatriz`, `atualizarDisplay`, `areaRenderizacao`, `bufferDisplay`, `maquinaEstado`, `deslocamentoPrograma`) e sem comentários explicativos inline.

---

## Bibliotecas e Headers Utilizados

### Bibliotecas Padrão do C
- `<stdio.h>`: suporte a entrada/saída padrão e formatação de strings (`printf`, `snprintf`).
- `<stdlib.h>`: utilidades gerais da linguagem C.
- `<string.h>`: manipulação de memória e buffers (`memset`).

### Bibliotecas do Raspberry Pi Pico SDK
- `"pico/stdlib.h"`: inicialização padrão do sistema (`stdio_init_all`), controle básico de GPIOs (`gpio_init`, `gpio_set_dir`, `gpio_pull_up`, `gpio_get`, `gpio_set_function`) e temporização (`sleep_ms`).
- `"hardware/i2c.h"`: comunicação I2C (`i2c_init`) utilizada com o display OLED.
- `"hardware/pio.h"`: controle do bloco PIO (`pio_add_program`, `pio_sm_put_blocking`) para acionar a matriz de LEDs WS2812.
- `"hardware/clocks.h"`: suporte de clocks do hardware no ambiente do Pico SDK.
- `"hardware/pwm.h"`: geração de sinal PWM (`pwm_gpio_to_slice_num`, `pwm_gpio_to_channel`, `pwm_config_*`, `pwm_init`, `pwm_set_chan_level`, `pwm_set_enabled`) para o buzzer.

### Headers Locais do Projeto
- `"inc/ssd1306.h"`: driver e funções gráficas do display OLED SSD1306 (`ssd1306_init`, `ssd1306_draw_string`, `ssd1306_draw_line`, `calculate_render_area_buffer_length`, `render_on_display`).
- `"ws2812.pio.h"`: programa PIO compilado para controle dos LEDs endereçáveis WS2812 (`ws2812_program`, `ws2812_program_init`).

---

## Mapeamento de Pinos (BitDogLab)

| Componente | Pino GPIO | Configuração / Interface |
| :--- | :--- | :--- |
| **Botão A (25 centavos)** | `GPIO 5` | Entrada digital com *pull-up* |
| **Botão B (50 centavos)** | `GPIO 6` | Entrada digital com *pull-up* |
| **Matriz LEDs WS2812** | `GPIO 7` | PIO (`pio0`, SM `0`, `800000 Hz`) |
| **Display OLED — SDA** | `GPIO 14` | `i2c1` (`GPIO_FUNC_I2C`, *pull-up*) |
| **Display OLED — SCL** | `GPIO 15` | `i2c1` (`GPIO_FUNC_I2C`, *pull-up*) |
| **Buzzer** | `GPIO 21` | PWM (`GPIO_FUNC_PWM`) |

---

## Compilação e Execução

1. Configure o ambiente com **VS Code** e **Raspberry Pi Pico SDK** (com `CMake` e `Ninja`/compilador ARM configurados).
2. Abra a pasta do projeto (`Maquina_Mealy_Refri_BitDogLab`) e compile o alvo principal (`Maquina_Mealy_Refri_BitDogLab.c`).
3. Envie o arquivo `.uf2` gerado na pasta `build/` para a placa **BitDogLab** em modo *BOOTSEL* (ou grave via interface de depuração/flash).
4. Pressione os botões **A** (`25`) e **B** (`50`) para testar as transições e acompanhar o saldo e a saída no display OLED e no terminal serial.