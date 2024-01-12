/*
		 * Author: Mohamed Saker
		 * Date : 10/1/2024
		 * Content : Functions Implementations of Bootloader
		 *
		 *
 */

#include "bootloader.h"


static uint32_t BL_CRC_Verify(uint8_t * pdata, uint32_t Data_Length, uint32_t HostCRC);
static void BL_Send_ACK(uint8_t Data_Len);
static void BL_Send_NACK();
static void BL_Get_Version(uint8_t *Host_Buffer);
static void BL_Get_Help(uint8_t *Host_Buffer);
static void BL_Get_Chip_Identification_Number(uint8_t *Host_Buffer);
static void BL_Flash_Erase(uint8_t *Host_Buffer);
static uint8_t Preform_Flash_Erase(uint32_t PageAddress, uint8_t Page_Number);
static void BL_Write_Data(uint8_t *Host_Buffer);
static uint8_t BL_Address_Verfication(uint32_t Address);

static uint8_t Flash_Memory_Payload_Write(uint16_t *pdata, uint32_t Start_Address, uint8_t Payload_Len);


static uint8_t Host_Buffer[200];

BL_status BL_FetchHostCommand()
{
	BL_status status = BL_NACK;
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint8_t Data_Length = 0;
	memset(Host_Buffer, 0, HOST_MAX_SIZE);
	HAL_Status = HAL_UART_Receive(&huart2, Host_Buffer, 1, HAL_MAX_DELAY);
	if(HAL_Status != HAL_OK)
	{
		status = BL_NACK;
	}
	else
	{
		Data_Length = Host_Buffer[0];
		HAL_Status = HAL_UART_Receive(&huart2, &Host_Buffer[1], Data_Length, HAL_MAX_DELAY);
		if(HAL_Status != HAL_OK)
		{
			status = BL_NACK;
		}
		else
		{
			switch(Host_Buffer[1])
			{
			case CBL_GET_VERSION_CMD : BL_SendMessage("Read The Version of BL"); BL_Get_Version(Host_Buffer); break;
			case CBL_GET_HELP_CMD : BL_SendMessage("Read The Help of BL"); BL_Get_Help(Host_Buffer); break;
			case CBL_GET_CID_CMD : BL_SendMessage("Read The ID of BL"); BL_Get_Chip_Identification_Number(Host_Buffer); break;
			case CBL_GO_TO_ADDR_CMD : BL_SendMessage("Jump to Address"); break;
			case CBL_FLASH_ERASE_CMD : BL_SendMessage("Erase the Flash Memory"); BL_Flash_Erase(Host_Buffer); break;
			case CBL_MEM_WRITE_CMD : BL_SendMessage("Write to the flash memory"); BL_Write_Data(Host_Buffer); break;
			default : status = BL_NACK;
			}
		}
	}
	return status;
}








void BL_SendMessage(char *format, ...)
{
	char message[100] = {0};
	va_list args;
	va_start(args, format);
	vsprintf(message, format, args);
	HAL_UART_Transmit(&huart2, (uint8_t *)message, sizeof(message), HAL_MAX_DELAY);
	va_end(args);
}


static uint32_t BL_CRC_Verify(uint8_t * pdata, uint32_t Data_Length, uint32_t HostCRC)
{
	uint8_t CRC_Status = CRC_VERFING_FAILED;
	uint32_t MCU_CRC = 0;
	uint32_t Data_Buffer = 0;
	for(uint8_t count = 0; count < Data_Length; count++)
	{
		Data_Buffer = (uint32_t)pdata[count];
		MCU_CRC = HAL_CRC_Accumulate(&hcrc, &Data_Buffer, 1);
	}
	__HAL_CRC_DR_RESET(&hcrc);
	if(HostCRC == MCU_CRC)
	{
		CRC_Status = CRC_VERFING_PASSED;
	}
	else
	{
		CRC_Status = CRC_VERFING_FAILED;
	}
	return CRC_Status;
}



static void BL_Send_ACK(uint8_t Data_Len)
{
	uint8_t ACK_val[2] = {0};
	ACK_val[0] = SEND_NACK;
	ACK_val[1] = Data_Len;
	HAL_UART_Transmit(&huart2, (uint8_t *)ACK_val, 2, HAL_MAX_DELAY);
}
static void BL_Send_NACK()
{
	uint8_t ACK_val = SEND_NACK;
	HAL_UART_Transmit(&huart2, &ACK_val, sizeof(ACK_val), HAL_MAX_DELAY);
}




