#define F_CPU 16000000UL
#define BAUD 115200 /* A little fast but appears to work. */

#include <util/setbaud.h>
#include <avr/io.h>

/*
 * Microwrite keyboard connection
 * 
 * AVR   Arduino Pin     MW Ribbon    Configure AVR pin          MW Key
 *                   +-+
 *                   | |
 *                   +-+
 *                   | |
 *                   +-+
 *                   | |
 *                   +-+
 *                   | |
 *                   +-+
 *                   | |
 *                   +-+
 * PB2            10 | | White         Output, Driven to Zero
 *                   +-+
 * PB1             9 | | Grey          Input with pull-up        Command
 *                   +-+
 * PB0             8 | | Blue          Input with pull-up        Thumb
 *                   +-+
 *                       (Missing)
 *                   +-+
 * PD7             7 | | Yellow        Input with pull-up        Index Finger
 *                   +-+
 * PD6             6 | | Orange        Input with pull-up        Middle Finger
 *                   +-+
 * PD5             5 | | Brown         Input with pull-up        Ring Finger 
 *                   +-+
 * PD4             4 | | Black         Input with pull-up        Little Finger
 *                   +-+
 * PD3             3 | |
 *                   +-+
 * PD2             2 | |
 *                   +-+
 * PD1             1 | |
 *                   +-+
 * PD0             0 | |
 *                   +-+
 */

void uart_putchar(char c)
{
   while (!(UCSR0A & 0x20));
   UDR0 = c;
}

void uart_init(void)
{
   UBRR0L = UBRRL_VALUE;
   UBRR0H = UBRRH_VALUE;
#if USE_2X
   UCSR0A |= _BV(U2X0);
#else
   UCSR0A &= ~(_BV(U2X0));
#endif
   UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* Eight bit data. */
   UCSR0B = _BV(RXEN0) | _BV(TXEN0); /* Enable both TX and RX. */
}

int main(void)
{
   DDRD = 0;
   PORTD = ~0;

   DDRB = 1 << 2;
   PORTB = ~(1 << 2);

   uart_init();

   for (;;)
   {
      while (!(UCSR0A & 0x80));
      char c = UDR0;

      if (c == '\r')
      {
         unsigned short pins = PINB << 8 | PIND << 0;

         for (int bit = 4; bit < 10; ++bit)
         {
            uart_putchar(pins & (1 << bit) ? '1' : '0');
         }
         uart_putchar('\r');
         uart_putchar('\n');
      }
   }
}
