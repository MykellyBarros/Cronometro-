#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "nokia5110.h"
#include <stdio.h>

#define B1_PIN PB6 // B1 (Botão 1) agora está em B6
#define B2_PIN PD6 // B2 (Botão 2) agora está em D6

uint8_t glyph[] = {
    0b11111111,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b11111111};

volatile uint16_t milliseconds = 0;
volatile uint8_t running = 0; // Flag para indicar se o cronômetro está em execução

void init_buttons()
{
    // Configurar pinos como entrada (botões)
    DDRB &= ~(1 << B1_PIN);
    DDRD &= ~(1 << B2_PIN);
    // Ativar pull-up interno
    PORTB |= (1 << B1_PIN);
    PORTD |= (1 << B2_PIN);
}

// Função para verificar o estado do botão B1 com debouncing
int is_b1_pressed()
{
    static uint8_t state = 0; // Estado atual do botão
    state = (state << 1) | bit_is_clear(PINB, B1_PIN) | 0xE0;
    if (state == 0xF0)
    {
        _delay_ms(10); // Debounce
        return 1;
    }
    return 0;
}

// Função para verificar o estado do botão B2 com debouncing
int is_b2_pressed()
{
    static uint8_t state = 0; // Estado atual do botão
    state = (state << 1) | bit_is_clear(PIND, B2_PIN) | 0xE0;
    if (state == 0xF0)
    {
        _delay_ms(10); // Debounce
        return 1;
    }
    return 0;
}

void update_timer()
{
    char timer_str[15];
    uint16_t minutes = milliseconds / (60UL * 1000);
    uint16_t seconds = (milliseconds / 1000) % 60;
    uint16_t millis = milliseconds % 1000;

    nokia_lcd_custom(1, glyph);

    char status_char = (running) ? '>' : '<'; // Triângulo para direita se contando, para esquerda se parado
    sprintf(timer_str, "%c %02d:%02d:%03d", status_char, minutes, seconds, millis);

    nokia_lcd_clear();
    nokia_lcd_write_string(timer_str, 1); // Usando fonte de tamanho 2
    nokia_lcd_render();
}

ISR(TIMER1_COMPA_vect)
{
    if (running)
    {
        milliseconds++;
    }
}

int main(void)
{
    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(1, glyph);

    init_buttons();

    // Configurar Timer/Counter1 para interrupções de 1 ms
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); // Modo CTC, prescaler 64
    OCR1A = 249;                                        // Interrupção a cada 1 ms para frequência de CPU de 16MHz
    TIMSK1 |= (1 << OCIE1A);                            // Habilitar interrupção Timer/Counter1 Compare Match A

    sei(); // Habilitar interrupções globais

    while (1)
    {
        // Verificar o estado do botão B1
        if (is_b1_pressed())
        {
            if (running)
            {
                running = 0; // Se estiver rodando, pause
            }
            else
            {
                running = 1; // Se não estiver rodando, inicie
            }
        }

        // Verificar o estado do botão B2
        if (is_b2_pressed())
        {
            if (!running)
            {
                // Somente resetar se o cronômetro estiver pausado
                milliseconds = 0;
            }
        }

        // Atualizar o cronômetro
        update_timer();
    }
}