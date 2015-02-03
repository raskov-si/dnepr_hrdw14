#include <intrinsics.h>
#include <stdio.h>
#include "ErrorManagement/assert.h"
#include "io5282.h"

/***********************************************************************
 *
 * This is the exception handler for all  exceptions common to all 
 * chips ColdFire.  Most exceptions do nothing, but some of the more 
 * important ones are handled to some extent.
 *
 * Called by asm_exception_handler 
 *
 * The ColdFire family of processors has a simplified exception stack
 * frame that looks like the following:
 *
 *              3322222222221111 111111
 *              1098765432109876 5432109876543210
 *           8 +----------------+----------------+
 *             |         Program Counter         |
 *           4 +----------------+----------------+
 *             |FS/Fmt/Vector/FS|      SR        |
 *   SP -->  0 +----------------+----------------+
 *
 * The stack self-aligns to a 4-byte boundary at an exception, with
 * the FS/Fmt/Vector/FS field indicating the size of the adjustment
 * (SP += 0,1,2,3 bytes).
 *             31     28 27      26 25    18 17      16 15                                  0
 *           4 +---------------------------------------+------------------------------------+
 *             | Format | FS[3..2] | Vector | FS[1..0] |                 SR                 |
 *   SP -->  0 +---------------------------------------+------------------------------------+
 */ 

#define MCF5XXX_RD_SF_FORMAT(PTR)   \
   ((*((unsigned short *)(PTR)) >> 12) & 0x00FF)

#define MCF5XXX_RD_SF_VECTOR(PTR)   \
   ((*((unsigned short *)(PTR)) >>  2) & 0x00FF)

#define MCF5XXX_RD_SF_FS(PTR)    \
   ( ((*((unsigned short *)(PTR)) & 0x0C00) >> 8) | (*((unsigned short *)(PTR)) & 0x0003) )

#define MCF5XXX_SF_SR(PTR)    *(((unsigned short *)(PTR))+1)

#define MCF5XXX_SF_PC(PTR)    *((unsigned long *)(PTR)+1)

#define MCF5XXX_EXCEPTFMT     "%s -- PC = %#08X\n"

/*************************************************************************
 * The exception handlers
 *************************************************************************/

void* s_ptr; //!< указатель на стэк в момент исключения, устанавливается в T8_Dnepr_vectortable.s68

//! пишем в лог assert'ов значение pc из s_ptr и висним
static void __isr_assert_messaging()
{
	char str[128];
    unsigned int pc ;
    unsigned char vector_num ;
    __disable_interrupts();
    pc = MCF5XXX_SF_PC(s_ptr) ; // теперь указывает на PC, где возникло исключение
    vector_num = MCF5XXX_RD_SF_VECTOR(s_ptr);

    str[0] = 0;
    sprintf( str, "PC: %u, VecNum: %u", pc, vector_num );

    ASSERT_FailedAssertionHandler( str, __FILE__, __func__, __LINE__ );
}

// это прерывание само по себе, без wrap'а
void ReservedHandler(void)
{
	__isr_assert_messaging();
}

// здесь и далее функции вызываются из wrap'ов соответствующих ISR (T8_Dnepr_isr_wraps.s68)
void AccessErrorHandler(void)
{
	__isr_assert_messaging();
}

void AddressErrorHandler(void)
{
	__isr_assert_messaging();
}

void InllegalInstrHandler(void)
{
    __isr_assert_messaging();
}

void DivByZeroHandler(void)
{
    __isr_assert_messaging();
}

void PrivViolationHandler(void)
{
    __isr_assert_messaging();
}

void TraceHandler(void)
{
    //printf("TraceHandler\n");
}

void UnimplemLineA_OpcOdeHandler(void)
{
    __isr_assert_messaging();
}

void UnimplemLineF_OpcOdeHandler(void)
{
    __isr_assert_messaging();
}

void DebugHandler(void)
{
    __isr_assert_messaging();
}

void FormatErrorHandler(void)
{
	__isr_assert_messaging();
}

void SpuriousIntrHandler(void)
{
    //printf("SpuriousIntrHandler\n");
}

void Trap0Handler(void)
{
    __isr_assert_messaging();
}

void Trap1Handler(void)
{
    __isr_assert_messaging();
}

void Trap2Handler(void)
{
    __isr_assert_messaging();
}

void Trap3Handler(void)
{
    __isr_assert_messaging();
}

void Trap4Handler(void)
{
    __isr_assert_messaging();
}

void Trap5Handler(void)
{
    __isr_assert_messaging();
}

void Trap6Handler(void)
{
    __isr_assert_messaging();
}

void Trap7Handler(void)
{
    __isr_assert_messaging();
}

void Trap8Handler(void)
{
    __isr_assert_messaging();
}

void Trap9Handler(void)
{
    __isr_assert_messaging();
}

void Trap10Handler(void)
{
    __isr_assert_messaging();
}

void Trap11Handler(void)
{
    __isr_assert_messaging();
}

void Trap12Handler(void)
{
    __isr_assert_messaging();
}

void Trap13Handler(void)
{
    __isr_assert_messaging();
}

void Trap14Handler(void)
{
    __isr_assert_messaging();
}

void Trap15Handler(void)
{
    __isr_assert_messaging();
}

void UART0_Handler(void)
{}

//void UART1_Handler(void)
//{}

void UART2_Handler(void)
{}

void DTIM0_Handler(void)
{}

void ADCA_ConvHandler(void)
{}

void ADCB_ConvHandler(void)
{}

void ADC_IntrHandler(void)
{}

void PIT0_IntrHandler(void)
{}

void RTC_IntrHandler(void)
{}

