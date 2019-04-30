/***************************************************
具有闹钟功能的电子时钟程序设计,晶振为12M
按下自锁开关调整时间键进入实时时间设置模式
按下闹钟1定时键进入实时时间设置模式
按下闹钟2定时键进入实时时间设置模式
在设置模式中点击时分秒键循环切换设置位
在设置模式中点击加、减键更改时间数值
闹钟响铃后，任意按键退出闹钟模式
按键长按，值一直加或者减
***************************************************/
#include<reg51.h>
#include"ds1302.h"
#define uchar unsigned char
#define uint unsigned int

uchar code W_RTC_ADDR[3] = {0x80, 0x82, 0x84};
uchar code W_CLOCK1_ADDR[3] = {0xC0, 0xC2, 0xC4}; 
uchar code R_CLOCK1_ADDR[3] = {0xC1, 0xC3, 0xC5};
uchar code W_CLOCK2_ADDR[3] = {0xC6, 0xC8, 0xCA}; 
uchar code R_CLOCK2_ADDR[3] = {0xC7, 0xC9, 0xCB};


/***数码管位选控制端****/
sbit s1 = P2 ^ 0;
sbit s2 = P2 ^ 1;
sbit s3 = P2 ^ 2;
sbit s4 = P2 ^ 3;
sbit s5 = P2 ^ 4;
sbit s6 = P2 ^ 5;
sbit s7 = P2 ^ 6;
sbit s8 = P2 ^ 7;


/*********按键引脚配置*******
time_setting  	―――― 设置时间
clock1_setting 	―――― 闹钟1定时
clock2_setting 	―――― 闹钟2定时
switching      	―――― 时分秒选择
up            	―――― 数字加
down           	―――― 数字减
sounder		   	―――― 扬声器
clock_enable	————闹钟启用键
****************************/
sbit time_setting = P1 ^ 0;
sbit clock1_setting = P1 ^ 1;
sbit clock2_setting = P1 ^ 2;
sbit switching = P1 ^ 3;
sbit up = P1 ^ 4;
sbit down = P1 ^ 5;
sbit clock_enable = P1 ^ 6;
sbit sounder = P3 ^ 7;

/*********初始值设置*********/
char count, count1, sec, min, hour, b, position, setting, setting_mode = 0; 
char sec1 = 1, min1 = 1, hour1 = 1, sec2 = 1, min2 = 1, hour2 = 1; 				//初始闹钟时间为1时1分1秒
char sec_h, sec_l, min_h, min_l, hour_h, hour_l;
uchar code cour[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};			//位扫描各位编码
uchar code mum[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x40}; //共阴数码管0-9及分隔符'-'编码

/************毫秒延时函数*************/
void delayms(uint k)
{
	uint i, j;
	for (i = k; i > 0; i--)
		for (j = 110; j > 0; j--);
}

/***********时钟位更新函数************/
void timecontrol()       														//定时器中断执行秒加1程序，如果秒等于60，那就分加1，如果分等于60那就时加1，如果时累加到24就变成0
{
	sec++;
	if (sec == 60)
	{
		sec = 0;
		min++;
		if (min == 60)
		{
			min = 0;
			hour++;
			if (hour == 24)
				hour = 0;
		}
	}
}

/***********数码管显示函数************/
void display(uchar zs, uchar zm, uchar zh)
{
	uchar a;
	for (a = 0; a < 8; a++)
	{
		P2 = cour[a]; //打开数码管显示的位置
		if (setting_mode == 1 && (count1<10))
			{
				switch(position)
				{
				case 0: P2 = P2 | 0xC0; break;
				case 1: P2 = P2 | 0x18; break;
				case 2: P2 = P2 | 0x03; break;
				}
			}

		switch (a) //对应数码管位置显示对应的值
		{
		case 0: P0 = mum[zh / 10]; break; //显示时的十位
		case 1: P0 = mum[zh % 10]; break; //显示时的个位
		case 2: P0 = mum[10];	   break; //显示-分隔符
		case 3: P0 = mum[zm / 10]; break; //显示分的十位
		case 4: P0 = mum[zm % 10]; break; //显示分的个位
		case 5: P0 = mum[10];      break; //显示-分隔符
		case 6: P0 = mum[zs / 10]; break; //显示秒的十位
		case 7: P0 = mum[zs % 10]; break; //显示秒的个位
		default :;
		}
		delayms(1);//消影
		P0 = 0x00;
		P2 = 0xFF; //数码管关闭显示
	}
}

