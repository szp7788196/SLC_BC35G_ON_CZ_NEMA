#include "task_main.h"
#include "common.h"
#include "delay.h"
#include "inventr.h"
#include "event.h"


TaskHandle_t xHandleTaskMAIN = NULL;

RemoteControl_S ContrastControl;
unsigned portBASE_TYPE MAIN_Satck;

void vTaskMAIN(void *pvParameters)
{
	time_t times_sec = 0;
	u8 up_date_strategy_state = 0;
	time_t time_cnt = 0;
	u8 get_e_para_ok = 0;

	SetLightLevel(CurrentControl);						//上电按照默认设置亮灯

	while(1)
	{
		if(GetSysTick1s() - times_sec >= 1)				//每1秒钟轮训一次
		{
			times_sec = GetSysTick1s();

			if(GetTimeOK != 0)							//系统时间状态
			{
				CheckSwitchStatus(&CurrentControl);		//查询当前开关应该处于的状态

				if(CurrentControl._switch == 1)			//只有在开关为开的状态时才轮询策略
				{
					if(CurrentControl._switch != ContrastControl._switch)	//开灯之后要更新策略状态表
					{
						up_date_strategy_state = 1;
					}

					LookUpStrategyList(ControlStrategy,&CurrentControl,&up_date_strategy_state);	//轮训策略列表
				}
			}
		}

		if(ContrastControl._switch != CurrentControl._switch ||				//开关状态有变化
		   ContrastControl.control_type != CurrentControl.control_type ||	//控制方式有变化
		   ContrastControl.brightness != CurrentControl.brightness ||		//亮度有变化
		   ContrastControl.interface != CurrentControl.interface)			//控制接口有变化
		{
			ContrastControl._switch = CurrentControl._switch;
			ContrastControl.control_type = CurrentControl.control_type;
			ContrastControl.brightness = CurrentControl.brightness;
			ContrastControl.interface = CurrentControl.interface;

			SetLightLevel(CurrentControl);	//调光

			time_cnt = GetSysTick1s();		//复位获取当前状态电参数计时器
			get_e_para_ok = 0;				//复位获取当前电参数标志 0:未获取 1:以获取
		}
		else
		{
			if(GetSysTick1s() - time_cnt >= EventDetectConf.current_detect_delay * 60 / 2)			//等到电流检测延时的1/2时采集电参数
			{
				if(get_e_para_ok == 0)		//未获取当前电参数
				{
					get_e_para_ok = 1;		//以获取当前电参数

					CurrentControl.current = InputCurrent;		//获取当前电流值
					CurrentControl.voltage = InputVoltage;		//获取当前电压值
				}
			}
		}

#ifdef EVENT_RECORD
		CheckEventsEC15(CurrentControl);	//单灯正常开灯记录
		CheckEventsEC16(CurrentControl);	//单灯正常关灯记录
		CheckEventsEC17(CurrentControl);	//单灯异常开灯记录
		CheckEventsEC18(CurrentControl);	//单灯异常关灯记录
		CheckEventsEC19(CurrentControl);	//单灯电流过大记录
		CheckEventsEC20(CurrentControl);	//单灯电流过小记录
		CheckEventsEC52(CurrentControl);	//单灯状态变化记录
#endif

		if(DeviceReset != 0x00 || ReConnectToServer == 0x01)	//接收到重启的命令
		{
			if(DeviceReset == 0x01)			//ReBoot标志
			{
				delay_ms(5000);				//延时5秒,等待最后的报文发送完成

				DeviceReset = 0;			//复位ReBoot标志

				__disable_fault_irq();		//关闭全局中断
				NVIC_SystemReset();			//重启设备
			}
			else if(DeviceReset == 0x02 || DeviceReset == 0x03)	//恢复出厂设置2:恢复出厂设置除网络参数外 3:恢复所有参数到出厂设置
			{
				RestoreFactorySettings(DeviceReset);			//恢复出厂设置

				DeviceReset = 0;			//复位恢复出厂设置标志
			}

			if(ReConnectToServer == 0x01)	//上行网络重连标志
			{
				delay_ms(5000);				//延时5秒,等待最后的报文发送完成

				ReConnectToServer = 0x81;	//置位重连标志
			}
		}

		if(FrameWareState.state == FIRMWARE_DOWNLOADED)		//固件下载完成,即将引导新程序
		{
			delay_ms(1000);									//延时1秒,等待固件状态存入EEPROM

			__disable_fault_irq();							//关闭全局中断
			NVIC_SystemReset();								//重启指令
		}
		else if(FrameWareState.state == FIRMWARE_DOWNLOAD_FAILED)
		{
			FrameWareState.state = FIRMWARE_FREE;			//暂时不进行固件下载,等到下次上电的时候再下载

#ifdef EVENT_RECORD
			CheckEventsEC51(0x01,DeviceInfo.software_ver);	//固件升级失败
#endif
		}

		delay_ms(100);

//		MAIN_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}

void CheckSwitchStatus(RemoteControl_S *ctrl)
{
	u16 i = 0;
	s16 sum = 0;
	u8 month = 0;
	u8 date = 0;
	u8 month_c = 0;
	u8 date_c = 0;
	u8 on_hour = 0;
	u8 on_minute = 0;
	u8 off_hour = 0;
	u8 off_minute = 0;

	u16 on_gate = 0;
	u16 off_gate = 0;
	u16 now_gate = 0;

	if(SwitchMode != 1)							//非年表控制
	{
		CurrentControl._switch = 1;				//默认开灯
	}
	else										//年表控制
	{
		if(LampsSwitchProject.total_days >= 1 &&
		   LampsSwitchProject.total_days <= 366)	//年表不为空
		{
			month = LampsSwitchProject.start_month;
			date = LampsSwitchProject.start_date;

			if(month >= 1 && month <= 12 && date >= 1 && date <= 31)
			{
				for(i = 0; i < LampsSwitchProject.total_days; i ++)
				{
					sum = get_day_num(month,date) + i;

					if(sum <= 366)
					{
						get_date_from_days(sum, &month_c, &date_c);

						if(month_c >= 1 && month_c <= 12 && date_c >= 1 && date_c <= 31)		//得到合法日期
						{
							if(month_c == calendar.w_month && date_c == calendar.w_date)
							{
								on_hour = (LampsSwitchProject.switch_time[i].on_time[1] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].on_time[1] & 0x0F);
								on_minute = (LampsSwitchProject.switch_time[i].on_time[0] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].on_time[0] & 0x0F);
								off_hour = (LampsSwitchProject.switch_time[i].off_time[1] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].off_time[1] & 0x0F);
								off_minute = (LampsSwitchProject.switch_time[i].off_time[0] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].off_time[0] & 0x0F);

								if(on_hour <= 23 && on_minute <= 59 && off_hour <= 23 && off_minute <= 59)		//得到合法时间
								{
									on_gate = on_hour * 60 + on_minute;
									off_gate = off_hour * 60 + off_minute;
									now_gate = calendar.hour * 60 + calendar.min;

									if(on_gate != off_gate)
									{
										if(on_gate < off_gate)		//先开灯后关灯
										{
											if(on_gate <= now_gate && now_gate < off_gate)	//当前时间大于等于开灯时间,小于关灯时间,则开灯
											{
												ctrl->_switch = 1;		//开灯
											}
											else	//当前时间小于开灯时间,或者大于等于关灯时间,则关灯
											{
												ctrl->_switch = 0;		//关灯
											}
										}
										else						//先关灯后开灯
										{
											if(off_gate <= now_gate && now_gate < on_gate)	//当前时间大于等于关灯时间,小于开灯时间,则关灯
											{
												ctrl->_switch = 0;		//关灯
											}
											else	//当前时间小于关灯时间,或者大于等于开灯时间,则开灯
											{
												ctrl->_switch = 1;		//开灯
											}
										}
									}
									else					//开关灯时间相同
									{
										ctrl->_switch = 1;		//默认亮灯
									}
								}
								else	//得到非法时间
								{
									ctrl->_switch = 1;		//默认亮灯
								}

								i = 0xFF00;		//大于366即可
							}
						}
						else	//得到非法日期
						{
							ctrl->_switch = 1;	//默认亮灯
						}
					}
					else		//日期超出范围
					{
						ctrl->_switch = 1;		//默认亮灯
					}
				}
			}
			else								//年表表头错误
			{
				ctrl->_switch = 1;				//默认亮灯
			}
		}
		else									//年表为空
		{
			ctrl->_switch = 1;					//默认亮灯
		}
	}
}

