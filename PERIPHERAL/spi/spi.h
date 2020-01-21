#ifndef __SPI_H
#define __SPI_H
#include "sys.h"
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

#define ATT7053_CS_HIGH		GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define ATT7053_CS_LOW		GPIO_ResetBits(GPIOB, GPIO_Pin_12)

#define ATT7053_SCK_HIGH	GPIO_SetBits(GPIOB, GPIO_Pin_13)
#define ATT7053_SCK_LOW		GPIO_ResetBits(GPIOB, GPIO_Pin_13)

#define ATT7053_MOSI_HIGH	GPIO_SetBits(GPIOB, GPIO_Pin_15)
#define ATT7053_MOSI_LOW	GPIO_ResetBits(GPIOB, GPIO_Pin_15)

#define ATT7053_MISO_VALUE	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)



void SPI1_Init(void);			 //初始化SPI1口
void SPI1_SetSpeed(u8 SpeedSet); //设置SPI1速度   
u8 SPI1_ReadWriteByte(u8 TxData);//SPI1总线读写一个字节

void SPI1_SendLenByte(u8 add,u8 *data,u8 len);
void SPI1_RecvLenByte(u8 add,u8 *data,u8 len);





unsigned long SPI_Read( unsigned char address );
void SPI_Write( unsigned char address, unsigned int write_data );
















		 
#endif

