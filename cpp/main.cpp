#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define UART_BAUD  9600
#define _BAUD (((F_CPU / (UART_BAUD * 16UL))) - 1)

static void uart_init(void)
{
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	UBRR0L = (u8)(_BAUD & 0xFF);
	UBRR0H = (u8)((_BAUD >> 8) & 0xFF);
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

static char uart_rx(void)
{
	while(!(UCSR0A & (1 << RXC0))) ;
	return UDR0;
}

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

class SRAM
{
private:
	u32 last_idx;
	u8 last_value, data[1];

public:
	SRAM()
	{
		last_idx = 0xFFFFFFFF;
		last_value = 0;
		data[0] = 0;
	}

	u8& operator[] (u32 i)
	{
		if(i != last_idx)
		{
			if(data[0] != last_value)
			{
				sram_write(last_idx, data, 1);
			}

			sram_read(i, data, 1);
			last_idx = i;
			last_value = data[0];
		}

		return data[0];
	}
};

int main(void)
{
	u16 i;
	char c;
	SRAM sram;
	spi_init();
	uart_init();
	for(;;)
	{
		i = 0;
		while((c = uart_rx()) != '\n' && c != '\r')
		{
			sram[i++] = c;
		}

		sram[i++] = '\n';
		sram[i] = '\0';
		uart_tx_str_P(PSTR("HERE\r\n"));
		for(i = 0; sram[i]; ++i)
		{
			uart_tx(sram[i]);
		}
	}

	return 0;
}

