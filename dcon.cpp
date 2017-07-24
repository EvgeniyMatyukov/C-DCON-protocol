//программа обрабатывает протокол dcon
//реализована поддержка команд
//$AAM запросить название модуля
//@AA считать данные с дискретных входов - реализована частично
//@AAdata установить дискретные выходы
//%AAAA Настроить параметры конфигурации модуля
unsigned char eeMainAddr10 EEMEM='0'; //первый адрес эмулируемого icpcon
unsigned char eeMainAddr11 EEMEM='0';
unsigned char eeMainAddr20 EEMEM='0'; //второй адрес эмулируемого icpcon
unsigned char eeMainAddr21 EEMEM='1';
unsigned char uartparity   EEMEM='0'; //контроль четности '0'-none, '1'-odd, '2'-even
unsigned char uartspeed    EEMEM='7'; //скорость '3'-1200, '4'-2400, '5'-4800, '6'-9600, '7'-19200, '8'-38400, '9'-57600, 'A'-115200

char MainAddr1[]="00";  //первый адрес эмулируемого icpcon
char MainAddr2[]="01";  //второй адрес эмулируемого icpcon

void setuart(void) //установка скорости и контроля четности уарт
{
	
	uart_parity=eeprom_read_byte(&uartparity); //контроль четности
	uart_speed=eeprom_read_byte(&uartspeed); //скорость
	
	switch(uart_parity) {
		case '0':	//без контроля четности
				UCSR0C=0x06; //без контроля четности
				break;
		case '1':	//odd
				UCSR0C=0x36; //odd
				break;
//		case '2':	//even
//				break;
//				UCSR0C=0x26; //even
		default:	//ошибка
					return;
	}
			
	switch(uart_speed) { //скорость
		case '3':	//1200
				UBRR0H=0x03; // 1200 @ 16 MHz
				UBRR0L=0x40;
				break;
		case '4':	//2400
				UBRR0H=0x01; //2400 @ 16 MHz
				UBRR0L=0xA0;
				break;
		case '5':	//4800
				UBRR0H=0x00;
				UBRR0L=0xCF; //4800 @16 MHz
				break;
		case '6':	//9600
				UBRR0H=0x00;
				UBRR0L=0x67; //9600 @ 16 MHz
				break;
		case '7':	//19200
				UBRR0H=0x00;
				UBRR0L=0x33; //19200 @16 MHz
				break;
		case '8':	//38400
				UBRR0H=0x00;
				UBRR0L=0x19; //38400 @16 MHz
				break;
		case '9':	//57600
				UBRR0H=0x00;
				UBRR0L=0x10; //57600 @ 16 MHz, >2% error
				break;
		case 'A':	//115200
				UBRR0H=0x00;
				UBRR0L=0x08; //115200 @ 16 MHz, >3.7% error
				break;
				
		default:	//ошибка
				return;
	}
}


//функция проверяет корректность четырех символов, начиная с третьего (должны быть 0..9 или A..F)
bool verifystring(char *string)
{
		for (unsigned char i = 0; i < 4; i++)  //четыре знака
		{
			if (string[i+3] < 48 || string[i+3] > 70 || (string[i+3] > 57 && string[i+3] < 65)) return false; //некорректная строка
		}
		if (string[7] != '\r') return false; //строка слишком длинная
	return true;
}

// Функция преобразования строки из четырёх шестнадцатеричных символов в число INT
// функция преобразует строку с третьего символа.
// нет проверки на корректность входной строки
unsigned int StrToInt(char *string)
{
	unsigned int answer=0; // итоговое число
	unsigned char temp = 0; // значение текущей цифры
		
	for (unsigned char i = 0; i < 4; i++)  //четыре знака 
	{
		answer<<=4;
		if (string[i+3] < 58)  temp = string[i+3]-48; //значит цифра от 0 до 9
		else temp = string[i+3]-55; //значит буква A..F
	
		answer+=temp; //прибавляем к итогу
	}
	return(answer);
}

//посимвольное сравнение двух строк
//функция игнорирует один символ в начале строки string1 и произвольное количество символов в конце строки string1
unsigned char compareline(char *string1, char *string2)
{
	unsigned char sym=0; //указатель на символ
	
	while (string1[sym+1]) //пока первая строка не кончится
	{
		if (string2[sym]==0) return 1; //если дошли до конца строки и строки совпадают
		if (string1[sym+1]!=string2[sym]) return 0; //если символ первой строки не совпадает с символом второй строки, то вернуть 0
		sym++;
	}
	return 1; //подстрока найдена в строке
}



