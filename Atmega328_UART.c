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

#include "def.h"
#include "lcd.c"
#include "adc.c"
#include "kbd.h"
//#include "fifo.c"
//#include <util/setbaud.h>
#include "SPI_slave.c"
#include "uart8.c"

//#include "onewire.c"
//#include "ds18x20.c"
//#include "crc8.c"



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


void setHomeCentral(void)
{
   //char header[]  = {"w,1,0,0,100,3,1,"};                 // no title

   //char footer[]  = {"w,5,0,49,100,1,0,"};
   // char window2[] = {"w,2,0,3,60,40,1,Data Webserver"};
   vga_command("r");
   //delay_ms(1000);
   uint8_t posy=0;
   //vga_command(header);       // Create header
   setFeld(1,0,0,100,3,1,"");
   vga_command("f,1");
   puts("Home Central Rueti");
   

   posy+= 3;
   setFeld(2,0,posy,100,6,1,""); // Heizung
   vga_command("f,2");
   puts("Heizung");

   posy+= 6;
   setFeld(3,0,posy,100,3,1,""); // Werkstatt
   vga_command("f,3");
   puts("Werkstatt");

   posy+= 3;
   setFeld(4,0,posy,100,3,1,""); // WoZi
   vga_command("f,1");
   puts("WoZi");

   

}

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

void readSR (void)
{
	// Schalterstellung in SR laden
	SR_PORT &= ~(1<<SR_LOAD_PIN); // PL> LO
	_delay_us(10);
	SR_PORT |= (1<<SR_LOAD_PIN); // PL> HI
	_delay_us(1);
	uint8_t i=0;
	Tastaturabfrage=0;
   return;
	//Read_Device=0x00;
	//Daten aus SR schieben
	for (i=0;i<8;i++)
	{
		//uint8_t pos=15-i;
		// Bit lesen
		if (SR_PIN & (1<<SR_DATA_PIN)) // PIN ist Hi, OFF
		{
			//Bit is HI
			if (i<8) // Byte 1
			{
				Tastaturabfrage |= (0<<(i)); // auf Device i soll geschrieben werden
			}
         /*
			else
			{
				Read_Device |= (0<<(i-8));	// von Device i soll gelesen werden
			}
			*/
			
		}
		else
		{
			//Bit is LO
			if (i<8) // Byte 1
			{
				Tastaturabfrage |= (1<<i); // auf Device i soll geschrieben werden
			}
         /*
			else
			{
				Read_Device |= (1<<(i-8));	// von Device i soll gelesen werden
			}
			*/
		}
		
		// SR weiterschieben
		
		SR_PORT &= ~(1<<SR_CLK_PIN); // CLK LO
		_delay_us(10);
		SR_PORT |= (1<<SR_CLK_PIN); // CLK HI, shift
		_delay_us(10);
		
	} // for i
	
}


