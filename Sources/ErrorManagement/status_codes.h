/*!
\file status_codes.h
\brief Коды ошибок и возвращаемые значения
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef __STATUS_CODES_H_
#define	__STATUS_CODES_H_

typedef enum __ReturnStatus{
	OK = 1,
	ERROR = 0xFFFFFFFF
} ReturnStatus ;

#endif // __STATUS_CODES_H_
