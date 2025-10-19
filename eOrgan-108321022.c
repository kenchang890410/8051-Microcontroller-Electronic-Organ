#include <8051.h>
#include "Keypad4x4.h"
#define TIMER_VAL 3036

void led_display(int dig, char num) {
	unsigned display_table[19]=
	{0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0x40,0x79,0x30,0x19,0x12
	,0x89,0xc7,0xff,0x8b,0x8c,0x88,0x91};
	//顯示燈圖案的table 順序對應:0,1,2,3,4,5,6,0.,1.,3.,4.,5.,H,L,空,N,P,A,Y 
	P2 = display_table[num];//套上相應圖案 
	P1_1=!(((1<<dig)>>3)&1);//左移再右移找到當前該亮的燈並賦予0，其他位數賦予1，輪替 
	P1_2=!(((1<<dig)>>2)&1);
	P1_3=!(((1<<dig)>>1)&1);
	P1_4=!((1<<dig)&1);
}
 
unsigned short led_data = 0xee0f,press_number, temp_led_data=0,start_record_count,play_state=0,play_one_beat=0,play_song=0;
unsigned short i,current_tone,recording=0,played_beat=0,start_record=0,all_beat=0,press_time=0;
unsigned char digit[4],d1, d2,scale=0,flag = 0,reverse=0;
/*
led_data,press_number,temp_led_data:存當前按的字給LED 
start_record_count:用於紀錄播音樂順序 
play_state:撥放狀態但不等於播音樂data模式
play_one_beat:播當前按的一個interrupt
play_song:播音樂data模式
current_tone:記錄當前按的音符
start_record、recording:開始錄音模式
all_beat、 played_beat:記開始播到結束
press_time:記按的時間 
flag: debounce
scale:記升降階
i,d1,d2:delay
reverse:正著播、反著播模式 
*/ 
const unsigned short tone[] =//聲音頻率table 
{ 
	65536-1000000/(2*262),  //do 0
	65536-1000000/(2*294),  //re 1
	65536-1000000/(2*330),  //mi 2
	65536-1000000/(2*349),  //fa 3 
	65536-1000000/(2*392),  //so 4
	65536-1000000/(2*440),  //la 5
	65536-1000000/(2*494),  //si 6
	
	65536-1000000/(2*524),  //do" 7
	65536-1000000/(2*588),  //re" 8
	65536-1000000/(2*660),  //mi" 9
	65536-1000000/(2*698),  //fa" 10
	65536-1000000/(2*784),  //so" 11
	65536-1000000/(2*880),  //la" 12
	65536-1000000/(2*988),  //si" 13
	
	65536-1000000/(2*1048), //do"" 14
	65536-1000000/(2*1176), //re"" 15
	65536-1000000/(2*1320), //mi"" 16
	65536-1000000/(2*1396), //fa"" 17
	65536-1000000/(2*1568), //so"" 18
	65536-1000000/(2*1760), //la"" 19
	65536-1000000/(2*1976), //si"" 20
	
	65536-1000000/(2*277),	//#do 21
	65536-1000000/(2*311),  //#re 22
	65536-1000000/(2*370),  //#fa 23
	65536-1000000/(2*415),  //#so 24
	65536-1000000/(2*466),  //#la 25
	0,//26
	0,//27
	65536-1000000/(2*554),  //#do" 28
	65536-1000000/(2*622),  //#re" 29
	65536-1000000/(2*740),  //#fa" 30
	65536-1000000/(2*830),  //#so" 31
	65536-1000000/(2*932),  //#la" 32
	0,//33
	0,//34
	65536-1000000/(2*1108),  //#do"" 35
	65536-1000000/(2*1244),  //#re"" 36
	65536-1000000/(2*1480),  //#fa"" 37
	65536-1000000/(2*1660),  //#so"" 38
	65536-1000000/(2*1864),  //#la"" 39
};

__xdata char song[1000]={26};//設26為空，將存音符的array清空 