//определить какой icpcon эмулировать: первый, второй или никакой (1,2,0)
//ответ 0 - не реагируем, адрес указан не наш
//1 - первый
//2 - второй
unsigned char defineaddr(char *string)
{
	//сравнить полученную строку с возможными адресами
	if (compareline(string, MainAddr1)) return 1;
	else if (compareline(string, MainAddr2)) return 2;
	else return 0;   //адрес не распознан
}


void readline(void) //обработка принятой строки
{

	char PrefixAnswerOk[]=">"; //префикс ответа - допустимая команда
	char PrefixAnswerErr[]="?"; //префикс ответа - недопустимая команда
	char PrefixAnswerIgnr[]="!"; //префикс ответа - команда проигнорирована
	 
	char NameThisModule[]="7043D\r"; //название этого модуля
	char EOS[]="\r"; //символ конца строки
	
	char string[32]; //здесь будет принятая строка
	unsigned char sym=0; //какой символ обрабатываем
	unsigned char addr_icpcon=0; //временное хранение ответа от defineline
	unsigned int OutLine=0; //состояние выходов
	
	readlineen=0;  //сбрасываем статус необработанной строки

	do {           //копируем в новую строку символы из приёмного буфера
		string[sym]=uart_getchar();
		sym++;       //к следующему символу
	}
	while(string[sym-1]!=0x0D); //пока символ не конец строки
	
	if (sym==1) return;    //если принят только один байт, то не обрабатываем строку (скорее всего это обрезок от нашей передачи)
	
	string[sym]=0; //добавляем символ конца строки
	
	
	if (string[0]=='$') // команда $
	{
		addr_icpcon = defineaddr(string);  //распознаём номер icpcon, соответствует первому или второму номеру
		if (!addr_icpcon) return; //адрес не распознан, выход, закомментровать эту строку если необходимо эмулировать присутствие всех icpcon============================================================

		if (string[3]=='M') //команда $AAM - запросить название модуля
		{
			//печатаем ответ !AA(данные)	
			writeline(PrefixAnswerIgnr); //печатаем префикс ответа (!)
			if (addr_icpcon==1) writeline(MainAddr1); //печатаем первый адрес эмулируемого icpcon
			else writeline(MainAddr2); //печатаем второй адрес эмулируемого icpcon
			writeline(NameThisModule); //печатаем название модуля
		}
		else return; // команда не распознана
	}
	else if (string[0]=='@') // команда @
	{
		addr_icpcon = defineaddr(string);  //распознаём номер icpcon, соответствует первому или второму номеру
		if (!addr_icpcon) return; //адрес не распознан, выход
		
		if (string[3]=='\r') //команда @AA - считать данные с дискретных выходов
		{
			//команда не реализована
			writeline(PrefixAnswerErr); 
			if (addr_icpcon == 1) writeline(MainAddr1); //печатаем адрес эмулируемого icpcon
			else writeline(MainAddr2); 
			writeline(EOS);
			return;
		}
		//команда @AAdddd - установить дискретные выходы
		
		if (!verifystring(string))  //если строка не корректна (данные), выход
		{
			writeline(PrefixAnswerErr);
			if (addr_icpcon == 1) writeline(MainAddr1); //печатаем адрес эмулируемого icpcon
			else writeline(MainAddr2);
			writeline(EOS);
			return;
		}
		
		OutLine=StrToInt(string); //здесь будущее состояние выходов
		
		if (addr_icpcon == 1) //эмулируем первый icpcon (порты A0,A1,A2,A3,A4,A5,A6,A7,C0,C1,C2,C3,C4,C5,C6,C7)
		{
			PORTA = (char) OutLine;
			PORTC = (char) (OutLine>>8);
			writeline(PrefixAnswerOk); //команда исполнена
			writeline(EOS);
		}

		else if (addr_icpcon==2) //эмулируем второй icpcon (порты D0,D1,E2,E3,E4,E5,E6,E7,B4,B5,B6,B7,G0,G1,G2,G3)
		{
			PORTD = (PORTD&0b11111100) | (((char) OutLine)&0b00000011);
			PORTE = (PORTE&0b00000011) | (((char) OutLine)&0b11111100);
			PORTB = (PORTB&0b00001111) | ((((char) (OutLine>>8))&0b00001111)<<4);
			PORTG = (PORTG&0b11110000) | ((((char) (OutLine>>8))&0b11110000)>>4);
			writeline(PrefixAnswerOk); //команда исполнена
			writeline(EOS);
		}
		
	}

	else if (string[0] == '%') // команда % - задать новый адрес модулю, записать в eeprom
	{
		//проверка на корректность нового адреса, адресом может быть (0..9, A..Z, a..z)
		for (unsigned char i = 0; i < 4; i++)  //четыре знака
		{
			if ((string[i+1] < 48) || (string[i+1] > 122) || ((string[i+1] > 57) && (string[i+1] < 65)) || ((string[i+1] > 90) && (string[i+1] < 97)) || string[7] != '\r' || (string[1] == string[3] && string[2] == string[4])) //некорректная строка
			{
				string[0] = '?';
				string[7] = '\r';
				string[8] = 0;
				writeline(string);
				return;
			}
		}
		
		
		switch(string[5]) {
			case '0':	//без контроля четности
					eeprom_write_byte (&uartparity, '0');
					break;
			case '1':	//odd
					eeprom_write_byte (&uartparity, '1');
					break;
//			case '2':	//even почему то работает некорректно
//					eeprom_write_byte (&uartparity, '2');
//					break;
					
			default:	//ошибка
					string[0] = '?';
					string[7] = '\r';
					string[8] = 0;
					writeline(string);
					return;
		}
		
		switch(string[6]) { //скорость
			case '3':	//1200
					eeprom_write_byte (&uartspeed, '3');
					break;
			case '4':	//2400
					eeprom_write_byte (&uartspeed, '4');
					break;
			case '5':	//4800
					eeprom_write_byte (&uartspeed, '5');
					break;
			case '6':	//9600
					eeprom_write_byte (&uartspeed, '6');
					break;
			case '7':	//19200
					eeprom_write_byte (&uartspeed, '7');
					break;
			case '8':	//38400
					eeprom_write_byte (&uartspeed, '8');
					break;
			case '9':	//57600
					eeprom_write_byte (&uartspeed, '9');
					break;
			case 'A':	//115200
					eeprom_write_byte (&uartspeed, 'A');
					break;
			
			default:	//ошибка
					string[0] = '?';
					string[7] = '\r';
					string[8] = 0;
					writeline(string);
					return;
		}
		
		//адрес корректен
		
		MainAddr1[0]=string[1];  //первый адрес эмулируемого icpcon
		MainAddr1[1]=string[2];
		
		MainAddr2[0]=string[3];  //второй адрес эмулируемого icpcon
		MainAddr2[1]=string[4];
		
		//записываем настройки в eeprom
		_delay_ms(1);
		eeprom_write_byte (&eeMainAddr10, MainAddr1[0]); //первый адрес эмулируемого icpcon
		_delay_ms(1);
		eeprom_write_byte (&eeMainAddr11, MainAddr1[1]);
		_delay_ms(1);
		eeprom_write_byte (&eeMainAddr20, MainAddr2[0]); //второй адрес эмулируемого icpcon
		_delay_ms(1);
		eeprom_write_byte (&eeMainAddr21, MainAddr2[1]);
		_delay_ms(1);
		
		string[0]='!';
		writeline(string); //команда исполнена
	
		switch(uart_speed) { //задержка на 10 символов
			case '3':	//1200
				_delay_us(100000);
				break;
			case '4':	//2400
				_delay_us(50000);
				break;
			case '5':	//4800
				_delay_us(25000);
				break;
			case '6':	//9600
				_delay_us(25000);
				break;
			case '7':	//19200
				_delay_us(12000);
				break;
			case '8':	//38400
				_delay_us(3000);
				break;
			case '9':	//57600
				_delay_us(2000);
				break;
			case 'A':	//115200
				_delay_us(1000);
				break;
			default:	//ошибка
				_delay_ms(100);
		}

		
		setuart(); //установить скорость и контроль четности уарт

	}
	else return; //команда не распознана, выход

}
