// Author: Ayslenon - 2019
// Game genius running on atmega328p 

//=============================================================================================================
// libs includes
//=============================================================================================================
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


//=============================================================================================================
// creating variables
//=============================================================================================================

int max_rounds = 4; 

int ovf0 = 0, difficulty_level = 0, pressed_button = 0, read_time = 0;

boolean gameover = false, answered = false, timeout = false,
        flagbutton1 = false, flagbutton2 = false, flagbutton3 = false, flagbutton4 = false;


//=============================================================================================================
// declaring functions
//=============================================================================================================

// function to initialize timer 0 (8bit timer)
void timers_ini(void) {
  TCCR0B |=   0x05;  //frequency divided by 1024 -> machine cicle: 1024/16 MHz = 64 us
  TCNT0   =   0x00;  //starts tnct counter on 0, it counts until 255
  TIMSK0 |=   0x01;  //discover timer mask interruption
  ovf0    =      0;  //variable that stores number of overflows counted
  sei();             //enable global interrupt key
}

// initialize adc and starts the first conversion
void ADC_init (void)
{
  ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADMUX |= (1 << REFS0);
}

// start a new conversion on channel 0, waits until its done and then returns
uint16_t ADC_read_LDR(void)
{
  ADMUX &= 0xF0;
  ADMUX |= 0x00;
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADCW;
}

// start a new conversion on channel 0, waits until its done and then returns
uint16_t ADC_read_POT (void)
{
  ADMUX &= 0xF0;
  ADMUX |= 0x01;
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADCW;
}

