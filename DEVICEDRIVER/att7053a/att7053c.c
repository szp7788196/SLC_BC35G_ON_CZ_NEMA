#include "att7053c.h"
#include "common.h"
#include "delay.h"


void ATT7053C_Init(void)
{
	SPI1_Init();	//SPI���߳�ʼ��
	
	SPI_Write(REG_WPC, 0x00BC); //��д����
	SPI_Write(REG_ANAEN, 0x0007); //��ADC2
	
	delay_ms(200);
}


//��ȡ��ѹ��Чֵ
float Att7053cGetVoltage(void)
{
	static float voltage = 0;
	u32 value = 0;

	value = SPI_Read(REG_V_RMS);
	
	voltage = (float)value * VOLTAGE_RATIO;

	return voltage;
}

//��ȡ������Чֵ
float Att7053cGetCurrent(void)
{
	static float current = 0;
	u32 value = 0;

	value = SPI_Read(REG_C_RMS);
	
	current = (float)value * CURRENT_RATIO;

	return current;
}

//��ȡ��ѹͨ����Ƶ��
float Att7053cGetVoltageFreq(void)
{
	static float freq_value = 0;
	u32 value = 0;

	value = SPI_Read(REG_FREQ);

	freq_value = 1000000.0f / 2.0f / (float)value;

	return freq_value;
}

//��ȡ�й�����
float Att7053cGetChannel1PowerP(void)
{
	static float power_p = 0;
	s32 value = 0;

	value = SPI_Read(REG_POWER_P);

	if(value & (1 << 23))
	{
		value = value - 0x00FFFFFF;
	}

	power_p = (float)value * POWER_RATIO;

	return power_p;
}

//��ȡ�޹�����
float Att7053cGetChannel1PowerQ(void)
{
	static float power_q = 0;
	s32 value = 0;

	value = SPI_Read(REG_POWER_Q);

	if(value & (1 << 23))
	{
		value = value - 0x00FFFFFF;
	}

	power_q = (float)value * POWER_RATIO;

	return power_q;
}

//��ȡ���ڹ���
float Att7053cGetChannel1PowerS(void)
{
	static float power_s = 0;
	s32 value = 0;

	value = SPI_Read(REG_POWER_S);

	if(value & (1 << 23))
	{
		value = value - 0x00FFFFFF;
	}

	power_s = (float)value * POWER_RATIO;

	return power_s;
}

//��ȡ�й�����
float Att7053cGetEnergyP(void)
{
	static float energy_p = 0;
	s32 value = 0;

	value = SPI_Read(REG_ENENRGY_P);

	energy_p = (float)value * ELECTRIC_ENERGY_METER_CONSTANT;

	return energy_p;
}

//��ȡ�޹�����
float Att7053cGetEnergyQ(void)
{
	static float energy_q = 0;
	s32 value = 0;

	value = SPI_Read(REG_ENENRGY_Q);

	energy_q = (float)value * ELECTRIC_ENERGY_METER_CONSTANT;

	return energy_q;
}

//type 0:����ѹ�������� 1:��������������
void Att7053cGetSamplingSequence(u8 type,float *buf,u8 len)
{
	s32 value = 0;
	u8 i = 0;
	u32 cmd = 0;

	
	if(type == 0)
	{
		cmd = 0xCCC0;
	}
	else if(type == 1)
	{
		cmd = 0xCCC1;
	}
	
	SPI_Write(0x48,cmd);
	
	delay_ms(50);
	
	for(i = 0; i < len; i ++)
	{
		value = SPI_Read(REG_WAVE_BUF);

		if(value & (1 << 15))
		{
			value = value - 0xFFFF;
		}
		
		*(buf + i) = value;
	}
}
























