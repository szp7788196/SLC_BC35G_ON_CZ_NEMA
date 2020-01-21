#ifndef __SPI_H
#define __SPI_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//SPI ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/5/6
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

#define ATT7053_CS_HIGH		GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define ATT7053_CS_LOW		GPIO_ResetBits(GPIOB, GPIO_Pin_12)

#define ATT7053_SCK_HIGH	GPIO_SetBits(GPIOB, GPIO_Pin_13)
#define ATT7053_SCK_LOW		GPIO_ResetBits(GPIOB, GPIO_Pin_13)

#define ATT7053_MOSI_HIGH	GPIO_SetBits(GPIOB, GPIO_Pin_15)
#define ATT7053_MOSI_LOW	GPIO_ResetBits(GPIOB, GPIO_Pin_15)

#define ATT7053_MISO_VALUE	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)



void SPI1_Init(void);			 //��ʼ��SPI1��
void SPI1_SetSpeed(u8 SpeedSet); //����SPI1�ٶ�   
u8 SPI1_ReadWriteByte(u8 TxData);//SPI1���߶�дһ���ֽ�

void SPI1_SendLenByte(u8 add,u8 *data,u8 len);
void SPI1_RecvLenByte(u8 add,u8 *data,u8 len);





unsigned long SPI_Read( unsigned char address );
void SPI_Write( unsigned char address, unsigned int write_data );
















		 
#endif

