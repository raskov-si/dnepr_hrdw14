#ifndef  CHEKSUM_H
#define  CHEKSUM_H


#ifdef	__cplusplus
extern "C" {
#endif

/*======================================================================================================*/

unsigned char cheksum_xorsum
(
  unsigned char previous_value,  /* (��) ���������� �������� ����������� �����   */
  unsigned char *buffer,        /* (��) ��������� �� ����� � �������            */
  unsigned int  length          /* (��) ���������� ���� ��� ��������            */
);


unsigned char cheksum_sum
(
  unsigned char *buffer,        /* (��) ��������� �� ����� � �������            */
  unsigned int  length          /* (��) ���������� ���� ��� ��������            */
);


unsigned char cheksum_zero_sum
(
  unsigned char *buffer,        /* (��) ��������� �� ����� � �������            */
  unsigned int  length          /* (��) ���������� ���� ��� ��������            */
);


/*======================================================================================================*/

#ifdef	__cplusplus
}
#endif

#endif   /* CHEKSUM_H */

