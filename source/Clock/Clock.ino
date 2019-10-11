/*
MIT License

Copyright (c) 2019 Waldo Wolmarans

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <SPI.h>
#include "epd2in13.h"
#include "SPI.h" 
#include "SPIFlash.h"

#define F_CPU  8000000

#define CSFLASH_PIN      8             // chip select pin for Flash 
#define TIME_OUT_LEVEL   3
#define MINUTE_OFFSET   24
#define MENU_OFFSET     83
volatile uint8_t readyDisplay = 0;


volatile uint8_t timeOutCounter = 0;

volatile unsigned int counterHour = 0;
volatile unsigned int counterMinute = 0;
volatile bool doLog = false;
volatile bool darkTheme = false;

Epd epd;


int analogIn;

void unusePin(int pin)
{
	pinMode(pin, INPUT_PULLUP);
	
}
// Set most of the pins to input pullup.  To minimize the current draw
void unusedPins()
{
	unusePin(P1_0);
	unusePin(P1_1);
	unusePin(P1_2);
	unusePin(P1_3);
	unusePin(P1_4);
	unusePin(P1_5);
	unusePin(P1_6);
	unusePin(P1_7);
	unusePin(P2_0);
	unusePin(P2_1);
	unusePin(P2_2);
	unusePin(P2_3);
	unusePin(P2_4);
	unusePin(P2_5);

}


SPIFlash flash(CSFLASH_PIN); //, 0xC840);

void setup()
{

	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	doLog = false;

	counterHour = 0;
	counterMinute = 0;
	readyDisplay = true;


	if (doLog)
		Serial.begin(57500);
	pinMode(P1_3, INPUT_PULLUP);
	for (int i = 0; i < 3; i++)
	{
		delay(500);
		if (doLog)
			Serial.print("\n Start \n\r");
		if (digitalRead(P1_3) == 0)
		{
			if (doLog)
				Serial.print("\n Upload \n\r");
			delay(1000);
			uploadFlash();
		}
	}

	unusedPins();

	delay(500);
	pinMode(PUSH2, INPUT_PULLUP);

	readyDisplay = true;

	setTimeSetup();
	
	Serial.println("Sleep setup");
	delay(500);



	BCSCTL1 |= DIVA_3;              // ACLK/8
	BCSCTL3 |= XCAP_3;              //12.5pF cap- setting for 32768Hz crystal

	CCTL0 = CCIE;                   // CCR0 interrupt enabled
	CCR0 = 30720;           // 512 -> 1 sec, 30720 -> 1 min 10240; //
	TACTL = TASSEL_1 + ID_3 + MC_1;         // ACLK, /8, upmode

	delay(200);


	//  TestDisplay(true);
	displayClock(FULL);
	delay(500);
	displayClock(FAST);

}

/* Stop with dying message */
void die(int pff_err)
{
	return;
	Serial.println();
	Serial.print("Failed with rc=");
	Serial.print(pff_err, DEC);
	//for (;;) ;
	epd.DisplayFrame();
	delay(300);
	epd.Sleep();
	SPI.end();
	digitalWrite(CSFLASH_PIN, HIGH);
	delay(200);
	//  digitalWrite(enable_sd, HIGH);
	delay(200);

	pinMode(CSFLASH_PIN, INPUT);
	pinMode(CS_PIN, INPUT);
	pinMode(RST_PIN, INPUT_PULLUP);
	pinMode(DC_PIN, INPUT_PULLUP);
	pinMode(BUSY_PIN, INPUT_PULLDOWN);


	delay(500);
	delay(500);

	readyDisplay = true;
	Serial.println("Enter LPM3 w/ interrupt");
	_BIS_SR(LPM3_bits);// + GIE);           // Enter LPM3 w/ interrupt
	Serial.println("After LPM3 w/ interrupt");


}




