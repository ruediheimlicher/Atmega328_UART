//
//  Tastenblinky.c
//  Tastenblinky
//
//  Created by Sysadmin on 03.10.07.
//  Copyright Ruedi Heimlihcer 2007. All rights reserved.
//



#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
//#include <avr/pgmspace.h>
//#include <avr/sleep.h>
#include <inttypes.h>
//#define F_CPU 4000000UL  // 4 MHz
#include <avr/delay.h>
#include <avr/wdt.h>

#include "lcd.c"
#include "adc.c"
#include "kbd.h"
//#include "fifo.c"
//#include <util/setbaud.h>
#include "SPI_slave.c"
#include "uart8.c"

#include "onewire.c"
#include "ds18x20.c"
#include "crc8.c"

uint16_t loopCount0=0;
uint16_t loopCount1=0;
uint16_t loopCount2=0;

#define TWI_PORT		PORTC
#define TWI_PIN		PINC
#define TWI_DDR		DDRC

#define UART_PORT		PORTC
#define UART_PIN		PINC
#define UART_DDR		DDRC
#define RTS_PIN      5
#define CTS_PIN      4



#define TWI_ERR_BIT				7


#define SDAPIN       4
#define SCLPIN       5


#define LOOPLED_PORT	PORTD
#define LOOPLED_DDR	DDRD
#define LOOPLED_PIN	7

/*
#define OSZIPORT     PORTD
#define OSZIDDR      DDRD
#define OSZIA        5
#define OSZIB        6
 */

#define OSZIA_HI  OSZIPORT |= (1<< OSZIA)
#define OSZIA_LO  OSZIPORT &= ~(1<< OSZIA)
#define OSZIB_HI  OSZIPORT |= (1<< OSZIB)
#define OSZIB_LO  OSZIPORT &= ~(1<< OSZIB)

#define TASTE1		19
#define TASTE2		29
#define TASTE3		44
#define TASTE4		67
#define TASTE5		94
#define TASTE6		122
#define TASTE7		155
#define TASTE8		186
#define TASTE9		205
#define TASTEL		223
#define TASTE0		236
#define TASTER		248
#define TASTATURPORT PORTC
#define TASTATURPIN		3

#define MANUELL_PORT		PORTD
#define MANUELL_DDR		DDRD
#define MANUELL_PIN		PIND

#define MANUELL			7	// Bit 7 von Status 
#define MANUELLPIN		6	// Pin 6 von PORT D fuer Anzeige Manuell
#define MANUELLNEU		7	// Pin 7 von Status. Gesetzt wenn neue Schalterposition eingestellt
#define MANUELLTIMEOUT	100 // Loopled-counts bis Manuell zurueckgesetzt wird. 02FF: ca. 100 s

volatile uint8_t					Programmstatus=0x00;
	uint8_t Tastenwert=0;
	uint8_t TastaturCount=0;
volatile uint16_t		Manuellcounter=0; // Counter fuer Timeout	
	uint16_t TastenStatus=0;
	uint16_t Tastencount=0;
	uint16_t Tastenprellen=0x01F;

volatile uint8_t uartstatus=0;
uint16_t startcounter=0x00;


void delay_ms(unsigned int ms)
/* delay for a minimum of <ms> */
{
	// we use a calibrated macro. This is more
	// accurate and not so much compiler dependent
	// as self made code.
	while(ms)
   {
		_delay_ms(0.96);
		ms--;
	}
}

//#define FOSC   7372000


// FIFO-Objekte und Puffer für die Ein- und Ausgabe

#define BUFSIZE_IN  0x40
uint8_t inbuf[BUFSIZE_IN];
//fifo_t infifo;

#define BUFSIZE_OUT 0x40
uint8_t outbuf[BUFSIZE_OUT];
//fifo_t outfifo;



uint8_t Tastenwahl(uint8_t Tastaturwert)
{
//lcd_gotoxy(0,1);
//lcd_putint(Tastaturwert);
if (Tastaturwert < TASTE1)
return 1;
if (Tastaturwert < TASTE2)
return 2;
if (Tastaturwert < TASTE3)
return 3;
if (Tastaturwert < TASTE4)
return 4;
if (Tastaturwert < TASTE5)
return 5;
if (Tastaturwert < TASTE6)
return 6;
if (Tastaturwert < TASTE7)
return 7;
if (Tastaturwert < TASTE8)
return 8;
if (Tastaturwert < TASTE9)
return 9;
if (Tastaturwert < TASTEL)
return 10;
if (Tastaturwert < TASTE0)
return 0;
if (Tastaturwert < TASTER)
return 12;

return -1;
}


