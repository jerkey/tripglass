#pragma sfr
#pragma nop
#pragma ei
#pragma di
#pragma STOP
#pragma vect RESET main
#pragma interrupt INTTMH1 isr_INTTMH1
#pragma interrupt INTTM000 isr_INTTM000
#pragma section @@CNST OPT AT 80H
const char OPTION=0x9c;

/*const unsigned long int Duration_Num[44]=
{600000,100000,200000,150000,150000,200000,100000,300000,50000,600000,  // 245
    620,   938,   620,   938,   620,   938,   620,   938,  620,   938,
 967.74,106.61,322.58,159.91,241.94,213.22,161.29,319.83,80.65,639.66,
 100000,300000,200000,300000,300000,150000,600000,150000,10000,150000,  // 226
   1960,   938,  1960,   938,  1960,   938,  1960,   938,  620,   938,
  51.02,319.83,102.04,319.83,153.06,159.91,306.12,159.91,16.13,159.91,
 600000,10000 ,100000,10000 ,100000,10000 ,300000,150000,10000,150000,  // 144
   1960,  3846,  1960,  3846,  1960,  3846,  1960,   938,  620,   938,
 306.12,  2.60, 51.02,  2.60, 51.02,  2.60,153.06,159.91,16.13,159.91,
 300000,150000,10000 ,200000,50000 ,200000,150000,150000,200000,100000, // 151
   1960,   938,   620,   938,   620,   938,   620,   938,   620,   938,
 153.06,159.91, 16.13,213.22, 80.65,213.22,241.94,159.91,322.58,106.61,
 250000,50000 ,600000, 0,};												//  90
   620,   938,    620, 0,
403.22, 53.30, 967.74, 0,
 */																		// (856*10000*0.1)/(1000*60)= 14.26mm
const unsigned int Duration_Num[44]=
{ 967,106,322,159,241,213,161,319, 80,639,
   51,319,102,319,153,159,306,159, 16,159,
  306,  2, 51,  2, 51,  2,153,159, 16,159,
  153,159, 16,213, 80,213,241,159,322,106,
  403, 53,967,  0,};

const unsigned char Num[44]=
{1,0,1,0,1,0,1,0,1,0,
 2,0,2,0,2,0,2,0,1,0,
 2,3,2,3,2,3,2,0,1,0,
 2,0,1,0,1,0,1,0,1,0,
 1,0,1,4,};


void install_cpu();
void install_data();
void do_brainwave_element(unsigned char i);
void blink_LED(unsigned int Duration,unsigned int OnTime,unsigned int OffTime);
void delay_ms(unsigned int DelayTime);
void delay_100us();

unsigned char count;
unsigned int Ctrl_CR000;
bit B_OFF;

void install_cpu()
{
//SYSTEM TIME ?T?R???????????
	PCC=0;
	PPCC=0;
	OSTS=0x02;
	WDTM=0x7f;
	LSRCM=0x01;
//80
//	TMC80=0x00;     // 0.1ms
//	CR80=0x0b;
//	TMIF80=0;
//	TMMK80=0;
//	TCE80=1;        // RUN
//CTC1 (left headphone channel)
	PM2.0=0;		//output mode for TOH1
	P2.0=0;
	CMP01=0x04;     //197.2Hz--197.8
	TMHMD1=0x41;    // f/(2^12)
	////TMHE1=1;    // RUN
	TMIFH1=0;
	TMMKH1=0;
//CTC2 (right headphone channel)
	PM2.1=0;		//output mode for TO00
	P2.1=0;
	CR000=73;		//217.014Hz
	CRC00=0;
	PRM00=0xa2;     //f/(2^8);
	TOC00=0x0f;
	TMC00=0;		//TMC00=0x0C run;
	TMIF000=0;
	TMMK000=0;
/////////////////
    INTM0=0x28;
/////////////////
	PM4.0=0;		//output mode for P40 (LED R)
	PM4.3=0;		//output mode for P43 (LED L)
	P4.0=0;
	P4.3=0;
/////////////////
	EI();
/////////////////
}
void install_data()
{
	TMHE1=1;
	B_OFF=0;
	TMC00=0x0C;
}
///////////
void main()
{
	#asm
	    movw  AX,#0FEFFH
    	movw  SP,AX
    #endasm

	install_cpu();
	install_data();
	count=0;
 	while(Num[count]<=3)
 	{
 		do_brainwave_element(count);
 		count++;
 	}
 	delay_ms(10000);        //delay 1s
 	#asm
		;OPB CSEG AT 0080h  //Option Byte is already set to this value in option.inc
		;DB 10011100b       //this allows changing LSRCM to 1 (stop low-speed internal oscillator
 	#endasm
	PM2.0=1;            //input mode for TOH1 (CTC1 -- left headphone channel)
	PM2.1=1;            //input mode for TO00 (CTC2 -- right headphone channel)
	PM4.0=1;            //input mode for P40 (LED R)
	PM4.3=1;            //input mode for P43 (LED L)
	LSRCM=1;
 	STOP();
}


void do_brainwave_element(unsigned char i)
{

	switch(Num[i])
	{
		case (0): //a
			Ctrl_CR000=75;   //T208//207.7-197.2=10.5 //208.6-197.8=10.8
			blink_LED(Duration_Num[i],469,469);//   469=10000/(10.65*2)
		break;
		case (1): //b
			Ctrl_CR000=73;   //T214//213.2-197.2=16.0 //214.1-197.8=16.3
			blink_LED(Duration_Num[i],310,310);//   310=10000/(16.15*2)
		break;
		case (2): //t
			Ctrl_CR000=77;   //T202//202.2-197.2=5.0 //203.0-197.8=5.2
			blink_LED(Duration_Num[i],980,980);//   980=10000/(5.1*2)
		break;
		case (3): //d
			Ctrl_CR000=78;   //T200//199.8-197.2=2.6 //200.4-197.8=2.6
			blink_LED(Duration_Num[i],1923,1923);//1923=10000/(2.6*2)
		break;
		default:
			B_OFF=1;
		break;
	}
}
void blink_LED(unsigned int Duration,unsigned int OnTime,unsigned int OffTime)
{
	unsigned int i;
	//for(i=0;i<(Duration/(OnTime+OffTime));i++)
	for(i=0;i<Duration;i++)
	{
		P4.0=1;
		P4.3=1;
		delay_ms(OnTime);
		P4.0=0;
		P4.3=0;
		delay_ms(OffTime);
	}
}
void delay_ms(unsigned int DelayTime)
{
	unsigned int j;
	for(j=0;j<DelayTime;j++)
	{
		delay_100us();
	}
}
void delay_100us()     // 0.125*(10^-6)*100= the  1  command
{
	unsigned char i;
	for(i=0;i<15;i++); //16.37
	i++;               //16.03
}
__interrupt void isr_INTTMH1()
{
	if(B_OFF==1)
	{
		TMHE1=0;
		TMMKH1=1;
		P2.0=0;
	}
}
__interrupt void isr_INTTM000()
{
	CR000=Ctrl_CR000;
	if(B_OFF==1)
	{
		TMC00=0;
		TOC00=0;
		TMMK000=1;
		P2.1=0;
	}
}
