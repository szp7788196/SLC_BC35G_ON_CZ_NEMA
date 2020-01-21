#ifndef __ATT7053C_H
#define __ATT7053C_H

#include "sys.h"
#include "spi.h"

#define REG_WPC			0x32		//д����
#define REG_FREQ_CFG	0x41		//���üĴ���
#define REG_ANAEN		0x43		//ADC��������
#define REG_V_RMS		0x08		//��ѹ��Чֵ
#define REG_FREQ		0x09		//����Ƶ��
#define REG_C_RMS		0x06		//������Чֵ
#define REG_POWER_P		0x0A		//�й�����
#define REG_POWER_Q		0x0B		//�޹�����
#define REG_POWER_S		0x0C		//���ڹ���
#define REG_ENENRGY_P	0x0D		//�й��ܺ�
#define REG_ENENRGY_Q	0x0E		//�޹��ܺ�
#define REG_WAVE_BUF	0x7F		//���λ���


#define POWER_RATIO						0.01777947713f		//����ת��ϵ��
#define ELECTRIC_ENERGY_METER_CONSTANT	3200.0f				//����ת��ϵ��
#define CURRENT_RATIO					0.01144472f			//����ת��ϵ��
#define VOLTAGE_RATIO					0.00019195212f		//��ѹת��ϵ��


void ATT7053C_Init(void);
float Att7053cGetVoltage(void);
float Att7053cGetVoltageFreq(void);
float Att7053cGetCurrent(void);
float Att7053cGetChannel1PowerP(void);
float Att7053cGetChannel1PowerQ(void);
float Att7053cGetChannel1PowerS(void);
float Att7053cGetEnergyP(void);
float Att7053cGetEnergyQ(void);
void Att7053cGetSamplingSequence(u8 type,float *buf,u8 len);































#endif
