/* Name: main.c
 * Project: hid-custom-rq example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */


#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
//#include "oddebug.h"        /* This is also an example for using debug macros */
//#include "requests.h"       /* The custom request numbers we use */

#include "usb_hid_sensors.h"

#include <string.h>  //memset

//warning! poor english comments and variable names

//at least 18ms in dth11 doc
#define START_SIGNAL_LENGTH 25

#define FLAG_DHT22 							0b00000001
#define FLAG_NO_AUTODETECT					0b00000010
#define FLAG_DEBUG_OUTPUT					0b00000100
#define FLAG_CHANGE_TO_FAST_TIMER_STROBE 	0b00001000
#define FLAG_CHANGE_TO_SLOW_TIMER_STROBE 	0b00010000
#define FLAG_LED_OUT_STROBE				 	0b00100000
/* ------------------------------------------------------------------------- */
//timeouts with 8 bit timer0
/* ------------------------------------------------------------------------- */

static uint16_t SendTimeout;
static uint16_t LineDownTimeout;
static uint16_t timeout = 0;

void calcTimeouts(uint16_t featureInterval)
{			
	// timer clock = cpu/1024 													|for 12MHz  
	//(fcpu/1024) = (timer ticks per second)									|= 11718
	//(timer ticks) per (timeout in milliseconds) :								|for 5s timeout
	// timer ticks per second*timeout_milliseconds/1000							|= 58590	
	//(timer ticks per timeout/255) = 8 bit timer overflows per timeout 		|= 382, max rest 254 

	//inaccurate because of mentioned rest and some overflows can not be counted
	//CheckTimeout() should be called more often than overflow occure 
	
	//(F_CPU<<10) = F_CPU / 1024
	uint32_t tmp = (F_CPU>>10)*featureInterval;
	SendTimeout = tmp/255000; //long running!?!
	tmp = (F_CPU>>10)*START_SIGNAL_LENGTH;
	LineDownTimeout = tmp/255000;
}

void SetTimeout(uint8_t type)
{
	if(type)
		timeout = SendTimeout;
	else
		timeout = LineDownTimeout;
	TCNT0 = 0; 
	TIFR0&=(1<<TOV0);
}