/**********实时时间设置函数***********/
void keyscan_settime()
{
	int realtime_sec, realtime_min, realtime_hour;
	if (time_setting == 0)														//如果调整时间按键按下
	{
		delayms(10);   															//防抖
		if (time_setting == 0)
		{
			realtime_sec = sec, realtime_min = min, realtime_hour = hour; 		//把时分秒赋值为初始闹钟值
			TR0 = 0;              												//关闭定时器
			setting_mode = 1;
			while (time_setting == 0)      										//等待用户按键，确定按下
			{
				display(realtime_sec, realtime_min, realtime_hour); 			//显示时分秒
				if (switching == 0)          									//switching按键是否按下
				{
					delayms(10);   												//防抖
					if (switching == 0)    										//再次确认按下
					{
						while (!switching);  									//检测松开上升沿
						position++;												//通过position选择时分秒
						if (position == 3)
							position = 0; 										//时分秒循环选择
					}
				}

				if (up == 0)             										//加数
				{
					delayms(10);												//去抖
					if (up == 0) 												//确认是否还按下
					{
						while (!up) 											//检测是否长按
						{
							b++;     											//长按计数
							display(realtime_sec, realtime_min, realtime_hour); //使在长按的时候数码管能够保持显示
							delayms(10);
							if (b >= 20)										//如果长按大于10*20=200ms
							{
								b = 0;
								while (!up) 									//如果还按下对应的时分秒往上加
								{
									switch (position)
									{
									case 0: realtime_sec++; if (realtime_sec == 60)realtime_sec = 0; break;
									case 1: realtime_min++; if (realtime_min == 60)realtime_min = 0; break;
									case 2: realtime_hour++; if (realtime_hour == 24)realtime_hour = 0; break;
									default :;
									}
									display(realtime_sec, realtime_min, realtime_hour); //加完继续显示
									delayms(10);								//延时
								}
							}
						}														//如果只是在200ms内短按就松开就对应的时分秒直接加1
						switch (position)
						{
						case 0: realtime_sec++; if (realtime_sec == 60)realtime_sec = 0; break;
						case 1: realtime_min++; if (realtime_min == 60)realtime_min = 0; break;
						case 2: realtime_hour++; if (realtime_hour == 24)realtime_hour = 0; break;
						default :;
						}
					}
				}
				if (down == 0)             										//减数同上加数
				{
					delayms(10);
					if (down == 0)
					{
						while (!down)
						{
							b++;
							display(realtime_sec, realtime_min, realtime_hour); 
							delayms(10);
							if (b >= 20)
							{
								b = 0;
								while (!down)
								{
									switch (position)
									{
									case 0: realtime_sec--; if (realtime_sec < 0)realtime_sec = 59; break;
									case 1: realtime_min--; if (realtime_min < 0)realtime_min = 59; break;
									case 2: realtime_hour--; if (realtime_hour < 0)realtime_hour = 23; break;
									default : ;
									}
									display(realtime_sec, realtime_min, realtime_hour);
									delayms(10);
								}
							}
						}
						switch (position)
						{
						case 0: realtime_sec--; if (realtime_sec < 0)realtime_sec = 59; break;
						case 1: realtime_min--; if (realtime_min < 0)realtime_min = 59; break;
						case 2: realtime_hour--; if (realtime_hour < 0)realtime_hour = 23; break;
						default : ;
						}
					}
				}
				b = 0;
			}
			setting_mode = 0;
			TR0 = 1;
			sec = realtime_sec, min = realtime_min, hour = realtime_hour;

			/***更新DS1302内存储的实时时间***/
			Ds1302Write(0x8E,0X00);		 										//关闭写保护功能
			Ds1302Write(W_RTC_ADDR[0],sec / 10*16+ sec % 10);					//写入时间时将十进制数调整为BCD码写入
			Ds1302Write(W_RTC_ADDR[1],min / 10*16+ min % 10);				
			Ds1302Write(W_RTC_ADDR[2],hour /10*16+ hour % 10);	
			Ds1302Write(0x8E,0x80);		 										//打开写保护功能

		}
	}
	display(sec, min, hour);
}

