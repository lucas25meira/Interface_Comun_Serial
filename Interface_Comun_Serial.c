/* Programa de capacitação EMBARCATECH
 * Unidade 4 - Microcontroladores 
 * Capítulo 6 - Interfaces de Comunicação Serial 3
 * Tarefa 1 - Aula Síncrona 04/02/2025
 *
 * Programa desenvolvido por:
 *      - Lucas Meira de Souza Leite
 * 
 * Objetivos: 
 *      - Usar interfaces de comunicação serial no RP2040
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

//Definições Display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

//Definições leds e botões
#define LED_VERDE 11
#define LED_AZUL 12
#define NUM_LEDS 25
#define WS2812_PIN 7
#define BOTAO_A 5
#define BOTAO_B 6
#define TEMPO_DEBOUNCE 200
#define TEMPO to_ms_since_boot(get_absolute_time())
volatile unsigned long TEMPODEBOUNCE; 

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B;};  // Três valores de 8-bits compõem um pixel.

typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[NUM_LEDS];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;
uint PosicaoAtual = 0;

//Arrays contendo os LEDS que irão compor a visualização de cada número 
const uint8_t Zero[12] = {1, 2, 3, 6, 8, 11, 13, 16, 18, 21, 22, 23};
const uint8_t Um[5] = {1, 8, 11, 18, 21};
const uint8_t Dois[11] = {1, 2, 3, 6, 11, 12, 13, 18, 21, 22, 23};
const uint8_t Tres[11] = {1, 2, 3, 8, 11, 12, 13, 18, 21, 22, 23};
const uint8_t Quatro[9] = {1, 8, 11, 12, 13, 16, 18, 21, 23};
const uint8_t Cinco[11] = {1, 2, 3, 8, 11, 12, 13, 16, 21, 22, 23};
const uint8_t Seis[12] = {1, 2, 3, 6, 8, 11, 12, 13, 16, 21, 22, 23};
const uint8_t Sete[7] = {1, 8, 11, 18, 21, 22, 23};
const uint8_t Oito[13] = {1, 2, 3, 6, 8, 11, 12, 13, 16, 18, 21, 22, 23};
const uint8_t Nove[12] ={1, 2, 3, 8, 11, 12, 13, 16, 18, 21, 22, 23};

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < NUM_LEDS; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < NUM_LEDS; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < NUM_LEDS; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
}

// Prototipo da função de interrupção
static void gpio_irq(uint gpio, uint32_t events);

ssd1306_t ssd; // Inicializa a estrutura do display

void main()
{
    i2c_init(I2C_PORT, 400 * 1000);

    stdio_init_all();
    TEMPODEBOUNCE = TEMPO;

    // ------------------------- DISPLAY ------------------------    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // ------------------------- BOTOES  E LEDS ------------------------
    gpio_init(LED_AZUL); //Inicializa o led
    gpio_set_dir(LED_AZUL, GPIO_OUT); //Define o led como saida

    gpio_init(LED_VERDE); //Inicializa o led
    gpio_set_dir(LED_VERDE, GPIO_OUT); //Define o led como saida

    gpio_init(BOTAO_A);                    // Inicializa o botão
    gpio_set_dir(BOTAO_A, GPIO_IN);        // Configura o pino como entrada
    gpio_pull_up(BOTAO_A);                 // Habilita o pull-up interno

    gpio_init(BOTAO_B);                    // Inicializa o botão
    gpio_set_dir(BOTAO_B, GPIO_IN);        // Configura o pino como entrada
    gpio_pull_up(BOTAO_B);                 // Habilita o pull-up interno

    
    npInit(WS2812_PIN);
    npClear(); // Inicia os leds apagados
    npWrite();

    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_RISE, true, &gpio_irq);    
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_RISE, true, &gpio_irq);

    while (true) {
        if (stdio_usb_connected()) { //Valida se o cabo conectado
            char c;
            if (scanf(" %c", &c) == 1){ // Lê caractere da entrada padrão
                
                // Limpa o display. O display inicia com todos os pixels apagados.
                ssd1306_fill(&ssd, false);
                ssd1306_send_data(&ssd);

                //Escreve mensagem display
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha um retângulo
                char str[2] = {c, '\0'};
                ssd1306_draw_string(&ssd, str, 60, 28); // Desenha no display o caracter digitado                             
                ssd1306_send_data(&ssd); // Atualiza o display
                
                //Comparações para acionamento dos leds da matriz 5x5
                if (str[0] == '0'){         // 0
                        npClear(); 
                        for (int i = 0; i < 12; ++i){
                            npSetLED(Zero[i], 0, 0, 100);
                        }    
                        npWrite();
                }else if (str[0] == '1'){         // 1
                        npClear(); 
                        for (int i = 0; i < 5; ++i){
                            npSetLED(Um[i], 0, 0, 100);
                        }    
                        npWrite();
                }else if (str[0] == '2'){         // 2
                        npClear(); 
                        for (int i = 0; i < 11; ++i){
                            npSetLED(Dois[i], 0, 0, 100);
                        }    
                        npWrite();                
                }else if (str[0] == '3'){         // 3
                        npClear(); 
                        for (int i = 0; i < 11; ++i){
                            npSetLED(Tres[i], 0, 0, 100);
                        }    
                        npWrite();                
                }else if (str[0] == '4'){         // 4
                        npClear(); 
                        for (int i = 0; i < 9; ++i){
                            npSetLED(Quatro[i], 0, 0, 100);
                        }    
                        npWrite();
                }else if (str[0] == '5'){         // 5
                        npClear(); 
                        for (int i = 0; i < 11; ++i){
                            npSetLED(Cinco[i], 0, 0, 100);
                        }    
                        npWrite();                
                }else if (str[0] == '6'){         // 6
                        npClear(); 
                        for (int i = 0; i < 12; ++i){
                            npSetLED(Seis[i], 0, 0, 100);
                        }    
                        npWrite();
                }else if (str[0] == '7'){         // 7
                        npClear(); 
                        for (int i = 0; i < 7; ++i){
                            npSetLED(Sete[i], 0, 0, 100);
                        }    
                        npWrite();                
                }else if (str[0] == '8'){         // 8
                        npClear(); 
                        for (int i = 0; i < 13; ++i){
                            npSetLED(Oito[i], 0, 0, 100);
                        }    
                        npWrite();
                }else if (str[0] == '9'){         // 9
                        npClear(); 
                        for (int i = 0; i < 12; ++i){
                            npSetLED(Nove[i], 0, 0, 100);
                        }    
                        npWrite();
                }
                else{                             //Apaga
                        npClear();
                        npWrite();
                }
            }
        }
        sleep_ms(100);    
    }
}

void gpio_irq(uint gpio, uint32_t events)
{   
    switch (gpio)
    {
        case BOTAO_A: //Botão responsável por acender LED VERDE
            if (TEMPO - TEMPODEBOUNCE > TEMPO_DEBOUNCE){
                bool lig_verde = gpio_get(LED_VERDE);
                gpio_put(LED_VERDE, !lig_verde); 
                         
                // Limpa o display. O display inicia com todos os pixels apagados.
                ssd1306_fill(&ssd, false);
                ssd1306_send_data(&ssd);

                //Escreve mensagem display
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha um retângulo
                ssd1306_draw_string(&ssd, "LED VERDE", 30, 20); // Desenha uma string
                if (lig_verde){
                    ssd1306_draw_string(&ssd, "Desligado", 31, 40);
                    printf("LED VERDE Desligado\n");
                }else{
                    ssd1306_draw_string(&ssd, "Ligado", 42, 40);
                    printf("LED VERDE Ligado\n");
                }
                ssd1306_send_data(&ssd); // Atualiza o display
            }
            break;
        case BOTAO_B: //Botão responsável por acender LED AZUL
            if (TEMPO - TEMPODEBOUNCE > TEMPO_DEBOUNCE){
                bool lig_azul = gpio_get(LED_AZUL);
                gpio_put(LED_AZUL, !lig_azul);

                // Limpa o display. O display inicia com todos os pixels apagados.
                ssd1306_fill(&ssd, false);
                ssd1306_send_data(&ssd);

                //Escreve mensagem display
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha um retângulo
                ssd1306_draw_string(&ssd, "LED AZUL", 33, 20); // Desenha uma string
                if (lig_azul){
                    ssd1306_draw_string(&ssd, "Desligado", 29, 40);
                    printf("LED AZUL Desligado\n");
                }else{
                    ssd1306_draw_string(&ssd, "Ligado", 42, 40);
                    printf("LED AZUL Ligado\n");
                }
                ssd1306_send_data(&ssd); // Atualiza o display            
            }            
            break;
    }
    TEMPODEBOUNCE = TEMPO;
}