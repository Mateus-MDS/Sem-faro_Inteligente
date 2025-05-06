// Inclusão de bibliotecas necessárias
#include "pico/stdlib.h"      // Biblioteca padrão do Raspberry Pi Pico
#include "hardware/gpio.h"    // Controle de hardware para GPIO
#include "hardware/i2c.h"     // Controle de hardware para I2C
#include "hardware/pwm.h"     // Controle de hardware para PWM
#include "lib/ssd1306.h"      // Driver para displays OLED SSD1306
#include "lib/font.h"         // Biblioteca de fontes para o display
#include "FreeRTOS.h"         // Kernel do FreeRTOS
#include "FreeRTOSConfig.h"   // Arquivo de configuração do FreeRTOS
#include "task.h"             // Funções para criação e gerenciamento de tarefas
#include <stdio.h>            // Biblioteca padrão de entrada/saída
#include "pico/bootrom.h"     // Para funções de bootsel
#include "hardware/clocks.h"  // Controle de clocks do hardware
#include "animacoes_led.pio.h"// Programa PIO para animações de LED
#include "hardware/pio.h"     // Interface para Programmable I/O (PIO)

// Definições para comunicação I2C
#define I2C_PORT i2c1  // Porta I2C a ser utilizada
#define I2C_SDA 14     // Pino para dados I2C (SDA)
#define I2C_SCL 15     // Pino para clock I2C (SCL)
#define endereco 0x3C  // Endereço I2C do display OLED

// Configuração do periféricos
#define led_green 11  // Pino do LED verde
#define led_red 13    // Pino do LED vermelho
#define botao_A 5     // Pino do botão A
#define botao_B 6     // Pino do botão B
#define buzzer 21     // Pino do buzzer

// Configurações da matriz de LEDs
#define NUM_PIXELS 25  // Quantidade de LEDs na matriz
#define matriz_leds 7  // Pino de controle da matriz de LEDs

// Variáveis globais
PIO pio;            // Controlador PIO (Programmable I/O)
uint sm;            // State Machine (Máquina de Estados) do PIO
uint contagem = 0;  // Contador para exibição na matriz

volatile bool estado_botao_A = true;  // Estado do botão A (true=ligado, false=desligado)

const char* cor;  // Variável para armazenar a cor atual
ssd1306_t ssd;    // Estrutura para controle do display OLED

// Função para inicializar LEDs com PWM
void iniciar_leds_PWM(){

    // Inicializa pino do LED verde
    gpio_init(led_green);
    gpio_set_dir(led_green, GPIO_OUT);

    // Inicializa pino do LED vermelho
    gpio_init(led_red);
    gpio_set_dir(led_red, GPIO_OUT);

    // Configurações do PWM
    const uint16_t WRAP_PERIOD = 4000; // Período máximo do contador PWM
    const float PWM_DIVISER = 50.0; // Divisor de clock para o PWM

    // Configura função PWM nos pinos
    gpio_set_function(led_red, GPIO_FUNC_PWM);
    gpio_set_function(led_green, GPIO_FUNC_PWM);

    // Obtém os canais PWM para cada LED
    uint slice_red = pwm_gpio_to_slice_num(led_red);
    uint slice_green = pwm_gpio_to_slice_num(led_green);

    // Configura divisão de clock do PWM
    pwm_set_clkdiv(slice_red, PWM_DIVISER);
    pwm_set_clkdiv(slice_green, PWM_DIVISER);

    // Configura período do PWM
    pwm_set_wrap(slice_red, WRAP_PERIOD);
    pwm_set_wrap(slice_green, WRAP_PERIOD);

    // Habilita o PWM
    pwm_set_enabled(slice_red, true);
    pwm_set_enabled(slice_green, true);
}

// Função para inicializar o display OLED
void iniciar_display(){

    // Inicializa interface I2C a 400kHz
    i2c_init(I2C_PORT, 400 * 1000);

    // Configura pinos I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);  // Habilita pull-up no SDA
    gpio_pull_up(I2C_SCL);  // Habilita pull-up no SCL
    
    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);  // Configura o display
    ssd1306_send_data(&ssd);  // Envia dados para o display
    
    // Limpa o display (todos pixels desligados)
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

/* ========== FUNÇÕES DE CONTROLE DE LEDS ========== */

// Converte intensidade em valor RGB para matriz de LEDs
uint32_t matrix_rgb(double intensity) {
    unsigned char value = intensity * 100;
    return (value << 16) | (value << 8) | value;
}