// make a brazilian electronic urn sound via buzzer
void urn_sound (void) {
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

// function to treat interruptions from timer 0 overflows
ISR (TIMER0_OVF_vect) {
  // time between 2 interrupts is aproximately 16.4 ms (1024*256/16MHz)
  // if a new game didnt start yet we can increase or decrease the number of rounds by pressing button 1 or button 2
  if (gameover) {
    if ((PINB & 0x01) && (!flagbutton1)) {
      flagbutton1 = true;
    }
    else if ((PINB & 0x02) && (!flagbutton2)) {
      flagbutton2 = true;
    }
  }

  // to avoid bouncing, we wait 5 overflows after press button1 or button2
  // its aproximately 82ms
  // when the button is released the max_rounds increases or descreases 
  if ((gameover) && (read_time > 5)) {
    if ((!(PINB & 0x01)) && (flagbutton1)) {
      max_rounds ++;
      Serial.println(max_rounds);
      read_time = 0;
      flagbutton1 = false;
    }
    else if ((!(PINB & 0x02)) && (flagbutton2)) {
      max_rounds--;
      Serial.println(max_rounds);
      if (max_rounds == 0) {
        max_rounds = 255;
      }
      read_time = 0;
      flagbutton2 = false;
    }
  }

  // if a new game started, and the player have to respond the sequence this part will execute
  if ((!timeout) && (!answered) && (read_time == 0)) {
    if ((PINB & 0x01) && (!flagbutton1)) {
      flagbutton1 = true;
    }
    else if ((PINB & 0x02) && (!flagbutton2)) {
      flagbutton2 = true;
    }
    else if ((PINB & 0x04) && (!flagbutton3)) {
      flagbutton3 = true;
    }
    else if ((PINB & 0x08) && (!flagbutton4)) {
      flagbutton4 = true;
    }
  }

  // here is the counter for avoid bouncing
  if ((flagbutton1) || (flagbutton2) || (flagbutton3) || (flagbutton4)) {
    read_time++;
  }

  // after 82ms and the player release the button, the pressed button is 'returned'
  if ((!timeout) && (!answered) && (read_time > 5)) {
    if ((!(PINB & 0x01)) && (flagbutton1)) {
      pressed_button = 1;
      answered = true;
      read_time = 0;
      flagbutton1 = false;
    }
    else if ((!(PINB & 0x02)) && (flagbutton2)) {
      pressed_button = 2;
      answered = true;
      read_time = 0;
      flagbutton2 = false;
    }
    else if ((!(PINB & 0x04)) && (flagbutton3)) {
      pressed_button = 4;
      answered = true;
      read_time = 0;
      flagbutton3 = false;
    }
    else if ((!(PINB & 0x08)) && (flagbutton4)) {
      pressed_button = 8;
      answered = true;
      read_time = 0;
      flagbutton4 = false;
    }
  }

  // if the player didnt answer yet, we start a count, it counts aproximately 3s, the player must answer within this time
  // if dont, the game ends
  if (!answered) {
    ovf0++; 
    if (ovf0 >= 183) {
      timeout = true;
      ovf0 = 0;
    }
  }
  else {
    ovf0 = 0;
  }
}

// main function
int main () {
  Serial.begin(9600);
  int main_counter = 0;
  // initialize adc and timers
  ADC_init();
  timers_ini();
  // setup from all outputs and inputs
  // output pins are from 4 to 7 for leds, pin3 for buzzer
  // pin 2 is a input button to start the system
  // pin 11 is a pwm pin connected to a led
  // pin 8 from 12 are buttons from responses
  // pin a0 is a input from ldr, and a1 is input from potentiometer
  DDRD |= 0xF8; // pinos 4 5 6 e 7 sao os leds // pino 3 vai ser o buzzer // pino 2 botao liga
  DDRB |= 0x00; // pinos 8 9 10 12 serao botoes
  // set all leds to low level
  PORTD = 0;

  // start a infinity loop to run the game 
  while (1) {
    // start new vectors, with the length of max rounds permmited
    // sequence shows how the leds will light 
    int sequence [max_rounds];
    // response stores the sequence of answers for each round
    int response  [max_rounds];

    
    difficulty_level = ADC_read_POT(); 
    
    // gameover indicate if the game ended (true by default)
    gameover = true;
    // answered indicate if the player answered the actual round
    answered = true;
    // timeout indicate if the player didnt answer in the right time
    timeout = true;

    // if the start button is pressed, a new game starts
    if (PIND & 0x04) { 
      main_counter = 0;
      gameover = false;

      // we start a new loop, it indicates the actual round in the game 
      // in each round the player will press buttons in sequence 
      // correspondig to the order of leds that were lighted up
      // if the player miss the sequence, the game ends, or if the player dont press any button
      for (main_counter; main_counter < max_rounds; main_counter++) {
        
        // to discover what led needs to be lighed up, we create a random value
        // and store on the sequence vector, in the main_counter position
        
        int rand_value = (rand() + (rand() / ADC_read_LDR()) + (ADC_read_LDR () / 10)) % 4;
        sequence[main_counter] = 1 << rand_value;

        // this for loop light the leds correspondig to its sequence  
        for (int showLevel = 0; showLevel <= main_counter; showLevel++) {
          if (sequence[showLevel] == 1) PORTD = 0x10;
          else if (sequence[showLevel] == 2) PORTD = 0x20;
          else if (sequence[showLevel] == 4) PORTD = 0x40;
          else if (sequence[showLevel] == 8) PORTD = 0x80;
          _delay_ms(200 + (difficulty_level - 223)*0.5);
          PORTD = 0x00;
          _delay_ms(50);
        }

        // this command is to reset the answer time
        answered = true; 
        
        // start a new loop for the player answer the sequence 
        for (int answerLevel = 0; answerLevel <= main_counter; answerLevel++) {
          timeout = false;
          answered = false; 
          pressed_button = 0;
          // this while loop waits until any button is pressed, and if dont, the timeout flag will be seted
          while (!timeout && !answered) {
            Serial.print("");
          }
          Serial.println("");
          // stores the pressed button on the vector response
          response[answerLevel] = pressed_button;

          // if the player didnt answer or choose the wrong led the game ends
          if ((timeout) || ((!timeout) && (sequence[answerLevel] != response[answerLevel]))) {
            answerLevel = main_counter + 1;
            main_counter = max_rounds + 1;
            PORTD = 0x00;
            // blink all the leds  
            for (int p = 0; p < 6; p ++) {
              PORTD ^= 0xF0;
              _delay_ms(50);
            }
            
            PORTD = 0x00;
            gameover = true;
            break;
          }
        }
      }
      // if the player win the game, without any problem, play a 'win song'
      if (!gameover) urn_sound ();
    }
  }
}