/********闹钟1及闹钟2设置函数*********/
void keyscan_clock()
{
	if (clock1_setting == 0||clock2_setting == 0)								//闹钟设置键按下
	{
		delayms(10);
		if (clock1_setting == 0||clock2_setting == 0)							//闹钟设置键消抖
		{
			if (clock1_setting == 0)											//设置闹钟1或闹钟2标志位
				setting = 0;
			else
				setting = 1;
			setting_mode = 1;
			while (clock1_setting == 0||clock2_setting == 0)      				//开始定时设置
			{
				if (setting == 0)
					display(sec1, min1, hour1);
				else
					display(sec2, min2, hour2);									//长按时保持显示
				if (switching == 0)												//切换键被按下
				{
					delayms(10);
					if (switching == 0)											//切换键消抖
					{
						while (!switching); 									//当切换键松开上升沿时
						position++;
						if (position == 3)
						    position = 0;
					}
				}

				if (up == 0)             										//加数
				{
					delayms(10);
					if (up == 0)
					{
						while (!up) 											//检测是否长按
						{
							b++;     											//长按计数
							if (setting == 0)
								display(sec1, min1, hour1);
							else
								display(sec2, min2, hour2); 					//使数码管在长按的时候能够保持显示
							delayms(10);
							if (b >= 20) 										//如果长按大于10*20=200ms
							{
								b = 0;
								while (!up) 									//如果仍为按下状态，通过标志位运算设置相应的闹钟变量对应的时分秒往上加
								{
									switch (position+3*setting)
									{
									case 0: sec1++; if (sec1 == 60)sec1 = 0; break;
									case 1: min1++; if (min1 == 60)min1 = 0; break;
									case 2: hour1++; if (hour1 == 24)hour1 = 0; break;
									case 3: sec2++; if (sec2 == 60)sec2 = 0; break;
									case 4: min2++; if (min2 == 60)min2 = 0; break;
									case 5: hour2++; if (hour2 == 24)hour2 = 0; break;
									default :;
									}
									if (setting == 0)
										display(sec1, min1, hour1);
									else
										display(sec2, min2, hour2);
									delayms(10);								//延时
								}
							}
						}														//如果只是在200ms内短按就松开就对应的时分秒直接加1
						switch (position+3*setting)
						{
						case 0: sec1++; if (sec1 == 60)sec1 = 0; break;
						case 1: min1++; if (min1 == 60)min1 = 0; break;
						case 2: hour1++; if (hour1 == 24)hour1 = 0; break;
						case 3: sec2++; if (sec2 == 60)sec2 = 0; break;
						case 4: min2++; if (min2 == 60)min2 = 0; break;
						case 5: hour2++; if (hour2 == 24)hour2 = 0; break;
						default :;
						}
					}
				}
				if (down == 0)             										//减数跟加数一样
				{
					delayms(10);
					if (down == 0)
					{
						while (!down)
						{
							b++;
							if (setting == 0)
								display(sec1, min1, hour1);
							else
								display(sec2, min2, hour2); 					//加这个显示只是为了在长按的时候能够一直显示
							delayms(10);
							if (b >= 20)
							{
								b = 0;
								while (!down)
								{
									switch (position+3*setting)
									{
									case 0: sec1--; if (sec1 < 0)sec1 = 59; break;
									case 1: min1--; if (min1 < 0)min1 = 59; break;
									case 2: hour1--; if (hour1 < 0)hour1 = 23; break;
									case 3: sec2--; if (sec2 < 0)sec2 = 59; break;
									case 4: min2--; if (min2 < 0)min2 = 59; break;
									case 5: hour2--; if (hour2 < 0)hour2 = 23; break;
									default :;
									}
									if (setting == 0)
										display(sec1, min1, hour1);
									else
										display(sec2, min2, hour2);
									delayms(10);
								}
							}
						}
						switch (position+3*setting)
						{
						case 0: sec1--; if (sec1 < 0)sec1 = 59; break;
						case 1: min1--; if (min1 < 0)min1 = 59; break;
						case 2: hour1--; if (hour1 < 0)hour1 = 23; break;
						case 3: sec2--; if (sec2 < 0)sec2 = 59; break;
						case 4: min2--; if (min2 < 0)min2 = 59; break;
						case 5: hour2--; if (hour2 < 0)hour2 = 23; break;
						default :;
						}
					}
				}
				b = 0;
				if (setting == 0)
					display(sec1, min1, hour1);
				else
					display(sec2, min2, hour2);

				/*****将闹钟设置值更新至Ds1302的RAM中*****/
				Ds1302Write(0x8E,0X00);		 										//关闭写保护功能
				Ds1302Write(W_CLOCK1_ADDR[0], sec1);	
				Ds1302Write(W_CLOCK1_ADDR[1], min1);	
				Ds1302Write(W_CLOCK1_ADDR[2], hour1);
				Ds1302Write(W_CLOCK2_ADDR[0], sec2);	
				Ds1302Write(W_CLOCK2_ADDR[1], min2);			
				Ds1302Write(W_CLOCK2_ADDR[2], hour2);
				Ds1302Write(0x8E,0x80);		 										//打开写保护功能

			}
			setting_mode = 0;
		}
	}
}

