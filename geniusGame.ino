#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int numero_de_rounds = 4;

int ovf0 = 0, nivel_de_dificuldade = 0, botao_apertado = 0, pisca = 0, tempo_leitura = 0, paradinha = 0;

boolean gameover = false, answer = false, timeout = false, tempo_de_piscar = false, eh_pra_piscar = false,
        flagbotao1 = false, flagbotao2 = false, flagbotao3 = false, flagbotao4 = false, para_um_pouqim = false;


void timers_ini(void) {
  TCCR0B |=   0x05;  //frequencia em 16 MHz/1024 -> ciclo de maquina: 1024/16 MHz = 64 us
  TCNT0   =   0x00;  //consegue contar 2^8  -> 256   vezes
  TIMSK0 |=   0x01;  //habilita a flag de aviso de overflow
  ovf0    =      0;  //zera a variável de contagem de estouros para o timer 0
  sei();             //habilita a chave global dos timers
}

void PWMpin11(uint16_t valor) {
  OCR2B = valor;                                          // Configurar o modo de compracao (ativar o PWM)
  TCCR2A |= (1 << COM2B1);                                // Set modo clear
  TCCR2A |= (1 << WGM21) | (1 << WGM20);                  // Configura a forma de onda para o modo Fast PWM
  TCCR2B |= (1 << CS21);                                  // Configura Prescale (divide o clock por 8)
}


void ADC_init (void)
{
  ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADMUX |= (1 << REFS0);
}

uint16_t ADC_read_LDR(void) // canal 0
{
  ADMUX &= 0xF0;
  ADMUX |= 0x00;
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADCW;
}

uint16_t ADC_read_POT (void) // canal 1
{
  ADMUX &= 0xF0;
  ADMUX |= 0x01;
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADCW;
}

void som_de_urna (void) {
  _delay_ms (300);
  int buzzer = 3;
  for (int j = 0; j < 4; j ++) {
    tone(buzzer, 2332);
    _delay_ms(80);
    tone(buzzer, 2432);
    _delay_ms(80);
  }
  tone(buzzer, 2332);
  _delay_ms(100);
  noTone (buzzer);
}


ISR (TIMER0_OVF_vect) {
  // 1 segundo /((1024 prescaler *256 counter)/16MHz) ~= 61.03515 estouros para 1s
  // para 1 ms -> 0.061 contagens. e multiplicando por 177+niveldedificuldade ms teremos a quantidade de estouros necessarios
  if (eh_pra_piscar) {
    pisca ++;
    if (pisca >= (144 + nivel_de_dificuldade) * 0.03) {
      tempo_de_piscar = false;
      pisca = 0;
      PORTD = 0x00;
    }
  }
  else pisca = 0;

  if (gameover) {
    if ((PINB & 0x01) && (!flagbotao1)) {
      flagbotao1 = true;
    }
    else if ((PINB & 0x02) && (!flagbotao2)) {
      flagbotao2 = true;
    }
  }

  if ((gameover) && (tempo_leitura > 5)) {
    if ((!(PINB & 0x01)) && (flagbotao1)) {
      numero_de_rounds ++;
      Serial.println(numero_de_rounds);
      tempo_leitura = 0;
      flagbotao1 = false;
    }
    else if ((!(PINB & 0x02)) && (flagbotao2)) {
      numero_de_rounds--;
      Serial.println(numero_de_rounds);
      if (numero_de_rounds == 0) {
        numero_de_rounds = 255;
      }
      tempo_leitura = 0;
      flagbotao2 = false;
    }
  }

  if (para_um_pouqim) {
    paradinha++;
    if (paradinha >= 3) {
      para_um_pouqim = false;
      paradinha = 0;
    }
  }
  else paradinha = 0;

  if ((!timeout) && (!answer) && (tempo_leitura == 0)) {
    if ((PINB & 0x01) && (!flagbotao1)) {
      flagbotao1 = true;
    }
    else if ((PINB & 0x02) && (!flagbotao2)) {
      flagbotao2 = true;
    }
    else if ((PINB & 0x04) && (!flagbotao3)) {
      flagbotao3 = true;
    }
    else if ((PINB & 0x08) && (!flagbotao4)) {
      flagbotao4 = true;
    }
  }

  if ((flagbotao1) || (flagbotao2) || (flagbotao3) || (flagbotao4)) {
    tempo_leitura++;
  }


  if ((!timeout) && (!answer) && (tempo_leitura > 5)) {
    if ((!(PINB & 0x01)) && (flagbotao1)) {
      botao_apertado = 1;
      answer = true;
      tempo_leitura = 0;
      flagbotao1 = false;
    }
    else if ((!(PINB & 0x02)) && (flagbotao2)) {
      botao_apertado = 2;
      answer = true;
      tempo_leitura = 0;
      flagbotao2 = false;
    }
    else if ((!(PINB & 0x04)) && (flagbotao3)) {
      botao_apertado = 4;
      answer = true;
      tempo_leitura = 0;
      flagbotao3 = false;
    }
    else if ((!(PINB & 0x08)) && (flagbotao4)) {
      botao_apertado = 8;
      answer = true;
      tempo_leitura = 0;
      flagbotao4 = false;
    }
  }


  if (!answer) {
    ovf0++; // 183 gera uma base de tempo para timeout de aproximadadamente 3 segundos para apertar um botao
    if (ovf0 >= 183) {
      timeout = true;
      ovf0 = 0;
    }
  }
  else {
    ovf0 = 0;
  }
}