/*-----------------------------------------------------------------------*/
/* Program Main                                                          */
/*-----------------------------------------------------------------------*/
void loop()
{

	if (doLog)
		Serial.print("-");

	if (doLog)
		Serial.println("L Enter LPM3 w/ interrupt");

	unusedPins(); // to minimize current draw when in sleep

	__bis_SR_register(LPM3_bits + GIE);           // Enter LPM3 w/ interrupt
	if (doLog)
		Serial.println("L After LPM3 w/ interrupt");

	if (readyDisplay == true)
	{
		displayClock(FAST);
	}

}


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	 
	if (doLog)
		Serial.println("T");


	counterMinute++;

	if (counterMinute > 59)
	{
		counterMinute = 0;
		counterHour++;
	}
	if (counterHour > 23)
	{
		counterHour = 0;
	}

	if (readyDisplay == false)
	{

		timeOutCounter++;
		if (timeOutCounter >= TIME_OUT_LEVEL)
		{
			if (doLog)
				Serial.println("D");
			WDTCTL = 0xDEAD;
		}
	}
	else
	{
		timeOutCounter = 0;
	}

	__bic_SR_register_on_exit(LPM3_bits);

}

void displayClock(char updatemode)
{


	if (doLog)
		Serial.println("N Start");
	if (!readyDisplay)
	{

		if (doLog)
			Serial.println("Not Ready");
		//   digitalWrite(LED, LOW); 
		return;
	}
	readyDisplay = false;

	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	pinMode(CS_PIN, OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	pinMode(RST_PIN, OUTPUT);
	pinMode(DC_PIN, OUTPUT);
	pinMode(BUSY_PIN, INPUT);

	SPI.begin();
	Epd epd;




	if (epd.Init(updatemode) != 0) {
		if (doLog)
			Serial.print("e-Paper init failed");
		die(0);

	}

	if (doLog)
		Serial.println("e-Paper init good");

	if (flash.initialize())
	{
		if (doLog)
			Serial.println("Flash Init OK!");
		flash.wakeup1();

		if (updatemode == FULL || counterMinute == 0)
		{
			epd.SetFrameMemory(flash, counterHour, 0, 0, 128, 125, darkTheme);  //left side
		}

		epd.SetFrameMemory(flash, counterMinute + MINUTE_OFFSET, 0, 125, 128, 125, darkTheme); //right side

		epd.DisplayFrame();

		if (updatemode == FAST)
		{
			epd.DisplayFrame();
		}

		flash.sleep();
	}
	else
	{
		if (doLog)
			Serial.println("Flash Init FAIL!");

	}


	if (doLog)
		Serial.println("EPD Going toSleep");
	epd.Sleep();
	if (doLog)
		Serial.println("DisplayFrame End");

	readyDisplay = true;

	if (doLog)
		Serial.println("N End");
	


}

void testPower()
{
	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, LOW);
}

