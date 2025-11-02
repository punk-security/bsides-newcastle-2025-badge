// 16Mhz, disable millis timer

// MORSE CODE CODES
/*
 *
 * MORSE CODE is between 1 and 5 pulses long
 *
 * To store this in one byte we have the length, (3 bits) and each position as either 0 (dot) or 1 (dash)
 * i.e.
 * A = 010 01 000
 * B = 100 1000 0
 * C = 100 1010 0
 */

#include <avr/io.h>-
#include <avr/pgmspace.h>
#include <EEPROM.h>

//Definitions

# define WAKE_TIME_MS 2400000

//COLOURS
#define OFF 0,0,0

#define RED 30,0,0
#define GREEN 0,8,0
#define BLUE 0,0,30
#define ORANGE 20,8,0
#define PINK 6,0,4
#define PURPLE 6,0,15
#define OLIVE 2,2,0
#define YELLOW 18,12,0
#define YELLOWGREEN 20,30,20

// LED Order

#define LED0 0
#define LED1 3
#define LED2 4
#define LED3 2
#define LED4 1

const uint8_t p[] = {LED0,LED1,LED2,LED3,LED4};

// A .-
#define m_A B01001000
// B -...
#define m_B B10010000
// C -.-.
#define m_C B10010100
// D -..
#define m_D B01110000
// E .
#define m_E B00100000
// F ..-.
#define m_F B10000100
// G --.
#define m_G B01111000
// H ....
#define m_H B10000000
// I ..
#define m_I B01000000
// J .---
#define m_J B10001110
// K -.-
#define m_K B01110100
// L .-..
#define m_L B10001000
// M --
#define m_M B01011000
// N -.
#define m_N B01010000
// O ---
#define m_O B01111100
// P .--.
#define m_P B10001100
// Q --.-
#define m_Q B10011010
// R .-.
#define m_R B01101000
// S ...
#define m_S B01100000
// T -
#define m_T B00110000
// U ..-
#define m_U B01100100
// V ...-
#define m_V B10000010
// W .--
#define m_W B01101100
// X -..-
#define m_X B10010010
// Y -.--
#define m_Y B10010110
// Z --..
#define m_Z B10011000
// 1 .----
#define m_1 B10101111
// 2 ..---
#define m_2 B10100111
// 3 ...--
#define m_3 B10100011
// 4 ....-
#define m_4 B10100001
// 5 .....
#define m_5 B10100000
// 6 -....
#define m_6 B10110000
// 7 --...
#define m_7 B10111000
// 8 ---..
#define m_8 B10111100
// 9 ----.
#define m_9 B10111110
// 0 -----
#define m_0 B10111111

// We need to time the high periods, and work out if we are a dot, dash or have finished the char / word
#define MORSE_CODE_PIN PIN_PA7
#define MORSE_CODE_MAX_DOT_MS 400 // LOW - Any longer than this and its a dash
#define MORCE_CODE_MAX_DASH_MS 2000 // LOW - Any longer than this and its an error
#define MORSE_CODE_MAX_CHAR_INTERVAL_MS 800 // LOW - Any longer than this and we end the character
#define MORSE_CODE_MAX_WORD_INTERVAL_MS MORSE_CODE_MAX_CHAR_INTERVAL_MS * 3 // LOW - Any longer than this and we end the secret code

#define MORSE_CODE_CHAR_ERROR 0

#define MORSE_CODE_MAX_HIGH_MS MORCE_CODE_MAX_DASH_MS


uint16_t time_pin_low(uint16_t max_ms)
{
  // blocking for up to max_ms
  if (digitalRead(MORSE_CODE_PIN) == HIGH)
  {
    return(0);
  }
  mini_sleep();
  mini_sleep();
  //delay(36); //debounce-
  uint16_t t = 36;
  while(digitalRead(MORSE_CODE_PIN) == LOW)
  {
    mini_sleep();
    //delay(5);
    // t = t + 5;
    t = t + 15;
    if ( t > max_ms )
      return(max_ms);
  }
  return(t);
}