// Matriz com representações dos números 0-9 para display 5x5
double numeros[11][25] = {
    {0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0}, //0
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0}, //1
    {1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0}, //2
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, //3
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1}, //4
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1}, //5
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1}, //6
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1}, //7
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1}, //8
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1}, //9
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} //todos LEDs apagados
};

/* ========== FUNÇÕES DE CONTROLE DE LEDS ========== */

// Atualiza a matriz de LEDs com o número atual
void Contagem_matriz_leds() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t valor_led = matrix_rgb(numeros[contagem][i]);  
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Converte nome da cor em valor RGB
uint32_t cor_semafaro_para_rgb(const char* cor) {
    if (strcmp(cor, "Preto") == 0)    return 0x000000FF;
    if (strcmp(cor, "Vermelho") == 0) return 0x00FF0000;
    if (strcmp(cor, "Amarelo") == 0)  return 0xFFFF0000;
    if (strcmp(cor, "Verde") == 0)    return 0xFF000000;
    return 0x000000; // Cor padrão (preto)
}

// Liga toda a matriz de LEDs com a cor atual
void Ligar_matriz_leds() {
    uint32_t cor_rgb = cor_semafaro_para_rgb(cor);
    for (int i = 0; i < NUM_PIXELS; i++) {   
        uint32_t valor_led = cor_rgb;
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Funções de som para cada estado do semáforo
void Som_semafaro_verde(){
    for (int i = 0; i < 3; i++) {
        gpio_put(buzzer, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_put(buzzer, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (!estado_botao_A) break;
    }
}

void Som_semafaro_amarelo(){
    for (int i = 0; i < 4; i++) {
        gpio_put(buzzer, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(buzzer, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
        if (!estado_botao_A) break;
    }
}

void Som_semafaro_vermelho(){
    for (int i = 0; i < 5; i++) {
        gpio_put(buzzer, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(buzzer, 0);
        vTaskDelay(pdMS_TO_TICKS(1500));
        if (!estado_botao_A) break;
    }
}

void Modo_noturno(){
    gpio_put(buzzer, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_put(buzzer, 0);
    vTaskDelay(pdMS_TO_TICKS(1900));
}

// Funções de exibição visual para cada estado do semáforo
void semafaro_verde(ssd1306_t *ssd){
    for (int i = 0; i < 6; i++) {
        bool cor_display = true;
        ssd1306_fill(ssd, !cor_display);                               // Apaga o display

        // Desenhando o semafáro
        ssd1306_rect(ssd, 3, 3, 38, 60, cor_display, !cor_display);    // Desenhar o retângulo maior
        ssd1306_line(ssd, 3, 24, 38, 24, cor_display);                 // Linha do meio 1
        ssd1306_line(ssd, 3, 43, 38, 43, cor_display);                 // Linha do meio 2
        ssd1306_rect(ssd, 8, 10, 22, 13, cor_display, !cor_display);   // Retângulo do Pare, sem preenchemento
        ssd1306_rect(ssd, 28, 10, 22, 13, cor_display, !cor_display);  // Retângulo do Atenção, sem preenchemento
        ssd1306_rect(ssd, 47, 10, 22, 13, cor_display, cor_display);   // Retângulo do Siga, com preenchemento
        ssd1306_draw_string(ssd, "SIGA", 50, 52);                      // Desenha a palavra siga na frente do seu retângulo
        ssd1306_send_data(ssd);                                        // Envia os dados para o display
        vTaskDelay(pdMS_TO_TICKS(1000));                               // Delay de 1 segundo
        if (!estado_botao_A) break;                                    // Verifica se ouve mudança de modo
    }
}

void semafaro_vermelho(ssd1306_t *ssd){
    for(int i = 0; i < 10; i++){
        bool cor_display = true;
        ssd1306_fill(ssd, !cor_display);                               // Apaga o display

        // Desenhando o semafáro
        ssd1306_rect(ssd, 3, 3, 38, 60, cor_display, !cor_display);    // Desenhar o retângulo maior
        ssd1306_line(ssd, 3, 24, 38, 24, cor_display);                 // Linha do meio 1
        ssd1306_line(ssd, 3, 43, 38, 43, cor_display);                 // Linha do meio 2
        ssd1306_rect(ssd, 8, 10, 22, 13, cor_display, cor_display);    // Retângulo do Pare, com preenchemento
        ssd1306_rect(ssd, 28, 10, 22, 13, cor_display, !cor_display);  // Retângulo do Atenção, sem preenchemento
        ssd1306_rect(ssd, 47, 10, 22, 13, cor_display, !cor_display);  // Retângulo do Siga, sem preenchemento
        ssd1306_draw_string(ssd, "PARE", 50, 10);                      // Desenha a palavra Pare na frente do seu retângulo
        ssd1306_send_data(ssd);                                        // Envia os dados para o display
        vTaskDelay(pdMS_TO_TICKS(1000));                               // Delay de 1 segundo
        if (!estado_botao_A) break;                                    // Verifica se ouve mudança de modo
    }
}

void semafaro_amarelo(ssd1306_t *ssd){
    for(int i = 0; i < 4; i++){
        bool cor_display = true;
        ssd1306_fill(ssd, !cor_display);                               // Apaga o display

        // Desenhando o semafáro
        ssd1306_rect(ssd, 3, 3, 38, 60, cor_display, !cor_display);    // Desenhar o retângulo maior
        ssd1306_line(ssd, 3, 24, 38, 24, cor_display);                 // Linha do meio 1
        ssd1306_line(ssd, 3, 43, 38, 43, cor_display);                 // Linha do meio 2
        ssd1306_rect(ssd, 8, 10, 22, 13, cor_display, !cor_display);   // Retângulo do Pare, sem preenchemento
        ssd1306_rect(ssd, 28, 10, 22, 13, cor_display, cor_display);   // Retângulo do Atenção, com preenchemento
        ssd1306_rect(ssd, 47, 10, 22, 13, cor_display, !cor_display);  // Retângulo do Siga, sem preenchemento
        ssd1306_draw_string(ssd, "ATENCAO", 50, 30);                   // Desenha a palavra atenção na frente do seu retângulo
        ssd1306_send_data(ssd);                                        // Envia os dados para o display
        vTaskDelay(pdMS_TO_TICKS(1000));                               // Delay de 1 segundo
        if (!estado_botao_A) break;                                    // Verifica se ouve mudança de modo
    }
}

void semafaro_modo_noturno(ssd1306_t *ssd){
    bool cor_display = true;
    ssd1306_fill(ssd, !cor_display);                               // Apaga o display

    // Desenhando o semafáro
    ssd1306_rect(ssd, 3, 3, 38, 60, cor_display, !cor_display);    // Desenhar o retângulo maior
    ssd1306_line(ssd, 3, 24, 38, 24, cor_display);                 // Linha do meio 1
    ssd1306_line(ssd, 3, 43, 38, 43, cor_display);                 // Linha do meio 2
    ssd1306_rect(ssd, 8, 10, 22, 13, cor_display, !cor_display);   // Retângulo do Pare, sem preenchemento
    ssd1306_rect(ssd, 28, 10, 22, 13, cor_display, !cor_display);  // Retângulo do Atenção, com preenchemento
    ssd1306_rect(ssd, 47, 10, 22, 13, cor_display, !cor_display);  // Retângulo do Siga, sem preenchemento
    ssd1306_draw_string(ssd, "MODO", 65, 30);                      // Desenha a palavra modo no display
    ssd1306_draw_string(ssd, "NOTURNO", 55, 40);                   // Desenha a palavra noturno no display
    ssd1306_send_data(ssd);                                        // Envia os dados para o display
    vTaskDelay(pdMS_TO_TICKS(2000));                               // Delay de 2 segundos
}

// Funções de temporização para cada estado do semáforo
void Tempo_sinal_vermelho(){
    contagem = 10;
    cor = "Vermelho";
    Ligar_matriz_leds();
    
    for(int i = 0; i < 10; i++){
        Contagem_matriz_leds();
        vTaskDelay(pdMS_TO_TICKS(1000));
        contagem--;
        if (!estado_botao_A) break;
    }
}

void Tempo_sinal_amarelo(){
    for(int i = 0; i < 4; i++){
        cor = "Amarelo";
        Ligar_matriz_leds();
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (!estado_botao_A) break;
    }
}

void Tempo_sinal_verde(){
    contagem = 6;
    cor = "Verde";
    Ligar_matriz_leds();
    
    for(int i = 0; i < 6; i++){
        Contagem_matriz_leds();
        vTaskDelay(pdMS_TO_TICKS(1000));
        contagem--;
        if (!estado_botao_A) break;
    }
}

void Matriz_modo_noturno(){
    
    cor = "Amarelo";
    Ligar_matriz_leds();
    vTaskDelay(pdMS_TO_TICKS(1000));

    cor = "Preto";
    Ligar_matriz_leds();
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// Tarefa para monitorar o botão A
void vMudar_estado_botao_A(){
    while (true) {
        static uint32_t last_time = 0;
        uint32_t current_time = to_us_since_boot(get_absolute_time());

        // Debouncing de 300ms
        if (current_time - last_time > 300000) {
            last_time = current_time;
            if (!gpio_get(botao_A)) {
                estado_botao_A = !estado_botao_A; // Alterna estado do botão
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Tarefa para controlar os LEDs RGB
void vBlinkled_RGB()
{
    iniciar_leds_PWM();

    while (true)
    {
        if (estado_botao_A) {
            // Vermelho por 10 segundos
            for (int i = 0; i < 10; i++) {
                pwm_set_gpio_level(led_red, 100);
                pwm_set_gpio_level(led_green, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (!estado_botao_A) break;
            }
    
            if (!estado_botao_A) continue;
    
            // Amarelo por 4 segundos
            for (int i = 0; i < 4; i++) {
                pwm_set_gpio_level(led_red, 100);
                pwm_set_gpio_level(led_green, 100);
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (!estado_botao_A) break;
            }
    
            if (!estado_botao_A) continue;
    
            // Verde por 6 segundos
            for (int i = 0; i < 6; i++) {
                pwm_set_gpio_level(led_green, 100);
                pwm_set_gpio_level(led_red, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (!estado_botao_A) break;
            }
        } else {
            // Modo noturno: piscar amarelo
            pwm_set_gpio_level(led_red, 100);
            pwm_set_gpio_level(led_green, 100);
            vTaskDelay(pdMS_TO_TICKS(1000));
    
            pwm_set_gpio_level(led_red, 0);
            pwm_set_gpio_level(led_green, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// Tarefa para controlar o buzzer
void vBuserTask(){
    gpio_init(buzzer);
    gpio_set_dir(buzzer, GPIO_OUT);

    while (true){
        if (estado_botao_A){
            Som_semafaro_vermelho();
            Som_semafaro_amarelo();
            Som_semafaro_verde();
        }
        else{
            Modo_noturno();
        }
    }
}

// Tarefa para controlar a matriz de LEDs
void vMatriz_Leds(){
    while (true){
        if (estado_botao_A){
            Tempo_sinal_vermelho();
            Tempo_sinal_amarelo();
            Tempo_sinal_verde();
        }
        else{
            Matriz_modo_noturno();
        }
    }
}

// Tarefa para controlar o display OLED
void vDisplay3Task()
{
    iniciar_display();

    char str_y[5]; // Buffer para strings
    int contador = 0;
    while (true)
    {
        sprintf(str_y, "%d", contador);
        contador++;

        if (estado_botao_A){
            semafaro_vermelho(&ssd);
            semafaro_amarelo(&ssd);
            semafaro_verde(&ssd);
        }
        else{
            semafaro_modo_noturno(&ssd);
        }
    }
}

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events);

// Função principal
int main()
{
    // Configuração do botão B para modo BOOTSEL
    gpio_init(botao_B);
    gpio_set_dir(botao_B, GPIO_IN);
    gpio_pull_up(botao_B);
    gpio_set_irq_enabled_with_callback(botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    // Configuração do botão A
    gpio_init(botao_A);
    gpio_set_dir(botao_A, GPIO_IN);
    gpio_pull_up(botao_A);

    // Inicialização PIO para matriz de LEDs
    pio = pio0;
    uint offset = pio_add_program(pio, &animacoes_led_program);
    sm = pio_claim_unused_sm(pio, true);
    animacoes_led_program_init(pio, sm, offset, matriz_leds);

    stdio_init_all(); // Inicializa stdio

    // Criação das tarefas do FreeRTOS
    xTaskCreate(vMudar_estado_botao_A, "Blink Task Mudar Estado", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vBlinkled_RGB, "Blink Task led_RGB", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuserTask, "Blink Task Buser", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplay3Task, "Cont Task Disp3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vMatriz_Leds, "Cont Task Matriz_Leds", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    
    vTaskStartScheduler(); // Inicia o escalonador de tarefas
    panic_unsupported(); // Função de fallback
}

/* ========== HANDLER DE INTERRUPÇÃO ========== */

// Tratamento das interrupções dos botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Debouncing de 300ms
    if (current_time - last_time > 300000) {
        last_time = current_time;

        // Verifica se foi o botão B pressionado
        if (gpio == botao_B && !gpio_get(botao_B)) {
            reset_usb_boot(0, 0); // Entra no modo BOOTSEL
        }
    }
}