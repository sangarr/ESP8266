#include "gpio.h"
#include "SensorData.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "c_types.h"


#define	OP_READ			129		// 器件地址以及读取操作,0xa1即为1010 0001B
#define	OP_WRITE 		128		// 器件地址以及写入操作,0xa1即为1010 0000B
#define SOFT_RESET		0xFE

#define SHT21_SCL    	5
#define SHT21_SDA    	13
#define SHT21_SDA_BIT 		13
uint16_t g_temperature = 0;
uint16_t g_humidity = 0;
uint8_t SerialNumber[10] = {0};

static void SetSDAInput(void)
{
//	GPIO_AS_INPUT(SHT21_SDA);

	GPIO_DIS_OUTPUT(GPIO_ID_PIN(13));
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTCK_U);
}
static uint8_t Read_SDA_State(void)
{
	uint8_t i;
	i = GPIO_INPUT_GET(SHT21_SDA);
	return i;
}
static void Start(void)
{
	GPIO_OUTPUT_SET(SHT21_SDA,1);//SDA = 1;
	GPIO_OUTPUT_SET(SHT21_SCL,1);//SCL = 1;
	os_delay_us(5);
	GPIO_OUTPUT_SET(SHT21_SDA,0);//SDA = 0;
	os_delay_us(5);
	GPIO_OUTPUT_SET(SHT21_SCL,0);//SCL = 0;
}

static void Stop(void)
{
	GPIO_OUTPUT_SET(SHT21_SDA,0);//SDA = 0;
	GPIO_OUTPUT_SET(SHT21_SCL,1);//SCL = 1;
	os_delay_us(5);
	GPIO_OUTPUT_SET(SHT21_SDA,1);//SDA = 1;
	os_delay_us(5);
	GPIO_OUTPUT_SET(SHT21_SDA,0);//SDA = 0;
	os_delay_us(2);
	GPIO_OUTPUT_SET(SHT21_SCL,0);//SCL = 0;
}
static void SendACK(void)
{
	//GPIO_OUTPUT_SET(SHT21_SDA);
	GPIO_OUTPUT_SET(SHT21_SDA,0);//SDA = 0;
	os_delay_us(5);//TWI_NOP();
	GPIO_OUTPUT_SET(SHT21_SCL,1);//SCL = 1;
	os_delay_us(5);//TWI_NOP();
	GPIO_OUTPUT_SET(SHT21_SCL,0);//SCL = 0;
	os_delay_us(5);//TWI_NOP();
	GPIO_OUTPUT_SET(SHT21_SDA,1);//SDA = 1;
}

static void SendNACK(void)
{
	//GPIO_OUTPUT_SET(SHT21_SDA);

	GPIO_OUTPUT_SET(SHT21_SDA,1);//SDA = 1;//	TWI_SDA_1();
	os_delay_us(5);//	TWI_NOP();
	GPIO_OUTPUT_SET(SHT21_SCL,1);//SCL = 1;//	TWI_SCL_1();
	os_delay_us(5);//	TWI_NOP();
	GPIO_OUTPUT_SET(SHT21_SCL,0);//SCL = 0;//	TWI_SCL_0();
	os_delay_us(5);//	TWI_NOP();
}

static uint8_t SendByte(u8 Data)
{
	u8 i;
	//GPIO_OUTPUT_SET(SHT21_SDA);

	GPIO_OUTPUT_SET(SHT21_SCL,0);//SCL = 0;
	for(i=0;i<8;i++)
	{
		if(Data&0x80)
		{
			GPIO_OUTPUT_SET(SHT21_SDA,1);//TWI_SDA_1();
		}
		else
		{
			GPIO_OUTPUT_SET(SHT21_SDA,0);//TWI_SDA_0();
		}

		os_delay_us(5);
		GPIO_OUTPUT_SET(SHT21_SCL,1);
		os_delay_us(5);
		GPIO_OUTPUT_SET(SHT21_SCL,0);
//		os_delay_us(5);
		Data<<=1;
	}
	GPIO_OUTPUT_SET(SHT21_SDA,1);
	os_delay_us(2);//	TWI_NOP();

	SetSDAInput();//set SDA input mode

	GPIO_OUTPUT_SET(SHT21_SCL,1);
	os_delay_us(5);//	TWI_NOP();
	if(GPIO_INPUT_GET(SHT21_SDA_BIT))
	{
		i = 1;
	}
	else
	{
		i = 0;
	}
	GPIO_OUTPUT_SET(SHT21_SCL,0);
	return i;

}

