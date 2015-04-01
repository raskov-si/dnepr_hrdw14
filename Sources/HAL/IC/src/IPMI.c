/*!
\file IPMI.c
\brief Работа с IPMI v1.0 (для чтения параметров блоков питания).
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#include <string.h>
#include <ctype.h>
#include "HAL/IC/inc/PMB_interface.h"
#include "HAL/IC/inc/PMBus_Commands.h"
#include "HAL/IC/inc/IPMI.h"
#include "common_lib/cheksum.h"
#include "uCOS_II.H"
#include "Binary.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    IPMI_STR_TYPE_BIN = 0,
    IPMI_STR_TYPE_BCD = 1,
    IPMI_STR_TYPE_6bASCII = 2,
    IPMI_STR_TYPE_LANG = 3
} IPMI_StrType_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static u8 __ipmi_len_2_type_len(IPMI_StrType_t type, const size_t len);
static size_t __ipmi_len_type_2_len(u8 type_n_len, IPMI_StrType_t *type);

static _BOOL __write_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 len, u8 *str);
static _BOOL __read_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 *len, u8 *str, u8 maxlen);

static _BOOL __PMB_WriteMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *anData, u8 len);
static _BOOL __PMB_ReadMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *anData, u8 len);
static _BOOL __PMB_ReadWord(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 *pwResult);
static _BOOL __PMB_GetAcknowledge(PMB_PeriphInterfaceTypedef *p, u8 mAddr);
static _BOOL __PMB_ReadByte(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *pwResult);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


_BOOL IPMI_Read_CommonHeader(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_CommonHeader *header)
{
    OSTimeDly(100);
    return __PMB_ReadMultipleBytes(pmb_bus, mAddr, IPMI_COMMONHEADER_ADDR, (u8 *)header, sizeof(IPMI_CommonHeader));
}

_BOOL IPMI_Read_ProductInfo(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, IPMI_ProductInfo *product_info)
{
    size_t i_offset = offset;
    size_t len_2_read;
    _BOOL ret = TRUE;

    // читаем IPMI_ProductInfo до первой строки (потому что длина строки становится известна только когда прочитаем)
    len_2_read = (u8 *)&product_info->manufacturer_name_length - (u8 *)product_info;
    OSTimeDly(100);
    ret = ret && __PMB_ReadMultipleBytes(pmb_bus, mAddr, i_offset, (u8 *)product_info, len_2_read);

    if ((product_info->format_version & 0xF) != 0x01)   {
        return FALSE;
    }

    i_offset += len_2_read;
    ret = ret && __read_sz_n_str(pmb_bus, mAddr, i_offset, &product_info->manufacturer_name_length, (u8 *)&product_info->manufacturer_name, IPMI_STRINGS_LEN);
    i_offset += product_info->manufacturer_name_length + 1;
    ret = ret && __read_sz_n_str(pmb_bus, mAddr, i_offset, &product_info->name_length, (u8 *)&product_info->name, IPMI_STRINGS_LEN);
    i_offset += product_info->name_length + 1;
    ret = ret && __read_sz_n_str(pmb_bus, mAddr, i_offset, &product_info->model_length, (u8 *)&product_info->model, IPMI_STRINGS_LEN);
    i_offset += product_info->model_length + 1;
    ret = ret && __read_sz_n_str(pmb_bus, mAddr, i_offset, &product_info->serial_number_len, (u8 *)&product_info->serial_number, IPMI_STRINGS_LEN);
    i_offset += product_info->serial_number_len + 1;
    ret = ret && __read_sz_n_str(pmb_bus, mAddr, i_offset, &product_info->asset_tag_len, (u8 *)&product_info->asset_tag, IPMI_STRINGS_LEN);
    i_offset += product_info->asset_tag_len + 1;
    ret = ret && __read_sz_n_str(pmb_bus, mAddr, i_offset, &product_info->fru_file_id_len, (u8 *)&product_info->fru_file_id, IPMI_STRINGS_LEN);
    i_offset += product_info->fru_file_id_len + 1;

    return ret;
}




/*=============================================================================================================*/
/*!  \brief в области MultiRecord находит запись с идентификатором 0x00 (PowerSupplyInformation) и читает из нее данные


     \return признак выполнения транзакции
     \retval TRUE, FALSE
     \sa __PMB_ReadMultipleBytes, PMB_PeriphInterfaceTypedef
*/
/*=============================================================================================================*/
extern f32 __PSU_PMBus_ConvertLinearVal(u16 val);