//轮训策略列表
u8 LookUpStrategyList(pControlStrategy strategy_head,RemoteControl_S *ctrl,u8 *update)
{
	u8 ret = 0;

	static u8 date = 0;

	time_t gate0 = 0;
	time_t gate24 = 86400;	//24*3600;

	static time_t start_time = 0;		//开始执行时间
	static time_t total_time = 0;		//执行结束时间
	time_t current_time = 0;			//当前时间
	time_t start_time_next = 0;			//下一个策略的开始执行时间
	static u8 y_m_d_effective = 0;		//年月日有效标志 bit2 = 1年有效 bit1 = 1月有效 bit0 = 1日有效
	static u8 y_m_d_matched = 0;		//年月日匹配成功标志 bit2 = 1年 bit1 = 1月 bit0 = 1日
	static u8 y_m_d_effective_next = 0;	//年月日有效标志 bit2 = 1年有效 bit1 = 1月有效 bit0 = 1日有效
	static u8 y_m_d_matched_next = 0;	//年月日匹配成功标志 bit2 = 1年 bit1 = 1月 bit0 = 1日

	static u8 appointment_control_valid = 0;	//预约控制生效标志
	static u8 appointment_control_valid_c = 0;	//预约控制生效标志 比较
	static u8 appointment_control_valid_r = 0;	//预约控制生效标志 刷新

	pControlStrategy tmp_strategy = NULL;

	appointment_control_valid = CheckAppointmentControlValid();		//查看预约控制是否生效

	if(appointment_control_valid_c != appointment_control_valid)	//预约控制状态有变化
	{
		appointment_control_valid_c = appointment_control_valid;

		appointment_control_valid_r = 1;
	}

	if(NeedUpdateStrategyList == 1 ||
	   appointment_control_valid_r == 1)	//需要更新策略
	{
		NeedUpdateStrategyList = 0;
		appointment_control_valid_r = 0;

		ret = UpdateControlStrategyList(appointment_control_valid);	//更新策略列表

		if(ret == 0)						//更新策略列表失败
		{
//			ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度

			return 0;
		}
		else								//策略列表更新成功
		{
			start_time = 0;
			total_time = 0;
			current_time = 0;
			start_time_next = 0;
			y_m_d_effective = 0;
			y_m_d_matched = 0;
			y_m_d_effective_next = 0;
			y_m_d_matched_next = 0;

			date = calendar.w_date;
		}
	}

	xSemaphoreTake(xMutex_STRATEGY, portMAX_DELAY);

	if(date != calendar.w_date)		//过了一天或重新设置了时间
	{
		date = calendar.w_date;

		StrategyListStateReset(strategy_head,1);	//复位绝对时间策略状态
	}

	if(*update == 1)				//开关状态有变化,由关变为开
	{
		*update = 0;

		StrategyListStateReset(strategy_head,0);	//复位所有策略状态
	}

	if(strategy_head != NULL && ControlStrategy->next != NULL)	//策略列表不为空
	{
//		FOR_LOOP:

		for(tmp_strategy = strategy_head->next; tmp_strategy != NULL; tmp_strategy = tmp_strategy->next)
		{
			if(tmp_strategy->state == RES_EXECUTE)	//查看此条策略是否为保留执行策略
			{
				tmp_strategy->state = EXECUTING;

				start_time = tmp_strategy->hour * 3600 +
						     tmp_strategy->minute * 60 +
						     tmp_strategy->second;
			}

			switch(tmp_strategy->mode)
			{
				case 0:		//相对时间方式
					switch(tmp_strategy->state)
					{
						case WAIT_EXECUTE:		//等待执行状态
							start_time = GetSysTick1s();			//获取开始执行时间 秒
							total_time = tmp_strategy->time_re;		//获取执行结束时间 秒

							ctrl->lock = 0;		//解锁远程控制

							tmp_strategy->state = EXECUTING;		//下个状态将切换为 正在执行状态

							if(tmp_strategy->state == EXECUTING)	//此判断无实际作用,置位消除编译器警告
							{
								goto GET_OUT;
							}
						break;

						case EXECUTING:			//正在执行状态
							current_time = GetSysTick1s();		//获取当前时间

							if(current_time - start_time <= total_time)
							{
								if(ctrl->lock == 0)				//远程控制已解锁
								{
									ctrl->control_type = tmp_strategy->type;
									ctrl->brightness = tmp_strategy->brightness;
								}

								if(current_time - start_time >= total_time)	//到了结束时间
								{
									tmp_strategy->state = EXECUTED;		//下个状态将切换为 已过期状态
								}

								ret = 1;

								goto GET_OUT;
							}
						break;

						case EXECUTED:			//已过期状态

						break;

						default:				//未知状态
//							ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
						break;
					}
				break;

				case 1:		//绝对时间方式
					switch(tmp_strategy->state)
					{
						case WAIT_EXECUTE:		//等待执行状态
							start_time = tmp_strategy->hour * 3600 +
						                 tmp_strategy->minute * 60 +
						                 tmp_strategy->second;

							if(tmp_strategy->year != 0)		//判断策略中年是否有效
							{
								y_m_d_effective |= 0x04;
							}

							if(tmp_strategy->month != 0)	//判断策略中月是否有效
							{
								y_m_d_effective |= 0x02;
							}

							if(tmp_strategy->date != 0)		//判断策略中日是否有效
							{
								y_m_d_effective |= 0x01;
							}

							y_m_d_matched = 0x07;			//初始化年月日匹配成功标志

							if(tmp_strategy->next != NULL)	//下个策略有效
							{
								if(tmp_strategy->next->mode == 1)		//下个策略也是绝对时间方式
								{
									if(tmp_strategy->next->year != 0)	//判断策略中年是否有效
									{
										y_m_d_effective_next |= 0x04;
									}

									if(tmp_strategy->next->month != 0)	//判断策略中月是否有效
									{
										y_m_d_effective_next |= 0x02;
									}

									if(tmp_strategy->next->date != 0)	//判断策略中日是否有效
									{
										y_m_d_effective_next |= 0x01;
									}

									y_m_d_matched_next = 0x07;			//初始化年月日匹配成功标志
								}
							}

							if(ctrl->lock == 1)				//判断是否需要解开远程控制所
							{
								if(y_m_d_effective & 0x04 != (u32)0x00000000 &&
								   tmp_strategy->year != calendar.w_year - 2000)
								{
									y_m_d_matched &= ~(0x04);
								}

								if(y_m_d_effective & 0x02 != (u32)0x00000000 &&
								   tmp_strategy->month != calendar.w_month)
								{
									y_m_d_matched &= ~(0x02);
								}

								if(y_m_d_effective & 0x01 != (u32)0x00000000 &&
								   tmp_strategy->date != calendar.w_date)
								{
									y_m_d_matched &= ~(0x01);
								}

								if(y_m_d_matched == 0x07)	//年月日全部匹配成功
								{
									current_time = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;

									if(tmp_strategy->next != NULL)				//下个策略有效
									{
										if(tmp_strategy->next->mode == 1)		//下个策略也是绝对时间方式
										{
											if(y_m_d_effective_next & 0x04 != (u32)0x00000000 &&
											   tmp_strategy->next->year != calendar.w_year - 2000)
											{
												y_m_d_matched_next &= ~(0x04);
											}

											if(y_m_d_effective_next & 0x02 != (u32)0x00000000 &&
											   tmp_strategy->next->month != calendar.w_month)
											{
												y_m_d_matched_next &= ~(0x02);
											}

											if(y_m_d_effective_next & 0x01 != (u32)0x00000000 &&
											   tmp_strategy->next->date != calendar.w_date)
											{
												y_m_d_matched_next &= ~(0x01);
											}

											if(y_m_d_matched_next == 0x07)		//下个策略的年月日全部匹配成功
											{
												start_time_next = tmp_strategy->next->hour * 3600 +
															      tmp_strategy->next->minute * 60 +
															      tmp_strategy->next->second;

												if(start_time < start_time_next)	//没有跨天
												{
													if(start_time <= current_time && current_time <= start_time_next)	//当前时间在当前策略和下个策略的时间段区间内
													{
														ctrl->lock = 0;				//解锁远程控制
													}
												}
												else if(start_time > start_time_next)//跨天
												{
													if(start_time <= current_time && current_time <= gate24)
													{
														ctrl->lock = 0;				//解锁远程控制
													}
													else if(gate0 <= current_time && current_time <= start_time_next)
													{
														ctrl->lock = 0;				//解锁远程控制
													}
												}
											}
											else	//下个策略的年月日没有全部匹配成功
											{
												if(start_time <= current_time)	//当前时间大于等于起始执行时间
												{
													ctrl->lock = 0;				//解锁远程控制
												}
											}
										}
										else	//下个策略是相对时间方式
										{
											if(start_time <= current_time)		//当前时间大于等于起始执行时间
											{
												ctrl->lock = 0;					//解锁远程控制
											}
										}
									}
									else	//当前策略是最后一个策略,没有下个策略
									{
										if(start_time <= current_time)			//当前时间大于等于起始执行时间
										{
											ctrl->lock = 0;						//解锁远程控制
										}
									}
								}
							}

							tmp_strategy->state = EXECUTING;	//下个状态将切换为 正在执行状态

							if(tmp_strategy->state == EXECUTING)	//此判断无实际作用,置位消除编译器警告
							{
								goto GET_OUT;
							}
						break;

						case EXECUTING:			//正在执行状态
							if(y_m_d_effective & 0x04 != (u32)0x00000000 &&
							   tmp_strategy->year != calendar.w_year - 2000)
							{
								y_m_d_matched &= ~(0x04);
							}

							if(y_m_d_effective & 0x02 != (u32)0x00000000 &&
							   tmp_strategy->month != calendar.w_month)
							{
								y_m_d_matched &= ~(0x02);
							}

							if(y_m_d_effective & 0x01 != (u32)0x00000000 &&
							   tmp_strategy->date != calendar.w_date)
							{
								y_m_d_matched &= ~(0x01);
							}

							if(y_m_d_matched == 0x07)	//年月日全部匹配成功
							{
								current_time = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;

								if(tmp_strategy->next != NULL)			//下个策略有效
								{
									if(tmp_strategy->next->mode == 1)	//下个策略也是绝对时间方式
									{
										if(y_m_d_effective_next & 0x04 != (u32)0x00000000 &&
										   tmp_strategy->next->year != calendar.w_year - 2000)
										{
											y_m_d_matched_next &= ~(0x04);
										}

										if(y_m_d_effective_next & 0x02 != (u32)0x00000000 &&
										   tmp_strategy->next->month != calendar.w_month)
										{
											y_m_d_matched_next &= ~(0x02);
										}

										if(y_m_d_effective_next & 0x01 != (u32)0x00000000 &&
										   tmp_strategy->next->date != calendar.w_date)
										{
											y_m_d_matched_next &= ~(0x01);
										}

										if(y_m_d_matched_next == 0x07)	//下个策略的年月日全部匹配成功
										{
											start_time_next = tmp_strategy->next->hour * 3600 +
						                                      tmp_strategy->next->minute * 60 +
						                                      tmp_strategy->next->second;

											if(start_time < start_time_next)	//没有跨天
											{
												if(start_time <= current_time && current_time <= start_time_next)	//当前时间在当前策略和下个策略的时间段区间内
												{
													if(start_time == current_time)
													{
														if(ctrl->lock == 1)				//判断是否需要解开远程控制所
														{
															ctrl->lock = 0;
														}
													}

													if(ctrl->lock == 0)		//远程控制已解锁
													{
														ctrl->control_type = tmp_strategy->type;
														ctrl->brightness = tmp_strategy->brightness;
													}

													if(current_time >= start_time_next)		//当前策略即将执行结束
													{
														tmp_strategy->state = EXECUTED;		//下个状态将切换为 已过期状态
													}

													ret = 1;

													goto GET_OUT;
												}
												else if(current_time > start_time_next)		//下个策略已过期
												{
													tmp_strategy->state = EXECUTED;			//当前策略已过期
												}
												else if(current_time < start_time)
												{
													goto GET_OUT;							//等待当前策略生效
												}
											}
											else if(start_time > start_time_next)			//跨天
											{
												if(start_time <= current_time && current_time <= gate24)
												{
													if(start_time == current_time)
													{
														if(ctrl->lock == 1)				//判断是否需要解开远程控制所
														{
															ctrl->lock = 0;
														}
													}

													if(ctrl->lock == 0)		//远程控制已解锁
													{
														ctrl->control_type = tmp_strategy->type;
														ctrl->brightness = tmp_strategy->brightness;
													}

//													if(current_time >= start_time_next)		//当前策略即将执行结束
//													{
//														tmp_strategy->state = EXECUTED;		//下个状态将切换为 已过期状态
//													}

													ret = 1;

													goto GET_OUT;
												}
												else if(gate0 <= current_time && current_time <= start_time_next)
												{
													if(gate0 == current_time)
													{
														if(ctrl->lock == 1)				//判断是否需要解开远程控制所
														{
															ctrl->lock = 0;
														}
													}

													if(ctrl->lock == 0)		//远程控制已解锁
													{
														ctrl->control_type = tmp_strategy->type;
														ctrl->brightness = tmp_strategy->brightness;
													}

													if(current_time >= start_time_next)		//当前策略即将执行结束
													{
														tmp_strategy->state = EXECUTED;		//下个状态将切换为 已过期状态
													}

													ret = 1;

													goto GET_OUT;
												}
												else if(current_time > start_time_next && current_time < start_time)		//下个策略已过期
												{
													tmp_strategy->state = RES_EXECUTE;			//当前策略需预留执行

//													tmp_strategy = tmp_strategy->next;
//													goto FOR_LOOP;
												}
												else if(current_time < start_time)
												{
													goto GET_OUT;							//等待当前策略生效
												}
											}
										}
										else	//下个策略的年月日没有全部匹配成功
										{
											start_time = tmp_strategy->hour * 3600 +
						                                 tmp_strategy->minute * 60 +
						                                 tmp_strategy->second;

											if(start_time <= current_time)	//当前时间大于等于起始执行时间
											{
												if(start_time == current_time)
												{
													if(ctrl->lock == 1)				//判断是否需要解开远程控制所
													{
														ctrl->lock = 0;
													}
												}

												if(ctrl->lock == 0)			//远程控制已解锁
												{
													ctrl->control_type = tmp_strategy->type;
													ctrl->brightness = tmp_strategy->brightness;
												}

												ret = 1;

												goto GET_OUT;
											}
										}
									}
									else	//下个策略是相对时间方式
									{
										start_time = tmp_strategy->hour * 3600 +
						                             tmp_strategy->minute * 60 +
						                             tmp_strategy->second;

										if(start_time <= current_time)	//当前时间大于等于起始执行时间
										{
											if(start_time == current_time)
											{
												if(ctrl->lock == 1)				//判断是否需要解开远程控制所
												{
													ctrl->lock = 0;
												}
											}

											if(ctrl->lock == 0)			//远程控制已解锁
											{
												ctrl->control_type = tmp_strategy->type;
												ctrl->brightness = tmp_strategy->brightness;
											}

											ret = 1;

											goto GET_OUT;
										}
									}
								}
								else	//当前策略是最后一个策略,没有下个策略
								{
									start_time = tmp_strategy->hour * 3600 +
						                         tmp_strategy->minute * 60 +
						                         tmp_strategy->second;

									if(start_time <= current_time)	//当前时间大于等于起始执行时间
									{
										if(start_time == current_time)
										{
											if(ctrl->lock == 1)				//判断是否需要解开远程控制所
											{
												ctrl->lock = 0;
											}
										}

										if(ctrl->lock == 0)			//远程控制已解锁
										{
											ctrl->control_type = tmp_strategy->type;
											ctrl->brightness = tmp_strategy->brightness;
										}

										ret = 1;

										goto GET_OUT;
									}
								}
							}
						break;

						case EXECUTED:			//已过期状态

						break;

						default:				//未知状态
//							ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
						break;
					}
				break;

				default:	//未知方式
//					ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
				break;
			}
		}
	}
	else	//策略列表无效 为空
	{
//		ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
	}

	GET_OUT:
	xSemaphoreGive(xMutex_STRATEGY);

	return ret;
}