static uint8_t ReadByte(void)
{
	uint8_t i,Dat;
	SetSDAInput();
	Dat=0;
	for(i=0;i<8;i++)
	{
		GPIO_OUTPUT_SET(SHT21_SCL,1);
		os_delay_us(5);
		Dat<<=1;
		if(GPIO_INPUT_GET(SHT21_SDA_BIT))
		{
			Dat|=0x01;
		}
		GPIO_OUTPUT_SET(SHT21_SCL,0);
		os_delay_us(5);
	}
	return Dat;
}





void GetSHT_Data(void)
{
	uint8_t i;
	/* Read from memory location 1 */
	Start();
	SendByte(OP_WRITE); //I2C address
	SendByte(0xFA); //Command for readout on-chip memory
	SendByte(0x0F); //on-chip memory address
	Start();
	SendByte(OP_READ); //I2C address
	SerialNumber[5] = ReadByte(); //Read SNB_3
	SendACK();
	ReadByte(); //Read CRC SNB_3 (CRC is not analyzed)
	SendACK();
	SerialNumber[4] = ReadByte(); //Read SNB_2
	SendACK();
	ReadByte(); //Read CRC SNB_2 (CRC is not analyzed)
	SendACK();
	SerialNumber[3] = ReadByte(); //Read SNB_1
	SendACK();
	ReadByte(); //Read CRC SNB_1 (CRC is not analyzed)
	SendACK();
	SerialNumber[2] = ReadByte(); //Read SNB_0
	SendACK();
	ReadByte(); //Read CRC SNB_0 (CRC is not analyzed)
	SendNACK();
	Stop();

	/* Read from memory location 2 */
	Start();
	SendByte(OP_WRITE); //I2C address
	SendByte(0xFC); //Command for readout on-chip memory
	SendByte(0xC9); //on-chip memory address
	Start();
	SendByte(OP_READ); //I2C address
	SerialNumber[1] = ReadByte(); //Read SNC_1
	SendACK();
	SerialNumber[0] = ReadByte(); //Read SNC_0
	SendACK();
	ReadByte(); //Read CRC SNC0/1 (CRC is not analyzed)
	SendACK();
	SerialNumber[7] = ReadByte(); //Read SNA_1
	SendACK();
	SerialNumber[6] = ReadByte(); //Read SNA_0
	SendACK();
	ReadByte(); //Read CRC SNA0/1 (CRC is not analyzed)
	SendNACK();
	Stop();

//	for(i = 0;i<10;i++)
//	{
//		printf("0x%02x ",SerialNumber[i]);
//	}
}

void SHT2x_MeasureTempHM(void)
{
	float TEMP = 2.55,HUMI;
    uint8_t tmp1 = 0, tmp2 = 0;
    uint16_t ST,SRH;

    Start();
    SendByte(OP_WRITE);
    SendByte(0xF3);

    os_delay_us(50000);//100ms
    os_delay_us(50000);
    Start();
    SendByte(OP_READ);
//
////    while(Bit_RESET == SHT2x_SCL_STATE())
////    {
////        SHT2x_Delay(20);
////    }
//
    tmp1 = ReadByte();
	SendACK();
    tmp2 = ReadByte();
	SendNACK();
    Stop();

//    printf("tmp1 is 0x%2x,tmp2 is 0x%2x\r\n",tmp1,tmp2);

    ST = (tmp1 << 8) | (tmp2 << 0);
    ST &= ~0x0003;
    TEMP = ((float)ST * 0.00268127) - 46.85;
    os_printf("Temperature is %d cent\r\n",(uint16_t)TEMP);
    g_temperature = (uint16_t)TEMP;

    Start();
    SendByte(OP_WRITE);
    SendByte(0xf5);
    os_delay_us(50000);//100ms
    os_delay_us(50000);


    Start();
    SendByte(OP_READ);

    tmp1 = ReadByte();
    SendACK();
    tmp2 = ReadByte();
    SendACK();
    Stop();

    SRH = (tmp1 << 8) | (tmp2 << 0);
    SRH &= ~0x0003;
    HUMI = ((float)SRH * 0.00190735) - 6;
    g_humidity = (uint16_t)HUMI;
    os_printf("Humidity is %d%%\r\n",(uint16_t)HUMI);

}

void SHT21_Init(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);//SCL
    GPIO_OUTPUT_SET(GPIO_ID_PIN(5), 1);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);//SDA
    GPIO_OUTPUT_SET(GPIO_ID_PIN(13), 1);

	Start();
	SendByte (OP_WRITE);     // I2C Adr
	SendByte (SOFT_RESET);    // Command
	Stop();
}
