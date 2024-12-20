#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "spi.h"
#include "gpio.h"


// WinBond W25Q128JVS SPI NOR flash instructions
#define CMD_WRITE_ENABLE    0x06
#define CMD_PAGE_PROGRAM    0x02
#define CMD_READ_DATA       0x03  // Normal read (no dummy cycles)
#define CMD_READ_STATUS     0x05
#define CMD_FAST_READ       0x0B  // Fast read command (dummy byte required)
#define CMD_SECTOR_ERASE    0x20
#define CMD_CHIP_ERASE      0xC7


// Wait timeout (adjust as needed)
#define SPI_TIMEOUT        1000

// Helper function: Pull CS low
static inline void NOR_CS_Low(void)
    {
    HAL_GPIO_WritePin(GPIONAME(SPI2_NSS), GPIO_PIN_RESET);
    }

// Helper function: Release CS (pull high)
static inline void NOR_CS_High(void)
    {
    HAL_GPIO_WritePin(GPIONAME(SPI2_NSS), GPIO_PIN_SET);
    }



// write data to the SPI-NOR
extern "C"
HAL_StatusTypeDef SPI_WritePage(
        SPI_HandleTypeDef *hspi,   // handle for the SPI port connected to the flash device
        uint32_t address,           // address of the block in the SPI flash to begin writing at
        uint8_t *data,              // address in memory of the data to be written
        uint32_t size)              // how many bytes to write
{

    HAL_StatusTypeDef status;
    uint8_t cmd[4];

    // Page size limit for W25Q128 is typically 256 bytes.
    if (size > 256)
        return HAL_ERROR;

    // 1. Write Enable
    uint8_t wren = CMD_WRITE_ENABLE;
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, &wren, 1, SPI_TIMEOUT);
    NOR_CS_High();
    if (status != HAL_OK)
        {
        return status;
        }

    // 2. Page Program command
    // CMD_PAGE_PROGRAM = 0x02 for single SPI
    // Send: [0x02, 24-bit address, then data]

    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (uint8_t)((address >> 16) & 0xFF);
    cmd[2] = (uint8_t)((address >> 8) & 0xFF);
    cmd[3] = (uint8_t)(address & 0xFF);

     // Send the command + address
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, cmd, 4, SPI_TIMEOUT);
    if (status != HAL_OK)
        {
        NOR_CS_High();
        return status;
        }

    // Send the data
    status = HAL_SPI_Transmit(hspi, data, size, SPI_TIMEOUT);
    NOR_CS_High();
    if (status != HAL_OK)
        {
        return status;
        }

    // 3. Poll the WIP bit in the Status Register until the write completes
    // Read status register (0x05) until WIP bit (bit 0) is cleared.
    while(1)
        {
        uint8_t cmd_status = CMD_READ_STATUS;
        uint8_t reg;

        NOR_CS_Low();
        status = HAL_SPI_Transmit(hspi, &cmd_status, 1, SPI_TIMEOUT);
        if (status != HAL_OK)
            {
            NOR_CS_High();
            return status;
            }

        status = HAL_SPI_Receive(hspi, &reg, 1, SPI_TIMEOUT);
        NOR_CS_High();
        if (status != HAL_OK)
            {
            return status;
            }

        // Check WIP bit
        if ((reg & 0x01) == 0x00)
            {
            break; // Write has completed
            }

        HAL_Delay(1); // small delay before next status read
        }

    return HAL_OK;
    }


/**
 * @brief  Read data from the SPI-NOR flash using SPI2.
 * @param  hspi:       SPI handle for SPI2.
 * @param  address:    Address in the SPI flash memory to begin reading from.
 * @param  data:       Pointer to the data buffer to store read bytes.
 * @param  size:       Number of bytes to read (up to 256 for a page).
 * @retval HAL status
 */

extern "C"
HAL_StatusTypeDef SPI_ReadPage(
    SPI_HandleTypeDef *hspi,
    uint32_t address,
    uint8_t *data,
    uint32_t size)
    {
    HAL_StatusTypeDef status;
    uint8_t cmd[5];

    // For fast read (0x0B): 1 byte command + 3 bytes address + 1 dummy byte = 5 bytes total before data.

    // Check size
    if (size > 256)
        {
        return HAL_ERROR;
        }

    // Prepare command sequence for Fast Read:
    // 0x0B + 24-bit address + 1 dummy byte
    cmd[0] = CMD_FAST_READ;
    cmd[1] = (uint8_t)((address >> 16) & 0xFF);
    cmd[2] = (uint8_t)((address >> 8) & 0xFF);
    cmd[3] = (uint8_t)(address & 0xFF);
    cmd[4] = 0x00; // dummy byte


    // Send command + address + dummy byte
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, cmd, 5, SPI_TIMEOUT);
    if (status != HAL_OK)
        {
        NOR_CS_High();
        return status;
        }

    // Now read the data
    status = HAL_SPI_Receive(hspi, data, size, SPI_TIMEOUT);
    NOR_CS_High();

    if (status != HAL_OK)
        {
        return status;
        }

    return HAL_OK;
    }



/**
 * @brief  Erase a sector in the SPI-NOR flash using SPI2.
 * @param  hspi:       SPI handle for SPI2.
 * @param  address:    The 24-bit sector address to be erased.
 * @retval HAL status
 */

