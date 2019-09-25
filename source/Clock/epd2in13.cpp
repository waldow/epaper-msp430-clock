/**
 *  @filename   :   epd2in13.cpp
 *  @brief      :   Implements for e-paper library
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     September 9 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include "epd2in13.h"
#include "SPIFlash.h"

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
};

int Epd::InitA(const unsigned char* lut) {
    /* this calls the peripheral hardware interface, see epdif */
    if (IfInit() != 0) {
        return -1;
    }
    /* EPD hardware init start */
    this->lut = lut;
    Reset();
    SendCommand(DRIVER_OUTPUT_CONTROL);
    SendData((EPD_HEIGHT - 1) & 0xFF);
    SendData(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    SendData(0x00);                     // GD = 0; SM = 0; TB = 0;
    SendCommand(BOOSTER_SOFT_START_CONTROL);
    SendData(0xD7);
    SendData(0xD6);
    SendData(0x9D);
    SendCommand(WRITE_VCOM_REGISTER);
    SendData(0xA8);                     // VCOM 7C
    SendCommand(SET_DUMMY_LINE_PERIOD);
    SendData(0x1A);                     // 4 dummy lines per gate
    SendCommand(SET_GATE_TIME);
    SendData(0x08);                     // 2us per line
    SendCommand(DATA_ENTRY_MODE_SETTING);
    SendData(0x03);                     // X increment; Y increment
    SetLut(this->lut);
    /* EPD hardware init end */
    return 0;
}

int Epd::Init(const unsigned char* lut,bool fullupdate) {
    /* this calls the peripheral hardware interface, see epdif */
    if (IfInit() != 0) {
        return -1;
    }
    /* EPD hardware init start */
    this->lut = lut;
    Reset();
      WaitUntilIdle();
      if(fullupdate)
      {
        SendCommand(0x12); // soft reset
        WaitUntilIdle();

        SendCommand(0x74); //set analog block control
        SendData(0x54);
        SendCommand(0x7E); //set digital block control
        SendData(0x3B);

        SendCommand(0x01); //Driver output control
        SendData(0xF9);
        SendData(0x00);
        SendData(0x00);

        SendCommand(0x11); //data entry mode
        SendData(0x01);

        SendCommand(0x44); //set Ram-X address start/end position
        SendData(0x00);
        SendData(0x0F);    //0x0C-->(15+1)*8=128

        SendCommand(0x45); //set Ram-Y address start/end position
        SendData(0xF9);   //0xF9-->(249+1)=250
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);

        SendCommand(0x3C); //BorderWavefrom
        SendData(0x03);

        SendCommand(0x2C);     //VCOM Voltage
        SendData(0x55);    //

        SendCommand(0x03);
        SendData(lut_full_update[70]);

        SendCommand(0x04); //
        SendData(lut[71]);
        SendData(lut[72]);
        SendData(lut[73]);

        SendCommand(0x3A);     //Dummy Line
        SendData(lut[74]);
        SendCommand(0x3B);     //Gate time
        SendData(lut[75]);

        SendCommand(0x32);
        for (int count = 0; count < 70; count++)
            SendData(lut[count]);

        SendCommand(0x4E);   // set RAM x address count to 0;
        SendData(0x00);
        SendCommand(0x4F);   // set RAM y address count to 0X127;
        SendData(0xF9);
        SendData(0x00);
        WaitUntilIdle();
      }else {
        SendCommand(0x2C);     //VCOM Voltage
        SendData(0x26);

        WaitUntilIdle();

        SendCommand(0x32);
        for (int count = 0; count < 70; count++)
            SendData(lut[count]);

        SendCommand(0x37);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x40);
        SendData(0x00);
        SendData(0x00);

        SendCommand(0x22);
        SendData(0xC0);
        SendCommand(0x20);
        WaitUntilIdle();

        SendCommand(0x3C); //BorderWavefrom
        SendData(0x01);
    }
    
    return 0;
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
    DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

/**
 *  @brief: Wait until the busy_pin goes LOW
 */
void Epd::WaitUntilIdle(void) {
    while(DigitalRead(busy_pin) == HIGH) {      //LOW: idle, HIGH: busy
        DelayMs(10);
    }      
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(20);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);    
}

/**
 *  @brief: set the look-up table register
 */
void Epd::SetLut(const unsigned char* lut) {
    this->lut = lut;
    SendCommand(WRITE_LUT_REGISTER);
    /* the length of look-up table is 30 bytes */
    for (int i = 0; i < 70; i++) {
        SendData(this->lut[i]);
    }
}

/**
 *  @brief: put an image buffer to the frame memory.
 *          this won't update the display.
 */