uint8_t CheckTimeout()
{
	if(timeout == 0)
		return 1;
		
	if(TIFR0&(1<<TOV0))
	{
		timeout--;
		TIFR0&=(1<<TOV0);
	}
	
	if(timeout == 0)
		return 1;
	else
		return 0;
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

//"my" descriptor definition, i believe it's incorrect
//win10 try to use sensordhid.dll, fails and device become
//visible as working properly hid-compliant device
PROGMEM const char usbHidReportDescriptor[] = {   

	//HID_USAGE_PAGE_SENSOR,
	//HID_USAGE_SENSOR_TYPE_OTHER_GENERIC,

		0x05,0x06, //Generic Device Controls Page 
		0x09,0x00, //undefined type

	HID_COLLECTION(Application),

		HID_USAGE_PAGE_SENSOR,

		HID_USAGE_SENSOR_PROPERTY_REPORT_INTERVAL,
		HID_REPORT_SIZE(16),
		HID_REPORT_COUNT(1),
		HID_UNIT_EXPONENT(0), //milliseconds
		HID_FEATURE(Data_Var_Abs),
		
		HID_USAGE_SENSOR_DATA_GENERIC_DATAFIELD,
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(1),
		HID_UNIT_EXPONENT(0), //custom for sensor device type
		HID_FEATURE(Data_Var_Abs),		
		
		HID_USAGE_SENSOR_DATA_ENVIRONMENTAL_TEMPERATURE,
		HID_REPORT_SIZE(16),
		HID_REPORT_COUNT(1),
		HID_UNIT_EXPONENT(0x0E), // scale default unit “Celsius” to provide 2 digits past the decimal point
		HID_INPUT(Const_Var_Abs),
		
		HID_USAGE_SENSOR_DATA_ENVIRONMENTAL_RELATIVE_HUMIDITY,
		HID_REPORT_SIZE(16),
		HID_REPORT_COUNT(1),
		HID_UNIT_EXPONENT(0x0E), // scale default unit “percent” to provide 2 digits past the decimal point
		HID_INPUT(Const_Var_Abs),		
		
	HID_END_COLLECTION
	
}; 
ct_assert(sizeof(usbHidReportDescriptor)>52);


/* ------------------------------------------------------------------------- */

static struct featureT
{
	uint16_t	interval;
	uint8_t		flags;
} feature;

static struct inputT
{
	int16_t		temp;
	int16_t		hum;
} input;

//it disregard all posibilities other than one i suppose is most possible
uchar usbFunctionWrite(uchar *data, uchar len)
{
	usbDisableAllRequests();
	
	if(len>sizeof(feature))
		len = sizeof(feature);
	memcpy(&feature,data,len);
	
	if(feature.flags&FLAG_CHANGE_TO_SLOW_TIMER_STROBE)
		TCCR1B |= (1<<CS11)|(1<<CS10); //16 bit timer clock = cpu/64
	if(feature.flags&FLAG_CHANGE_TO_FAST_TIMER_STROBE)
		TCCR1B |= (1<<CS11); //16 bit timer clock = cpu/8
	if(feature.flags&FLAG_LED_OUT_STROBE)		
		DDRA |= (1<<DDA2);
	else
		DDRA &= ~(1<<DDA2);
		
	calcTimeouts(feature.interval); 
	
	usbEnableAllRequests();
	return 1; //0xff = STALL
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;
	
	if(rq->bRequest == USBRQ_HID_GET_REPORT && rq->wValue.bytes[1] == REPORT_TYPE_FEATURE)
	{
		usbMsgPtr = (unsigned short)&feature;
		return sizeof(feature);
	}
	else
	if(rq->bRequest == USBRQ_HID_SET_REPORT && rq->wValue.bytes[1] == REPORT_TYPE_FEATURE)
	{
		return USB_NO_MSG;
	}
	else
	if(rq->bRequest == USBRQ_HID_GET_REPORT && rq->wValue.bytes[1] == REPORT_TYPE_INPUT)
	{
		usbMsgPtr = (unsigned short)&input;
		return sizeof(input);
	}
	else	
		return 0;
}

/* ------------------------------------------------------------------------- */
//for debugging with linux hidraw driver
void out_char(uchar c)
{
	while(!usbInterruptIsReady()){usbPoll();wdt_reset();}
	usbSetInterrupt((void *)&c, 1);
}
void numOut(unsigned long long num, uchar eol)
{
	uchar tmp[10];
	schar len;
	if(num==0)
	{
		out_char('0');
	}
	else
	{
		for(len=0;len<10;len++)
			tmp[len] = 0;
		
		len = 0;
		
		while(num!=0)
		{
			tmp[len++] = '0'+num%10;
			num /= 10;
		}
		
		for(len=9;len>=0;len--)
		{	
			if(tmp[len] != 0)
				out_char(tmp[len]);
		}
	}	
	if(eol)
	{
		out_char('\n');
	}
	else
	{
		out_char(' ');
	}	
}
/* ------------------------------------------------------------------------- */
#define SPACE_SIZE 44
uint8_t space[SPACE_SIZE];
uint8_t spaceUsed;

enum enCommState {	
	csDelay,
	csLineIsDown, 
	csReceiving, 
	csInterpreting, 
	csCalibrating, 
	csWritingResult};
	
static enum enCommState CommState = csDelay;

static uint8_t bitCnt;
static uint16_t lastTimerValue;
static uint8_t dhtThreshold = 0;
static uint8_t res[5];
static uint8_t failCnt = 0;

static uint8_t dhtThresholdMin = 0;

uint8_t dhtPoll()
{
	if(CommState == csDelay && CheckTimeout())
	{
		PORTA |= (1<<PORTA2); //LED on 
		
		//pull-down
		DDRA |= (1<<DDA7);
		PORTA &= ~(1<<PORTA7);
	
		SetTimeout(0);			//0 is short timeout 
				
		CommState = csLineIsDown;
	}else
	if (CommState == csLineIsDown && CheckTimeout())
	{
		SetTimeout(1);			//1 is long timeout
		
		//pull-up for read
		DDRA &= ~(1<<DDA7);
		PORTA |= (1<<PORTA7);
		
		TIFR1&=(1<<TOV1);		//clear timer1 overflow flag
		TIFR1&=(1<<ICF1); 		//clear trigger flag
		bitCnt = 0;				//dht bit number
		TCNT1 = 0; 				//timer1 = 0
		lastTimerValue = 0;		//'earlier' input capture value								
		
		CommState = csReceiving;
	}else 	 
	if (CommState == csReceiving) 
	{
		//timer1 'input capture' triggered on rising edge from dht
		if(TIFR1&(1<<ICF1))
		{			
			if (bitCnt < SPACE_SIZE)
				space[bitCnt] = ICR1-lastTimerValue; //save time
			bitCnt++;
			lastTimerValue = ICR1;
			TIFR1&=(1<<ICF1); // clear trigger flag
		}
		
		// timer1 overflow as read timeout is only possible end of reading
		//about 35ms with 20MHz
		if (TIFR1&(1<<TOV1)) { 
			//pull-up
			DDRA |= (1<<DDA7);
			PORTA |= (1<<PORTA7);
			
			if(bitCnt > 40)  
			{//ok
				PORTA &= ~(1<<PORTA2); //LED off
			
				 //for debugging with linux hidraw driver
				if(feature.flags&FLAG_DEBUG_OUTPUT)
				{
						out_char('A');
						uint8_t i;
						for(i=0;i<SPACE_SIZE;i++)
							numOut(space[i],0);
						out_char('B');
				}
			
				spaceUsed = bitCnt;//dht22 = 42 ; dht11 = 43 
				memset(res,0,sizeof(res)); //clear interpret result variable
				CommState = csInterpreting; 
			}
			else
			{//fail 
				SetTimeout(0); //try again after short time
				CommState = csDelay; 
			}			
		}				
	}else
	if(CommState == csInterpreting)
	{
		if(dhtThreshold != 0)
		{
			if(bitCnt == 0) //interpreting finished
			{
				if(res[4] == (uchar)(res[0]+res[1]+res[2]+res[3]))
				{//checksum ok
					failCnt = 0;
					CommState = csWritingResult;
					
				}
				else 
				{//fail to interpret
					if(failCnt++ > 5)
					{
						dhtThreshold = 0; //reCalibrate
						failCnt = 0;
					}

					CommState = csDelay;
				}
			}
			else
			{
				//interpreting, result is in 'res'

				bitCnt--;
				
				uint8_t iter = spaceUsed-(bitCnt+1);
				
				if(	iter >= (spaceUsed-40) && 
					space[iter] > dhtThreshold )
				{						
					uchar bit = iter-(spaceUsed-40);//spaceUsed-40 = extraBits
					uchar byte = bit/8;

					bit %= 8;
					res[byte] |= (1<<(7-bit));		 
				}
			}
		}
		else
		{
			CommState = csCalibrating;
		}
		
	}else
	if(CommState == csCalibrating)
	{
		if(bitCnt == 0) //interpreting finished
		{
			uint8_t working = 
				*((uint32_t*)res)!=0x00000000	&& //not empty
				*((uint32_t*)res)!=0xFFFFFFFF	&& //not empty
				(res[4] == (uchar)(res[0]+res[1]+res[2]+res[3]))//checksum ok
				;
				
			memset(res,0,sizeof(res)); //clear interpret result variable
			
			//last value checked
			if(dhtThreshold == 254)
			{
				if(!working)
				{
					//fail at all
					SetTimeout(0); //try again after short time
					CommState = csDelay; 
					return 0;
				}
				else
				{
					//success at top of counter
					
					//calc threshold in the middle of working values
					dhtThreshold = dhtThresholdMin+
						((dhtThreshold-dhtThresholdMin)/2);
						
					bitCnt = spaceUsed;
					CommState = csInterpreting;	
					return 0;				
				}					
			}
			else
			{				
				if(working)//save first working value
				{
					if(dhtThresholdMin == 0)
						dhtThresholdMin = dhtThreshold;		
				}
				else
				if(dhtThresholdMin != 0)//first not working value - finish
				{
					//calc threshold in the middle of working values
					dhtThreshold = dhtThresholdMin+
						((dhtThreshold-dhtThresholdMin)/2);
					
					bitCnt = spaceUsed;
					CommState = csInterpreting;		
					return 0;
				}				

				bitCnt = spaceUsed;
				dhtThreshold++;
			}
		}
		else 
		{
			//interpreting, result is in 'res'

			bitCnt--;
			
			uint8_t iter = spaceUsed-(bitCnt+1);
			
			if(	iter >= (spaceUsed-40) && 
				space[iter] > dhtThreshold )
			{						
				uchar bit = iter-(spaceUsed-40);//spaceUsed-40 = extraBits
				uchar byte = bit/8;

				bit %= 8;
				res[byte] |= (1<<(7-bit));		 
			}
		}

	}else
	if(CommState == csWritingResult)
	{
		if( !(feature.flags&FLAG_DHT22) && //not dht22 flag is set
			!(feature.flags&FLAG_NO_AUTODETECT) && //not autodetect suppress is set
			(res[1]|res[3]) )
			feature.flags |= FLAG_DHT22; //set dht22 flag

		uchar minus = res[2]&0x80;
		res[2] &= ~0x80;

		if(feature.flags&FLAG_DHT22)
		{//dht22
			input.hum = (uint16_t)res[1]|((uint16_t)res[0]<<8);	
			input.hum *= 10;
			input.temp = (uint16_t)res[3]|((uint16_t)res[2]<<8);			
			input.temp *= 10;
		}
		else
		{//dht11
			input.hum = res[0];	
			input.hum *= 100;
			input.temp = res[2];		
			input.temp *=100;
		}
		if(minus)
			input.temp = -input.temp;

		CommState = csDelay;

		return 1;
	}
	
	return 0;
}

void dhtInit()
{
	//timeouts measure helper timer
	TCCR0B |= (1<<CS02)|(1<<CS00); //8 bit timer clock = cpu/1024

    feature.flags = 0;
    feature.interval = 5000;//5s
    
    calcTimeouts(feature.interval);
	SetTimeout(1);
	
	//dht measuring 16 bit timer
	//measurement nedd to fit 8bit, it still not exceed with 20GHz but is close	
	//faster clock is more precise but slower is enough too
#if F_CPU > 12000000 
	TCCR1B |= (1<<CS11)|(1<<CS10); //16 bit timer clock = cpu/64
#else
	TCCR1B |= (1<<CS11); //16 bit timer clock = cpu/8
#endif
    TCCR1B |= (1<<ICES1);//trigger output compare at rising edge
    
    //pull-up
    DDRA |= (1<<DDA7);
    PORTA |= (1<<PORTA7);
    
    //LED
    //DDRA |= (1<<DDA2);
    PORTA &= ~(1<<PORTA2);
}
/* ------------------------------------------------------------------------- */
int __attribute__((noreturn)) main(void)
{
    dhtInit();

    wdt_enable(WDTO_4S);
    usbInit();

    usbDeviceDisconnect();  // enforce re-enumeration, do this while interrupts are disabled! 
    uchar i = 0;
    while(--i){             // fake USB disconnect for > 250 ms 
        wdt_reset();
        if(!(i%15))			//not best way to use led
			PORTA |= (1<<PORTA2);
        _delay_ms(1);
        PORTA &= ~(1<<PORTA2);
    }
    usbDeviceConnect();

    sei();
    for(;;){          
        wdt_reset();    
		if(	dhtPoll() && usbInterruptIsReady() 
			&& !(feature.flags&FLAG_DEBUG_OUTPUT) 
		  )
			usbSetInterrupt((void *)&input, sizeof(input));
        usbPoll();
	}
}
/* ------------------------------------------------------------------------- */

