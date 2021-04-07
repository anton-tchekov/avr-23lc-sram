#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

#define SPI_DIR               DDRB
#define SPI_OUT               PORTB
#define SPI_MOSI             3
#define SPI_MISO             4
#define SPI_SCK              5
#define SPI_CS_0             0
#define SPI_CS_1             1
#define SPI_CS_2             2

#define SPI_SELECT_BANK_0()   SPI_OUT &= ~(1 << SPI_CS_0);
#define SPI_DESELECT_BANK_0() SPI_OUT |= (1 << SPI_CS_0);

#define SPI_SELECT_BANK_1()   SPI_OUT &= ~(1 << SPI_CS_1);
#define SPI_DESELECT_BANK_1() SPI_OUT |= (1 << SPI_CS_1);

#define SPI_SELECT_BANK_2()   SPI_OUT &= ~(1 << SPI_CS_2);
#define SPI_DESELECT_BANK_2() SPI_OUT |= (1 << SPI_CS_2);

#define SRAM_COMMAND_READ    3
#define SRAM_COMMAND_WRITE   2

static void spi_init(void);
static u8 spi_xchg(u8 data);

static void sram_read(u32 addr, void *data, u16 size);
static void sram_write(u32 addr, void *data, u16 size);

static void spi_init(void)
{
	SPI_DIR = (1 << SPI_MOSI) | (1 << SPI_SCK) |
			(1 << SPI_CS_0) | (1 << SPI_CS_1) | (1 << SPI_CS_2);
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);
}

static u8 spi_xchg(u8 data)
{
	SPDR = data;
	while(!(SPSR & (1 << SPIF))) ;
	return SPDR;
}

static void sram_read(u32 addr, void *data, u16 size)
{
	u16 i;
	u8 *data8;
	SPI_SELECT_BANK_0();
	data8 = (u8 *)data;
	spi_xchg(SRAM_COMMAND_READ);
	spi_xchg((u8)((addr >> 16) & 0xFF));
	spi_xchg((u8)((addr >> 8) & 0xFF));
	spi_xchg((u8)(addr & 0xFF));
	for(i = 0; i < size; ++i)
	{
		data8[i] = spi_xchg(0xFF);
	}

	SPI_DESELECT_BANK_0();
}

static void sram_write(u32 addr, void *data, u16 size)
{
	u16 i;
	u8 *data8;
	SPI_SELECT_BANK_0();
	data8 = (u8 *)data;
	spi_xchg(SRAM_COMMAND_WRITE);
	spi_xchg((u8)((addr >> 16) & 0xFF));
	spi_xchg((u8)((addr >> 8) & 0xFF));
	spi_xchg((u8)(addr & 0xFF));
	for(i = 0; i < size; ++i)
	{
		spi_xchg(data8[i]);
	}

	SPI_DESELECT_BANK_0();
}

#define UART_BAUD  9600
#define _BAUD (((F_CPU / (UART_BAUD * 16UL))) - 1)

static void uart_init(void)
{
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	UBRR0L = (uint8_t)(_BAUD & 0xFF);
	UBRR0H = (uint8_t)((_BAUD >> 8) & 0xFF);
}

static void uart_tx(char c)
{
	while(!(UCSR0A & (1 << UDRE0))) ;
	UDR0 = c;
}

static void uart_tx_str(const char *s)
{
	register char c;
	while((c = *s++)) { uart_tx(c); }
}

static void uart_tx_str_P(const char *s)
{
	register char c;
	while((c = pgm_read_byte(s++))) { uart_tx(c); }
}

int main(void)
{
	char buf0[] = "Hello World! This is a Memory Test String that is being copied three times (once to every sram bank).\r\n", buf1[512];

	uart_init();
	uart_tx_str_P(PSTR("23LC1024 Memory Test\r\n"));
	spi_init();

	sram_write(0, buf0, sizeof(buf0));
	sram_read(0, buf1, sizeof(buf0));
	uart_tx_str(buf1);

	uart_tx_str_P(PSTR("Test Complete\r\n"));

	for(;;)
	{
	}

	return 0;
}
