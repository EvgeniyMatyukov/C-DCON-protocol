//функции работы с отдельными битами
#define bit(x,n) (((x)>>(n))&1) //узнать состояние бита
#define setbit(x,n) ((x)|=(1<<(n))) //установить бит
#define clrbit(x,n) ((x)&=~(1<<(n))) //сбросить бит
#define invbit(x,n) ((x)^=(1<<(n))) //инвертировать бит

//установить 1 на выходе порта
void setout(volatile unsigned char *port, unsigned char pin)
{
	*port |= 1 << pin;
}

//установить 0 на выходе порта
void clrout(volatile unsigned char *port, unsigned char pin)
{
	*port &= ~(1 << pin);
}

//инвертировать бит на выходе порта
void invout(volatile unsigned char *port, unsigned char pin)
{
	*port ^= 1 << pin;
}

//проверить бит на выходе порта
bool pinout(volatile unsigned char *port, unsigned char pin)
{
	return (((*port)>>(pin))&1);
}