_BOOL IPMI_Find_n_Read_PSU_MultiArea
(
    PMB_PeriphInterfaceTypedef                *pmb_bus,       /*!< [in] выбор шины по которой осуществляется транзакция                               */
    u8                                        mAddr,          /*!< [in] адрес EEPROM БП на i2c шине                                                   */
    u8                                        m2Addr,         /*!< [in] адрес мастер-контроллера БП на i2c шине                                       */
    IPMI_FRU                                  *fru,           /*!< [in] */
    IPMI_MultiRecordHeader                    *multi_header,  /*!< [out] указатель для чтения смещений в мультиполе                                   */
    IPMI_MultiRecord_PowerSupplyInformation   *psu_area       /*!< [out] указатель для чтения данных Power Supply Information (Record Type 0x00)      */
    )
{
    _BOOL   ret     = FALSE;
    size_t  nAreas  = 0;
    size_t  offset;

    assert(pmb_bus);
    assert(fru);
    assert(multi_header);
    assert(psu_area);

    if (fru->header.multirecord_area_offset != 0) {

        /* если присутствует область multirecord, то считываем находим подобласть с типом IPMI_MRT_POWER_SUPPLY=0 и считываем параметры с нее */
        offset = fru->header.multirecord_area_offset * 8;
        do {
            OSTimeDly(100);
            ret = __PMB_ReadMultipleBytes(pmb_bus, mAddr, offset, (u8 *)multi_header, sizeof(IPMI_MultiRecordHeader));
            offset += sizeof(IPMI_MultiRecordHeader) + multi_header->length;
            ++nAreas;
        } while (((multi_header->end_n_format & 0x80) == 0) && (nAreas < 10) && (multi_header->type_id != IPMI_MRT_POWER_SUPPLY));

        if (multi_header->type_id == IPMI_MRT_POWER_SUPPLY) {

            offset -= multi_header->length;
            OSTimeDly(100);

            ret = ret && __PMB_ReadMultipleBytes(pmb_bus, mAddr, offset, (u8 *)psu_area, sizeof(IPMI_MultiRecord_PowerSupplyInformation));
            offset += multi_header->length;

            /*!< \todo  добавить проверку контрольной суммы   */

            // переворачиваем все многобайтовые поля
//      #define SWAP_BYTES_16W(x)   x = (_WORD(_LSB(x), _MSB(x) ))
            SWAP_BYTES_16W(psu_area->overall_capacity);
            SWAP_BYTES_16W(psu_area->peak_va);
            SWAP_BYTES_16W(psu_area->low_end_input_voltage_range_1);
            SWAP_BYTES_16W(psu_area->high_end_input_voltage_range_1);
            SWAP_BYTES_16W(psu_area->low_end_input_voltage_range_2);
            SWAP_BYTES_16W(psu_area->high_end_input_voltage_range_2);
            SWAP_BYTES_16W(psu_area->peak_watt);

            // XXX: динамически определяем порядок байт (если напряжение больше 300 В или меньше 0 -- меняем его) ???
            if ((psu_area->low_end_input_voltage_range_1 > 30000) || (psu_area->low_end_input_voltage_range_1 < 0)) {
                SWAP_BYTES_16W(psu_area->low_end_input_voltage_range_1);
            }
            if ((psu_area->high_end_input_voltage_range_1 > 30000) || (psu_area->high_end_input_voltage_range_1 < 0)) {
                SWAP_BYTES_16W(psu_area->high_end_input_voltage_range_1);
            }
            if ((psu_area->low_end_input_voltage_range_2 > 30000) || (psu_area->low_end_input_voltage_range_2 < 0)) {
                SWAP_BYTES_16W(psu_area->low_end_input_voltage_range_2);
            }
            if ((psu_area->high_end_input_voltage_range_2 > 30000) || (psu_area->high_end_input_voltage_range_2 < 0)) {
                SWAP_BYTES_16W(psu_area->high_end_input_voltage_range_2);
            }
        } else {
            ret = FALSE;
        }
    } /* if ( fru->header.multirecord_area_offset != 0 ) */

    /* если у блока нет нужной записи в multirecord то считывать параметры PSU с мастер-контроллера БП */
    if (ret == FALSE) {
//            u16   param;
//            u8 i;
//            u8  i_hit[10];
//            u8  hit_index = 0;
//            memset(i_hit, 0, 10);
//
//            OSTimeDly( 50000 );
//            for ( i = 1; i < 128; i++ )
//            {
//      OSTimeDly( 50 );
//
//                ret = __PMB_GetAcknowledge( pmb_bus, i<<1 );
//
//                if ( (ret == TRUE) )
//                {
//                    i_hit[hit_index++] = i;
//                }
//            }
//
//
//            /* PMBUS */
//      OSTimeDly( 50 );
//            ret = __PMB_ReadWord(pmb_bus, m2Addr, PMB_MFR_POUT_MAX, &param );
//            if ( ret == TRUE )  {
//                psu_area->overall_capacity = (s16)__PSU_PMBus_ConvertLinearVal(param);
//            }
//
//            OSTimeDly( 50 );
//            ret = ( ret && __PMB_ReadWord(pmb_bus, m2Addr, PMB_MFR_VIN_MIN, &param ) );
//            if ( ret == TRUE ) {
//              psu_area->low_end_input_voltage_range_1 = (s16)__PSU_PMBus_ConvertLinearVal(param) * 100;
//            }
//
//            OSTimeDly( 50 );
//            ret = ( ret && __PMB_ReadWord(pmb_bus, m2Addr, PMB_MFR_VIN_MAX, &param ) );
//            if ( ret == TRUE )  {
//              psu_area->high_end_input_voltage_range_1 = (s16)__PSU_PMBus_ConvertLinearVal(param) * 100;
//            }
//
    } /* if ( ret == FALSE ) */

    if ((psu_area->overall_capacity == 0) || (psu_area->overall_capacity > 2000)) {ret = FALSE; }

    return ret;
}