uint16_t time_pin_high(uint16_t max_ms)
{
  // blocking for up to max_ms
  if (digitalRead(MORSE_CODE_PIN) == LOW)
  {
    return(0);
  }
  delay(50); //debounce-
  uint16_t t = 50;
  while(digitalRead(MORSE_CODE_PIN) == HIGH)
  {
    delay(5);
    t = t + 5;
    if ( t > max_ms )
      return(max_ms);
  }
  return(t);
}

byte read_morse_char()
{
  // this function blocks
  byte code = B0000000;
  uint16_t lastPulseLength;
  uint8_t pos = 0;
  while(pos < 5)
  {
    lastPulseLength = time_pin_low(MORCE_CODE_MAX_DASH_MS);
    if (lastPulseLength == 0 || lastPulseLength == MORCE_CODE_MAX_DASH_MS) {return MORSE_CODE_CHAR_ERROR;} // return null byte on error
    if (lastPulseLength < MORSE_CODE_MAX_DOT_MS)
    {
      // set dot as the LOW was less than a dash
      // dots are zero so nothing to set but bump the pos
      pos++;
    }
    else
    {
      // set dash
      bitSet(code, (4 - pos)); // bits in byte are 76543210, we start at 4 and move right
      pos++;
    }
    // At this point we have read the low dash or dot but we dont know if its the end of the char.
    lastPulseLength = time_pin_high(MORSE_CODE_MAX_CHAR_INTERVAL_MS);
    if (lastPulseLength == MORSE_CODE_MAX_CHAR_INTERVAL_MS)
    {
      // The low time was higher than the intra-character spacing max so must end this character
     code |= pos << 5; // set the pos bits
     return(code); 
    }
  }
    // We have already got all 5 dots/dashes, cant RX any more
    return MORSE_CODE_CHAR_ERROR; //return Error
}

#define MAX_SECRET_CODE_LENGTH 4

void read_morse_word(char code[])
{
  uint8_t pos = 0;
  pos = 0;
  byte lastChar;
  int8_t lastPulseLength;
  while(pos < MAX_SECRET_CODE_LENGTH)
  {
    lastChar = read_morse_char();
    if (lastChar == MORSE_CODE_CHAR_ERROR)
      return "";
    // We should still be HIGH here - we left the function because the wait was longer than the intra char interval
    code[pos] = lastChar;
    pos++;
    // Should we end the string early or capture the next character?
    if (time_pin_high(MORSE_CODE_MAX_CHAR_INTERVAL_MS) == MORSE_CODE_MAX_CHAR_INTERVAL_MS)
    {
      // The char interval has already passed inside the read_morse_char func.
      // If we go that length AGAIN, the code is over
      // i.e.  CHAR INT = 1s, WORD INT = 2s, 1s has elapsed so wait up to 1s for the next LOW to start
      return;
    }
  }
  // At MAX code length, so return
  return;
}

#include <tinyNeoPixel_Static.h>
#define NUMLEDS 6
byte pixels[NUMLEDS * 3];
tinyNeoPixel strip = tinyNeoPixel(NUMLEDS, PIN_PA3, NEO_GRB, pixels);





void setAllPixels(int r, int g, int b, bool show = false)
{
  for (int i = 0; i < NUMLEDS; i++) 
  {
    strip.setPixelColor(i,r,g,b);
  }
  if(show)
    strip.show();
}

int police(int x)
{
  setAllPixels(255,0,0);
  if( x & 1)
  {
    strip.setPixelColor(LED1,0,0,255);
    strip.setPixelColor(LED3,0,0,255);
  }
  else
  {
    strip.setPixelColor(LED0,BLUE);
    strip.setPixelColor(LED2,BLUE);
    strip.setPixelColor(LED4,BLUE);
  }
  strip.show();
  return 200;
}



int chase(int x, uint8_t r,uint8_t g, uint8_t b)
{
  strip.setPixelColor((x - 1) % 5,OFF); // turn last pixel off
  strip.setPixelColor((x % 5),r,g,b); // set current pixel
  strip.show();
  return 200;
}

int pumpkin(int x)
{
  chase(x+1,2,1,0);
  chase(x,ORANGE);
  chase(x-1,2,1,0);
  return 500;
}

