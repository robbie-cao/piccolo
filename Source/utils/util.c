#include "util.h"
#include <hci.h>
#include "bcomdef.h"

/*********************************************************************
 * @fn      delayUS
 *
 * @brief   Delay micro seconds
 *
 * @return  none
 */
void delayUS(uint16 us)
{
  uint8 cnt = 0;
  while (us--)
  {
    /* 32 NOPs == 1 usecs */
    while (cnt++ < 32)
    {
      asm("NOP");
    }
  }
}

void delayMS(uint16 ms)
{
  while (ms--)
  {
    delayUS(1000);
  }
}

/*********************************************************************
 * @fn      bdAddr2Str
 *
 * @brief   Convert Bluetooth address to string
 *
 * @return  none
 */
char *bdAddr2Str( uint8 *pAddr )
{
  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[MAC_ADDR_STR_LEN];
  char        *pStr = str;

#if 0
  *pStr++ = '0';
  *pStr++ = 'x';
#endif

  // Start from end of addr
  pAddr += B_ADDR_LEN;

  for (i = B_ADDR_LEN; i > 0; i--)
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }

  *pStr = 0;

  return str;
}

/*********************************************************************
 * @fn      bdPartialAddr2Str
 *
 * @brief   Convert Bluetooth address to string
 *
 * @return  none
 */
char *bdPartialAddr2Str(uint8 *pAddr)
{
  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[12] = "PRU[0000]\r\n\0";
  char        *pStr = str;

  pStr  += 4;  // Starting from '['
  pAddr += 2;  // Starting from second byte of BLE address

  for (i = 2; i > 0; i--)
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }

  return str;
}


/*********************************************************************
 * @fn      bdAddr2DemStr
 *
 * @brief   Convert Bluetooth address to string divided by '-'
 *
 * @return  none
 */
char *bdAddr2DemStr(uint8 *pAddr)
{
  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[MAC_ADDR_DEMSTR_LEN];
  char        *pStr = str;

  // Start from end of addr
  pAddr += B_ADDR_LEN;

  for (i = B_ADDR_LEN; i > 0; i-- )
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
    *pStr++ = '-';
  }
  // Delete the last '-'
  *(--pStr) = 0;

  return str;
}

char *arrHex2Str(uint8 *pArr, char *str, uint8 size)
{
  char  hex[] = "0123456789ABCDEF";
  char  *pStr = str;

  if (!pArr || !str) {
    return str;
  }

  for (uint8 i = 0; i < size; i++)
  {
    *pStr++ = hex[*pArr >> 4];
    *pStr++ = hex[*pArr & 0x0F];
    pArr++;
  }
  *pStr = 0;

  return str;
}

/*********************************************************************
 * @fn         getTxLevelFromDbm
 *
 * @brief      Converts dBm to the Tx power level that TI supports
 *
 * @param dbm  the Tx power in dBm
 * @return     the converted Tx power level
 */
uint8 getTxLevelFromDbm(int8 dbm)
{
  // The CC2541 data sheet indicates no support for LL_EXT_TX_POWER_4_DBM.
  if (dbm > -3)
  {
    return LL_EXT_TX_POWER_0_DBM;
  }
  if (dbm > -14)
  {
    return LL_EXT_TX_POWER_MINUS_6_DBM;
  }

  return LL_EXT_TX_POWER_MINUS_23_DBM;
}

/* vim: set ts=2 sw=2 tw=0 list : */