static void BL_Get_Version(uint8_t *Host_Buffer)
{
	uint8_t Version[4] = {CBL_VENDOR_ID, CBL_SW_MAJOR_VERSION, CBL_SW_MINOR_VERSION, CBL_SW_PATCH_VERSION};

	uint16_t Host_Packet_Len = 0;

	uint32_t CRC_Val = 0;

	Host_Packet_Len = Host_Buffer[0] + 1;
	CRC_Val = *(uint32_t *)(Host_Buffer + Host_Packet_Len -4);
	if(CRC_VERFING_PASSED == BL_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_Packet_Len - 4, CRC_Val))
	{
		BL_Send_ACK(4);
		HAL_UART_Transmit(&huart2, (uint8_t *)Version, 4, HAL_MAX_DELAY);

	}
	else
	{
		BL_Send_NACK();
	}


}
static void BL_Get_Help(uint8_t *Host_Buffer)
{
	uint8_t BL_supported_CMS[] = {CBL_GET_VERSION_CMD, CBL_GET_HELP_CMD, CBL_GET_CID_CMD, CBL_GO_TO_ADDR_CMD, CBL_FLASH_ERASE_CMD, CBL_MEM_WRITE_CMD};

	uint16_t Host_Packet_Len = 0;

	uint32_t CRC_Val = 0;

	Host_Packet_Len = Host_Buffer[0] + 1;
	CRC_Val = *(uint32_t *)(Host_Buffer + Host_Packet_Len -4);
	if(CRC_VERFING_PASSED == BL_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_Packet_Len - 4, CRC_Val))
	{
		BL_Send_ACK(6);
		HAL_UART_Transmit(&huart2, (uint8_t *)BL_supported_CMS, 4, HAL_MAX_DELAY);

	}
	else
	{
		BL_Send_NACK();
	}
}
static void BL_Get_Chip_Identification_Number(uint8_t *Host_Buffer)
{
	uint16_t Chip_ID = 0;

	uint16_t Host_Packet_Len = 0;

	uint32_t CRC_Val = 0;

	Host_Packet_Len = Host_Buffer[0] + 1;
	CRC_Val = *(uint32_t *)(Host_Buffer + Host_Packet_Len -4);
	if(CRC_VERFING_PASSED == BL_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_Packet_Len - 4, CRC_Val))
	{
		Chip_ID = ((uint16_t)DBGMCU -> IDCODE & 0x00000FFF);
		BL_Send_ACK(6);
		HAL_UART_Transmit(&huart2, (uint8_t *)Chip_ID, 4, HAL_MAX_DELAY);

	}
	else
	{
		BL_Send_NACK();
	}
}