void slaveinit(void)
{
	MANUELL_DDR |= (1<<MANUELLPIN);		//Pin 5 von PORT D als Ausgang fuer Manuell
	MANUELL_PORT &= ~(1<<MANUELLPIN);
 	//DDRD |= (1<<CONTROL_A);	//Pin 6 von PORT D als Ausgang fuer Servo-Enable
	//DDRD |= (1<<CONTROL_B);	//Pin 7 von PORT D als Ausgang fuer Servo-Impuls
	LOOPLED_DDR |= (1<<LOOPLED_PIN);
	//PORTD &= ~(1<<CONTROL_B);
	//PORTD &= ~(1<<CONTROL_A);

	//DDRB &= ~(1<<PORTB2);	//Bit 2 von PORT B als Eingang fuer Brennerpin
	//PORTB |= (1<<PORTB2);	//HI
	
//	DDRB |= (1<<PORTB2);	//Bit 2 von PORT B als Ausgang fuer PWM
//	PORTB &= ~(1<<PORTB2);	//LO

	DDRB |= (1<<PORTB1);	//Bit 1 von PORT B als Ausgang fuer PWM
	PORTB &= ~(1<<PORTB1);	//LO
   
	

	//LCD
	LCD_DDR |= (1<<LCD_RSDS_PIN);	//Pin 5 von PORT B als Ausgang fuer LCD
 	LCD_DDR |= (1<<LCD_ENABLE_PIN);	//Pin 6 von PORT B als Ausgang fuer LCD
	LCD_DDR |= (1<<LCD_CLOCK_PIN);	//Pin 7 von PORT B als Ausgang fuer LCD

   /*
	// TWI vorbereiten
	TWI_DDR &= ~(1<<SDAPIN);//Bit 4 von PORT C als Eingang für SDA
	TWI_PORT |= (1<<SDAPIN); // HI
	
	TWI_DDR &= ~(1<<SCLPIN);//Bit 5 von PORT C als Eingang für SCL
	TWI_PORT |= (1<<SCLPIN); // HI
*/

	/*
	DDRC &= ~(1<<PORTC0);	//Pin 0 von PORT C als Eingang fuer Vorlauf 	
//	PORTC |= (1<<DDC0); //Pull-up
	DDRC &= ~(1<<PORTC1);	//Pin 1 von PORT C als Eingang fuer Ruecklauf 	
//	PORTC |= (1<<DDC1); //Pull-up
	DDRC &= ~(1<<PORTC2);	//Pin 2 von PORT C als Eingang fuer Aussen	
//	PORTC |= (1<<DDC2); //Pull-up
	DDRC &= ~(1<<PORTC3);	//Pin 3 von PORT C als Eingang fuer Tastatur 	
	//PORTC &= ~(1<<DDC3); //Pull-up
	*/
   
   /*
   OSZIDDR |= (1<<OSZIA);
   OSZIPORT |= (1<<OSZIA);
   OSZIDDR |= (1<<OSZIB);
   OSZIPORT |= (1<<OSZIB);
    */
}


// Interrupt Routine Slave Mode (interrupt controlled)
// Aufgerufen bei fallender Flanke an INT0