//            u8 i;
//            char temp[64];
//            u8  i_hit[10];
//            u8  hit_index = 0;
///            memset(temp, 0, 64);
//            memset(i_hit, 0, 10);
//
//
//            for ( i = 0; i < 128; i++ )
//            {
//      OSTimeDly( 50 );
//                if ( (i<<1) == mAddr)
//                { continue;  }
//                ret = __PMB_ReadMultipleBytes( pmb_bus, i<<1, PMB_MFR_MODEL, (u8*)temp, 10 );
//
//                if ( (ret == TRUE) && isalnum(temp[1]) )
//                {
//                    memset(temp, 0, 64);
//                    i_hit[hit_index++] = i;
//                }
//            }
//    OSTimeDly( 50 );
//          ret = __PMB_ReadWord(pmb_bus, i_hit[0]<<1, PMB_MFR_VIN_MIN, &param );
//    SWAP_BYTES_16W( param );
//
//            for ( i = 1; i < 128; i++ )
//            {
//      OSTimeDly( 50 );
//
//                ret = __PMB_GetAcknowledge( pmb_bus, i<<1 );
//
//                if ( (ret == TRUE) )
//                {
//                    i_hit[hit_index++] = i;
//                }
//            }


/*=============================================================================================================*/
/*! \brief   Чтение common заголовка IMPI с информацией о присутсвии других полей.

    \return признгак прочитаной информации
    \retval TRUE, FALSE
    \sa  PMB_PeriphInterfaceTypedef, IPMI_FRU, IPMI_Read_ProductInfo, IPMI_COMMONHEADER_ADDR
*/
/*=============================================================================================================*/
_BOOL ipmi_get_adress_acknowledge
(
    PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
    u8                          mAddr         /*!< [in]   адрес I2C EEPROM блока питания                          */
    )
{
    u8 temp;

    assert(pmb_bus);

    OSTimeDly(100);
    return __PMB_ReadByte(pmb_bus, mAddr, PMB_OPERATION, &temp);
    /* emerson 650 отказывается работать когда первая транзакция на шине оказывается незавершенной, 
       после этого не отвечает ни на какие комманды */
//  return __PMB_GetAcknowledge( pmb_bus, mAddr );
}



