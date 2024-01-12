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


>> поставил приоритет прерывания TIM6 на 4 и снял галку UsesFreeRtos function согласно доке: https://support.touchgfx.com/docs/development/touchgfx-hal-development/scenarios/scenarios-configure-rtos
>> freertos.cpp >>264-265 заменил на обёртку для ui над hal
>> снова создал таск на vsync, прописал там как было вызов touchgfxSignalVSync() и заэкстернил extern "C" void touchgfxSignalVSync(void);
>> TouchGFXHAL.cpp >>161
>> gfx adapter used + tim callback starts vsync, main.cpp include gfx_adapter
>> gfx adapter hpp cpp

>> Возвращаюсь на partial buffer, наделал много изменений в interface controller, надо их не закоммитить + при пуше или мерже не добавлять изменения в эту папку
>> Нашёл ошибку, не был в таске прописан TouchGFX_Process()
>> Разбираюсь, почему в новом проекте всё виснет из-за HAL_Delay, а в старом - нет
>> Возможно это и-за стека в DTCMRAM, в старом проекте стек как по дефолту был в RAM_D1, проверяю
>> Я ещё закомментил всё из аудио в дефолт таске на всякий
>> Не помогло, всё равно виснет
>> Может неправильные инклюды? Картинки всё равно нет. А ещё рисование битмапов надо проверить

>> В таске StartTouchGFXUpdateTask заменил osDelay(1) на osThreadTerminate(osThreadGetId()); как в старом проекте, этого не замечал
>> В итоге не срабатывал таск Vsync, его надо было создавать не в конце, какая-то хрень с приоритетами
>> Ещё нао не забывать комментить в начале TouchGFXInit() до инита дисплея
>> 