Перенос тачгфх
> открыть юtouchgfx файл в текст.редакторе и переименовать имя .ioc внизу
> добавляем специальный таск
> генерируем тачгфх
> добавляем драйверы UI
> в main.h копируем структуры
> Добавляем всё во фриртос
> MemRegions in Core/src
> _it.c   extern volatile unsigned long ulStatsTimerTicks;  ulStatsTimerTicks++;
> переносим freertos и код в таски + Application в конце
> В очереди иниц.перефирии Do not genFuncCall для: SDMMC, FATFS, USB_Device - перепишем потом в другом порядке и во freertos.c
> MX_USB_DEVICE_DeInit();
> MX_TouchGFX_Init() переместить после lcdInit();
> билдим проект

> Выключаем иниты, вставляем в правильном порядке

> Debug Config > Enable RTOS Proxy (Driver: FreeRTOS, Port: ARM_CM7), uncheck Enable Live Expressions