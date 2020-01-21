#ifndef __ATT7053C_H
#define __ATT7053C_H

#include "sys.h"
#include "spi.h"

#define REG_WPC			0x32		//写保护
#define REG_FREQ_CFG	0x41		//配置寄存器
#define REG_ANAEN		0x43		//ADC开关配置
#define REG_V_RMS		0x08		//电压有效值
#define REG_FREQ		0x09		//电网频率
#define REG_C_RMS		0x06		//电流有效值
#define REG_POWER_P		0x0A		//有功功率
#define REG_POWER_Q		0x0B		//无功功率
#define REG_POWER_S		0x0C		//视在功率
#define REG_ENENRGY_P	0x0D		//有功能耗
#define REG_ENENRGY_Q	0x0E		//无功能耗
#define REG_WAVE_BUF	0x7F		//波形缓存


#define POWER_RATIO						0.01777947713f		//功率转换系数
#define ELECTRIC_ENERGY_METER_CONSTANT	3200.0f				//电能转换系数
#define CURRENT_RATIO					0.01144472f			//电流转换系数
#define VOLTAGE_RATIO					0.00019195212f		//电压转换系数


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