/*********闹钟检测及报警函数*********/
void buz()					
{
	if ((((hour == hour1) && (min == min1) && (sec == sec1)) || ((hour == hour2) && (min == min2) && (sec == sec2))) && (clock_enable == 0) //时分秒与设置的闹钟一致，开始响铃
	{
		char temp_hour, temp_min, temp_sec;
		temp_hour = hour;
		temp_min = min;
		temp_sec = sec;
		while ((time_setting && clock1_setting && switching && up && down) == 1) 									//任意按键按下退出闹钟模式,不按下按键就一直显示时间，闹铃
		{
			sounder = !sounder;       																				//打开蜂鸣器
			//delayms(1);
			display(temp_sec, temp_min, temp_hour); 																//显示闹钟时间
			//delayms(1);
			P0 = 0x00; 																								//关闭显示，制作闹铃闪烁
		}

	}
}

/*********重载函数*********/
void ReloadTime(){
	Ds1302ReadTime();
	hour_h = TIME[2]/16;			//时
	hour_l = TIME[2]&0x0f;				 
	hour = hour_h * 10 + hour_l;
	min_h = TIME[1]/16;				//分
	min_l = TIME[1]&0x0f;
	min = min_h * 10 + min_l;
	sec_h = TIME[0]/16;				//秒
	sec_l = TIME[0]&0x0f;
	sec = sec_h * 10 + sec_l;
	sec1 = Ds1302Read(R_CLOCK1_ADDR[0]);	//闹钟1秒位
	min1 = Ds1302Read(R_CLOCK1_ADDR[1]);	//闹钟1分位
	hour1 = Ds1302Read(R_CLOCK1_ADDR[2]);	//闹钟1时位
	sec2 = Ds1302Read(R_CLOCK2_ADDR[0]);	//闹钟2秒位
	min2 = Ds1302Read(R_CLOCK2_ADDR[1]);	//闹钟2分位
	hour2 = Ds1302Read(R_CLOCK2_ADDR[2]);	//闹钟2时位
}


/*********主函数*********/
void main()
{
	TMOD = 0x11;                    //定时器工作在方式1
	TH0 = (65536 - 50000) / 256;    //定时器0装初值，50ms
	TL0 = (65536 - 50000) % 256;
	TH1 = (65536 - 50000) / 256;    //定时器1装初值，50ms
	TL1 = (65536 - 50000) % 256;
	EA = 1;                			//总中断允许
	ET0 = 1;               			//T0中断允许
	TR0 = 1;              			//开定时器0
	ET1 = 1;               			//T1中断允许
	TR1 = 1;              			//开定时器1
	//Ds1302Init();		  			//初始化DS1302（备用电池断电后使用，清除Ds1302中数据）
	ReloadTime();					//将断电前Ds1302中存储的数据重载入单片机
	while (1)						//计时扫描
	{
		keyscan_settime();			//按键扫描
		keyscan_clock();    		//定时扫描
		buz();       				//蜂鸣器报警
	}
}


void T0_ms() interrupt 1          	//实时时钟计时器
{
	TH0 = (65536 - 50000) / 256;    //重新装初值
	TL0 = (65536 - 50000) % 256;
	count++;
	if (count == 20)              	//判定1s时间是否已到
	{
		count = 0;
		timecontrol();   			//时分秒计数
	}
}

void T1_ms() interrupt 3          	//闪烁计时器
{
	TH1 = (65536 - 50000) / 256;    //重新装初值
	TL1 = (65536 - 50000) % 256;
	count1++;
	if (count1 == 20)              	//判定1s时间是否已到
		count1 = 0;
}