int sone(int x)
{
  setAllPixels(PURPLE);
  if(x % 2 == 0)
  {
    strip.setPixelColor(LED0,20,20,20);
    strip.setPixelColor(LED3,20,20,20);
  }
  else
  {
    strip.setPixelColor(LED1,20,20,20);
    strip.setPixelColor(LED2,20,20,20);
    strip.setPixelColor(LED4,20,20,20);
  }
  strip.show();
  return 300;
}

int pink(int x, int r, int g, int b, int r2, int g2, int b2)
{
  if(x % 2 == 0)
  {
    strip.setPixelColor(LED0,r,g,b);
    strip.setPixelColor(LED1,r2,g2,b2);
    strip.setPixelColor(LED2,r,g,b);
    strip.setPixelColor(LED3,r2,g2,b2);
    strip.setPixelColor(LED4,r,g,b);
  }
  else
  {
    strip.setPixelColor(LED0,r2,g2,b2);
    strip.setPixelColor(LED1,r,g,b);
    strip.setPixelColor(LED2,r2,g2,b2);
    strip.setPixelColor(LED3,r,g,b);
    strip.setPixelColor(LED4,r2,g2,b2);
  }
  strip.show();
  return 100;
}

int fill(int x, int r, int g, int b)
{
  int step = x % 10;
  switch(step)
  {
    case 0:
      setAllPixels(OFF);
      break;
    case 1 ... 6:
      strip.setPixelColor(p[step - 1], r,g,b);
      break;
    default:
      break;
  }
  strip.show();
  return 100;
}

#include <avr/sleep.h>

void sleep()
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  PORTA.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc; // enable pullup and interrupt
  digitalWrite(PIN_PA1, LOW); // turn off LED power rail
  sleep_enable();
  sleep_cpu();
  // sleep resumes here
  PORTA.PIN7CTRL = PORT_PULLUPEN_bm; // renable pullup but no interrupt
  digitalWrite(PIN_PA1, HIGH);  // turn on LED power rail
}

RTC_PERIOD_enum period = RTC_PERIOD_CYC16_gc;

void RTC_init()
{
  /* Initialize RTC: */
  while (RTC.STATUS > 0)
  {
    ;                                   /* Wait for all register to be synchronized */
  }
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;    /* 1kHz Internal Ultra-Low-Power Oscillator (OSCULP32K) */
}

void mini_sleep()
{
  RTC.PITINTCTRL = RTC_PI_bm;  // Enable RTC interrupt
  RTC.PITCTRLA = period | RTC_PITEN_bm; // Set timer to 2s
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  RTC.PITINTCTRL = ~(RTC_PI_bm); // Disable RTC interrupt
}

ISR(RTC_PIT_vect)
{
  RTC.PITINTFLAGS = RTC_PI_bm;  // Clear RTC interrupt flag otherwise keep coming back here
}

ISR(PORTA_PORT_vect) {
  PORTA.INTFLAGS = PORT_INT7_bm; // Clear Pin 7 interrupt flag otherwise keep coming back here
}

uint8_t state;

void setup()
{
  // Power save
  // http://www.technoblogy.com/show?2RA3
  // https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/PowerSave.md
  ADC0.CTRLA &= ~ADC_ENABLE_bm; // Disable ADC
  pinMode(PIN_PA1, OUTPUT);
  pinMode(PIN_PA2, OUTPUT);
  pinMode(PIN_PA6, OUTPUT);
  // UPDI does not need setting (PA0)

  // Pin setup
  pinMode(PIN_PA3, OUTPUT);
  pinMode(PIN_PA7, INPUT_PULLUP);
  digitalWrite(PIN_PA1, HIGH);

  
  state = EEPROM.read(0);
  RTC_init();
}

void flash_morse(byte b)
{
  for(int8_t i = 7; i > -1; i--)
  {
    setAllPixels(0,0,0,true);
    delay(100);
    if(b & (1 << i))
    {
      setAllPixels(5,0,0,true);
    }
    else
    {
      setAllPixels(0,0,5,true);
    }
    delay(500);
  }
}