static void BL_Flash_Erase(uint8_t *Host_Buffer)
{
	uint8_t Erase_Status = UNSUCESSFUL_ERASE;

	uint16_t Host_Packet_Len = 0;

	uint32_t CRC_Val = 0;

	Host_Packet_Len = Host_Buffer[0] + 1;
	CRC_Val = *(uint32_t *)(Host_Buffer + Host_Packet_Len -4);
	if(CRC_VERFING_PASSED == BL_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_Packet_Len - 4, CRC_Val))
	{
		Erase_Status = Preform_Flash_Erase((*(uint32_t *)&Host_Buffer[7]), Host_Buffer[6]);

		BL_Send_ACK(1);



		HAL_UART_Transmit(&huart2, (uint8_t *)&Erase_Status, 4, HAL_MAX_DELAY);

	}
	else
	{
		BL_Send_NACK();
	}

}
static uint8_t Preform_Flash_Erase(uint32_t PageAddress, uint8_t Page_Number)
{
	FLASH_EraseInitTypeDef pEraseInit;

	HAL_StatusTypeDef Hal_Status = HAL_ERROR;

	uint32_t PageError = 0;

	uint8_t Page_Status = INVALID_PAGE_NUMBER;

	if(Page_Number > CBL_FLASH_MAX_PAGE_NUMBER)
	{
		Page_Status = INVALID_PAGE_NUMBER;
	}
	else
	{
		Page_Status = VALID_PAGE_NUMBER;
		if(Page_Number <= (CBL_FLASH_MAX_PAGE_NUMBER -1)  || (PageAddress == CBL_FLASH_MASS_ERASE))
		{
			if(PageAddress == CBL_FLASH_MASS_ERASE)
			{
				pEraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
				pEraseInit.Banks       = FLASH_BANK_1;
				pEraseInit.PageAddress = 0x8008000;
				pEraseInit.NbPages     = 12;
			}
			else
			{
				pEraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
				pEraseInit.Banks       = FLASH_BANK_1;
				pEraseInit.PageAddress = 0x8008000;
				pEraseInit.NbPages     = Page_Number;
			}

			HAL_FLASH_Unlock();

			Hal_Status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);

			HAL_FLASH_Lock();

			if(PageError == HAL_SUCCESSFUL_ERASE)
			{
				Page_Status = SUCCESSFUL_ERASE;
			}
			else
			{
				Page_Status = UNSUCESSFUL_ERASE;
			}
		}
		else
		{
			Page_Status = INVALID_PAGE_NUMBER;
		}
	}
	return Page_Status;
}
static void BL_Write_Data(uint8_t *Host_Buffer)
{
	uint8_t Address_verify = ADDRESS_IS_INVALID;

	uint32_t Address_Host = 0;

	uint8_t DataLen = 0;

	uint16_t Host_Packet_Len = 0;

	uint32_t CRC_Val = 0;

	uint8_t PayLoad_Status = FLASH_PAYLOAD_WRITE_FAILED;

	Host_Packet_Len = Host_Buffer[0] + 1;
	CRC_Val = *(uint32_t *)(Host_Buffer + Host_Packet_Len -4);
	if(CRC_VERFING_PASSED == BL_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_Packet_Len - 4, CRC_Val))
	{
		BL_Send_ACK(1);

		Address_Host = *((uint32_t *)&Host_Buffer[2]);
		DataLen = Host_Buffer[6];
		Address_verify = BL_Address_Verfication(Address_Host);
		if(Address_verify == ADDRESS_IS_VALID)
		{

			PayLoad_Status = Flash_Memory_Payload_Write((uint16_t *)&Host_Buffer[7], Address_Host, DataLen);
			HAL_UART_Transmit(&huart2, (uint8_t *)&PayLoad_Status, 4, HAL_MAX_DELAY);
		}
		else
		{
			HAL_UART_Transmit(&huart2, (uint8_t *)&Address_verify, 4, HAL_MAX_DELAY);
		}




	}
	else
	{
		BL_Send_NACK();
	}

}

static uint8_t BL_Address_Verfication(uint32_t Address)
{
	uint8_t Address_verify = ADDRESS_IS_INVALID;

	if((Address >= FLASH_BASE) && (Address <= STM32F103_FLASH_END))
	{
		Address_verify = ADDRESS_IS_VALID;
	}
	else if((Address >= SRAM_BASE) && (Address <= STM32F103_FLASH_END))
	{
		Address_verify = ADDRESS_IS_VALID;
	}
	else
	{
		Address_verify = ADDRESS_IS_INVALID;
	}
	return Address_verify;
}
static uint8_t Flash_Memory_Payload_Write(uint16_t *pdata, uint32_t Start_Address, uint8_t Payload_Len)
{
	uint32_t Address = 0;

	uint8_t UpdateAddress = 0;

	HAL_StatusTypeDef Hal_Status = HAL_ERROR;

	uint8_t PayLoad_Status = FLASH_PAYLOAD_WRITE_FAILED;

	HAL_FLASH_Unlock();


	for(uint8_t payload_count = 0; payload_count < (Payload_Len /2); payload_count++)
	{
		Address = Start_Address + UpdateAddress;

		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, pdata[payload_count]);

		if(Hal_Status != HAL_OK)
		{
			PayLoad_Status = FLASH_PAYLOAD_WRITE_FAILED;
		}
		else
		{
			PayLoad_Status = FLASH_PAYLOAD_WRITE_PASSED;
		}

	}

	return PayLoad_Status;

}



