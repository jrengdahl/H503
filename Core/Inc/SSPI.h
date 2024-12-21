#ifndef SSPI_H
#define SSPI_H

#include <stdint.h>
#include "main.h"
#include "spi.h"


#ifdef __cplusplus
extern "C" {
#endif

#define SPI_PAGE_SIZE 256
#define SPI_LBA_SIZE 512
#define SPI_BLOCK_SIZE 4096
#define SPI_TOTAL_SIZE (16 * 1024 * 1024) // 16 MB for example


#define READ_STATUS_REG_1_CMD 0x05
#define READ_STATUS_REG_2_CMD 0x35
#define READ_STATUS_REG_3_CMD 0x15

HAL_StatusTypeDef SPI_WritePage(SPI_HandleTypeDef *hspi, uint32_t address, uint8_t *data, uint32_t size);
HAL_StatusTypeDef SPI_ReadPage(SPI_HandleTypeDef *hspi, uint32_t address, uint8_t *data, uint32_t size);
HAL_StatusTypeDef SPI_EraseSector(SPI_HandleTypeDef *ospi, uint32_t address);
HAL_StatusTypeDef SPI_EraseChip(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef SPI_ReadStatusReg(SPI_HandleTypeDef *hspi, uint8_t regCommand, uint8_t *regValue);

#ifdef __cplusplus
}
#endif

#endif // SSPI_H