void Epd::SetFrameMemory(
    const unsigned char* image_buffer,
    int x,
    int y,
    int image_width,
    int image_height
) {
    int x_end;
    int y_end;

    if (
        image_buffer == NULL ||
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        return;
    }
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= this->width) {
        x_end = this->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= this->height) {
        y_end = this->height - 1;
    } else {
        y_end = y + image_height - 1;
    }
    SetMemoryArea(x, y, x_end, y_end);
    /* set the frame memory line by line */
    for (int j = y; j <= y_end; j++) {
        SetMemoryPointer(x, j);
        SendCommand(WRITE_RAM);
        for (int i = x / 8; i <= x_end / 8; i++) {
            SendData(image_buffer[(i - x / 8) + (j - y) * (image_width / 8)]);
        }
    }
}

/**
 *  @brief: put an image buffer to the frame memory.
 *          this won't update the display.
 *
 *          Question: When do you use this function instead of 
 *          void SetFrameMemory(
 *              const unsigned char* image_buffer,
 *              int x,
 *              int y,
 *              int image_width,
 *              int image_height
 *          );
 *          Answer: SetFrameMemory with parameters only reads image data
 *          from the RAM but not from the flash in AVR chips (for AVR chips,
 *          you have to use the function pgm_read_byte to read buffers 
 *          from the flash).
 */
void Epd::SetFrameMemory(const unsigned char* image_buffer) {
    SetMemoryArea(0, 0, this->width - 1, this->height - 1);
    /* set the frame memory line by line */
    for (int j = 0; j < this->height; j++) {
        SetMemoryPointer(0, j);
        SendCommand(WRITE_RAM);
        for (int i = 0; i < this->width / 8; i++) {
            SendData(pgm_read_byte(&image_buffer[i + j * (this->width / 8)]));
        }
    }
}




void Epd::SetBoxMemory(
        int x,
        int y,
        int box_width,
        int box_height,
        unsigned char pattern,
        unsigned char  frame
    ){

int x_end;
    int y_end;
  
    unsigned char byte1;
    
    
    if (
       
        x < 0 || box_width < 0 ||
        y < 0 || box_height < 0
    ) {
        return;
    }
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    x &= 0xF8;
    box_width &= 0xF8;
    if (x + box_width >= this->width) {
        x_end = this->width - 1;
    } else {
        x_end = x + box_width - 1;
    }
    if (y + box_height >= this->height) {
        y_end = this->height - 1;
    } else {
        y_end = y + box_height - 1;
    }
    SetMemoryArea(x, y, x_end, y_end);
    /* set the frame memory line by line */
    for (int j = y; j <= y_end; j++) {
        SetMemoryPointer(x, j);
        SendCommand(frame);
        for (int i = x / 8; i <= x_end / 8; i++) {
            
             SendData(pattern);
          }
        
    }

      
    }
   void Epd::SetFrameMemory(
    SPIFlash spiflush,
        uint16_t picid,
    int x,
    int y,
    int image_width,
    int image_height,
      unsigned char  frame,
     unsigned char filter,
    bool invert
) {
    int x_end;
    int y_end;
  unsigned char buffer1[16];
    unsigned char byte1;
    unsigned short int br;
    uint32_t addr=(uint32_t) picid * (2000 *1)  ;
    
    if (
       
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        return;
    }
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= this->width) {
        x_end = this->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= this->height) {
        y_end = this->height - 1;
    } else {
        y_end = y + image_height - 1;
    }
    SetMemoryArea(x, y, x_end, y_end);
    /* set the frame memory line by line */
    for (int j = y; j <= y_end; j++) {
        SetMemoryPointer(x, j);
       
           SendCommand(frame);
           
        for (int i = x / 8; i <= x_end / 8; i++) {
            
             byte1 = spiflush.readByte(addr); 
        
          addr++; //=sizeof(buffer1);
          if(i >= 0)
          {
            if(invert)
             SendData(byte1 | filter);
            else  
             SendData(~(byte1 | filter));
              
          }
        }
    }
} 

/**
 *  @brief: clear the frame memory with the specified color.
 *          this won't update the display.
 */
void Epd::ClearFrameMemory(unsigned char color,unsigned char frame) {
    SetMemoryArea(0, 0, this->width - 1, this->height - 1);
    /* set the frame memory line by line */
    for (int j = 0; j < this->height; j++) {
        SetMemoryPointer(0, j);
        SendCommand(frame);
        for (int i = 0; i < this->width / 8; i++) {
            SendData(color);
        }
    }
}
void Epd::ClearFrameMemoryB(unsigned char color,unsigned char frame) {
    SetMemoryArea(0, 0, this->width - 1, this->height - 1);
        SetMemoryPointer(0, 0);
        SendCommand(frame);
         for(unsigned int i = 0; i < (this->width * this->height) / 8; i++) {
          SendData(color);  
       }  
      
}
void Epd::ClearFrame() {
    SetMemoryArea(0, 0, this->width - 1, this->height - 1);
    /* set the frame memory line by line */
    for (int j = 0; j < this->height; j++) {
        SetMemoryPointer(0, j);
        SendCommand(WRITE_RAM);
        for (int i = 0; i < this->width / 8; i++) {
            SendData(0x0);
        }
    }
}

