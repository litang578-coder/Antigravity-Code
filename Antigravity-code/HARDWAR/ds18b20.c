#include "ds18b20.h"

#define DS18B20_CMD_SKIP_ROM	0xCC
#define DS18B20_CMD_CONVERT_T	0x44
#define DS18B20_CMD_READ_SCRATCH	0xBE

static void DS18B20_PinOutput(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	RCC_APB2PeriphClockCmd(DS18B20_GPIO_RCC, ENABLE);

	gpio_init_structure.GPIO_Pin = DS18B20_GPIO_PIN;
	gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init_structure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(DS18B20_GPIO_PORT, &gpio_init_structure);
}

static void DS18B20_PinInput(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	RCC_APB2PeriphClockCmd(DS18B20_GPIO_RCC, ENABLE);

	gpio_init_structure.GPIO_Pin = DS18B20_GPIO_PIN;
	gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init_structure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(DS18B20_GPIO_PORT, &gpio_init_structure);
}

static void DS18B20_WriteLevel(BitAction level)
{
	GPIO_WriteBit(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, level);
}

static u8 DS18B20_ReadLevel(void)
{
	return GPIO_ReadInputDataBit(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN);
}

static void DS18B20_WriteBit(u8 bit_value)
{
	DS18B20_PinOutput();
	DS18B20_WriteLevel(Bit_RESET);

	if(bit_value)
	{
		delay_us(2);
		DS18B20_WriteLevel(Bit_SET);
		delay_us(60);
	}
	else
	{
		delay_us(60);
		DS18B20_WriteLevel(Bit_SET);
		delay_us(2);
	}
}

static u8 DS18B20_ReadBit(void)
{
	u8 bit_value;

	DS18B20_PinOutput();
	DS18B20_WriteLevel(Bit_RESET);
	delay_us(2);

	DS18B20_WriteLevel(Bit_SET);
	DS18B20_PinInput();
	delay_us(8);

	bit_value = DS18B20_ReadLevel();
	delay_us(50);

	return bit_value;
}

static void DS18B20_WriteByte(u8 data)
{
	u8 i;

	for(i = 0; i < 8; i++)
	{
		DS18B20_WriteBit(data & 0x01);
		data >>= 1;
	}
}

static u8 DS18B20_ReadByte(void)
{
	u8 i;
	u8 data = 0;

	for(i = 0; i < 8; i++)
	{
		if(DS18B20_ReadBit())
			data |= (1 << i);
	}

	return data;
}

void DS18B20_Init(void)
{
	DS18B20_PinOutput();
	DS18B20_WriteLevel(Bit_SET);
}

u8 DS18B20_Reset(void)
{
	u16 retry = 0;

	DS18B20_PinOutput();
	DS18B20_WriteLevel(Bit_RESET);
	delay_us(750);
	DS18B20_WriteLevel(Bit_SET);
	delay_us(15);

	DS18B20_PinInput();
	while(DS18B20_ReadLevel() && retry < 200)
	{
		retry++;
		delay_us(1);
	}
	if(retry >= 200)
		return 1;

	retry = 0;
	while((DS18B20_ReadLevel() == 0) && retry < 240)
	{
		retry++;
		delay_us(1);
	}

	return 0;
}

void DS18B20_StartConvert(void)
{
	if(DS18B20_Reset() != 0)
		return;

	DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
	DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);
}

u8 DS18B20_ReadTemp(float *temperature)
{
	short raw_temp;
	u8 temp_l;
	u8 temp_h;

	if(temperature == 0)
		return 1;

	if(DS18B20_Reset() != 0)
		return 1;

	DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
	DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCH);

	temp_l = DS18B20_ReadByte();
	temp_h = DS18B20_ReadByte();
	raw_temp = (short)((temp_h << 8) | temp_l);

	*temperature = (float)raw_temp / 16.0f;

	return 0;
}
