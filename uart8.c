#include <avr/io.h>
#include <string.h>
#include <stdlib.h>

#define FOSC   7372000

#define BAUD 9600

#define UART_PORT		PORTC
#define UART_PIN		PINC
#define UART_DDR		DDRC
//#define RTS_PIN      5
//#define CTS_PIN      4

#define UART_STOP       7 // Ausgabe anhalten

#define UART_DATA       6 // Daten anzeigen

//defines max coordinates for checking overflow
#define MAX_X 100
#define MAX_Y 50

#define VGA_LF 0x0D


extern void delay_ms(unsigned int);


// Window Creation strings (without initial escape sequence)
char header[]  = {"w,1,0,0,100,3,1,"};                 // no title
char window2[] = {"w,2,0,3,60,40,1,Data Webserver"};
char window3[] = {"w,3,70,3,30,32,1,Calculated Values"};
char window4[] = {"w,4,0,35,100,14,1,Terminal Input"};
char footer[]  = {"w,5,0,49,100,1,0,"};                // no border, no title


#define USR UCSRA


void uart_init (void)
{
   uint8_t sreg = SREG;
   
   /*
    uint16_t ubrr = (uint16_t) ((uint32_t) FOSC/(16UL*BAUDRATE) - 1);
    
    UBRRH = (uint8_t) (ubrr>>8);
    UBRRL = (uint8_t) (ubrr);
    */
   UBRR0H = (((FOSC/16)/BAUD-1)>>8);  // The high byte, UBRR0H
   UBRR0L = ((FOSC/16)/BAUD-1);       // The low byte, UBRR0L
   
   // Interrupts kurz deaktivieren
   
   // UART Receiver und Transmitter anschalten, Receive-Interrupt aktivieren
   // Data mode 8N1, asynchron
   UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);//|(1 << UDRIE0);
   
   UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);// 8-bit, 1 stop bit, no parity, asynchronous UART
   
   // Flush Receive-Buffer (entfernen evtl. vorhandener ungültiger Werte)
   do
   {
      // UDR auslesen (Wert wird nicht verwendet)
      UDR0;
   }
   while (UCSR0A & (1 << RXC0));
    
   // Rücksetzen von Receive und Transmit Complete-Flags
   UCSR0A = (1 << RXC0) | (1 << TXC0);
   
   // Global Interrupt-Flag wieder herstellen
   //SREG = sreg;
   
}



void vga_putch (char ch)
{

	while(!(UCSR0A & (1<<UDRE0))); //transmit buffer is ready to receive data
	UDR0 = ch;    // send character
	while(!(UCSR0A & (1<<TXC0))); //wait for char to be send
	UCSR0A &= ~(1<<TXC0 || 1<<UDRE0);
}

// Einen 0-terminierten String übertragen.
void vga_puts(const char *s)
{
   do
   {
      vga_putch (*s);
   }
   while (*s++);
}


void vga_command(const char *command_str)
{
   // Function to easily send command to VGA board
  
   vga_putch('^');
   vga_putch('[');          // send escape sequence

   vga_puts(command_str);   // send Command string
   vga_putch(0x0D);

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
   vga_command(window2);       // Create window 1
   vga_command("f,1");
   vga_puts("HomeCentral Rueti");
}
        
void vga_leerschlag(uint8_t leer)
{
   uint8_t i=0;
   for (i=0;i<leer;i++)
   {
      vga_putch(' ');
   }
}

uint8_t gotoxy(uint8_t x, uint8_t y)
{
   if (x>MAX_X || y>MAX_Y)
   {
      return 1;
   }
   
   vga_putch('^');
   vga_putch('[');
   vga_putch('p');
   vga_putch(',');
   
   vga_putch((x/10)+'0');
   vga_putch((x%10)+'0');
   
   vga_putch(',');
   vga_putch((y/10)+'0');
   vga_putch((y%10)+'0');
   vga_putch(0);

   vga_putch(0xD0);
  
   return 0;
}

void vga_putint(uint8_t zahl)
{
   char string[4];
   int8_t i;                             // Loop counter
   
   string[3]='\0';                       // String Terminator
   for(i=2; i>=0; i--)
   {
      string[i]=((zahl % 10) +'0');      // Modulo div, add ASCII-Code '0'
      zahl /= 10;
   }
   vga_puts(string);
}

void vga_putint_right(uint8_t zahl)
{
   char string[4];
   int8_t i;                             // Loop counter
   
   string[3]='\0';                       // String Terminator
   if (zahl == 0)
   {
      string[2]= '0';
      string[1]= ' ';
      string[0]= ' ';
      vga_puts(string);
      return;
   }
   for(i=2; i>=0; i--)
   {
      if (zahl==0)
      {
         string[i] = ' ';                // blank
      }
      else
      {
         string[i]=((zahl % 10) +'0');       // Modulo rechnen, dann den ASCII-Code von '0' addieren
      }
      zahl /= 10;
   }
   vga_puts(string);
}

void vga_putint2(uint8_t zahl)
{
   char string[3];
   string[2]='\0';                        // String Terminator
   if (zahl == 0)
   {
      string[1]= '0';
      string[0]= ' ';
      vga_puts(string);
      return;
   }

   int8_t i;                             // schleifenzähler
   
   if (zahl>99)
   {
      zahl %= 100;
   }
   string[2]='\0';                       // String Terminator
   for(i=1; i>=0; i--)
   {
      string[i]=((zahl % 10) +'0');         // Modulo rechnen, dann den ASCII-Code von '0' addieren
      zahl /= 10;
   }
   vga_puts(string);
}

void vga_putint2_right(uint8_t zahl)
{
   char string[3];
   int8_t i;                             // schleifenzähler
   if (zahl>99)
   {
      zahl %= 100;
   }

   string[2]='\0';                       // String Terminator
   for(i=1; i>=0; i--)
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
   vga_puts(string);
}

void vga_putint1(uint8_t zahl)
{
   char string[2];
   
   if (zahl>9)
   {
      zahl %= 10;
   }
   string[1]='\0';                       // String Terminator
   string[0]=(zahl +'0');         // Modulo rechnen, dann den ASCII-Code von '0' addieren
   vga_puts(string);
}

void vga_puthex(uint8_t hexzahl)
{
   
}

void setFeld(uint8_t number, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t border, char* title)
{
   /*
    height unter 5:
    ohne Titel:
    minheight = 1
    
    mit Titel:
    minheight = 1. Keine border gezeigt. Titel abgeschnitten
    
    */
   vga_putch('^');
   vga_putch('[');
   vga_putch('w');
   vga_putch(',');
   vga_putch((number%10)+'0');
   vga_putch(',');
   
   vga_putch((left/10)+'0');
   vga_putch((left%10)+'0');
   vga_putch(',');

   vga_putch((top/10)+'0');
   vga_putch((top%10)+'0');
   vga_putch(',');

   //vga_putch((width/10)+'0');
   if (width >99)
   {
      width =99;
   }
   
   vga_putch((width/10)+'0');
   vga_putch((width%10)+'0');
   vga_putch(',');

   vga_putch((height/10)+'0');
   //vga_putch(1+'0');
   vga_putch((height%10)+'0');
   vga_putch(',');
  
   vga_putch(border%10+'0');
   vga_putch(',');
   if (strlen(title))
   {
      vga_puts(title);
   }
   else
      {
         vga_puts("");
      }
 
   vga_putch(0xD0);

}





void newline(void)
{
   vga_putch(0x0D);
   
}




