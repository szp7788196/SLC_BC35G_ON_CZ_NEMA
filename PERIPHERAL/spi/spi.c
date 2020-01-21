#include "spi.h"
#include "delay.h"
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


//������SPIģ��ĳ�ʼ�����룬���ó�����ģʽ
//SPI�ڳ�ʼ��
//�������Ƕ�SPI1�ĳ�ʼ��
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
	ATT7053_SCK_LOW;                        /* ȷ��CLK��ʼ״̬�� */
	delay_us( 10 );
	ATT7053_CS_HIGH;
	delay_us( 20 );
	ATT7053_CS_LOW;
	delay_us( 50 );
	ATT7053_SCK_HIGH;                       /* �������ݴ��� */
	delay_us( 10 );
	ATT7053_MOSI_LOW;                       /* 0��ʾ������ӻ������� */
	delay_us( 10 );
	ATT7053_SCK_LOW;                        /* �ӻ���Ҫ���½���ȡ���� */
	delay_us( 10 );

	for ( i = 6; i >= 0; i-- )
	{
		ATT7053_SCK_HIGH;               /* �������ݴ��� */
		delay_us( 10 );
		
		if ( (address & (0x01 << i) ) >> i )
			ATT7053_MOSI_HIGH;      /* address ��ַ��0x00��0x7F */
		else
			ATT7053_MOSI_LOW;
		
		delay_us( 10 );
		ATT7053_SCK_LOW;                /* �ӻ���Ҫ���½���ȡ���� */
		delay_us( 10 );
	}
	/* �ӻ����յ�����󣬿�ʼ�ͳ����ݣ������������� */
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
	ATT7053_SCK_LOW;                                /* ȷ��CLK��ʼ״̬��0 */
	delay_us( 100 );
	ATT7053_CS_HIGH;
	delay_us( 50 );
	ATT7053_CS_LOW;
	delay_us( 100 );

	ATT7053_SCK_HIGH;                               /* �������ݴ��� */
	delay_us( 10 );
	ATT7053_MOSI_HIGH;                              /* 1��ʾ������ӻ�д���� */
	delay_us( 10 );
	ATT7053_SCK_LOW;                                /* �ӻ���Ҫ���½���ȡ���� */
	delay_us( 10 );

	for ( i = 6; i >= 0; i-- )
	{
		ATT7053_SCK_HIGH;                       /* �������ݴ��� */
		delay_us( 10 );
		
		if ( (address & (0x01 << i) ) >> i )    /* address ��ַ��0x00��0x7F */
		{
			ATT7053_MOSI_HIGH;
		}
		else 
			ATT7053_MOSI_LOW;
		
		delay_us( 10 );
		
		ATT7053_SCK_LOW;                        /* �ӻ���Ҫ���½���ȡ���� */
		delay_us( 10 );
	}
	/* �ӻ����յ�����󣬿�ʼ�ͳ����ݣ������������� */
	delay_us( 40 );

	for ( i = 23; i >= 0; i-- )
	{
		ATT7053_SCK_HIGH;                               /* �������ݴ��� */
		delay_us( 10 );
		
		if ( (write_data & (temp_data << i) ) >> i )    /* address ��ַ��0x00��0x7F */
		{
			ATT7053_MOSI_HIGH;
		}
		else 
			ATT7053_MOSI_LOW;
		
		delay_us( 10 );
		
		ATT7053_SCK_LOW;                                /* �ӻ���Ҫ���½���ȡ���� */
		delay_us( 10 );
	}
	
	ATT7053_CS_HIGH;
} 






