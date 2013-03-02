


// UART
#define UART_PORT		PORTC
#define UART_PIN		PINC
#define UART_DDR		DDRC
//#define RTS_PIN      5
//#define CTS_PIN      4


// SR
#define SR_PORT               PORTB
#define SR_DDR                DDRB
#define SR_PIN                PINB
#define SR_CLK_PIN            4
#define SR_DATA_PIN				5
#define SR_LOAD_PIN				6

// TWI

#define TWI_PORT		PORTC
#define TWI_PIN		PINC
#define TWI_DDR		DDRC

#define SDAPIN       4
#define SCLPIN       5
#define TWI_ERR_BIT				7


#define LOOPLED_PORT	PORTD
#define LOOPLED_DDR	DDRD
#define LOOPLED_PIN	7

// Tastatur
#define TASTE1          19
#define TASTE2          29
#define TASTE3          44
#define TASTE4          67
#define TASTE5          94
#define TASTE6          122
#define TASTE7          155
#define TASTE8          180
#define TASTE9          205
#define TASTEL          220
#define TASTE0          236
#define TASTER          248
#define TASTATURPORT    PORTC
#define TASTATURPIN		3


// Manuell
#define MANUELL_PORT		PORTD
#define MANUELL_DDR		DDRD
#define MANUELL_PIN		PIND

#define MANUELL			7	// Bit 7 von Status
#define MANUELLPIN		6	// Pin 6 von PORT D fuer Anzeige Manuell
#define MANUELLNEU		7	// Pin 7 von Status. Gesetzt wenn neue Schalterposition eingestellt
#define MANUELLTIMEOUT	100 // Loopled-counts bis Manuell zurueckgesetzt wird. 02FF: ca. 100 s




// OSZI
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

uint16_t loopCount0=0;
uint16_t loopCount1=0;
uint16_t loopCount2=0;



volatile uint8_t					Programmstatus=0x00;
uint8_t Tastenwert=0;
uint8_t TastaturCount=0;
uint8_t Tastaturabfrage=0;
uint8_t tastaturstatus=0;

uint8_t Taste = 0;
volatile uint16_t		Manuellcounter=0; // Counter fuer Timeout
uint16_t TastenStatus=0;
uint16_t Tastencount=0;
uint16_t Tastenprellen=0x01F;

volatile uint8_t uartstatus=0;
uint16_t startcounter=0x00;

#define CURSORX   2
#define CURSORY   2
#define MAXMENUEBENE 2
uint8_t cursorx=0,cursory=0;
uint8_t menuebene=0;