/*=============================================================================================================*/
/*! \brief   Чтение common заголовка IMPI с информацией о присутсвии других полей.

    \return признгак прочитаной информации
    \retval TRUE, FALSE
    \sa  PMB_PeriphInterfaceTypedef, IPMI_FRU, IPMI_Read_ProductInfo, IPMI_COMMONHEADER_ADDR
*/
/*=============================================================================================================*/
_BOOL ipmi_read_common_header
(
    PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
    u8                          mAddr,        /*!< [in]   адрес I2C EEPROM блока питания                          */
    IPMI_FRU                    *fru          /*!< [out]  структура для считываемых параметров                    */
    )
{
    _BOOL ret = FALSE;
    u8    try_cnt = 3;

    assert(pmb_bus);
    assert(fru);

    while ((ret != TRUE) && (try_cnt--)) {
        OSTimeDly(100);
        /* читаем Common Header */
        ret = __PMB_ReadMultipleBytes(pmb_bus, mAddr, IPMI_COMMONHEADER_ADDR, (u8 *)&fru->header, sizeof(IPMI_CommonHeader));
        if (ret == TRUE)  {
            if (cheksum_zero_sum((u8 *)&fru->header, sizeof(IPMI_CommonHeader) - 1) != ((u8 *)&fru->header)[sizeof(IPMI_CommonHeader) - 1])  {
                ret = FALSE;
                continue;
            }

            if (fru->header.format_version != 0x01)   {
                ret = FALSE;
                continue;
            }
        }
    }

    return ret;
}


/*=============================================================================================================*/
/*! \brief   Чтение области Product Info.

    \return признгак прочитаной информации
    \retval TRUE, FALSE
    \sa  PMB_PeriphInterfaceTypedef, IPMI_FRU, IPMI_Read_ProductInfo, IPMI_COMMONHEADER_ADDR
*/
/*=============================================================================================================*/
_BOOL ipmi_read_product_info
(
    PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
    u8                          mAddr,        /*!< [in]   адрес I2C EEPROM блока питания                          */
    u8                          m2Addr,       /*!< [in] адрес мастер-контроллера БП на i2c шине                     */
    IPMI_FRU                    *fru          /*!< [out]  структура для считываемых параметров                    */
    )
{
    _BOOL ret = FALSE;

    assert(pmb_bus);
    assert(fru);


    if (fru->header.product_area_offset != 0) {
        /* читаем Product Info если есть */
        OSTimeDly(100);
        ret = IPMI_Read_ProductInfo(pmb_bus, mAddr, fru->header.product_area_offset * 8, &fru->product_info);


    }

    return ret;
}


/*=============================================================================================================*/
/*! \brief   Чтение common заголовка IMPI с информацией о присутсвии других полей. Чтение области Product Info

    \return признгак прочитаной информации
    \retval TRUE, FALSE
    \sa  PMB_PeriphInterfaceTypedef, IPMI_FRU, IPMI_Read_ProductInfo, IPMI_COMMONHEADER_ADDR
*/
/*=============================================================================================================*/

_BOOL IPMI_Read_FRU_Headers
(
    PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
    u8                          mAddr,        /*!< [in]   адрес I2C EEPROM блока питания                          */
    IPMI_FRU                    *fru          /*!< [out]  структура для считываемых параметров                    */
    )
{
    _BOOL ret = TRUE;

    assert(pmb_bus);
    assert(fru);

    /* читаем Common Header */
    OSTimeDly(100);
    ret = __PMB_ReadMultipleBytes(pmb_bus, mAddr, IPMI_COMMONHEADER_ADDR, (u8 *)&fru->header, sizeof(IPMI_CommonHeader));

    if (fru->header.product_area_offset != 0) {
        /* читаем Product Info если есть */
        ret = IPMI_Read_ProductInfo(pmb_bus, mAddr, fru->header.product_area_offset * 8, &fru->product_info);
    }

    return ret;
}


