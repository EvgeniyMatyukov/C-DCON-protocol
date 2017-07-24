/*
 * DCON-RS485-32OUT-Controller.cpp
 *
 * Created: 26.01.2017 13:10:45
 * Author : МатюковЕ
 */ 
__attribute__((optimize(0)))

#define wdt_reset() \
		__asm__ __volatile__ ("wdr")

#define F_CPU 16000000L
#include <avr/io.h>
//#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

//здесь хранятся актуальные настройки уарт
volatile char uart_parity='0'; //none/odd
volatile char uart_speed='7';  //speed

#include "bitwise_operation.cpp" //функции работы с отдельными битами
#include "uart_scrollbox.cpp" //работа с uart
#include "MCU_Init.cpp" //инициализация микроконтроллера
#include "rtos.cpp"
#include "dcon.cpp"

//таймер0 вызывается каждую 1 мс
ISR (TIMER0_COMP_vect)
{
	TCNT0=0x00; //сброс таймера 0
	
	RTOS_Timer(); //rtos	

}

void printtest(void)
{
	
	
	char testOk[]="-\r"; 
	writeline(testOk);
	
	PORTA^=0b00000001;
	
	
}
	



int main(void)
{
	MCU_Init(); //инициализация микроконтроллера
	RTOS_Init();

	// Global enable interrupts
	asm("sei");

	//считываем настройки из eeprom
	MainAddr1[0]=eeprom_read_byte(&eeMainAddr10); //первый адрес эмулируемого icpcon
	MainAddr1[1]=eeprom_read_byte(&eeMainAddr11);
	MainAddr2[0]=eeprom_read_byte(&eeMainAddr20); //второй адрес эмулируемого icpcon
	MainAddr2[1]=eeprom_read_byte(&eeMainAddr21);
	
	setuart(); //установить скорость и контроль четности уарт

//	RTOS_SetTask(printtest,100,500);
	
	while (1)
	{
		wdt_reset();
		RTOS_DispatchTask(); //rtos
		
		
		if (readlineen) readline();  //если установлен флаг принятой строки, то обработать эту строку
		
	//	if (!(tx_counter0 || ((UCSR0A & DATA_REGISTER_EMPTY)==0))) {
	
		if (!tx_counter0 && (UCSR0A & DATA_REGISTER_EMPTY) && !(UCSR0A & TXC)) { //если строка отправилась
			switch(uart_speed) { ////задержка на один символ, чтобы последний символ отправился
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

		
		
		
			PORTD&=0b11001111;//TXEN=0; //даём команду max485 перейти в режим приёма

			writelineen=0;  //предыдущая строка отправилась? разрешаем прием
		}

	}
}

