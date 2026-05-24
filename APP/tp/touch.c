#include <stdio.h>
#include <string.h>
#include "touch.h"
#include "tftlcd.h"
#include "stdlib.h"
#include "math.h"
#include "spi.h"
#include "gpio.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////

//触摸屏驱动（支持ADS7843/7846/UH7843/7846/XPT2046/TSC2046/OTT2001A等） 代码
//STM32F4工程模板-库函数版本
//淘宝店铺：/genbotter.com
//********************************************************************************
//修改说明
//V1.1 20140721
//修正MDK在-O2优化时,触摸屏数据无法读取的bug.在TP_Write_Byte函数添加一个延时,解决问题.
//////////////////////////////////////////////////////////////////////////////////

_m_tp_dev tp_dev=
        {
                TP_Init,
                TP_Scan,
                TP_Adjust,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,

        };
//默认为touchtype=0的数据.
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;

//SPI写数据
//向触摸屏IC写入1byte数据
//num:要写入的数据
void TP_Write_Byte(uint8_t num)
{
    uint8_t count=0;
    HAL_SPI_Transmit(&hspi1,&num,1,100);
//	for(count=0;count<8;count++)
//	{
//		if(num&0x80)TD IN=1;
//		else TDIN=0;
//		num<<=1;
//		TCLK=0;
//		delay_us(1);
//		HAL_Delay(1);
//		TCLK=1;		//上升沿有效
//	}
}
//SPI读数据
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据
uint16_t TP_Read_AD(uint8_t CMD)
{
//	uint8_t count=0;
    uint16_t Num=0;
    uint8_t data[2];
    uint8_t trans[2]={0};
//	TCLK=0;		//先拉低时钟
//	TDIN=0; 	//拉低数据线
//	TCS=0; 		//选中触摸屏IC
    HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,0);
    TP_Write_Byte(CMD);//发送命令字
//HAL_Delay(1);//ADS7846的转换时间最长为6us
//	TCLK=0;
//	delay_us(1);
//	TCLK=1;		//给1个时钟，清除BUSY
//	delay_us(1);
//	TCLK=0;
    HAL_SPI_TransmitReceive(&hspi1,trans,data,2,100);
    //HAL_SPI_Receive(&hspi1,data,2,100);