_BOOL IPMI_Write_FRU(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_FRU *fru,
    IPMI_MultiRecordHeader *multi_header, IPMI_MultiRecord_PowerSupplyInformation *psu_area)
{
    size_t i_offset = IPMI_COMMONHEADER_ADDR;
    _BOOL ret = TRUE;
    assert(pmb_bus);
    assert(fru);

    // Пишем Common Header
    OSTimeDly(5);
    ret = ret && __PMB_WriteMultipleBytes(pmb_bus, mAddr, i_offset, (u8 *)&fru->header, sizeof(IPMI_CommonHeader));

    // Пишем Product Info
    OSTimeDly(5);
    i_offset = fru->header.product_area_offset * 8;
    // пишем до первой строки
    ret = ret && __PMB_WriteMultipleBytes(pmb_bus, mAddr,
        i_offset, (u8 *)&fru->product_info,
        (u8 *)&fru->product_info.manufacturer_name_length - (u8 *)&fru->product_info);
    OSTimeDly(5);

    i_offset += (u8 *)&fru->product_info.manufacturer_name_length - (u8 *)&fru->product_info;

    ret = ret && __write_sz_n_str(pmb_bus, mAddr, i_offset, fru->product_info.manufacturer_name_length, fru->product_info.manufacturer_name);

    i_offset += fru->product_info.manufacturer_name_length + 1;
    ret = ret && __write_sz_n_str(pmb_bus, mAddr, i_offset, fru->product_info.name_length, fru->product_info.name);
    i_offset += fru->product_info.name_length + 1;
    ret = ret && __write_sz_n_str(pmb_bus, mAddr, i_offset, fru->product_info.model_length, fru->product_info.model);
    i_offset += fru->product_info.model_length + 1;
    ret = ret && __write_sz_n_str(pmb_bus, mAddr, i_offset, fru->product_info.serial_number_len, fru->product_info.serial_number);
    i_offset += fru->product_info.serial_number_len + 1;
    ret = ret && __write_sz_n_str(pmb_bus, mAddr, i_offset, fru->product_info.asset_tag_len, fru->product_info.asset_tag);
    i_offset += fru->product_info.asset_tag_len + 1;
    ret = ret && __write_sz_n_str(pmb_bus, mAddr, i_offset, fru->product_info.fru_file_id_len, fru->product_info.fru_file_id);
    i_offset += fru->product_info.fru_file_id_len + 1;

    i_offset = fru->header.multirecord_area_offset * 8;
    // пишем multi header area БП
    ret = ret && __PMB_WriteMultipleBytes(pmb_bus, mAddr,
        i_offset, (u8 *)multi_header, sizeof(IPMI_MultiRecordHeader));
    i_offset += sizeof(IPMI_MultiRecordHeader);
    // пишем собсно информацию о БП
    ret = ret && __PMB_WriteMultipleBytes(pmb_bus, mAddr,
        i_offset, (u8 *)psu_area, sizeof(IPMI_MultiRecord_PowerSupplyInformation));

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t __ipmi_len_type_2_len(u8 type_n_len, IPMI_StrType_t *type)
{
    if (type) {
        switch ((type_n_len & 0xC0) >> 6) {
        case 0:
            *type = IPMI_STR_TYPE_BIN;
            break;
        case 1:
            *type = IPMI_STR_TYPE_BCD;
            break;
        case 2:
            *type = IPMI_STR_TYPE_6bASCII;
            break;
        case 3:
            *type = IPMI_STR_TYPE_LANG;
            break;
        }
    }
    return type_n_len & 0x3F;
}

static u8 __ipmi_len_2_type_len(IPMI_StrType_t type, const size_t len)
{
    u8 res = 0;

    res = ((u8)type) << 6;
    res |= (len & 0x3F);

    return res;
}

// читает побайтно из адреса mAddr и смещения offset сначала байт с длиной строки, сразу следом строку
static _BOOL __read_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 *len, u8 *str, u8 maxlen)
{
    _BOOL ret;
    IPMI_StrType_t type;

    OSTimeDly(100);
    ret = pmb_bus->PMB_ReadByte(pmb_bus, mAddr, offset, len, POWERUNIT_PMBUS_TIMEOUTMS, 3);
    *len = __ipmi_len_type_2_len(*len, &type);
    *len = (*len > maxlen) ? maxlen : *len;
    // если тип строки не 3 (см раздел 13 документа Platform Management FRU Information Storage Definition)
    // разбирать его не умеем
    if (type != IPMI_STR_TYPE_LANG) {
        str[0] = 0;
    } else {
        OSTimeDly(100);
        ret = ret && __PMB_ReadMultipleBytes(pmb_bus, mAddr, offset + 1, str, *len);
        str[*len] = 0;
    }
    return ret;
}

static _BOOL __write_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 len, u8 *str)
{
    _BOOL ret;
    u8 type_n_len;

    type_n_len = __ipmi_len_2_type_len(IPMI_STR_TYPE_LANG, len);
    ret = __PMB_WriteMultipleBytes(pmb_bus, mAddr, offset, &type_n_len, 1);
    ret = ret && __PMB_WriteMultipleBytes(pmb_bus, mAddr, offset + 1, str, len);

    return ret;
}

