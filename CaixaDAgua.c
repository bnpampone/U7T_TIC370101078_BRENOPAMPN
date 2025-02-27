#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pio_matrix.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define I2C_PORT i2c1  // Define a porta I2C usada
#define I2C_SDA 14     // Pino do SDA (dados)
#define I2C_SCL 15     // Pino do SCL (clock)
#define endereco 0x3C  // Endereço do display OLED
#define LED_GREEN 11   // LED verde (indica nível OK)
#define LED_RED 13     // LED vermelho (indica nível máximo)
#define OUT_PIN 7      // Pino de saída para os LEDs
#define CAPACIDADE_MAXIMA 500  // Limite da caixa d’água
#define NUM_PIXELS 25   // Quantidade de pixels da matriz

int nivel_agua = 0;  // Inicia zerado devido a Caixa D' água iniciar vazia
ssd1306_t ssd;  // Estrutura para o display OLED

// Matrizes que representam os níveis da água para a matriz de LEDs
const double desenhos[5][25] = {
    {0.0, 0.0, 0.0, 0.0, 0.0, 
     0.0, 0.0, 0.0, 0.0, 0.0, 
     0.0, 0.0, 0.0, 0.0, 0.0, 
     0.0, 0.0, 0.0, 0.0, 0.0, 
     1.0, 1.0, 1.0, 1.0, 1.0}, // 100L

    {0.0, 0.0, 0.0, 0.0, 0.0, 
     0.0, 0.0, 0.0, 0.0, 0.0, 
     0.0, 0.0, 0.0, 0.0, 0.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0}, // 200L

    {0.0, 0.0, 0.0, 0.0, 0.0, 
     0.0, 0.0, 0.0, 0.0, 0.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0}, // 300L

    {0.0, 0.0, 0.0, 0.0, 0.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0}, // 400L

    {1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0, 
     1.0, 1.0, 1.0, 1.0, 1.0} // 500L
};


// Converte valores RGB para formato da matriz de LEDs
uint32_t matrix_rgb(double r, double g, double b) {
    return ((int)(g * 255) << 24) | ((int)(r * 255) << 16) | ((int)(b * 255) << 8);
}

// Atualiza a matriz de LEDs conforme o nível da água
void atualizar_matriz(PIO pio, uint sm, int nivel) {
    int indice = (nivel / 100) - 1;
    if (indice < 0) indice = 0;
    if (indice > 4) indice = 4;

    for (int i = NUM_PIXELS - 1; i >= 0; i--) {
        double brilho = desenhos[indice][i];  
        uint32_t valor_led = matrix_rgb(0.0, 0.0, brilho);  
        pio_sm_put_blocking(pio, sm, valor_led);
    }
    sleep_ms(10);
}

// Configura os LEDs
void setup_pins() {
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 1);  // Liga o LED verde

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, 0);  // LED vermelho desligado
}

// Verifica se atingiu o limite da caixa d’água
void verificar_nivel() {
    if (nivel_agua >= CAPACIDADE_MAXIMA) {
        nivel_agua = CAPACIDADE_MAXIMA;
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_RED, 1);  // Liga o LED vermelho
    } else {
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_RED, 0);
    }
}

// Atualiza o display OLED com o nível atual da água
void atualizar_display() {
    char str_nivel[10];
    sprintf(str_nivel, "%dL", nivel_agua);

    ssd1306_fill(&ssd, false); // Limpa a Tela
    ssd1306_draw_string(&ssd, "NIVEL DA AGUA", 10, 10);
    ssd1306_draw_string(&ssd, str_nivel, 50, 30);
    ssd1306_send_data(&ssd); // Atualiza a Tela
}

int main() {
    PIO pio = pio0;  
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    stdio_init_all();
    setup_pins();

    // Inicializa o I2C e o display OLED
i2c_init(I2C_PORT, 400 * 1000);  // Inicializa a comunicação I2C na porta definida (i2c1) com velocidade de 400 kHz.
gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  
gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  // Define os pinos SDA (14) e SCL (15) como funções I2C.
gpio_pull_up(I2C_SDA);  
gpio_pull_up(I2C_SCL); // Ativa resistores de pull-up nos pinos SDA e SCL para comunicação estável.
ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT);  // Inicializa o display OLED SSD1306 com resolução de 128x64 pixels, utilizando o I2C na porta definida.
ssd1306_config(&ssd);  // Configura o display para o funcionamento correto.
ssd1306_send_data(&ssd);  // Envia os dados de configuração para o display.
ssd1306_fill(&ssd, false);  // Limpa a tela do display preenchendo com a cor de fundo (preto).
ssd1306_send_data(&ssd);  // Atualiza o display com as mudanças feitas.


    printf("Sistema de Controle de Caixa D'Água Iniciado.\n");
    printf("Capacidade máxima: %dL\n", CAPACIDADE_MAXIMA);
    printf("Digite: 'A' (100L), 'B' (200L), 'C' (300L), 'D' (400L), 'E' (500L)\n");

    while (true) {
        if (stdio_usb_connected()) {  // Se o USB estiver conectado
            char opcao;
            if (scanf(" %c", &opcao) == 1) {
                int incremento = 0;
                switch (opcao) {
                    case 'A': incremento = 100; break;
                    case 'B': incremento = 200; break;
                    case 'C': incremento = 300; break;
                    case 'D': incremento = 400; break;
                    case 'E': incremento = 500; break;
                    default:
                        printf("Opção inválida! Use A, B, C, D ou E.\n");
                        continue;
                }
                
                // Impede ultrapassar o limite
                if (nivel_agua == CAPACIDADE_MAXIMA) {
                    printf("Erro: Capacidade máxima atingida!\n");
                    continue;
                }
                if (nivel_agua + incremento > CAPACIDADE_MAXIMA) {
                    printf("Erro: Adição de %dL ultrapassa a capacidade!\n", incremento);
                    continue;
                }

                // Atualiza o nível da água e os displays
                nivel_agua += incremento;
                printf("Nível atual da caixa: %dL\n", nivel_agua);
                verificar_nivel();
                atualizar_matriz(pio, sm, nivel_agua);
                atualizar_display();
            }
        }
        sleep_ms(100);
    }
    return 0;
}
