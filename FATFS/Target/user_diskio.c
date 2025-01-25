/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <ctype.h>        // For character handling functions
#include "user_diskio.h"
#include "SSPI.h"
#include "FATFS_SD.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;


/* Private function prototypes -----------------------------------------------*/

DSTATUS SPI_initialize (BYTE pdrv);
DSTATUS SPI_status (BYTE pdrv);
DRESULT SPI_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
DRESULT SPI_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
DRESULT SPI_ioctl (BYTE pdrv, BYTE cmd, void *buff);

Diskio_drvTypeDef  SPI_Driver =
  {
  SPI_initialize,
  SPI_status,
  SPI_read,
  SPI_write,
  SPI_ioctl,
  };

Diskio_drvTypeDef  SD_disk_Driver =
  {
  SD_disk_initialize,
  SD_disk_status,
  SD_disk_read,
  SD_disk_write,
  SD_disk_ioctl,
  };

Disk_drvTypeDef disk =
  {
  {0, 0},
  {&SD_disk_Driver, &SPI_Driver},
  3
  };

/* USER CODE END DECL */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS SPI_initialize (
	BYTE pdrv           /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
    // Initialize the SPI-NOR flash (already initialized in main)
    return RES_OK;
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS SPI_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
    // Return the status of the SPI-NOR flash
    return RES_OK;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SPI_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */

    uint32_t address = sector * SPI_PAGE_SIZE * 2;
    for (UINT i = 0; i < count*2; i++)
        {
        if (SPI_ReadPage(&hspi2, address, buff + i * SPI_PAGE_SIZE, SPI_PAGE_SIZE) != HAL_OK) return RES_ERROR;
        address += SPI_PAGE_SIZE;
        }

    return RES_OK;

  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SPI_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */

    uint32_t address = sector * SPI_PAGE_SIZE * 2;
    for (UINT i = 0; i < count*2; i++)
        {
        if((address & (SPI_BLOCK_SIZE-1)) == 0
        && SPI_EraseSector(&hspi2, address) != HAL_OK)
            {
            return RES_ERROR;
            }
        if (SPI_WritePage(&hspi2, address, (uint8_t *)buff + i * SPI_PAGE_SIZE, SPI_PAGE_SIZE) != HAL_OK) return RES_ERROR;
        address += SPI_PAGE_SIZE;
        }

    return RES_OK;

    /* USER CODE END WRITE */
}

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
DRESULT SPI_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */

    switch (cmd)
        {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(DWORD *)buff = SPI_TOTAL_SIZE / SPI_LBA_SIZE;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = SPI_LBA_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = SPI_BLOCK_SIZE;
        return RES_OK;
    default:
        return RES_PARERR;
        }

  /* USER CODE END IOCTL */

}


DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}



/**
 * @brief Converts a single character between OEM and Unicode.
 *
 * This implementation handles only ASCII characters.
 * Characters outside the ASCII range are replaced with '?'.
 *
 * @param chr The character to convert.
 * @param dir The direction of conversion:
 *            0 - OEM to Unicode (Decode)
 *            1 - Unicode to OEM (Encode)
 * @return WCHAR The converted character.
 */
WCHAR ff_convert(WCHAR chr, UINT dir) {
    // ASCII range check
    if (chr < 128) {
        return (WCHAR)chr;
    } else {
        return (WCHAR)'?';  // Placeholder for non-ASCII characters
    }
}



/**
 * @brief Converts a single ASCII character to uppercase.
 *
 * Only lowercase ASCII letters ('a'-'z') are converted.
 * All other characters are returned unchanged.
 *
 * @param wc The character to convert.
 * @return WCHAR The uppercase equivalent if applicable.
 */
WCHAR ff_wtoupper(WCHAR wc) {
    if(wc >= 'a' && wc <= 'z') {
        return (WCHAR)(wc - ('a' - 'A'));
    }
    return wc;  // Return unchanged for non-lowercase letters
}

