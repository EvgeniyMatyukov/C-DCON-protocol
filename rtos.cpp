
typedef unsigned char u08; //объявляем новые типы
typedef unsigned int u16;
#define MAX_TASKS 128  //максимальное количество задач в диспетчере 255

volatile u08 arrayTail; //количество задач в диспетчере

typedef struct task //добавлять задачи в планировщик будем в таком виде
{
	void (*pFunc) (void); // указатель на функцию
	u16 delay; // задержка перед первым запуском задачи
	u16 period; // период запуска задачи
	u08 run; // флаг готовности задачи к запуску
}task;

volatile static task TaskArray[MAX_TASKS]; // очередь задач

void RTOS_Init() //инициализация диспетчера задач
{
	arrayTail = 0; // "хвост" в 0
}

void RTOS_Timer()
{					// Таймерная служба РТОС (должна вызываться из прерывания аппаратного таймера, например каждую 1 мс)
	u08 i;
	for (i=0; i<arrayTail; i++)                        // проходим по списку задач
	{
		if  (TaskArray[i].delay == 0)                   // если время до выполнения истекло
		TaskArray[i].run = 1;                      // взводим флаг запуска,
		else TaskArray[i].delay--;                      // иначе уменьшаем время
	}
}

void RTOS_SetTask (void (*taskFunc)(void), u16 taskDelay, u16 taskPeriod) //Добавление задачи в буфер диспетчера задач
{
	u08 i;
	if(!taskFunc) return;
	for(i = 0; i < arrayTail; i++)  {                    // поиск задачи в текущем списке
		if(TaskArray[i].pFunc == taskFunc)  {              // если нашли, то обновляем переменные
			asm("cli");

			TaskArray[i].delay  = taskDelay;
			TaskArray[i].period = taskPeriod;
			TaskArray[i].run    = 0;

			asm("sei");
			return;                                      // обновив, выходим
		}
	}

	if (arrayTail < MAX_TASKS)  {                        // если такой задачи в списке нет и есть место,то добавляем
		asm("cli");
		
		TaskArray[arrayTail].pFunc  = taskFunc;
		TaskArray[arrayTail].delay  = taskDelay;
		TaskArray[arrayTail].period = taskPeriod;
		TaskArray[arrayTail].run    = 0;

		arrayTail++;                                    // увеличиваем "хвост"
		asm("sei");
	}
}

void RTOS_DeleteTask (void (*taskFunc)(void)) //Удаление задачи из списка
{
	u08 i;
	
	for (i=0; i<arrayTail; i++) {                        // проходим по списку задач
		if(TaskArray[i].pFunc == taskFunc) {              // если задача в списке найдена
			asm("cli");
			if(i != (arrayTail - 1)) {                     // переносим последнюю задачу на место удаляемой
				TaskArray[i].pFunc  = TaskArray[arrayTail-1].pFunc;
				TaskArray[i].delay  = TaskArray[arrayTail-1].delay;
				TaskArray[i].period = TaskArray[arrayTail-1].period;
				TaskArray[i].run    = TaskArray[arrayTail-1].run;
			}
			arrayTail--;                                 // уменьшаем указатель "хвоста"
			asm("sei");
			return;
		}
	}
}

void RTOS_DispatchTask()  //Диспетчер РТОС, циклично вызывается в main (внутри основного цикла while)
{
	u08 i;
	void (*function) (void);
	
	for (i=0; i<arrayTail; i++)                        // проходим по списку задач
	{
		if (TaskArray[i].run == 1)                      // если флаг на выполнение взведен,
		{                                               // запоминаем задачу, т.к. во
			function = TaskArray[i].pFunc;               // время выполнения может
			// изменится индекс
			if(TaskArray[i].period == 0)
			{                                            // если период равен 0
				RTOS_DeleteTask(TaskArray[i].pFunc);      // удаляем задачу из списка,
			}
			else
			{
				TaskArray[i].run = 0;                     // иначе снимаем флаг запуска
				if(!TaskArray[i].delay)                   // если задача не изменила задержку
				{                                         // задаем ее
					TaskArray[i].delay = TaskArray[i].period-1;
				}                                         // задача для себя может сделать паузу
			}

			(*function)();                               // выполняем задачу
		}
	}
}