extern "C"
HAL_StatusTypeDef SPI_EraseSector(SPI_HandleTypeDef *hspi, uint32_t address)
{
    HAL_StatusTypeDef status;
    uint8_t cmd[4];
    uint8_t reg;

    // 1. Write Enable
    uint8_t wren = CMD_WRITE_ENABLE;
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, &wren, 1, SPI_TIMEOUT);
    NOR_CS_High();
    if (status != HAL_OK)
        {
        return status;
        }

    // 2. Sector Erase Command
    // CMD_SECTOR_ERASE = 0x20 for W25Q128
    // Format: [0x20, 24-bit Address]

    cmd[0] = CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)((address >> 16) & 0xFF);
    cmd[2] = (uint8_t)((address >> 8) & 0xFF);
    cmd[3] = (uint8_t)(address & 0xFF);

    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, cmd, 4, SPI_TIMEOUT);
    NOR_CS_High();
    if (status != HAL_OK)
        {
        return status;
        }

    // 3. Poll WIP bit in Status Register until erase completes
    // Use CMD_READ_STATUS (0x05) and read one byte. WIP is bit 0.
    while(1)
        {
        uint8_t cmd_status = CMD_READ_STATUS;

        NOR_CS_Low();
        status = HAL_SPI_Transmit(hspi, &cmd_status, 1, SPI_TIMEOUT);
        if (status != HAL_OK)
            {
            NOR_CS_High();
            return status;
            }

        status = HAL_SPI_Receive(hspi, &reg, 1, SPI_TIMEOUT);
        NOR_CS_High();
        if (status != HAL_OK)
            {
            return status;
            }

        // Check WIP bit
        // If WIP = 1, the device is busy erasing the sector
        // If WIP = 0, erase is complete
        if ((reg & 0x01) == 0x00)
            {
            break;
            }

        HAL_Delay(1); // small delay before next status read
        }

    return HAL_OK;
    }


/**
 * @brief  Erase the entire SPI-NOR flash chip using SPI2.
 * @param  hspi: SPI handle for SPI2.
 * @retval HAL status
 */

extern "C"
HAL_StatusTypeDef SPI_EraseChip(SPI_HandleTypeDef *hspi)
    {
    HAL_StatusTypeDef status;
    uint8_t cmd;
    uint8_t reg;

    // 1. Write Enable
    cmd = CMD_WRITE_ENABLE;
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, &cmd, 1, SPI_TIMEOUT);
    NOR_CS_Low();
    if (status != HAL_OK)
        {
        NOR_CS_Low();
        return status;
        }


    // 2. Chip Erase command (0xC7)
    cmd = CMD_CHIP_ERASE;
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, &cmd, 1, SPI_TIMEOUT);
    NOR_CS_High();
    if (status != HAL_OK)
        {
        return status;
        }


    // 3. Poll the WIP bit in the status register until erase completes
    // CMD_READ_STATUS (0x05), read 1 byte of status
    while(1)
        {
        uint8_t cmd_status = CMD_READ_STATUS;

        NOR_CS_Low();
        status = HAL_SPI_Transmit(hspi, &cmd_status, 1, SPI_TIMEOUT);
        if (status != HAL_OK)
            {
            NOR_CS_High();
            return status;
            }

        status = HAL_SPI_Receive(hspi, &reg, 1, SPI_TIMEOUT);
        NOR_CS_High();
        if (status != HAL_OK)
            {
            return status;
            }

        // Check WIP bit (bit 0)
        // While (reg & 0x01) == 1, the chip is busy erasing
        if ((reg & 0x01) == 0x00)
            {
            break;
            }

        HAL_Delay(10); // Chip erase can take significant time, wait a bit before checking again
        }

    return HAL_OK;
    }


/**
 * @brief  Read a status register from the SPI-NOR flash.
 * @param  hspi:       SPI handle for SPI2.
 * @param  regCommand: The command byte to read the desired status register (e.g. 0x05).
 * @param  regValue:   Pointer to store the read register value (1 byte).
 * @retval HAL status
 */
extern "C"
HAL_StatusTypeDef SPI_ReadStatusReg(
    SPI_HandleTypeDef *hspi,
    uint8_t regCommand,
    uint8_t *regValue)
    {
    HAL_StatusTypeDef status;

    // Send the register read command
    NOR_CS_Low();
    status = HAL_SPI_Transmit(hspi, &regCommand, 1, SPI_TIMEOUT);
    if (status != HAL_OK)
        {
        NOR_CS_High();
        return status;
        }

    // Receive the 1-byte status register value
    status = HAL_SPI_Receive(hspi, regValue, 1, SPI_TIMEOUT);
    NOR_CS_High();
    if (status != HAL_OK)
        {
        NOR_CS_High();
        return status;
        }

    return HAL_OK;
    }


void print_status_register(uint8_t regCommand)
    {
    uint8_t statusReg = 0;

    if (SPI_ReadStatusReg(&hspi2, regCommand, &statusReg) != HAL_OK)
        {
        printf("Error reading Status Register %02x\n", regCommand);
        }
    else
        {
        printf("Status Register %02x: 0x%02X\n", regCommand, statusReg);
        }
    }


