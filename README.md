Semáforo Inteligente com Raspberry Pi Pico
Projeto desenvolvido por Mateus Moreira da Silva para simular um sistema de semáforo inclusivo, utilizando o Raspberry Pi Pico com suporte a FreeRTOS, LEDs RGB, matriz de LEDs, display OLED e buzzer.

Objetivo
O objetivo do projeto é criar um semáforo inteligente e acessível que ofereça sinalização visual e sonora, atendendo tanto pessoas sem deficiência quanto pessoas com deficiência visual. O sistema simula os estados de um semáforo urbano (vermelho, amarelo e verde) com transições programadas e também oferece um modo noturno de baixa intensidade.

Funcionalidades
Modo Normal (Automático)
PARE:

Display OLED com imagem e texto "PARE"

Matriz de LEDs em vermelho + contagem regressiva de 10s

LED RGB em vermelho

Buzzer: 0,5s ligado, 1,5s desligado

ATENÇÃO:

Display OLED com imagem e texto "ATENÇÃO"

Matriz de LEDs em amarelo por 4s

LED RGB em amarelo (vermelho + verde via PWM)

Buzzer: 0,5s ligado, 0,5s desligado

SIGA:

Display OLED com imagem e texto "SIGA"

Matriz de LEDs em verde + contagem regressiva de 6s

LED RGB em verde

Buzzer: 1s ligado, 1s desligado

Modo Noturno
Display OLED com texto “Modo Noturno”

LEDs e matriz piscando em amarelo (1s intervalo)

Buzzer com bip lento: 0,1s ligado, 1,9s desligado

Multitarefa com FreeRTOS
O projeto usa FreeRTOS para controle paralelo dos periféricos:

Cada componente (OLED, matriz de LEDs, buzzer, LED RGB, botões) possui uma tarefa dedicada.

Garante resposta rápida e sincronização entre os sinais.

Controles por Botão
Botão A (GPIO 5): alterna entre modo normal e modo noturno.

Botão B (GPIO 6): entra em modo BOOTSEL (reprogramação via USB).

Componentes e GPIOs Utilizados
Componente	GPIO	Função
Display OLED	14/15	Exibição de estado
Matriz de LEDs 5x5	7	Feedback visual + contagem
Buzzer	21	Sinalização sonora
LED RGB (PWM)	11/13	Iluminação semafórica
Botão A	5	Alternância de modo
Botão B	6	Acesso ao modo BOOTSEL

Técnicas Implementadas
PWM: controle de intensidade dos LEDs RGB

Debounce: para evitar múltiplos acionamentos falsos nos botões

Interrupção: para resposta imediata ao Botão B

Código e Vídeo

Github: https://github.com/Mateus-MDS/Sem-faro_Inteligente.git
Youtube: https://youtu.be/QejOpELxVy8