void main() 
{
	TH1 = TIMER_VAL >> 8;//初始化interrupt~
	TL1 = TIMER_VAL & 0xff;
	TH0 = tone[0] >> 8;
	TL0 = tone[0] & 0xff;
	TMOD = 0x11;
	TCON = 0x50;
	IE = 0x8a;//~初始化interrupt
	P0 = 0xef;//keypad 設定 
	while (1) 
	{
		if(play_song==0)//當再錄音時把按的音符顯示並依序左移顯示 
		{
			digit[0] = led_data >> 12;//最高位(左一)
			digit[1] = (led_data << 4) >> 12;//(左2)
			digit[2] = (led_data << 8) >> 12;//(右2)
			digit[3] = (led_data << 12) >> 12;//(右1)
		}
		else if(reverse==0)//當在倒著播音樂data時LED顯示YALP 
		{	
			digit[0]=0x10;
			digit[1]=0x0d;
			digit[2]=0x11;
			digit[3]=0x12;
		} 
		else//當在正向著播音樂data時LED顯示PLAY 
		{	
			digit[0]=0x12;
			digit[1]=0x11;
			digit[2]=0x0d;
			digit[3]=0x10;
		} 
		for (i = 0; i < 4; i++) 
		{
			led_display(i, digit[i]);//依序顯示LED 
			for (d1 = 0; d1 < 100; d1 ++) //延遲 
			{
				for (d2 = 0; d2 < 100; d2 ++) 
				{
				}
			}
		}
		if(scale==0)//當沒升階小燈泡不亮 
		{
			P1_5=0;
		}
		else if(scale==7)//當升一階小燈泡閃爍 (微亮) 
		{
			P1_5=1;
			for (d1 = 0; d1 < 100; d1 ++) {
				for (d2 = 0; d2 < 100; d2 ++) {
				}
			}
			P1_5=0;
		}
		else if(scale==14)//當升兩階小燈泡亮 
		{
			P1_5=1;
		}
		
		if(played_beat == all_beat)//當音樂播完重製回待錄音狀態，並清空data 
		{
			play_song=0;//關閉播音樂data模式 
			play_state=0;//關閉撥放狀態 
			start_record=0;//重製回待錄音狀態 
			led_data=0xee0f;//LED亮ON表示錄音 
			for(i=0;i<1000;i++)//清空data 
			{
				song[i]=26;
			}
			reverse=0;//正著播模式 
		}
		press_number = number();//抓keypad按的字 
		if (press_number != 0x10) 
		{
			temp_led_data = press_number;
			if(flag==0)//開始記錄按住播放鍵時間 
			{
				press_time=0; //開始紀錄按住時間 
			}
			flag = 1; //debounce
			
		}
		if (press_number == 0x10 && flag == 1) //放開時存入 
		{
			led_data = (led_data << 4) + temp_led_data;//左移存到顯示LED的變數 
			flag = 0;//debounce
			if(temp_led_data==0x0c&&scale<14)//當按C時升階 
			{
				scale+=7;//表示升階 
			}	
			if(temp_led_data==0x0d&&scale>0)//當按D時降階 
			{
				scale-=7;//表示降階 
			}
			
			if(temp_led_data==0x0f&&press_time<10)//當按住播放鍵F小於10個interrupt進入正常播放模式 
			{
				play_state=1;
				start_record_count=0;//從data 0位置開始播 
				play_song=1;
				played_beat=0;//已播放音符數量歸零 
				recording=0;//關閉錄音模式 
			}
			else if(temp_led_data==0x0f&&press_time>10)//當按住播放鍵F超過10個interrupt進入倒著播放模式 
			{
				play_state=1;
				start_record_count=all_beat-10;//從音符數量開始倒著播 
				play_song=1;
				played_beat=0;//已播放音符數量歸零 
				recording=0;//關閉錄音模式 
				reverse=1;//倒著播 
			}
			
			if(temp_led_data==0x0e)//按E中止狀態回到待錄音模式 
			{
				play_song=0;
				play_state=0;
				start_record=0;//待錄音模式  
				reverse=0;//預設正著播 
				scale=0;//回到最低階音 
				led_data=0xee0f;//LED亮ON表示錄音 
				for(i=0;i<1000;i++)//清空 
				{
					song[i]=26;
				}
			}
		}
		if(press_number<7)//當按0~6播DOREME...一個interrupt時間 
		{
			play_state=1;//播當前按的 
			play_one_beat=0;//計算一個interrupt 時間 
			current_tone=press_number+scale;//紀錄當前音符頻率 
			start_record++;//由待錄音模式轉到錄音模式 
		}
		if(press_number>6&&press_number<12)//當按789AB播#DO#ME...一個interrupt時間 
		{
			play_state=1;//播當前按的 
			play_one_beat=0;//計算一個interrupt 時間 
			current_tone=press_number+scale+14;//紀錄當前音符頻率 
			start_record++;//由待錄音模式轉到錄音模式 
		}
		if(start_record==1)//因為要從0開始記錄，防止裡面變數一直重製記到重複位置 
		{
		    recording=1;//錄音模式開啟 
			start_record_count=0;//從0開始記錄音符 
			all_beat=0;//記音符數量歸零 
		}
		if(play_one_beat==1)//當前按下音符播完一個interrupt關閉播放模式 
		{
			play_state=0;//關閉播放模式  
			current_tone=26;//紀錄當前為空(甚麼都沒按) 
		}
			
	}
}

void timer_isr1 (void) __interrupt (3) __using (3)//interrupt 記位置、數量 
{
	if(recording==1)//錄音模式下才開始存入data 
	{
		song[start_record_count]=current_tone;//存入當前音符 
		all_beat++;//紀錄總共音符數量 
	}
	play_one_beat++;//播當前音符一個interrupt時間 
	press_time++;//記按住時間 
	if(reverse==0)//正著播音樂data 
	{
		start_record_count++; 
		played_beat++;//播到 played_beat=all_beat，played_beat開始播時已經重製 
	}
	else//反著播音樂data
	{
		start_record_count--;//已將all_beat傳入start_record_count，然後一直--直到等於0 
		all_beat=start_record_count;
		//已將played_beat歸零，然後持續更新all_beat直到等於0停止(同一個判斷式played_beat=all_beat) 
	}
		
	if(start_record_count==1000)//防止超出範圍所以覆蓋前面記錄的 
	{
		start_record_count=0;
	}
	TH1 = TIMER_VAL >> 8;//reset timer1 0.0625 
	TL1 = TIMER_VAL & 0xff;//reset timer1
}

void timer_isr (void) __interrupt (1) __using (1) //interrupt 記音符頻率及播放
{
	//(data當前不是空或是是錄音模式)且是播放模式，才發出聲音
	//防止音樂data跑到空的，當前按的發不出聲音 
	if(play_state==1&&(song[start_record_count]!=26||recording!=0)) 
	{
		P1_0=!P1_0;
	}
	else
		P1_0=P1_0;

	if(play_song==1)//播音樂data模式照data順序播放 
	{
		TH0 = tone[song[start_record_count]] >> 8;//reset timer0
		TL0 = tone[song[start_record_count]] & 0xff;//reset timer0
	}
	else//非播音樂模式但是是播放模式，查找table播放 
	{
		TH0 = tone[current_tone] >> 8;//reset timer0
		TL0 = tone[current_tone] & 0xff;//reset timer0
	}
}