void flash_morse_word(char *s)
{
  for(int i = 0; i < MAX_SECRET_CODE_LENGTH; i++)
  {
    if(s[i] == 0)
      return;   
    flash_morse(s[i]);
    setAllPixels(0,5,0,true);
    delay(300);
  }
  return;
}

void t_morse()
{
  char m[MAX_SECRET_CODE_LENGTH] = {0};
  setAllPixels(0,0,0,true);
  while(digitalRead(MORSE_CODE_PIN) == HIGH)
  {
    delay(10);
  }
  read_morse_word(m);
  flash_morse_word(m);
}

bool c_array(char array_1[],char array_2[])
{
  for (int i = 0;  i < MAX_SECRET_CODE_LENGTH; i++)
  {
  if( array_1[i] != array_2[i] ) 
    return false;
  }
  return true;
}

uint8_t success(uint8_t mode)
{
    EEPROM.update(0, state);
    setAllPixels(0,5,0,true);
    delay(500);
    return mode;
}

uint8_t fail(uint8_t mode)
{
    setAllPixels(5,0,0,true);
    delay(500);
    return mode;
}

uint8_t c_morse(uint8_t mode)
{
  // SECRET CODES
  char secret1[MAX_SECRET_CODE_LENGTH] = {m_S, m_A, m_G, m_E}; // GREEN
  char secret2[MAX_SECRET_CODE_LENGTH] = {m_S, m_O, m_S, 0}; // POLICE
  char secret3[MAX_SECRET_CODE_LENGTH] = {m_1, m_9, m_5, m_7};  // SPUTNIK
  char secret4[MAX_SECRET_CODE_LENGTH] = {m_5, m_0, m_4, m_B}; // PINK
  char secret5[MAX_SECRET_CODE_LENGTH] = {m_D, m_N, m_S, 0};  // INFOBLOX
  char secret6[MAX_SECRET_CODE_LENGTH] = {m_N, m_A, m_S, m_A}; // RED
  char secret7[MAX_SECRET_CODE_LENGTH] = {m_P, m_U, m_N, m_K}; // PUNK
  char secret8[MAX_SECRET_CODE_LENGTH] = {m_S, m_O, m_N, m_E}; // SENTINEL ONE
  char secret_reset[MAX_SECRET_CODE_LENGTH] = {m_R, m_S, m_T, 0};
  //char secret_all[MAX_SECRET_CODE_LENGTH] = {m_1, m_3, m_3, m_7};
  // BUFFER TO READ TO
  char i[MAX_SECRET_CODE_LENGTH] = {0};
  // SET PIXELS TO SHOW WE ARE READIING
  setAllPixels(0,0,5,true);
  // READ INPUT
  while(digitalRead(MORSE_CODE_PIN) == HIGH)
    delay(10);
  read_morse_word(i);
  //flash_morse_word(i);
  // TEST FOR A MATCH
  if (c_array(i, secret1))
  {
    state = state & ~B00000001;
    return success(3);
  }
  if (c_array(i, secret2))
  {
    state = state & ~B00000010;
    return success(4);
  }
  if (c_array(i, secret3))
  {
    state = state & ~B00000100;
    return success(5);
  }
  if (c_array(i, secret4))
  {
    state = state & ~B00001000;
    return success(6);
  }
  if (c_array(i, secret5))
  {
    state = state & ~B00010000;
    return success(7);
  }
  if (c_array(i, secret6))
  {
    state = state & ~B00100000;
    return success(8);
  }
  if (c_array(i, secret7))
  {
    state = state & ~B01000000;
    return success(9);
  }
  if (c_array(i, secret8))
  {
    state = state & ~B10000000;
    return success(10);
  }
  if (c_array(i, secret_reset))
  {
    state = B11111111;
    return success(0);
  }
//  if (c_array(i, secret_all))
//  {
//    state = B0;
//    return success(11);
//  }
  return fail(mode);
}