ISR( INT1_vect )
{
	
	if (spistatus & (1<<ACTIVE_BIT))									// CS ist LO, Interrupt ist OK
	{
      
      _delay_us(10);																	// PIN lesen:
		
		if (spistatus & (1<<STARTDATEN_BIT))						// out_startdaten senden, in_startdaten laden
		{
         
			if (SPI_CONTROL_PORTPIN & (1<<SPI_CONTROL_MOSI))	// bit ist HI
			{
				in_startdaten |= (1<<(7-bitpos));
            
				// Echo laden
				//SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				in_startdaten |= (0<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			// Output laden
			if (out_startdaten & (1<<(7-bitpos)))
			{
				SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else
			{
				SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			bitpos++;
         
         
			if (bitpos>=8) // Byte fertig
			{
				complement=	~in_startdaten;		// Zweiercomplement aus Startdaten bestimmen fuer Enddaten
            
				spistatus &= ~(1<<STARTDATEN_BIT);					// Bit fuer Startdaten zuruecksetzen
				spistatus |= (1<<LB_BIT);								// Bit fuer lb setzen
            
				bitpos=0;
				
			}
			
		}
		
		
		//	LB
		else if (spistatus & (1<<LB_BIT))					// out_lbdaten senden, in_lbdaten laden
		{
			
			if (SPI_CONTROL_PORTPIN & (1<<SPI_CONTROL_MOSI))	// bit ist HI
			{
				in_lbdaten |= (1<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				in_lbdaten |= (0<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			// Output laden
			if (out_lbdaten & (1<<(7-bitpos)))						// bit ist HI
			{
				SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			bitpos++;
			
			
			if (bitpos>=8)	// Byte fertig
			{
				spistatus &= ~(1<<LB_BIT);						// Bit fuer lb zuruecksetzen
				spistatus |= (1<<HB_BIT);						// Bit fuer hb setzen
				bitpos=0;
			}
			
		}
		//LB end
      
		//	HB
		else if (spistatus & (1<<HB_BIT))					// out_hbdaten senden, in_hbdaten laden
		{
			
			if (SPI_CONTROL_PORTPIN & (1<<SPI_CONTROL_MOSI))	// bit ist HI
			{
				in_hbdaten |= (1<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				in_hbdaten |= (0<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			// Output laden
			if (out_hbdaten & (1<<(7-bitpos)))						// bit ist HI
			{
				SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			bitpos++;
			
			if (bitpos>=8)	// Byte fertig
			{
				spistatus &= ~(1<<HB_BIT);						// Bit fuer hb zuruecksetzen
				
				bitpos=0;
			}
			
		}
		//HB end
      
      
		
		
		else if (spistatus & (1<<ENDDATEN_BIT))					// out_enddaten senden, in_enddaten laden
		{
			
			if (SPI_CONTROL_PORTPIN & (1<<SPI_CONTROL_MOSI))	// bit ist HI
			{
				in_enddaten |= (1<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				in_enddaten |= (0<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			// Output laden
         //			if (out_enddaten & (1<<(7-bitpos)))						// bit ist HI
			if (complement & (1<<(7-bitpos)))						// bit ist HI
			{
				SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			bitpos++;
			
			
			if (bitpos>=8)	// Byte fertig
			{
				spistatus &= ~(1<<ENDDATEN_BIT);						// Bit fuer Enddaten zuruecksetzen
				bitpos=0;
				if (out_startdaten + in_enddaten==0xFF)
				{
					//lcd_putc('+');
					//spistatus |= (1<<SUCCESS_BIT);					// Datenserie korrekt geladen
					
				}
				else
				{
					//lcd_putc('-');
					//spistatus &= ~(1<<SUCCESS_BIT);					// Datenserie nicht korrekt geladen
					errCounter++;
				}
				// 24.6.2010
            //				out_startdaten=0xC0; ergab nicht korrekte Pruefsumme mit in_enddaten
            
            //18.7.10
            //				out_hbdaten=0;
            //				out_lbdaten=0;
				
			}
			
		}
		
		else			// Datenarray in inbuffer laden, Daten von outbuffer senden
         
		{
			if (SPI_CONTROL_PORTPIN & (1<<SPI_CONTROL_MOSI))	// bit ist HI
			{
				
				inbuffer[ByteCounter] |= (1<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				inbuffer[ByteCounter] |= (0<<(7-bitpos));
				
				// Echo laden
				//SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			// Output laden
			if (outbuffer[ByteCounter] & (1<<(7-bitpos)))		// bit ist HI
			{
				SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO);
			}
			else																// bit ist LO
			{
				SPI_CONTROL_PORT &= ~(1<<SPI_CONTROL_MISO);
			}
			
			
			bitpos++;
			
			if (bitpos>=8) // Byte fertig
			{
				if (ByteCounter<SPI_BUFSIZE -1 )						// zweitletztes Byte
				{
					ByteCounter++;
				}
				else
				{
					spistatus |= (1<<ENDDATEN_BIT);					// Bit fuer Enddaten setzen
				}
				bitpos=0;
			}	// if bitpos
		}						//  Datenarray in inbuffer laden, Daten von outbuffer senden
	}						// if (spistatus & (1<<ACTIVE_BIT))
	
}		// ISR


int main (void)
{
	/* INITIALIZE */
//	LCD_DDR |=(1<<LCD_RSDS_PIN);
//	LCD_DDR |=(1<<LCD_ENABLE_PIN);
//	LCD_DDR |=(1<<LCD_CLOCK_PIN);
	
	
	
	slaveinit();
	
	lcd_initialize(LCD_FUNCTION_8x2, LCD_CMD_ENTRY_INC, LCD_CMD_ON);
	lcd_puts("Guten Tag\0");
	delay_ms(1000);
   
	lcd_cls();
	lcd_puts("READY\0");
	
	// DS1820 init-stuff begin
   
   uart_init();
   
   InitSPI_Slave();
   
  // sei();   // Wird nur gebraucht bei der Interrupt-Version
   
  volatile char text[8]="********";
   
   UART_PORT &= ~(1<<CTS_PIN);
   _delay_ms(2);
   
    
   
    
   
   
 //  _textcolor(MAGENTA);
 //  _textbackground(GREEN);
   
  // _textcolor(WHITE);
   
   //gotoxy(10,2);
   /*
   lcd_putc('C');
   _putch('R');
   lcd_putc('D');
   _putch('u');
   _putch('e');
   _putch('d');
   _putch('i');
   _putch(' ');
   _putch('H');
   _putch('e');
   _putch('i');
   _putch('m');
   _putch('l');
   _putch('i');
   _putch('c');
   _putch('h');
   _putch('e');
   _putch('r');
*/
  // _newline();
   
//   UART_PORT |= (1<<CTS_PIN);
   uint8_t linecounter=0;
   
   uint8_t SPI_Call_count0=0;

   lcd_puts(" UART\0");
#pragma mark while
  // OSZIA_HI;
   char teststring[] = {"p,10,12"};
   
   lcd_gotoxy(0,0);
   lcd_puts(teststring);
   /*
    _putch(0x1B);
    _putch('[');
    _putch((y/10)+'0');
    _putch((y%10)+'0');
    _putch(';');
    _putch((x/10)+'0');
    _putch((x%10)+'0');
    _putch('f');

    */
   
   lcd_gotoxy(0,1);
   lcd_putint2(strlen(teststring));
   lcd_putc(' ');
   lcd_putc(teststring[0]);
   lcd_putc(' ');
   lcd_putc(teststring[1]);
   lcd_putc(' ');
   lcd_putc(teststring[2]);

   lcd_putc(' ');
   lcd_putc(teststring[3]);
   lcd_putc(' ');
   lcd_putc(teststring[4]);

   lcd_putc(' ');
   lcd_putc(teststring[5]);
   lcd_putc(' ');
   lcd_putc(teststring[6]);
   lcd_putc('*');

   
   vga_start();
   
   startcounter = 0;
	while (1)
	{
		
		loopCount0 ++;
		
		if (loopCount0 >=0x00FF)
		{
			
			loopCount1++;
			
			if ((loopCount1 >0x02FF) )//&& (!(Programmstatus & (1<<MANUELL))))
			{
             LOOPLED_PORT ^= (1<<LOOPLED_PIN);
             {
               loopCount2++;
               vga_command("f,2");
               puts("HomeCentral ");
               puts("Rueti ");
               putch(' ');
              
               if (loopCount2 > 2)
               {
                  newline();
                  linecounter++;
                  if (linecounter>50)
                  {
                     linecounter=0;
                     vga_command("e");
                  }
                  if (linecounter %3==0)
                  {
                     //linecounter =0;
                     linecounter+=1;
                     gotoxy(8,linecounter);
                     vga_command("f,2");
                     puts("Stop");
                     putint_right(linecounter);
                     putch(' ');
                     putint1(linecounter);
                     //newline();
                     vga_command("f,1");
                     //vga_command("e");
                     puts("Daten");
                     vga_command("f,2");
                     //vga_command("e");
                     
                     gotoxy(0,linecounter);
                     vga_command("f,2");
                     newline();
                  }
                  loopCount2=0;
               }
               
               loopCount1=0;
            } // if startcounter
            
         }
			
			loopCount0 =0;
		}
      
      /* *** SPI begin **************************************************************/
		
		//lcd_gotoxy(19,0);
		//lcd_putc('-');
		
		// ***********************
		
      if (SPI_CONTROL_PORTPIN & (1<< SPI_CONTROL_CS_HC)) // SPI ist Passiv
		{
			// ***********************
			/*
			 Eine Uebertragung hat stattgefunden.
			 (Die out-Daten sind auf dem Webserver.)
			 Die in-Daten vom Webserver sind geladen.
			 Sie muessen noch je nach in_startdaten ausgewertet werden.
			 */
         
			// ***********************
//			SPI_CONTROL_PORT |= (1<<SPI_CONTROL_MISO); // MISO ist HI in Pausen
			
#pragma mark PASSIVE
         
			if (spistatus &(1<<ACTIVE_BIT)) // Slave ist neu passiv geworden. Aufraeumen, Daten uebernehmen
			{
				
				wdt_reset();
				SPI_Call_count0++;
				// Eingang von Interrupt-Routine, Daten von Webserver
				lcd_gotoxy(19,0);
				lcd_putc(' ');
				
				// in lcd verschoben
				lcd_clr_line(2);
				lcd_gotoxy(0,2);
				
				// Eingang anzeigen
				lcd_puts("iW \0");
				lcd_puthex(in_startdaten);
				lcd_putc(' ');
				lcd_puthex(in_hbdaten);
				lcd_puthex(in_lbdaten);
				lcd_putc(' ');
				uint8_t j=0;
				for (j=0;j<4;j++)
				{
					//lcd_putc(' ');
					lcd_puthex(inbuffer[j]);
					//lcd_putc(inbuffer[j]);
				}
				OutCounter++;
				
				// Uebertragung pruefen
				
				lcd_gotoxy(6,0);
				lcd_puts("bc:\0");
				lcd_puthex(ByteCounter);
            
            lcd_gotoxy(0,0);
				lcd_puts("      \0");
            
            
				lcd_gotoxy(19,0);
				lcd_putc(' ');
				lcd_gotoxy(19,0);
				if (ByteCounter == SPI_BUFSIZE-1) // Uebertragung war vollstaendig
				{
					if (out_startdaten + in_enddaten==0xFF)
					{
						lcd_putc('+');
						spistatus |= (1<<SUCCESS_BIT); // Bit fuer vollstaendige und korrekte  Uebertragung setzen
						lcd_gotoxy(19,0);
						lcd_putc(' ');
						//lcd_clr_line(3);
						//lcd_gotoxy(0,1);
						//lcd_puthex(loopCounterSPI++);
						//lcd_puts("OK \0");
                  
						//lcd_puthex(out_startdaten + in_enddaten);
						//					if (out_startdaten==0xB1)
						{
							SendOKCounter++;
						}
						spistatus |= (1<<SPI_SHIFT_IN_OK_BIT);
					}
					else
					{
						spistatus &= ~(1<<SUCCESS_BIT); // Uebertragung fehlerhaft, Bit loeschen
						lcd_putc('-');
						lcd_clr_line(1);
						lcd_gotoxy(0,1);
						lcd_puts("ER1\0");
                  lcd_putc(' ');
						lcd_puthex(out_startdaten);
						lcd_puthex(in_enddaten);
						lcd_putc(' ');
						lcd_puthex(out_startdaten + in_enddaten);
                  
                  spistatus &= ~(1<<SPI_SHIFT_IN_OK_BIT);
 						{
							SendErrCounter++;
						}
						//errCounter++;
					}
					
				}
				else
				{
					spistatus &= ~(1<<SUCCESS_BIT); //  Uebertragung unvollstaendig, Bit loeschen
					lcd_clr_line(0);
					lcd_gotoxy(0,0);
					lcd_puts("ER2\0");
					lcd_putc(' ');
               lcd_puthex(out_startdaten);
               lcd_puthex(in_enddaten);
               lcd_putc(' ');
               lcd_puthex(out_startdaten + in_enddaten);
               
					//delay_ms(100);
					//errCounter++;
					IncompleteCounter++;
               spistatus &= ~(1<<SPI_SHIFT_IN_OK_BIT);
				}
				
				//lcd_gotoxy(11, 1);							// Events zahelen
				//lcd_puthex(OutCounter);
				/*
				 lcd_puthex(SendOKCounter);
				 lcd_puthex(SendErrCounter);
				 lcd_puthex(IncompleteCounter);
				 */
				/*
				 lcd_gotoxy(0,0);
				 lcd_putc('i');
				 lcd_puthex(in_startdaten);
				 lcd_puthex(complement);
				 lcd_putc(' ');
				 lcd_putc('a');
				 lcd_puthex(out_startdaten);
				 lcd_puthex(in_enddaten);
				 lcd_putc(' ');
				 lcd_putc('l');
				 lcd_puthex(in_lbdaten);
				 lcd_putc(' ');
				 lcd_putc('h');
				 lcd_puthex(in_hbdaten);
				 out_hbdaten++;
				 out_lbdaten--;
				 
				 lcd_putc(out_startdaten);
				 */
				/*
				 lcd_gotoxy(0,0);
				 lcd_puthex(inbuffer[9]);
				 lcd_puthex(inbuffer[10]);
				 lcd_puthex(inbuffer[11]);
				 lcd_puthex(inbuffer[12]);
				 lcd_puthex(inbuffer[13]);
				 */
				//lcd_gotoxy(13,0);								// SPI - Fehler zaehlen
				//lcd_puts("ERR    \0");
				//lcd_gotoxy(17,0);
				//lcd_puthex(errCounter);
				
				// Bits im Zusammenhang mit der Uebertragung zuruecksetzen. Wurden in ISR gesetzt
				spistatus &= ~(1<<ACTIVE_BIT);		// Bit 0 loeschen
				spistatus &= ~(1<<STARTDATEN_BIT);	// Bit 1 loeschen
				spistatus &= ~(1<<ENDDATEN_BIT);		// Bit 2 loeschen
				spistatus &= ~(1<<SUCCESS_BIT);		// Bit 3 loeschen
				spistatus &= ~(1<<LB_BIT);				// Bit 4 loeschen
				spistatus &= ~(1<<HB_BIT);				// Bit 5 loeschen
				
				// aufraeumen
				out_startdaten=0x00;
				out_hbdaten=0;
				out_lbdaten=0;
				for (int i=0;i<SPI_BUFSIZE;i++)
				{
					outbuffer[i]=0;
				}
				
				/*
             lcd_gotoxy(0,0);				// Fehler zaehlen
             lcd_puts("IC   \0");
             lcd_gotoxy(2,0);
             lcd_puthex(IncompleteCounter);
             lcd_gotoxy(5,0);
             lcd_puts("TW   \0");
             lcd_gotoxy(7,0);
             lcd_puthex(TWI_errCounter);
             
             lcd_gotoxy(5,1);
             lcd_puts("SE   \0");
             lcd_gotoxy(7,1);
             lcd_puthex(SendErrCounter);
             */
			} // if Active-Bit
         
#pragma mark HomeCentral-Tasks 
         
		} // neu Passiv
      
		// letzte Daten vom Webserver sind in inbuffer und in in_startdaten, in_lbdaten, in_hbdaten
		
      
		else						// (IS_CS_HC_ACTIVE)
		{
			if (!(spistatus & (1<<ACTIVE_BIT))) // CS ist neu aktiv geworden, Daten werden gesendet, Active-Bit 0 ist noch nicht gesetzt
			{
				// Aufnahme der Daten vom Webserver vorbereiten
				uint8_t j=0;
				in_startdaten=0;
				in_enddaten=0;
				in_lbdaten=0;
				in_hbdaten=0;
				for (j=0;j<SPI_BUFSIZE;j++)
				{
					inbuffer[j]=0;
				}
				
				spistatus |=(1<<ACTIVE_BIT); // Bit 0 setzen: neue Datenserie
				spistatus |=(1<<STARTDATEN_BIT); // Bit 1 setzen: erster Wert ergibt StartDaten
				
				bitpos=0;
				ByteCounter=0;
				//timer0(); // Ueberwachung der Zeit zwischen zwei Bytes. ISR setzt bitpos und ByteCounter zurueck, loescht Bit 0 in spistatus
				
				// Anzeige, das  rxdata vorhanden ist
				lcd_gotoxy(19,0);
				lcd_putc('$');
            
				
				
				
			}//										if (!(spistatus & (1<<ACTIVE_BIT)))
		}//											(IS_CS_HC_ACTIVE) 
		
		/* *** SPI end **************************************************************/
      
      
      
	} // while
	
	
	return 0;
}