void testDisplay(bool fullupdate)
{
	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	pinMode(CS_PIN, OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	pinMode(RST_PIN, OUTPUT);
	pinMode(DC_PIN, OUTPUT);
	pinMode(BUSY_PIN, INPUT);

	delay(100);

	digitalWrite(RST_PIN, LOW);
	delay(20);
	digitalWrite(RST_PIN, HIGH);
	delay(200);
	SPI.begin();
	Epd epd;

	if (fullupdate)
	{
		epd.Init(FULL);


	}
	else
	{
		epd.Init(FAST);
	}

	epd.DisplayFrame();
	delay(200);
	epd.Sleep();
	delay(200);
	SPI.end();
	delay(100);
}

void setTimeSetup()
{
	int setlevel = 0;
	bool dosettime = false;
	unsigned char tfilter1 = 0;
	int lowdigit = 0;
	int highdigit = 0;
	unsigned int setuptimeout = 0;

	dosettime = true;
	setlevel = 1;


	pinMode(P1_2, INPUT_PULLUP);

	pinMode(P1_1, INPUT_PULLUP);



	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	pinMode(CS_PIN, OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	pinMode(RST_PIN, OUTPUT);
	pinMode(DC_PIN, OUTPUT);
	pinMode(BUSY_PIN, INPUT);

	delay(100);

	digitalWrite(RST_PIN, LOW);
	delay(20);
	digitalWrite(RST_PIN, HIGH);
	delay(20);
	SPI.begin();
	Epd epd;


	delay(20);

	if (epd.Init(FULL) != 0) {
		if (doLog)
			Serial.print("e-Paper init failed");
		die(0);

	}
  
	if (flash.initialize())
	{

		flash.wakeup1();
    displaySettings(epd, flash, setlevel, counterHour, true);
		
		epd.Init(FAST);

		counterMinute = 0;
		if (doLog)
			Serial.println("Init OK!");
		displaySettings(epd, flash, setlevel, counterHour, true);


		while (dosettime)
		{
			setuptimeout++;
			if (setuptimeout > 10000)
			{
				dosettime = false;
				displayClock(FULL);
				return;
			}

			if (digitalRead(P1_1) == 0)
			{
				setuptimeout = 0;
				setlevel++;
				if (setlevel > 2)
				{
					displaySettings(epd, flash, setlevel, counterMinute, true);
				}
				else
				{
					displaySettings(epd, flash, setlevel, counterHour, true);
				}

				if (setlevel > 6)
				{
					setlevel = 1;
					displaySettings(epd, flash, setlevel, counterHour, true);
				}

			}
			else
				if (digitalRead(P1_2) == 0)
				{
					setuptimeout = 0;
					if (setlevel == 1) //hours first digit
					{

						counterHour += 10;
						if (counterHour > 20)
						{
							counterHour = 0;

						}
						highdigit = counterHour;
						displaySettings(epd, flash, setlevel, counterHour, true);


					}
					if (setlevel == 2) //hours second digit
					{

						counterHour += 1;
						if (counterHour > highdigit + 9 || counterHour > 23)
						{
							counterHour = highdigit;

						}
						displaySettings(epd, flash, setlevel, counterHour, true);


					}
					if (setlevel == 3) // minutes fist digit
					{

						counterMinute += 10;
						if (counterMinute > 59)
						{
							counterMinute = 0;

						}
						highdigit = counterMinute;
						displaySettings(epd, flash, setlevel, counterMinute, true);


					}

					if (setlevel == 4)  // minutes second digit
					{

						counterMinute += 1;
						if (counterMinute > highdigit + 9)
						{
							counterMinute = highdigit;

						}
						displaySettings(epd, flash, setlevel, counterMinute, true);


					}
					if (setlevel == 5) //dark theme
					{

						darkTheme = !darkTheme;
						displaySettings(epd, flash, setlevel, darkTheme, true);
					}
					if (setlevel == 6)  //exit
					{

				
						return;
					}



				}
				else
				{
					delay(10);
				}
		}

	}
	else
	{
		if (doLog)
			Serial.println("Init FAIL!");

	}
	/* Deep sleep */
	delay(20);
	epd.Sleep();

}

void displaySettings(Epd epd, SPIFlash spiflush, int menuid, int value, bool init)
{
	if (init)
	{

		epd.SetFrameMemory(flash, menuid + MENU_OFFSET, 0, 4, 128, 150, 1);
		epd.SetFrameMemory(flash, value+MINUTE_OFFSET, 0, 128, 128, 122, 1);
		epd.DisplayFrame();


	}
	else
	{
		epd.SetFrameMemory(flash, value+MINUTE_OFFSET, 0, 128, 128, 122, 0);
		epd.DisplayFrame();
		
	}
}

void uploadFlash() {


	uint32_t addr1 = 0; //5808;
	char input = 0;

	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);


	delay(500);
	SPI.begin();
	if (flash.initialize())
	{
		Serial.println("Flash Init OK!");

		Serial.print("DeviceID: ");
		Serial.println(flash.readDeviceId(), HEX);

		Serial.print("Erasing Flash chip ... ");
		flash.chipErase();
		while (flash.busy());
		Serial.println("DONE");

		Serial.println("Flash content:");
		int counter = 0;
		addr1 = 0;
		while (counter < 4000) {
			Serial.print(flash.readByte(addr1++), HEX);
			counter++;
			
		}

		Serial.println();
		addr1 = 0; 
		for (;;)
		{
			if (Serial.available() > 0) {
				input = Serial.read();


				flash.writeByte(addr1, (uint8_t)input);
				addr1++;
				Serial.print('.');



			}
			else
			{
				Serial.print('*');
				delay(100);
			}
		}
	}
	else
	{
		Serial.println("Flash Init FAILED!");
	}


}
