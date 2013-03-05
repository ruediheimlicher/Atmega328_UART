#include <stdlib.h>
#include <string.h>

/*
 
 Funktion zur Umwandlung einer vorzeichenlosen 32 Bit Zahl in einen String
 
 */




void r_itoa8(int8_t zahl, char* string)
{
   uint8_t i;
   
   string[4]='\0';                  // String Terminator
   if( zahl < 0 )
   {                  // ist die Zahl negativ?
      string[0] = '-';
      zahl = -zahl;
   }
   else string[0] = ' ';             // Zahl ist positiv
   
   for(i=3; i>=1; i--)
   {
      string[i]=(zahl % 10) +'0';     // Modulo rechnen, dann den ASCII-Code von '0' addieren
      zahl /= 10;
   }
}

void r_uitoa8(int8_t zahl, char* string)
{
   uint8_t i;
   
   string[3]='\0';                  // String Terminator
   for(i=3; i>=0; i--)
   {
      string[i]=(zahl % 10) +'0';     // Modulo rechnen, dann den ASCII-Code von '0' addieren
      zahl /= 10;
   }
}




uint8_t Hex2Int(char *s)
{
   long res;
   char *Chars = "0123456789ABCDEF", *p;
   
   if (strlen(s) > 8)
   /* Error ... */ ;
   
   for (res = 0L; *s; s++) {
      if ((p = strchr(Chars, toupper(*s))) == NULL)
      /* Error ... */ ;
      res = (res << 4) + (p-Chars);
   }
   
   return res;
}

void int2hexstring(uint8_t zahl,char* hexstring)
{
	//char string[5];
	//char string[3];
	uint8_t i,l,h;                             // schleifenzähler
	
	hexstring[2]='\0';                       // String Terminator
	l=(zahl % 16);
	if (l<10)
      hexstring[1]=l +'0';
	else
	{
      l%=10;
      hexstring[1]=l + 'A';
      
	}
	zahl /=16;
	h= zahl % 16;
	if (h<10)
      hexstring[0]=h +'0';
	else
	{
      h%=10;
      hexstring[0]=h + 'A';
	}
	
	
	
}



void vga_tempbis99(uint16_t temperatur,char*tempbuffer)
{
	char buffer[8]={};
	//uint16_t temp=(temperatur-127)*5;
	uint16_t temp=temperatur*5;
	
	//itoa(temp, buffer,10);
	
	r_itoa16(temp,buffer);
	
	
	//char outstring[7]={};
	
	tempbuffer[6]='\0';
	tempbuffer[5]=' ';
	tempbuffer[4]=buffer[6];
	tempbuffer[3]='.';
	tempbuffer[2]=buffer[5];
	if (abs(temp)<100)
	{
		tempbuffer[1]=' ';
		
	}
	else
	{
		tempbuffer[1]=buffer[4];
		
	}
	tempbuffer[0]=buffer[0];
	
	
}

void tempAbMinus20(uint16_t temperatur,char*tempbuffer)
{
   
   char buffer[8]={};
   int16_t temp=(temperatur)*5;
   temp -=200;
   char Vorzeichen=' ';
   if (temp < 0)
   {
      Vorzeichen='-';
   }
   
   r_itoa16(temp,buffer);
   //		lcd_puts(buffer);
   //		lcd_putc(' * ');
   
   //		char outstring[7]={};
   
   tempbuffer[6]='\0';
   //outstring[5]=0xDF; // Grad-Zeichen
   tempbuffer[5]=' ';
   tempbuffer[4]=buffer[6];
   tempbuffer[3]='.';
   tempbuffer[2]=buffer[5];
   if (abs(temp)<100)
   {
		tempbuffer[1]=Vorzeichen;
		tempbuffer[0]=' ';
   }
   else
   {
		tempbuffer[1]=buffer[4];
		tempbuffer[0]=Vorzeichen;
   }
   //		lcd_puts(outstring);
}