//	for(count=0;count<16;count++)//读出16位数据,只有高12位有效
//	{
//		Num<<=1;
// 		if(data[count]==1)Num++;
//	}
    Num=data[0]<<8|data[1];
    Num>>=4;   	//只有高12位有效.
    HAL_GPIO_WritePin(CS_GPIO_Port,CS_Pin,GPIO_PIN_SET);
    //TCS=1;		//释放片选
    return(Num);
}
//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 	//读取次数
#define LOST_VAL 1	  	//丢弃值
uint16_t TP_Read_XOY(uint8_t xy)
{
    uint16_t i, j;
    uint16_t buf[READ_TIMES];
    uint16_t sum=0;
    uint16_t temp;
    for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);
    for(i=0;i<READ_TIMES-1; i++)//排序
    {
        for(j=i+1;j<READ_TIMES;j++)
        {
            if(buf[i]>buf[j])//升序排列
            {
                temp=buf[i];
                buf[i]=buf[j];
                buf[j]=temp;
            }
        }
    }
    sum=0;
    for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
    temp=sum/(READ_TIMES-2*LOST_VAL);
    return temp;
}
//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
uint8_t TP_Read_XY(uint16_t *x,uint16_t *y)
{
    uint16_t xtemp,ytemp;
    xtemp=TP_Read_XOY(CMD_RDX);
    ytemp=TP_Read_XOY(CMD_RDY);
    //if(xtemp<100||ytemp<100)return 0;//读数失败
    *x=xtemp;
    *y=ytemp;
    return 1;//读数成功
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围
uint8_t TP_Read_XY2(uint16_t *x,uint16_t *y)
{
    uint16_t x1,y1;
    uint16_t x2,y2;
    uint8_t flag;
    flag=TP_Read_XY(&x1,&y1);
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);
    if(flag==0)return(0);
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
       &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;
}
//////////////////////////////////////////////////////////////////////////////////
//与LCD部分有关的函数
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(uint16_t x,uint16_t y,uint16_t color)
{
    FRONT_COLOR=color;
    LCD_DrawLine(x-12,y,x+13,y);//横线
    LCD_DrawLine(x,y-12,x,y+13);//竖线
    LCD_DrawPoint(x+1,y+1);
    LCD_DrawPoint(x-1,y+1);
    LCD_DrawPoint(x+1,y-1);
    LCD_DrawPoint(x-1,y-1);
    LCD_Draw_Circle(x,y,6);//画中心圈
}
//画一个大点(2*2的点)
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(uint16_t x,uint16_t y,uint16_t color)
{
    FRONT_COLOR=color;
    LCD_DrawPoint(x,y);//中心点
    LCD_DrawPoint(x+1,y);
    LCD_DrawPoint(x,y+1);
    LCD_DrawPoint(x+1,y+1);
}
//////////////////////////////////////////////////////////////////////////////////
//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
uint8_t TP_Scan(uint8_t tp)
{
    uint8_t res = 0;

    if(HAL_GPIO_ReadPin(PEN_GPIO_Port, PEN_Pin) == GPIO_PIN_RESET)
    {
        if(tp)
        {
            res = TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0]);
        }
        else
        {
            res = TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0]);
            if(res)
            {
                tp_dev.x[0] = tp_dev.xfac * tp_dev.x[0] + tp_dev.xoff;
                tp_dev.y[0] = tp_dev.yfac * tp_dev.y[0] + tp_dev.yoff;

                // 320×480 边界限制
                if(tp_dev.x[0] >= 320) tp_dev.x[0] = 319;
                if(tp_dev.y[0] >= 480) tp_dev.y[0] = 479;
            }
        }

        if(res)
        {
            if((tp_dev.sta & TP_PRES_DOWN) == 0)
            {
                tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;
                tp_dev.x[4] = tp_dev.x[0];
                tp_dev.y[4] = tp_dev.y[0];
            }
        }
    }
    else
    {
        if(tp_dev.sta & TP_PRES_DOWN)
        {
            tp_dev.sta &= ~(1<<7);
        }
        else
        {
            tp_dev.x[4] = 0;
            tp_dev.y[4] = 0;
            tp_dev.x[0] = 0xFFFF;
            tp_dev.y[0] = 0xFFFF;
        }
    }

    return tp_dev.sta & TP_PRES_DOWN;
}

//////////////////////////////////////////////////////////////////////////
//保存在EEPROM里面的地址区间基址,占用13个字节(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+12)
#define SAVE_ADDR_BASE 40
//保存校准参数
//void TP_Save_Adjdata(void)
//{
//	int32_t temp;
//	//保存校正结果!
//	temp=tp_dev.xfac*100000000;//保存x校正因素
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE,temp,4);
//	temp=tp_dev.yfac*100000000;//保存y校正因素
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE+4,temp,4);
//	//保存x偏移量
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE+8,tp_dev.xoff,2);
//	//保存y偏移量
//	AT24CXX_WriteLenByte(SAVE_ADDR_BASE+10,tp_dev.yoff,2);
//	//保存触屏类型
//	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+12,tp_dev.touchtype);
//	temp=0X0A;//标记校准过了
//	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+13,temp);
//}
//得到保存在EEPROM里面的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
//uint8_t TP_Get_Adjdata(void)
//{
//	int32_t tempfac;
//	//tempfac=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+13);//读取标记字,看是否校准过！
//	if(tempfac==0X0A)//触摸屏已经校准过了
//	{
//		tempfac=AT24CXX_ReadLenByte(SAVE_ADDR_BASE,4);
//		tp_dev.xfac=(float)tempfac/100000000;//得到x校准参数
//		tempfac=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+4,4);
//		tp_dev.yfac=(float)tempfac/100000000;//得到y校准参数
//	    //得到x偏移量
//		tp_dev.xoff=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+8,2);
// 	    //得到y偏移量
//		tp_dev.yoff=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+10,2);
// 		tp_dev.touchtype=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+12);//读取触屏类型标记
//		if(tp_dev.touchtype)//X,Y方向与屏幕相反
//		{
//			CMD_RDX=0X90;
//			CMD_RDY=0XD0;
//		}else				   //X,Y方向与屏幕相同
//		{
//			CMD_RDX=0XD0;
//			CMD_RDY=0X90;
//		}
//		return 1;
//	}
//	return 0;
//}
//提示字符串
uint8_t* const TP_REMIND_MSG_TBL=(uint8_t *)"Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";

