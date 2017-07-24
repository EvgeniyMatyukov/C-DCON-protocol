//работа с uart
#define RXB8 1
#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7
#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<DOR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)

// USART0 Transmitter buffer
#define TX_BUFFER_SIZE0 128
volatile unsigned char tx_buffer0[TX_BUFFER_SIZE0];

#if TX_BUFFER_SIZE0<256
volatile unsigned char tx_wr_index0=0,tx_rd_index0=0,tx_counter0=0;
#else
volatile unsigned int tx_wr_index0=0,tx_rd_index0=0,tx_counter0=0;
#endif

// USART0 Receiver buffer
#define RX_BUFFER_SIZE0 128
volatile unsigned char rx_buffer0[RX_BUFFER_SIZE0];

#if RX_BUFFER_SIZE0<256
volatile unsigned char rx_wr_index0=0,rx_rd_index0=0,rx_counter0=0;
#else
volatile unsigned int rx_wr_index0=0,rx_rd_index0=0,rx_counter0=0;
#endif

// This flag is set on USART0 Receiver buffer overflow
volatile bool rx_buffer_overflow0=0;

volatile bool readlineen=0;
volatile bool writelineen=0; //отправляется строка из буфера?

//#define TXEN PORTE,2      //при передаче через уарт включать эту линию, чтобы max485 переключила режим


// USART0 Receiver interrupt service routine
ISR (USART0_RX_vect)
{
	char status,data;
	status=UCSR0A;
	data=UDR0;
//	PORTA^=0xFF;
	
	if (!rx_counter0 && (data!='@') && (data!='$') && (data!='%')) return; //если первый символ не равен @, $, % выход 

	if (writelineen) return; //не будем принимать то, что сами отправляем ТОЛЬКО ДЛЯ СКРОЛЛБОКСА!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
	{
		rx_buffer0[rx_wr_index0]=data;
		if (data==0x0D) {
			readlineen=1; //если принята строка, то обработать её
	//	PORTA^=0xFF;
		}

		if (++rx_wr_index0 == RX_BUFFER_SIZE0) rx_wr_index0=0;
		if (++rx_counter0 == RX_BUFFER_SIZE0)
		{
			rx_counter0=0;
			rx_buffer_overflow0=1;
		};

	};
}




char uart_getchar(void)
{
	char data;
	while (rx_counter0==0);
	data=rx_buffer0[rx_rd_index0];
	if (++rx_rd_index0 == RX_BUFFER_SIZE0) rx_rd_index0=0;
	asm("cli");
	--rx_counter0;
	asm("sei");
	return data;
}



// USART0 Transmitter interrupt service routine
ISR (USART0_TX_vect)
{
	if (tx_counter0)
	{
		--tx_counter0;
		UDR0=tx_buffer0[tx_rd_index0];
		if (++tx_rd_index0 == TX_BUFFER_SIZE0) tx_rd_index0=0;
	};
}


void uart_putchar(char c)
{
	while (tx_counter0 == TX_BUFFER_SIZE0);
	asm("cli");
	if (tx_counter0 || ((UCSR0A & DATA_REGISTER_EMPTY)==0))
	{
		tx_buffer0[tx_wr_index0]=c;
		if (++tx_wr_index0 == TX_BUFFER_SIZE0) tx_wr_index0=0;
		++tx_counter0;
	}
	else
	UDR0=c;
	asm("sei");
}


void uart_writeline(char *string) //отправить строку через буфер (на прерываниях)
{
	unsigned char sym=0;     //указатель на текущий символ

	while(string[sym])  {     //пока символ не нулевой
		uart_putchar(string[sym]);  //отправить текущий символ
		sym++;                 //обработать следующий символ
	}

}

//для скроллбокса
void writeline(char *string) //отправить строку через буфер (на прерываниях)
{
	unsigned char sym=0;     //указатель на текущий символ
//	PORTA^=0b00000001;
	
	writelineen=1;  //устанавливаем флаг, что мы отправляем строку
	PORTD|=0b00110000; //TXEN=1; //даём команду max485 переключиться в режим передачи
	
	switch(uart_speed) { //задержка на один символ для защиты от помех
		case '3':	//1200
			_delay_us(10000);
			break;
		case '4':	//2400
			_delay_us(5000);
			break;
		case '5':	//4800
			_delay_us(2500);
			break;
		case '6':	//9600
			_delay_us(2500);
			break;
		case '7':	//19200
			_delay_us(1200);
			break;
		case '8':	//38400
			_delay_us(300);
			break;
		case '9':	//57600
			_delay_us(200);
			break;
		case 'A':	//115200
			_delay_us(100);
			break;
				
		default:	//ошибка
			_delay_ms(100);
	}
	
	while(string[sym])  {     //пока символ не нулевой
		uart_putchar(string[sym]);  //отправить текущий символ
		sym++;                 //обработать следующий символ
	}
}



void uart_writeline(char *string, unsigned int sym) //отправить строку через буфер (на прерываниях), sym - количество знаков в строке
{
	unsigned int length=sym;
	while(sym)  {     //пока символ не нулевой
		uart_putchar(string[length-sym]);  //отправить текущий символ
		sym--;
	}
}

