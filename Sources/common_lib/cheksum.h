#ifndef  CHEKSUM_H
#define  CHEKSUM_H


#ifdef	__cplusplus
extern "C" {
#endif

/*======================================================================================================*/

unsigned char cheksum_xorsum
(
  unsigned char previous_value,  /* (вх) предыдущее значение контрольной суммы   */
  unsigned char *buffer,        /* (вх) указатель на буфер с данными            */
  unsigned int  length          /* (вх) количество байт для подсчета            */
);


unsigned char cheksum_sum
(
  unsigned char *buffer,        /* (вх) указатель на буфер с данными            */
  unsigned int  length          /* (вх) количество байт для подсчета            */
);


unsigned char cheksum_zero_sum
(
  unsigned char *buffer,        /* (вх) указатель на буфер с данными            */
  unsigned int  length          /* (вх) количество байт для подсчета            */
);


/*======================================================================================================*/

#ifdef	__cplusplus
}
#endif

#endif   /* CHEKSUM_H */