//提示校准结果(各个参数)
void TP_Adj_Info_Show(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t x3,uint16_t y3,uint16_t fac)
{
    FRONT_COLOR=RED;
    LCD_ShowString(40,160,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"x1:");
    LCD_ShowString(40+80,160,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"y1:");
    LCD_ShowString(40,180,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"x2:");
    LCD_ShowString(40+80,180,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"y2:");
    LCD_ShowString(40,200,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"x3:");
    LCD_ShowString(40+80,200,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"y3:");
    LCD_ShowString(40,220,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"x4:");
    LCD_ShowString(40+80,220,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"y4:");
    LCD_ShowString(40,240,tftlcd_data.width,tftlcd_data.height,16,(uint8_t *)"fac is:");
    LCD_ShowNum(40+24,160,x0,4,16);		//显示数值
    LCD_ShowNum(40+24+80,160,y0,4,16);	//显示数值
    LCD_ShowNum(40+24,180,x1,4,16);		//显示数值
    LCD_ShowNum(40+24+80,180,y1,4,16);	//显示数值
    LCD_ShowNum(40+24,200,x2,4,16);		//显示数值
    LCD_ShowNum(40+24+80,200,y2,4,16);	//显示数值
    LCD_ShowNum(40+24,220,x3,4,16);		//显示数值
    LCD_ShowNum(40+24+80,220,y3,4,16);	//显示数值
    LCD_ShowNum(40+56,240,fac,3,16); 	//显示数值,该数值必须在95~105范围之内.

}

