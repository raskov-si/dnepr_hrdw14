/*!
\file threadWatchdog.h
\brief поток для перезапуска watchdog'а, имеет самый низкий приоритет после статистики и idle
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2012
*/

void taskWatchdog(void *pdata);
