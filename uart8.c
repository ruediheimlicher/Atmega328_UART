#include <stdlib.h>

#define FOSC   7372000

#define BAUD 9600

#define UART_PORT		PORTC
#define UART_PIN		PINC
#define UART_DDR		DDRC
#define RTS_PIN      5
#define CTS_PIN      4

//defines max coordinates for checking overflow
#define MAX_X 100
#define MAX_Y 50

#define VGA_LF 0x0D


extern void delay_ms(unsigned int);


// Window Creation strings (without initial escape sequence)
char header[]  = {"w,1,0,0,100,3,1,"};                 // no title
char window2[] = {"w,2,0,3,70,32,1,Data Webserver"};
char window3[] = {"w,3,70,3,30,32,1,Calculated Values"};
char window4[] = {"w,4,0,35,100,14,1,Terminal Input"};
char footer[]  = {"w,5,0,49,100,1,0,"};                // no border, no title

char header_colour[] = {"l,0,2,020,000"}; // White text on purple lines 0-2
char window2_colour[] = {"l,35,48,222,001"}; // white text on blue lines 35-48
char footer_colour[] = {"l,49,49,000,111"}; // black text on grey line 49



#define USR UCSRA


void uart_init (void)
{
   uint8_t sreg = SREG;
   
   /*
    uint16_t ubrr = (uint16_t) ((uint32_t) FOSC/(16UL*BAUDRATE) - 1);
    
    UBRRH = (uint8_t) (ubrr>>8);
    UBRRL = (uint8_t) (ubrr);
    */
   UBRRH = (((FOSC/16)/BAUD-1)>>8);  // The high byte, UBRR0H
   UBRRL = ((FOSC/16)/BAUD-1);       // The low byte, UBRR0L
      
   // Interrupts kurz deaktivieren
   
   // UART Receiver und Transmitter anschalten, Receive-Interrupt aktivieren
   // Data mode 8N1, asynchron
   UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
   UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
   
   // Flush Receive-Buffer (entfernen evtl. vorhandener ungültiger Werte)
   do
   {
      // UDR auslesen (Wert wird nicht verwendet)
      UDR;
   }
   while (UCSRA & (1 << RXC));
   
   // Rücksetzen von Receive und Transmit Complete-Flags
   UCSRA = (1 << RXC) | (1 << TXC);
   
   // Global Interrupt-Flag wieder herstellen
   SREG = sreg;
   
   // FIFOs für Ein- und Ausgabe initialisieren
   //   fifo_init (&infifo,   inbuf, BUFSIZE_IN);
   //   fifo_init (&outfifo, outbuf, BUFSIZE_OUT);
   
   UART_DDR &= ~(1<<RTS_PIN); // Eingang
   UART_PORT |= ~(1<<RTS_PIN);
   
   UART_DDR |= (1<< CTS_PIN); // Ausgang
   UART_PORT |= ~(1<<CTS_PIN); // HI
}



void putch (char ch)
{
   //lcd_putc('0');
	//while (UART_PIN & (1<<RTS_PIN));
   // _delay_ms(1);
#ifdef USR
   //lcd_putc('1');
	while(!(USR & (1<<UDRE))); //transmit buffer is ready to receive data
	UDR = ch;    // send character
	while(!(USR & (1<<TXC))); //wait for char to be send
   //_delay_ms(1);
	USR &= ~(1<<TXC || 1<<UDRE);
#else //2 UART MCU
	while(!(UCSR0A & (1<<UDRE0))); //transmit buffer is ready to receivce data
	//lcd_putc('4');
   //_delay_ms(1);
	UDR0 = ch;    // send character
	while(!(UCSR0A & (1<<TXC0))); //wait for char to be send
   //_delay_ms(1);
   //lcd_putc('5');
	UCSR0A &= ~(1<<TXC0 || 1<<UDRE0);
   
#endif
}



// Einen 0-terminierten String übertragen.
void puts (const char *s)
{
   do
   {
      putch (*s);
      
   }
   while (*s++);
}


void vga_command(const char *command_str)
{
   // Function to easily send command to VGA board
  
   putch('^');
   putch('['); // send escape sequence
   
   //delay_ms(2);
   puts(command_str);
   putch(0x0D);
   // send Command string
   //delay_ms(2);
   // Most commands don't take very long, but need a small delay to complete
   // The Reboot cammand needs 2 seconds
   if(command_str[0]=='r')
   {
      delay_ms(2000);  // Wait 2 seconds for reboot

   }
   if((command_str[0]=='w') || (command_str[0]=='p')) _delay_ms(2000);
   // Small delay for window commands
   delay_ms(2);                             // Other commands need a tiny delay
   
}

void vga_start(void)
{
   

   vga_command("r");          // reboot VGA board
   vga_command(header);       // Create header
   vga_command(header_colour);
   vga_command(window2);       // Create window 1
   vga_command("f,1");
   puts("Hallo VGA");
        }
        

void clrscr(void)
{
   delay_ms(1);
   putch('\033');
   putch('[');
   
   putch('2');
   putch('J');
}

void gotoxy(uint8_t x, uint8_t y)
{
   if (x>MAX_X || y>MAX_Y)
      return;
   
   x--;
   y--;
   
   char gotostring[] = {"p,10,10"};
   char command = 'p';
   char params[10] = {};
   
   
   //sprintf(params,"p,%d,%d",x,y);
   
   putch('^');
   putch('[');
   putch('p');
   putch(',');
   putch((x/10)+'0');
   putch((x%10)+'0');
   
   putch(',');
   putch((y/10)+'0');
   putch((y%10)+'0');
      //putch(0);
   /*
   putch((y/10)+'0');
   putch((y%10)+'0');
   putch(',');
   putch((x/10)+'0');
   putch((x%10)+'0');
    */
   putch(0xD0);
   
}

void putint(uint8_t zahl)
{
   char string[4];
   int8_t i;                             // schleifenzähler
   
   string[3]='\0';                       // String Terminator
   for(i=2; i>=0; i--)
   {
      string[i]=((zahl % 10) +'0');         // Modulo rechnen, dann den ASCII-Code von '0' addieren
      zahl /= 10;
   }
   puts(string);
}
void putint_right(uint8_t zahl)
{
   char string[4];
   int8_t i;                             // schleifenzähler
   
   string[3]='\0';                       // String Terminator
   for(i=2; i>=0; i--)
   {
      if (zahl==0)
      {
         string[i] = ' '; // Leerschlag
      }
      else
      {
         string[i]=((zahl % 10) +'0');         // Modulo rechnen, dann den ASCII-Code von '0' addieren
      }
      zahl /= 10;
   }
   puts(string);
}


void _textcolor(int color)
{
   putch('\033');
   putch('[');
   if (color & 0x8)
   {
      putch('1');
   }
   else
   {
      putch('2');
   }
   
   putch('m');
   putch('\033');
   
   putch('[');
   putch('3');
   putch(((color&0x7)%10)+'0');
   putch('m');
}


void _textbackground(int color)
{
   putch('\033');
   putch('[');
   if (color & 0x8)
      putch('5');
   else putch('6');
   putch('m');
   
   putch('\033');
   putch('[');
   putch('4');
   putch((color&0x7)+'0');
   putch('m');
}

void newline(void)
{
   putch(0x0D);
   
}

void cursoroff(void)
{
   putch('\033');
   putch('[');
   putch('2');
   putch('5');
   putch('l');
}