int fill_cycle(int x)
{
  int color_step = x % 60;
  switch (color_step)
  {
    case 1 ... 9:
      return fill(color_step,RED);    
    case 11 ... 19:
      return fill(color_step,GREEN);
    case 21 ... 29:
      return fill(color_step,ORANGE);
    case 31 ... 39:
      return fill(color_step,PURPLE);
    case 41 ... 49:
      return fill(color_step,BLUE);    
  }
  return 0;
}



int twinkle(uint8_t i){
  int b = i % 10;
  int l = i / 50;
  setAllPixels(OFF);
  strip.setPixelColor(l, 30 - b, 20 -b , 0);
  strip.show();
  return 50;
}



int loading(int x)
{
  
  int i = x % 25;
  setAllPixels(OFF);
  if ( i < 5 )
  {
    strip.setPixelColor(p[i],RED);
  }
  else if ( i < 9 )
  {
    strip.setPixelColor(1,RED);
    strip.setPixelColor(p[i-5],GREEN);
  }
  else if ( i < 12 )
  {
    strip.setPixelColor(1,RED);
    strip.setPixelColor(2,GREEN);
    strip.setPixelColor(p[i-9],ORANGE);
  }
  else if ( i < 15 )
  {
    strip.setPixelColor(1,RED);
    strip.setPixelColor(2,GREEN);
    strip.setPixelColor(4,ORANGE);
    strip.setPixelColor(p[i-13],PURPLE);
  }
  else
  {
    strip.setPixelColor(1,RED);
    strip.setPixelColor(2,GREEN);
    strip.setPixelColor(4,ORANGE);
    strip.setPixelColor(3,PURPLE);
    strip.setPixelColor(0,BLUE);
  }
  strip.show();
  return (100 + (i * 10));
}

int knightrider(uint8_t i, int r, int g, int b, int r2, int g2, int b2)
{
  setAllPixels(r2,g2,b2);
  int n = i % 12;
  if (n < 5)
  {
    strip.setPixelColor(p[n],r,g,b);
  }
  else if (n > 5)
  {
    strip.setPixelColor(p[10 - n],r,g,b);
  }
  strip.show();
  return 50;
}

int infoblox(int i)
{
  setAllPixels(20,20,20);
  strip.setPixelColor(LED1,0,i % 20,0);
  strip.show();
  return 20;
}

int sputnik(uint8_t i)
{
  
  int n = i % 4;
  if (n == 0)
  {
    setAllPixels(10,0,0);
    strip.setPixelColor(LED1,18,12,0);
  }
  else
  {
    setAllPixels(3,0,0);
    strip.setPixelColor(LED1,3,2,0);
  }
  strip.show();
  return 200;
}

uint8_t punkwork_led = 0;

uint8_t punkwork(int i)
{
  setAllPixels(OFF);
  uint8_t s = i % 10;
  uint8_t c = i / 10;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  if (c & 8) r = 10;
  if (c & 16) g = 10;
  if (c & 32) b = 10;
  if (c & 1) r = 30;
  if (c & 2) g = 30;
  if (c & 4) b = 30;
  if (c == 0) b=255;
  if (s == 0)
  {
    punkwork_led++;
    if (punkwork_led > 5) punkwork_led = 0;
    strip.setPixelColor(punkwork_led, 255, 255 , 255);
  }
  else if (s > 4);
  {
    strip.setPixelColor(punkwork_led, r / s, g / s, b / s);
  }
  strip.show();
  return 100;
}