//触摸屏校准代码
//得到四个校准参数
void TP_Adjust(void)
{
    uint16_t pos_temp[4][2];//坐标缓存值
    uint8_t  cnt=0;
    uint16_t d1,d2;
    uint32_t tem1,tem2;
    double fac;
    uint16_t outtime=0;
    cnt=0;
    FRONT_COLOR=BLUE;
    BACK_COLOR =WHITE;
    LCD_Clear(WHITE);//清屏
    FRONT_COLOR=RED;//红色
    LCD_Clear(WHITE);//清屏
    FRONT_COLOR=BLACK;
    LCD_ShowString(40,40,160,100,16,(uint8_t*)TP_REMIND_MSG_TBL);//显示提示信息
    TP_Drow_Touch_Point(20,20,RED);//画点1
    tp_dev.sta=0;//消除触发信号
    tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误
    while(1)//如果连续10秒钟没有按下,则自动退出
    {
        tp_dev.scan(1);//扫描物理坐标
        if((tp_dev.sta&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
        {
            //outtime=0;
            tp_dev.sta&=~(1<<6);//标记按键已经被处理过了.

            pos_temp[cnt][0]=tp_dev.x[0];
            pos_temp[cnt][1]=tp_dev.y[0];
            cnt++;
            switch(cnt)
            {
                case 1:
                    TP_Drow_Touch_Point(20,20,WHITE);				//清除点1
                    TP_Drow_Touch_Point(tftlcd_data.width-20,20,RED);	//画点2
                    break;
                case 2:
                    TP_Drow_Touch_Point(tftlcd_data.width-20,20,WHITE);	//清除点2
                    TP_Drow_Touch_Point(20,tftlcd_data.height-20,RED);	//画点3
                    break;
                case 3:
                    TP_Drow_Touch_Point(20,tftlcd_data.height-20,WHITE);			//清除点3
                    TP_Drow_Touch_Point(tftlcd_data.width-20,tftlcd_data.height-20,RED);	//画点4
                    break;
                case 4:	 //全部四个点已经得到
                    //对边相等
                    tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
                    tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
                    tem1*=tem1;
                    tem2*=tem2;
                    d1=sqrt(tem1+tem2);//得到1,2的距离

                    tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
                    tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
                    tem1*=tem1;
                    tem2*=tem2;
                    d2=sqrt(tem1+tem2);//得到3,4的距离
                    fac=(float)d1/d2;
                    if(fac<0.95||fac>1.05||d1==0||d2==0)//不合格
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(tftlcd_data.width-20,tftlcd_data.height-20,WHITE);	//清除点4
                        TP_Drow_Touch_Point(20,20,RED);								//画点1
                        TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据
                        continue;
                    }
                    tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
                    tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
                    tem1*=tem1;
                    tem2*=tem2;
                    d1=sqrt(tem1+tem2);//得到1,3的距离

                    tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
                    tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
                    tem1*=tem1;
                    tem2*=tem2;
                    d2=sqrt(tem1+tem2);//得到2,4的距离
                    fac=(float)d1/d2;
                    if(fac<0.95||fac>1.05)//不合格
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(tftlcd_data.width-20,tftlcd_data.height-20,WHITE);	//清除点4
                        TP_Drow_Touch_Point(20,20,RED);								//画点1
                        TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据
                        continue;
                    }//正确了

                    //对角线相等
                    tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
                    tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
                    tem1*=tem1;
                    tem2*=tem2;
                    d1=sqrt(tem1+tem2);//得到1,4的距离

                    tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
                    tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
                    tem1*=tem1;
                    tem2*=tem2;
                    d2=sqrt(tem1+tem2);//得到2,3的距离
                    fac=(float)d1/d2;
                    if(fac<0.95||fac>1.05)//不合格
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(tftlcd_data.width-20,tftlcd_data.height-20,WHITE);	//清除点4
                        TP_Drow_Touch_Point(20,20,RED);								//画点1
                        TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据
                        continue;
                    }//正确了
                    //计算结果
                    tp_dev.xfac=(float)(tftlcd_data.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac
                    tp_dev.xoff=(tftlcd_data.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff

                    tp_dev.yfac=(float)(tftlcd_data.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
                    tp_dev.yoff=(tftlcd_data.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff
                    if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
                    {
                        cnt=0;
                        TP_Drow_Touch_Point(tftlcd_data.width-20,tftlcd_data.height-20,WHITE);	//清除点4
                        TP_Drow_Touch_Point(20,20,RED);								//画点1
                        LCD_ShowString(40,26,tftlcd_data.width,tftlcd_data.height,16,"TP Need readjust!");
                        tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
                        if(tp_dev.touchtype)//X,Y方向与屏幕相反
                        {
                            CMD_RDX=0X90;
                            CMD_RDY=0XD0;
                        }else				   //X,Y方向与屏幕相同
                        {
                            CMD_RDX=0XD0;
                            CMD_RDY=0X90;
                        }
                        continue;
                    }
                    FRONT_COLOR=BLUE;
                    LCD_Clear(WHITE);//清屏
                    LCD_ShowString(35,110,tftlcd_data.width,tftlcd_data.height,16,"Touch Screen Adjust OK!");//校正完成


                    HAL_Delay(1000);
                    //TP_Save_Adjdata();
                    LCD_Clear(WHITE);//清屏
                    return;//校正完成
            }
        }
        HAL_Delay(10);
//	outtime++;
//	if(outtime>1000)
//	{
//		//TP_Get_Adjdata();
//		break;
//	}
    }
}
//触摸屏初始化
//返回值:0,没有进行校准
//       1,进行过校准
uint8_t TP_Init(void)
{
    static uint8_t inited = 0;
    if(!inited)
    {
        // 确保 hspi1 已初始化（由 CubeMX 生成）
        // 如有需要，在这里添加 PEN 引脚上拉
        inited = 1;
    }

    // 320×480 竖屏校准参数
    tp_dev.xfac = 0.20187456f;
    tp_dev.yfac = 0.27227724f;
    tp_dev.xoff = -45;
    tp_dev.yoff = -30;
    tp_dev.touchtype = 0;
    CMD_RDX = 0xD0;
    CMD_RDY = 0x90;

    tp_dev.sta = 0;
    tp_dev.x[0] = 0xFFFF;
    tp_dev.y[0] = 0xFFFF;

    return 1;
}
//可以draw的提示
void load_draw_hint(void)
{
    LCD_Clear(WHITE);//清屏
    FRONT_COLOR=BLACK;//设置字体为蓝色
    LCD_ShowString(10,5,200,16,16, (uint8_t *)"You can draw now...");
    FRONT_COLOR=BLUE;//设置画笔蓝色
}
////////////////////////////////////////////////////////////////////////////////
//电容触摸屏专有部分
//画水平线
//x0,y0:坐标
//len:线长度
//color:颜色
void gui_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
    if(len==0)return;
    LCD_Fill(x0,y0,x0+len-1,y0,color);
}
//画实心圆
//x0,y0:坐标
//r:半径
//color:颜色
void gui_fill_circle(uint16_t x0,uint16_t y0,uint16_t r,uint16_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r*707)/1000+1;
    uint32_t sqmax = (uint32_t)r*(uint32_t)r+(uint32_t)r/2;
    uint32_t x=r;
    gui_draw_hline(x0-r,y0,2*r,color);
    for (i=1;i<=imax;i++)
    {
        if ((i*i+x*x)>sqmax)// draw lines from outside
        {
            if (x>imax)
            {
                gui_draw_hline (x0-i+1,y0+x,2*(i-1),color);
                gui_draw_hline (x0-i+1,y0-x,2*(i-1),color);
            }
            x--;
        }
        // draw lines from inside (center)
        gui_draw_hline(x0-x,y0+i,2*x,color);
        gui_draw_hline(x0-x,y0-i,2*x,color);
    }
}

//两个数之差的绝对值
//x1,x2：需取差值的两个数
//返回值：|x1-x2|
uint16_t my_abs(uint16_t x1,uint16_t x2)
{
    if(x1>x2)return x1-x2;
    else return x2-x1;
}

//画一条粗线
//(x1,y1),(x2,y2):线条的起始坐标
//size：线条的粗细程度
//color：线条的颜色
void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint8_t size,uint16_t color)
{
    uint16_t t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    if(x1<size|| x2<size||y1<size|| y2<size)return;
    delta_x=x2-x1; //计算坐标增量
    delta_y=y2-y1;
    uRow=x1;
    uCol=y1;
    if(delta_x>0)incx=1; //设置单步方向
    else if(delta_x==0)incx=0;//垂直线
    else {incx=-1;delta_x=-delta_x;}
    if(delta_y>0)incy=1;
    else if(delta_y==0)incy=0;//水平线
    else{incy=-1;delta_y=-delta_y;}
    if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
    else distance=delta_y;
    for(t=0;t<=distance+1;t++ )//画线输出
    {
        gui_fill_circle(uRow,uCol,size,color);//画点
        xerr+=delta_x ;
        yerr+=delta_y ;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////

//电阻触摸屏测试函数
void rtp_test(void)
{
    while(1)
    {
        tp_dev.scan(0);

        if(tp_dev.sta & TP_PRES_DOWN)
        {
            // 调试：串口输出坐标
            printf("Touch: x=%d, y=%d\r\n", tp_dev.x[0], tp_dev.y[0]);

            if(tp_dev.x[0] < 320 && tp_dev.y[0] < 480)  // 确保在范围内
            {
                // 清除按钮区域（右上角 60×30，更容易点到）
                if(tp_dev.x[0] > 260 && tp_dev.y[0] < 30)
                {
                    load_draw_hint();
                    HAL_Delay(300);  // 防抖
                }
                else
                {
                    TP_Draw_Big_Point(tp_dev.x[0], tp_dev.y[0], BLUE);
                }
            }
        }
        else
        {
            HAL_Delay(10);
        }
    }
}