void slaveinit(void)
{
	MANUELL_DDR |= (1<<MANUELLPIN);		//Pin 5 von PORT D als Ausgang fuer Manuell
	MANUELL_PORT &= ~(1<<MANUELLPIN);

	LOOPLED_DDR |= (1<<LOOPLED_PIN);

	DDRB &= ~(1<<PORTB0);	//Bit 1 von PORT B als Eingang fuer Taste
	PORTB |= (1<<PORTB0);	//HI
   
	

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
   SR_DDR |= (1<<SR_PIN); // Schiebetakt
   SR_PORT |= (1<<SR_PIN);

   ADC_DDR &= ~(1<<TASTATURKANAL);	//Pin 4 von PORT C als Eingang fuer Tastatur
   ADC_PORT &= ~(1<<TASTATURKANAL); //Pull-up

   
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
      BitCounter++;
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
   
	//lcd_cls();
	//lcd_puts("READY\0");
	delay_ms(1000);
	// DS1820 init-stuff begin
   
   uart_init();
   
   InitSPI_Slave();
   
   uint8_t linecounter=0;
   
   uint8_t SPI_Call_count0=0;
   lcd_cls();
   lcd_puts("UART\0");
   
#pragma mark while
  // OSZIA_HI;
   char teststring[] = {"p,10,12"};
   
   
   initADC(TASTATURKANAL);
   
 //  vga_start();
   
   setHomeCentral();
   vga_command("f,1");
   startcounter = 0;
   linecounter=0;
   uint8_t lastrand=rand();
   srand(1);
   sei();
   uint8_t x=0;
	while (1)
	{
		loopCount0 ++;
		
		if (loopCount0 >=0x00FF)
		{
			//readSR();
			loopCount1++;
			
			if ((loopCount1 >0x01FF) )//&& (!(Programmstatus & (1<<MANUELL))))
			{
             LOOPLED_PORT ^= (1<<LOOPLED_PIN);
             {
             lcd_gotoxy(15,3);
            lcd_putint(BitCounter);
                /*
               loopCount2++;
               vga_command("f,2");
               //puts("HomeCentral ");
                puts("Tastenwert ");
               putint(Tastenwert);
               putch(' ');
              newline();
                 */
                /*
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
                     putint(Tastenwert);
                     //newline();
                     vga_command("f,1");
                     vga_command("e");
                     puts("Daten");
                     putint_right(linecounter);
                     vga_command("f,2");
                     //vga_command("e");
                     
                     gotoxy(0,linecounter);
                     vga_command("f,2");
                     newline();
                  }
                  loopCount2=0;
               }
               */
               loopCount1=0;
               
            } // if startcounter
            
         }
			
			loopCount0 =0;
      //
         
         goto NEXT;
         {
         cli();
         
         {
         Tastenwert=(uint8_t)(readKanal(TASTATURKANAL)>>2);
         
         //lcd_gotoxy(12,1);
         //lcd_puts("TW:\0");
         //lcd_putint(Tastenwert);
         if (Tastenwert>5) // ca Minimalwert der Matrix
         {
             tastaturstatus |= (1<<1);
            //			wdt_reset();
            /*
             0: Wochenplaninit
             1: IOW 8* 2 Bytes auf Bus laden
             2: Menu der aktuellen Ebene nach oben
             3: IOW 2 Bytes vom Bus in Reg laden
             4: Auf aktueller Ebene nach rechts (Heizung: Vortag lesen und anzeigen)
             5: Ebene tiefer
             6: Auf aktueller Ebene nach links (Heizung: Folgetag lesen und anzeigen)
             7:
             8: Menu der aktuellen Ebene nach unten
             9: DCF77 lesen
             
             12: Ebene höher
             */
            TastaturCount++;
            if (tastaturstatus & (1<<1))
            {
               
            }
            
            if (TastaturCount>=80)	//	Prellen
            {
               tastaturstatus &= ~(1<<1);
               Taste=Tastenwahl(Tastenwert);
               lcd_gotoxy(0,1);
               lcd_puts("T:\0");
               //
               lcd_putc(' ');
               lcd_putint(Tastenwert);
               lcd_putc(' ');
                if (Taste >=0)
               {
                  lcd_putint2(Taste);
               }
               else
               {
                  lcd_putc('*');
               }
               
               lcd_gotoxy(14,1);
               
               lcd_putint(linecounter);
               lcd_putc(' ');
               lcd_putint((uint8_t)rand()%40);
               
               switch (Taste)
               {
                  case 1:
                  {
                     vga_command("f,2");
                     gotoxy(0,2);
                     vga_command("f,2");
                     puts("Brenner: ");
                  }break;
                  case 2:
                  {
                     vga_command("f,3");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch(' ');

                     cursory--;
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch('>');
                     
                  }break;
                  case 3:
                  {
                     
                  }break;
                  case 4:
                  {
                     if (menuebene)
                     {
                        menuebene--;
                     if (cursorx>10)
                     {
                     vga_command("f,3");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch(' ');
                     cursorx-=10;
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch('>');
                     }
                     }

                  }break;
                  case 5:
                  {
                     vga_command("f,3");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch('>');

                  }break;
                  case 6:
                  {
                     if (menuebene<MAXMENUEBENE)
                     {
                        menuebene++;
                     if (cursorx<40)
                     {
                        vga_command("f,3");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch(' ');
                     
                     cursorx+=10;
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch('>');
                     }
                     }
                  }break;
                  case 7:
                  {
                     
                     setFeld(3,70,3,30,32,1,"");
                     vga_command("f,3");
                     //putch('x');
                     //lcd_putc('+');
                     
                  }break;
                  case 8:
                  {
                     vga_command("f,3");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch(' ');

                     cursory++;
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch('>');

                  }break;
                  case 9:
                  {
                     
                  }break;
                  case 10:
                  {
                     setHomeCentral();
                  }break;
                  case 12:
                  {
                     vga_command("f,3");
                     vga_command("e");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch(' ');
                     cursorx = CURSORX;
                     cursory = CURSORY;
                     gotoxy(CURSORX+1,CURSORY);
                     vga_command("f,3");
                     puts("Alpha");
                     gotoxy(CURSORX+1,CURSORY+1);
                     vga_command("f,3");
                     puts("Beta");
                     gotoxy(CURSORX+1,CURSORY+2);
                     vga_command("f,3");
                     puts("Gamma");
                     gotoxy(CURSORX+1,CURSORY+3);
                     vga_command("f,3");
                     puts("Delta");
                     gotoxy(cursorx,cursory);
                     vga_command("f,3");
                     putch('>');

                  }break;
                     
                     
               } // switch Taste
               
               
               
               lastrand = rand();
               vga_command("f,2");
               //putch(' ');
               
               //gotoxy(4,linecounter);
               //lcd_gotoxy(16,1);
               //lcd_putint(erg);
               /*
               putint(linecounter);
               putch(' ');
               putint_right(Tastenwert);
               putch(' ');
               putint_right(Taste);
               */
               //newline();
               linecounter++;
               TastaturCount=0;

            }
            
         } // if Tastenwert
            
         }
         
         }
      NEXT:
         x=0;
         sei();
         //
		}
      
      /* *** SPI begin **************************************************************/
		
		//lcd_gotoxy(19,0);
		//lcd_putc('-');
		
		// ***********************
		//goto ENDSPI;
      if (SPI_CONTROL_PORTPIN & (1<< SPI_CONTROL_CS_HC)) // PIN ist Hi, SPI ist Passiv
		{
        // lcd_gotoxy(19,1);
       //  lcd_putc('*');

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
				//lcd_clr_line(2);
				//lcd_gotoxy(0,2);
				
				// Eingang anzeigen
				lcd_puts("iW \0");
				lcd_puthex(in_startdaten);
				lcd_putc(' ');
				lcd_puthex(in_hbdaten);
				lcd_puthex(in_lbdaten);
				lcd_putc(' ');
				uint8_t j=0;
				for (j=0;j<1;j++)
				{
					//lcd_putc(' ');
					lcd_puthex(inbuffer[j]);
					//lcd_putc(inbuffer[j]);
				}
				OutCounter++;
				
				// Uebertragung pruefen
				
				//lcd_gotoxy(6,0);
				//lcd_puts("bc:\0");
				//lcd_puthex(ByteCounter);
            
            //lcd_gotoxy(0,0);
				//lcd_puts("      \0");
            
            
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
               lcd_puthex(ByteCounter);
               lcd_putc(' ');
               lcd_puthex(BitCounter);

               /*
					lcd_putc(' ');
               lcd_puthex(out_startdaten);
               lcd_puthex(in_enddaten);
               lcd_putc(' ');
               lcd_puthex(out_startdaten + in_enddaten);
               */
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
   ENDSPI:
		/* *** SPI end **************************************************************/
      
# pragma mark Tasten	
      
      if (!(PINB & (1<<PORTB0))) // Taste 0
		{
			//lcd_gotoxy(12,0);
			//lcd_puts("P0 Down\0");
			//wdt_reset();
			if (! (TastenStatus & (1<<PORTB0))) //Taste 0 war nicht nicht gedrueckt
			{
				TastenStatus |= (1<<PORTB0);
				Tastencount=0;
				//lcd_gotoxy(12,0);
				//lcd_puts("P0\0");
				//lcd_putint(TastenStatus);
				//delay_ms(800);
			}
			else
			{
				
				Tastencount ++;
				//lcd_gotoxy(7,1);
				//lcd_puts("TC \0");
				//lcd_putint(Tastencount);
				wdt_reset();
				if (Tastencount >= Tastenprellen)
				{
					//lcd_gotoxy(18,0);
					//lcd_puts("ON\0");
					//lcd_putint(TastenStatus);
					
					Tastencount=0;
					TastenStatus &= ~(1<<PORTB0);
					
				}
			}//else
			
		}

      
#pragma mark Tastatur
 
	} // while
	
	
   
	return 0;
}