static _BOOL __PMB_WriteMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 offset, u8 *anData, u8 len)
{
    _BOOL ret = TRUE;
    u8 cur_len = 0;
    size_t cur_offset = offset;
    size_t i = 0;
    do {
        // сколько пишем в текущей транзакции
        cur_len = MIN(((cur_offset + 8) & 0xF8) - cur_offset, len - cur_len);

        ret = ret && p->PMB_WriteMultipleBytes(p, mAddr, cur_offset, &anData[i], cur_len, POWERUNIT_PMBUS_TIMEOUTMS, 3);
        OSTimeDly(5);
        if (!ret) {
            ++i;
            --i;
        }
        i += cur_len;
        cur_offset += cur_len;
    } while ((cur_offset - offset) < len);

    return ret;
}


/*=============================================================================================================*/
/*! \brief  чтение нескольких байт по шине i2c/pmbus

    \return признак выполнения транзакции
    \retval TRUE, FALSE
    \sa PMB_PeriphInterfaceTypedef
*/
/*=============================================================================================================*/
static _BOOL __PMB_ReadMultipleBytes
(
    PMB_PeriphInterfaceTypedef *p,          /*!< [in] выбор шины по которой осуществляется транзакция */
    u8                            mAddr,      /*!< [in] i2c адрес с которого читаются данные            */
    u8                            mCmd,       /*!< [in] i2c комманда для устройтсва                     */
    u8                            *anData,    /*!< [out] указатель на буффер для чтения                 */
    u8                            len         /*!< [in] количество читаемых данных                      */
    )
{
    return p->PMB_ReadMultipleBytes(p, mAddr, mCmd, anData, len, POWERUNIT_PMBUS_TIMEOUTMS, 3);
}


/*=============================================================================================================*/
/*! \brief  чтение 2-байтного слова по шине i2c/pmbus

    \return признак выполнения транзакции
    \retval TRUE, FALSE
    \sa PMB_PeriphInterfaceTypedef
*/
/*=============================================================================================================*/
static _BOOL __PMB_ReadWord
(
    PMB_PeriphInterfaceTypedef  *p,         /*!< [in] выбор шины по которой осуществляется транзакция */
    u8                          mAddr,      /*!< [in] i2c адрес с которого читаются данные            */
    u8                          mCmd,       /*!< [in] i2c комманда для устройтсва                     */
    u16                         *pwResult   /*!< [out] слово в которое происходит чтение              */
    )
{
    return p->PMB_ReadWord(p, mAddr, mCmd, pwResult, POWERUNIT_PMBUS_TIMEOUTMS, 3);
}


/*=============================================================================================================*/
/*! \brief  чтение байта слова по шине i2c/pmbus

    \return признак выполнения транзакции
    \retval TRUE, FALSE
    \sa PMB_PeriphInterfaceTypedef
*/
/*=============================================================================================================*/
static _BOOL __PMB_ReadByte
(
    PMB_PeriphInterfaceTypedef  *p,         /*!< [in] выбор шины по которой осуществляется транзакция */
    u8                          mAddr,      /*!< [in] i2c адрес с которого читаются данные            */
    u8                          mCmd,       /*!< [in] i2c комманда для устройтсва                     */
    u8                          *pwResult   /*!< [out] байт в которой происходит чтение              */
    )
{
    return p->PMB_ReadByte(p, mAddr, mCmd, pwResult, POWERUNIT_PMBUS_TIMEOUTMS, 3);
}


/*=============================================================================================================*/
/*! \brief  получение отклика на шине i2c/pmbus

    \return признак выполнения транзакции
    \retval TRUE, FALSE
    \sa PMB_PeriphInterfaceTypedef
*/
/*=============================================================================================================*/
static _BOOL __PMB_GetAcknowledge
(
    PMB_PeriphInterfaceTypedef  *p,         /*!< [in] выбор шины по которой осуществляется транзакция */
    u8                          mAddr       /*!< [in] i2c адрес с которого читаются данные            */
    )
{
    return p->PMB_GetAcknowledge(p, mAddr, POWERUNIT_PMBUS_TIMEOUTMS, 0);
}