uint16_t win(uint8_t i)
{
  strip.setPixelColor(LED0,0,0,7);
  strip.setPixelColor(LED1,2,0,5);
  strip.setPixelColor(LED2,3,2,0);
  strip.setPixelColor(LED3,0,3,0);
  strip.setPixelColor(LED4,3,0,0);
  const uint8_t code[] = { m_2, m_7, m_9, m_2, m_5 };
  const uint8_t code_lengths[] = { 5,5,5,5,5 };
  int n = i % 50;
  int b = n / 5;
  int s = n % 5;
  if (s == 0)
  {
    if (code_lengths[0] > b)
    strip.setPixelColor(LED0,BLUE);
    if (code_lengths[1] > b)
    strip.setPixelColor(LED1,PURPLE);
    if (code_lengths[2] > b)
    strip.setPixelColor(LED2,ORANGE);
    if (code_lengths[3] > b)
    strip.setPixelColor(LED3,GREEN);
    if (code_lengths[4] > b)
    strip.setPixelColor(LED4,RED);
  }
  if (s == 1 || s == 2)
  { 
    if ((code_lengths[0] > b) && (code[0] & (1 << 7 - (b + 3))))
    {
      strip.setPixelColor(LED0,BLUE);
    }
    if ((code_lengths[1] > b) && (code[1] & (1 << 7 - (b + 3))))
    {
      strip.setPixelColor(LED1,PURPLE);
    }
    if ((code_lengths[2] > b) && (code[2] & (1 << 7 - (b + 3))))
    {
      strip.setPixelColor(LED2,ORANGE);
    }
    if ((code_lengths[3] > b) && (code[3] & (1 << 7 - (b + 3))))
    {
      strip.setPixelColor(LED3,GREEN);
    }
    if ((code_lengths[4] > b) && (code[4] & (1 << 7 - (b + 3))))
    {
      strip.setPixelColor(LED4,RED);
    }
  }
  strip.show();
  return 200;
}

void loop()
{
  int mode = 0;
  uint16_t interval;
  uint16_t button_low_time = 0;
  uint32_t total_interval = 0;
  strip.begin();
  int i = 0;
  //state = 0;
  while(true)
  {
    if ( mode == 0 )
    {
      interval = twinkle(i);
    }
    else if ( mode == 1 )
    {
      interval = punkwork(i);
    }
    else if ( mode == 2 )
    {
      interval = pink(i, GREEN, PURPLE);
    }
    else if ( mode == 3 && !(state & B00000001 ))
    {
      interval = knightrider(i, GREEN, 0,1,0);
    }
    else if ( mode == 4 && !(state & B00000010 ))
    {
      interval = police(i);
    }
    else if ( mode == 5 && !(state & B00000100 ))
    {
      interval = sputnik(i);
    }
    else if ( mode == 6 && !(state & B00001000 ))
    {
      interval = pink(i, PINK, PURPLE);
    }
    else if ( mode == 7 && !(state & B00010000 ))
    {
      interval = infoblox(i);
    }
    else if ( mode == 8 && !(state & B00100000 ))
    {
      interval = knightrider(i, 255,0,0, 255,255,255);
    }
    else if ( mode == 9 && !(state & B01000000 ))
    {
      interval = loading(i);
    }
    else if ( mode == 10 && !(state & B10000000 ))
    {
      interval = sone(i);
    }
    else if ( mode == 11 && (state == 0 )) // FULL SET
    {
      interval = win(i);
    }
    else if (mode > 12)
    {
      mode = 0;
    }
    else
    {
      mode++;
      continue;
    }
    i++;
    // This section breaks down the sleep interval to catch button presses
    total_interval = total_interval + interval ;
    while(interval > 0)
    {
      mini_sleep();
      //delay(10);
      interval = interval - 10;
      button_low_time = time_pin_low(2000);
      if (button_low_time > 200)
      {
       /*
       * MAIN MENU
       * NO PRESS = CONTINUE
       * SHORT PRESS = CHANGE FLASHY MODE
       * LONG PRESS = ENTER MORSE CODE MODE
       */
       if (button_low_time > 1500)
        {
          setAllPixels(RED,true);
          while(digitalRead(MORSE_CODE_PIN) == LOW)
          {
            // WAIT FOR RELEASE BEFORE SLEEPING otherwise we wake back up!
            delay(5);
          }
          delay(50); //DEBOUNCE
          sleep();
        }
        else if (button_low_time > 600)
        {
          setAllPixels(BLUE,true);
          while(digitalRead(MORSE_CODE_PIN) == LOW)
            delay(10);
          mode = c_morse(mode);
        }
        else
        {
          mode = mode +1;
        }
        // reset timer
        i = 0;
        total_interval = 0;
      }

    }
    // At the end of each interval, see if we need to sleep
    if (total_interval > WAKE_TIME_MS)
    {
      total_interval = 0;
      i = 0;
      sleep();
    }
  }
}