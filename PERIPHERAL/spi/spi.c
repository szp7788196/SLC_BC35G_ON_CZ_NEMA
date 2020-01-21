#include "spi.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//SPI 驱动代码
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/6
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////


//以下是SPI模块的初始化代码，配置成主机模式
//SPI口初始化
//这里针是对SPI1的初始化
void SPI1_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_SetBits(GPIOB, GPIO_Pin_13);
	GPIO_SetBits(GPIOB, GPIO_Pin_15);
}


unsigned long SPI_Read( unsigned char address )
{
	signed char i;
	unsigned char miso_val = 0;
	unsigned long temp_data;
	
	temp_data = 0;
	ATT7053_SCK_LOW;                        /* 确保CLK初始状态是 */
	delay_us( 10 );
	ATT7053_CS_HIGH;
	delay_us( 20 );
	ATT7053_CS_LOW;
	delay_us( 50 );
	ATT7053_SCK_HIGH;                       /* 启动数据传输 */
	delay_us( 10 );
	ATT7053_MOSI_LOW;                       /* 0表示主机向从机读数据 */
	delay_us( 10 );
	ATT7053_SCK_LOW;                        /* 从机需要在下降沿取数据 */
	delay_us( 10 );

	for ( i = 6; i >= 0; i-- )
	{
		ATT7053_SCK_HIGH;               /* 启动数据传输 */
		delay_us( 10 );
		
		if ( (address & (0x01 << i) ) >> i )
			ATT7053_MOSI_HIGH;      /* address 地址从0x00到0x7F */
		else
			ATT7053_MOSI_LOW;
		
		delay_us( 10 );
		ATT7053_SCK_LOW;                /* 从机需要在下降沿取数据 */
		delay_us( 10 );
	}
	/* 从机接收到命令后，开始送出数据，上升沿送数据 */
	delay_us( 10 );
	ATT7053_SCK_HIGH;
	delay_us( 10 );
	ATT7053_SCK_LOW;
	if ( ATT7053_MISO_VALUE )
		temp_data = temp_data | 1;
	
	delay_us( 10 );
	
	for ( i = 0; i < 23; i++ )
	{
		ATT7053_SCK_HIGH;
		delay_us( 10 );
		ATT7053_SCK_LOW;
		
		if ( ATT7053_MISO_VALUE )
			miso_val = 1;
		else
			miso_val = 0;
		
		temp_data = (temp_data << 1) | miso_val;
		delay_us( 10 );
	}
	
	ATT7053_CS_HIGH;
	return(temp_data);
}


void SPI_Write( unsigned char address, unsigned int write_data )
{
	signed char i;
	unsigned int temp_data;

	temp_data = 1;
	ATT7053_SCK_LOW;                                /* 确保CLK初始状态是0 */
	delay_us( 100 );
	ATT7053_CS_HIGH;
	delay_us( 50 );
	ATT7053_CS_LOW;
	delay_us( 100 );

	ATT7053_SCK_HIGH;                               /* 启动数据传输 */
	delay_us( 10 );
	ATT7053_MOSI_HIGH;                              /* 1表示主机向从机写数据 */
	delay_us( 10 );
	ATT7053_SCK_LOW;                                /* 从机需要在下降沿取数据 */
	delay_us( 10 );

	for ( i = 6; i >= 0; i-- )
	{
		ATT7053_SCK_HIGH;                       /* 启动数据传输 */
		delay_us( 10 );
		
		if ( (address & (0x01 << i) ) >> i )    /* address 地址从0x00到0x7F */
		{
			ATT7053_MOSI_HIGH;
		}
		else 
			ATT7053_MOSI_LOW;
		
		delay_us( 10 );
		
		ATT7053_SCK_LOW;                        /* 从机需要在下降沿取数据 */
		delay_us( 10 );
	}
	/* 从机接收到命令后，开始送出数据，上升沿送数据 */
	delay_us( 40 );

	for ( i = 23; i >= 0; i-- )
	{
		ATT7053_SCK_HIGH;                               /* 启动数据传输 */
		delay_us( 10 );
		
		if ( (write_data & (temp_data << i) ) >> i )    /* address 地址从0x00到0x7F */
		{
			ATT7053_MOSI_HIGH;
		}
		else 
			ATT7053_MOSI_LOW;
		
		delay_us( 10 );
		
		ATT7053_SCK_LOW;                                /* 从机需要在下降沿取数据 */
		delay_us( 10 );
	}
	
	ATT7053_CS_HIGH;
} 