/**
 *  @brief: update the display
 *          there are 2 memory areas embedded in the e-paper display
 *          but once this function is called,
 *          the the next action of SetFrameMemory or ClearFrame will 
 *          set the other memory area.
 */
void Epd::DisplayFrame(void) {
    SendCommand(0x22);
    SendData(0xC7);
    //EPD_SendData(0x0c);
    SendCommand(0x20);
    WaitUntilIdle();
    /*
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xC4);
    SendCommand(MASTER_ACTIVATION);
    SendCommand(TERMINATE_FRAME_READ_WRITE);
    WaitUntilIdle();
    */
}
void Epd::DisplayFrameNoWait(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xC4);
    SendCommand(MASTER_ACTIVATION);
    SendCommand(TERMINATE_FRAME_READ_WRITE);
    WaitUntilIdle();
}
void Epd::SetFrame1(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_1);
    SendData(0xC4);
     SendCommand(MASTER_ACTIVATION);
    SendCommand(TERMINATE_FRAME_READ_WRITE);
    WaitUntilIdle();
 
}
void Epd::SetFrame2(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xC4);
 
}

/**
 *  @brief: private function to specify the memory area for data R/W
 */
void Epd::SetMemoryArea(int x_start, int y_start, int x_end, int y_end) {
    SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    SendData((x_start >> 3) & 0xFF);
    SendData((x_end >> 3) & 0xFF);
    SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    SendData(y_start & 0xFF);
    SendData((y_start >> 8) & 0xFF);
    SendData(y_end & 0xFF);
    SendData((y_end >> 8) & 0xFF);
}

/**
 *  @brief: private function to specify the start point for data R/W
 */
void Epd::SetMemoryPointer(int x, int y) {
    SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    SendData((x >> 3) & 0xFF);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    SendData(y & 0xFF);
    SendData((y >> 8) & 0xFF);
    WaitUntilIdle();
}

/**
 *  @brief: After this command is transmitted, the chip would enter the 
 *          deep-sleep mode to save power. 
 *          The deep sleep mode would return to standby by hardware reset. 
 *          You can use Epd::Init() to awaken
 */
void Epd::Sleep() {

   SendCommand(0x22); //POWER OFF
    SendData(0xC3);
    SendCommand(0x20);
  WaitUntilIdle();
    SendCommand(0x10); //enter deep sleep
    SendData(0x01);
  //  DelayMs(100);

    return;
  // SendCommand(POWER_OFF);
  //  WaitUntilIdle();
   // DelayMs(200);
    SendCommand(DEEP_SLEEP_MODE);
    SendData(0x1);     // check code
    //WaitUntilIdle();
}
void Epd::PowerOff() {

   SendCommand(0x22); //POWER OFF
    SendData(0xC3);
    SendCommand(0x20);

    SendCommand(0x10); //enter deep sleep
    SendData(0x01);
    DelayMs(100);

    return;
    
   SendCommand(POWER_OFF);
    WaitUntilIdle();
    DelayMs(200);
 //   SendCommand(DEEP_SLEEP);
 //   SendData(0xA5);     // check code
}
const unsigned char lut_full_update_old[] =
{
    0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_fast_update_old[] =
{
    0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_partial_update_old[] =
{
    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_full_update[] = {
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,       //LUT0: BB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,       //LUT1: BW:     VS 0 ~7
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,       //LUT2: WB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,       //LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT4: VCOM:   VS 0 ~7

    0x03, 0x03, 0x00, 0x00, 0x02,                   // TP0 A~D RP0
    0x09, 0x09, 0x00, 0x00, 0x02,                   // TP1 A~D RP1
    0x03, 0x03, 0x00, 0x00, 0x02,                   // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

const unsigned char lut_fast_update[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT0: BB:     VS 0 ~7
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,       //LUT1: BW:     VS 0 ~7
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT2: WB:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT4: VCOM:   VS 0 ~7

    0x0A, 0x00, 0x00, 0x00, 0x00,                   // TP0 A~D RP0
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP1 A~D RP1
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

const unsigned char lut_partial_update[] = { //20 bytes
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT0: BB:     VS 0 ~7
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT1: BW:     VS 0 ~7
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT2: WB:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT4: VCOM:   VS 0 ~7

    0x0A, 0x00, 0x00, 0x00, 0x00,                   // TP0 A~D RP0
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP1 A~D RP1
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

/* END OF FILE */