int main () {
  Serial.begin(9600);
  int main_counter = 0;
  ADC_init();
  timers_ini();
  DDRD |= 0xF8; // pinos 4 5 6 e 7 sao os leds // pino 3 vai ser o buzzer // pino 2 botao liga
  DDRB |= 0x80; // pino 11 é o pwm  // pinos 8 9 10 12 serao botoes
  PORTD = 0;
  while (1) {

    int sequencia [numero_de_rounds];
    int resposta  [numero_de_rounds];

    nivel_de_dificuldade = ADC_read_POT(); // usar este valor depois
    gameover = true;
    answer = true;
    timeout = true;

    if (PIND & 0x04) { // entrar em novo jogo
      main_counter = 0;
      gameover = false;
      for (main_counter; main_counter < numero_de_rounds; main_counter++) {
        sequencia[main_counter] = 1;
        int valor_ldr   = ADC_read_LDR();
        int valor_aleat = (rand() + (rand() / ADC_read_LDR()) + (ADC_read_LDR () / 10)) % 4;
        for (int aux = 0; aux < valor_aleat; aux++) {
          sequencia[main_counter] *= 0x02;
        }
        for (int i = 0; i <= main_counter; i++) {
          if (sequencia[i] == 1) PORTD = 0x10;
          else if (sequencia[i] == 2) PORTD = 0x20;
          else if (sequencia[i] == 4) PORTD = 0x40;
          else if (sequencia[i] == 8) PORTD = 0x80;

          eh_pra_piscar = true;
          tempo_de_piscar = true;
          while (tempo_de_piscar) {
            Serial.print("");
          }
          PORTD = 0x00;
          para_um_pouqim = true;
          while (para_um_pouqim) {
            Serial.print("");
          }
          Serial.println("");
          eh_pra_piscar = false;
        }
        answer = true; // garante que ovf0 vai zerar antes da proxima contagem
        for (int o = 0; o <= main_counter; o++) {
          timeout = false;
          answer = false; // estou gerando esta linha na isr apos soltar o botao
          botao_apertado = 0;
          while (!timeout && !answer) {
            Serial.print("");
          }
          // colocar delay para separar leituras
          Serial.println("");
          // configurar o timeout a partir da ISR ( ajustar de acordo com o valor do POT )
          //usar timeout na ISR para habilitar a espera pelo proximo botao e identificar que botao foi esse

          resposta[o] = botao_apertado;

          if ((timeout) || ((!timeout) && (sequencia[o] != resposta[o]))) {
            o = main_counter + 1;
            main_counter = numero_de_rounds + 1;
            PORTD = 0x00;
            for (int p = 0; p < 6; p ++) {
              PWMpin11(p * 40);
              PORTD ^= 0xF0;
              para_um_pouqim = true;
              while (para_um_pouqim) {
                Serial.print("");
              }
              Serial.println("");
            }
            PWMpin11(0);
            PORTD = 0x00;
            gameover = true;
            break;
          }
        }
      }
      PWMpin11(0);
      if (!gameover) som_de_urna (); // criar a funcao p tocar a urna ao vencer o jogo // botar na ISR os leds para piscarem (!gameover && main_counter >= numero_de_rounds)
    }
  }
}
