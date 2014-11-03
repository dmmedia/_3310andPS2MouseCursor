#include <ps2.h>
#include <avr/pgmspace.h>

//#define verbose
/*
 * Pin 8 is the mouse data pin, pin 9 is the clock pin
 * Feel free to use whatever pins are convenient.
 */
PS2 mouse(9, 8);

//  arduino pinout for LCD.
int CONTROL_LED   = 13;
int LCD_POWER_PIN =	12;
int LCD_DC_PIN	  =	2; //  PB0  4  Data Command
int LCD_CE_PIN	  =	3; //  PB2  5  /CS   active low chip select ??
int SPI_MOSI_PIN  =	4; //  PB3  3  Serial   line
int LCD_RST_PIN	  =	5; //  PB4  8   /RES RESET
int SPI_CLK_PIN	  =	6; //  PB5  2  CLOCK

#define LCD_X_RES 84
#define LCD_Y_RES 48

#define LCD_CACHE_SIZE ((LCD_X_RES * LCD_Y_RES) / 8)

byte cursorX = 42;
byte cursorY = 24;

/*--------------------------------------------------------------------------------------------------
 Type definitions
 --------------------------------------------------------------------------------------------------*/

//#define    LCD_CMD  0
//#define    LCD_DATA  1

enum LcdMsgType
{
    LCD_CMD = 0,
    LCD_DATA
};

#define    PIXEL_OFF   0
#define    PIXEL_ON    1
#define    PIXEL_XOR   2

#define    FONT_1X  1
#define    FONT_2X  2

typedef unsigned int word;

// Public function prototypes

void LcdInit(void);
void LcdClear(void);
void LcdUpdate(void);
void LcdGotoXY(byte x, byte y);
void LcdChr(int size, byte ch);
void LcdStr(int size, char *dataPtr);
void LcdPixel(byte x, byte y, int mode);
void LcdLine(byte x1, byte y1, byte x2, byte y2, int mode);
void LcdSendCmd(byte data, int cd);

// This table defines the standard ASCII characters in a 5x7 dot format.

// Global Variables

byte LcdCache[LCD_CACHE_SIZE];

int LcdCacheIdx;
int LoWaterMark;

int HiWaterMark;
boolean UpdateLcd;
char sign;

// This table defines the standard ASCII characters in a 5x7 dot format.
PROGMEM const char FontLookup[127][5] = {
                                    // DEC OCT
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 000 000 empty square []
    {0x04, 0x02, 0x7F, 0x02, 0x04}, // 001 001 arrow up
    {0x10, 0x20, 0x7F, 0x20, 0x10}, // 002 002 arrow down
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // 003 003 arrow right
    {0x08, 0x1C, 0x2A, 0x08, 0x08}, // 004 004 arrow left
    {0x7F, 0x01, 0x01, 0x01, 0x01}, // 005 005 top left corner
    {0x01, 0x01, 0x01, 0x01, 0x7F}, // 006 006 top right corner
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 007 007 bottom left corner
    {0x40, 0x40, 0x40, 0x40, 0x7F}, // 008 010 bottom right corner
    {0x60, 0x78, 0x7E, 0x78, 0x60}, // 009 011 triangle up
    {0x03, 0x0F, 0x3F, 0x0F, 0x03}, // 010 012 triangle down
    {0x3E, 0x1C, 0x1C, 0x08, 0x08}, // 011 013 triangle right
    {0x08, 0x08, 0x1C, 0x1C, 0x3E}, // 012 014 triangle left
    {0x0E, 0x0A, 0x0E, 0x00, 0x00}, // 013 015 degree sign
    {0x10, 0x10, 0x54, 0x10, 0x10}, // 014 016 division sign
    {0x7F, 0x7F, 0x7F, 0x7F, 0x7F}, // 015 017 full square []
    {0x44, 0x44, 0x5F, 0x44, 0x44}, // 016 020 +/-
    {0x2A, 0x2A, 0x2A, 0x2A, 0x2A}, // 017 021 ===
    {0x40, 0x44, 0x4A, 0x51, 0x40}, // 018 022 less than or equal
    {0x40, 0x51, 0x4A, 0x44, 0x40}, // 019 023 greater than or equal
    {0x10, 0x20, 0x7F, 0x01, 0x01}, // 020 024 square root
    {0x01, 0x01, 0x01, 0x01, 0x01}, // 021 025 top bar
    {0x60, 0x58, 0x46, 0x58, 0x60}, // 022 026 empty triangle up
    {0x03, 0x0D, 0x31, 0x0D, 0x03}, // 023 027 empty triangle down
    {0x3E, 0x14, 0x14, 0x08, 0x08}, // 024 030 empty triangle right
    {0x08, 0x08, 0x14, 0x14, 0x3E}, // 025 031 empty triangle left
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 026 032 []
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 027 033 []
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 028 034 []
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 029 035 []
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 030 036 []
    {0x7F, 0x41, 0x41, 0x41, 0x7F}, // 031 037 []
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 032 040 
    {0x00, 0x00, 0x4F, 0x00, 0x00}, // 033 041 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 034 042 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 035 043 #
    {0x24, 0x2E, 0x7F, 0x3A, 0x12}, // 036 044 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 037 045 %
    {0x36, 0x49, 0x56, 0x20, 0x58}, // 038 046 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 039 047 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 040 050 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 041 051 )
    {0x22, 0x14, 0x7F, 0x14, 0x22}, // 042 052 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 043 053 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 044 054 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 045 055 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 046 056 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 047 057 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 048 060 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 049 061 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 050 062 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 051 063 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 052 064 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 053 065 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 054 066 6
    {0x01, 0x01, 0x71, 0x0D, 0x03}, // 055 067 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 056 070 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 057 071 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 058 072 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 059 073 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 060 074 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 061 075 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 062 076 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 063 077 ?
    {0x3E, 0x41, 0x5D, 0x5D, 0x1E}, // 064 100 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 065 101 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 066 102 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 067 103 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 068 104 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 069 105 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 070 106 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 071 107 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 072 110 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 073 111 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 074 112 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 075 113 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 076 114 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 077 115 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 078 116 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 079 117 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 080 120 P
    {0x3E, 0x41, 0x41, 0x61, 0x7E}, // 081 121 Q
    {0x7F, 0x09, 0x09, 0x09, 0x76}, // 082 122 R
    {0x06, 0x49, 0x49, 0x49, 0x30}, // 083 123 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 084 124 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 085 125 U
    {0x03, 0x1C, 0x60, 0x1C, 0x03}, // 086 126 V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 087 127 W
    {0x41, 0x36, 0x08, 0x36, 0x41}, // 088 130 X
    {0x01, 0x06, 0x78, 0x06, 0x01}, // 089 131 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 090 132 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 091 133 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 092 134 \
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 093 135 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 094 136 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 095 137 _
    {0x00, 0x00, 0x03, 0x05, 0x00}, // 096 140 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 097 141 a
    {0x7F, 0x44, 0x44, 0x44, 0x38}, // 098 142 b
    {0x38, 0x44, 0x44, 0x44, 0x44}, // 099 143 c
    {0x38, 0x44, 0x44, 0x44, 0x7F}, // 100 144 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 145 e
    {0x04, 0x04, 0x7E, 0x05, 0x05}, // 102 146 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 147 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 150 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 151 i
    {0x20, 0x40, 0x44, 0x7D, 0x00}, // 106 152 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 153 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 154 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 155 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 156 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 157 o
    {0x7C, 0x14, 0x14, 0x14, 0x10}, // 112 160 p
    {0x10, 0x14, 0x14, 0x14, 0x7C}, // 113 161 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 162 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 163 s
    {0x04, 0x04, 0x3F, 0x44, 0x44}, // 116 164 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 165 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 166 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 167 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 170 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 171 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 172 z
    {0x00, 0x08, 0x36, 0x41, 0x41}, // 123 173 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 174 |
    {0x41, 0x41, 0x36, 0x08, 0x00}, // 125 175 }
    {0x02, 0x01, 0x02, 0x04, 0x02}, // 126 176 ~
};

PROGMEM const char CursorIcon[5] = {
  0x7F, 0x22, 0x44, 0xB8, 0x50}; // pointer 5x8 turned 180 deg - sets cursor outline as filled pixels
PROGMEM const char CursorMask[5] = {
  0x00, 0x1C, 0x38, 0x40, 0x00}; // pointer mask 5x8 turned 180 deg - sets blank pixels inside cursor

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdSendCmd
 
 Description  :  Sends data to display controller.
 
 Argument(s)  :  data -> Data to be sent
 cd   -> Command or data (see/use enum)
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdSendCmd(byte data, int cd) {
  byte i = 8;
  byte mask;

  //
  if (cd == LCD_DATA) {
    digitalWrite(LCD_DC_PIN, HIGH);
  } 
  else {
    digitalWrite(LCD_DC_PIN, LOW) ;
  }

  while (0 < i) {
    mask = 0x01 << --i; // get bitmask and move to least significant bit
    // cause edge to fall
    digitalWrite(SPI_CLK_PIN, LOW); // tick
    delayMicroseconds(1);
    // set out byte
    if (data & mask) { // choose bit
      digitalWrite(SPI_MOSI_PIN, HIGH); // send 1
    } 
    else {
      digitalWrite(SPI_MOSI_PIN, LOW); // send 0
    }

    // cause edge to rise
    digitalWrite(SPI_CLK_PIN, HIGH); // tock
    delayMicroseconds(1);
  }
}

//end of part 1
/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdInit
 
 Description  :  Performs MCU SPI & LCD controller initialization.
 
 Argument(s)  :  None.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdInit(void) {
  int m;

  //  Toggle display reset pin.
  digitalWrite(LCD_RST_PIN, LOW);
  delay(40);
  digitalWrite(LCD_RST_PIN, HIGH);

  //  Enable SPI port: No interrupt, MSBit first, Master mode, CPOL->0, CPHA->0, Clk/4
  // SPCR = 0x50;

  LcdSendCmd(0x21, LCD_CMD);  // LCD Extended Commands.
  LcdSendCmd(0x99, LCD_CMD);  // Set LCD Vop (Contrast). //B3 //B1 Too dark
  LcdSendCmd(0x04, LCD_CMD);  // Set Temp coefficent. //0x04

  LcdSendCmd(0x13, LCD_CMD);  // LCD bias mode 1:48. //0x13
  LcdSendCmd(0x20, LCD_CMD);  // LCD Standard Commands, Horizontal addressing mode.

  LcdSendCmd(0x0C, LCD_CMD);  // LCD in normal mode. 0x0d for inverse
  // LcdSendCmd( 0x0D, LCD_CMD );

  //  Reset watermark pointers.
  LoWaterMark = LCD_CACHE_SIZE;
  HiWaterMark = 0;

  LcdClear();
  LcdUpdate();
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdContrast
 
 Description  :  Set display contrast.
 
 Argument(s)  :  contrast -> Contrast value from 0x00 to 0x7F.
 
 Return value :  None.
 
 Notes	  :  No change visible at ambient temperature.
 
 --------------------------------------------------------------------------------------------------*/
void LcdContrast(byte contrast) {
  //  LCD Extended Commands.
  LcdSendCmd(0x21, LCD_CMD);

  // Set LCD Vop (Contrast).
  LcdSendCmd(0x80 | contrast, LCD_CMD);

  //  LCD Standard Commands, horizontal addressing mode.
  LcdSendCmd(0x20, LCD_CMD);
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdClear
 
 Description  :  Clears the display. LcdUpdate must be called next.
 
 Argument(s)  :  None.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdClear (void) {
  int i;

  for (i = 0; i < LCD_CACHE_SIZE; i++) {
    LcdCache[i] = 0x00;
  }

  //  Reset watermark pointers.
  LoWaterMark = 0;
  HiWaterMark = LCD_CACHE_SIZE - 1;

  UpdateLcd = true;
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdGotoXY
 
 Description  :  Sets cursor location to xy location corresponding to basic font size.
 
 Argument(s)  :  x, y -> Coordinate for new cursor position. Range: 1,1 .. 14,6
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdGotoXY(byte x, byte y) {
  LcdCacheIdx = (x - 1) * 6 + (y - 1) * 84;
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdChr
 
 Description  :  Displays a character at current cursor location and increment cursor location.
 
 Argument(s)  :  size -> Font size. See enum.
 ch   -> Character to write.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdChr(int size, byte ch) {
  byte i, c;
  byte b1, b2;
  int tmpIdx;

  if (LcdCacheIdx < LoWaterMark) {
    //  Update low marker.
    LoWaterMark = LcdCacheIdx;
  }

  if (ch > 0x7F) {
    //  Convert to a printable character.
    ch = 0;
  }

  if (size == FONT_1X) {
    char buffer;
    for (i = 0; i < 5; i++) {
      buffer = pgm_read_byte(&(FontLookup[ch][i]));
      LcdCache[LcdCacheIdx++] = buffer << 1;
    }
  } 
  else if (size == FONT_2X) {
    tmpIdx = LcdCacheIdx - 84;

    if (tmpIdx < LoWaterMark) {
      LoWaterMark = tmpIdx;
    }

    if (tmpIdx < 0) return;

    char buffer;
    for (i = 0; i < 5; i++) {
      buffer = pgm_read_byte(&(FontLookup[ch][i]));
      c = buffer << 1;
      b1 = (c & 0x01) * 3;
      b1 |= (c & 0x02) * 6;
      b1 |= (c & 0x04) * 12;
      b1 |= (c & 0x08) * 24;

      c >>= 4;
      b2 = (c & 0x01) * 3;
      b2 |= (c & 0x02) * 6;
      b2 |= (c & 0x04) * 12;
      b2 |= (c & 0x08) * 24;

      LcdCache[tmpIdx++] = b1;
      LcdCache[tmpIdx++] = b1;
      LcdCache[tmpIdx + 82] = b2;
      LcdCache[tmpIdx + 83] = b2;
    }

    //  Update x cursor position.
    LcdCacheIdx += 11;
  }

  if (LcdCacheIdx > HiWaterMark) {
    //  Update high marker.
    HiWaterMark = LcdCacheIdx;
  }

  //  Horizontal gap between characters.
  LcdCache[LcdCacheIdx++] = 0x00;
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdStr
 
 Description  :  Displays a character at current cursor location and increment cursor location
 according to font size.
 
 Argument(s)  :  size    -> Font size. See enum.
 dataPtr -> Pointer to null terminated ASCII string to display.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdStr(int size, char *dataPtr) {
  while (*dataPtr) {
    LcdChr(size, *dataPtr++);
  }
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdPixel
 
 Description  :  Displays a pixel at given absolute (x, y) location.
 
 Argument(s)  :  x, y -> Absolute pixel coordinates
 mode -> Off, On or Xor. See enum.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdPixel(byte x, byte y, int mode) {
  word index;
  byte offset;
  byte data;

  if (x > LCD_X_RES) return;
  if (y > LCD_Y_RES) return;

  index = ((y / 8) * 84) + x;
  offset = y - ((y / 8) * 8);

  data = LcdCache[index];

  if (mode == PIXEL_OFF) {
    data &= (~(0x01 << offset));
  } 
  else if (mode == PIXEL_ON) {
    data |= (0x01 << offset);
  } 
  else if (mode  == PIXEL_XOR) {
    data ^= (0x01 << offset);
  }

  LcdCache[index] = data;

  if (index < LoWaterMark) {
    //  Update low marker.
    LoWaterMark = index;
  }

  if (index > HiWaterMark) {
    //  Update high marker.
    HiWaterMark = index;
  }
}

class SpriteBgCache {
  public:
    SpriteBgCache(void);
    void save(byte x, byte y, byte w, byte h);
    void restore(void);
  private:
    byte *_data;
    byte _x;
    byte _y;
    byte _w;
    byte _h;
    word _size;
    boolean _hasData;
    byte _startX;
    byte _endX;
    byte _startY;
    byte _endY;
};

SpriteBgCache bgCache;

SpriteBgCache::SpriteBgCache(void) {
  _data = 0;
  _x = 0;
  _y = 0;
  _w = 0;
  _h = 0;
  _size = 0;
  _hasData = false;
  _startX = 0;
  _endX = 0;
  _startY = 0;
  _endY = 0;
}

int spriteMode;

void SpriteBgCache::save(byte x, byte y, byte w, byte h) {
  if (spriteMode == PIXEL_XOR) {
    _hasData = true;
    return;
  }

  _x = x;
  _y = y;

#ifdef verbose
  Serial.print("spriteBgCacheX: ");
  Serial.println(_x, DEC);
  Serial.print("spriteBgCacheY: ");
  Serial.println(_y, DEC);
#endif

  _startX = x;
  if (_startX >= LCD_X_RES) {
    _startX = LCD_X_RES - 1;
  } else if (_startX < 0) {
    _startX = 0;
  }
  _endX = x + w - 1;
  if (_endX >= LCD_X_RES) {
    _endX = LCD_X_RES - 1;
  } else if (_endX < 0) {
    _endX = 0;
  }
  _startY = y / 8;
  if (_startY >= 6) { // 48 / 8bit = 6 rows
    _startY = 5; // rows are numbered 0 .. 5
  } else if (_startY < 0) {
    _startY = 0;
  }
  _endY = ((y + h) / 8);
  if ((y + h) % 8) {
    _endY += 1;
  }
  if (_endY >= 6) {
    _endY = 5;
  } else if (_endY < 0) {
    _endY = 0;
  }
#ifdef verbose
  Serial.print("spriteBgCacheSatrtX: ");
  Serial.println(_startX, DEC);
  Serial.print("spriteBgCacheEndX: ");
  Serial.println(_endX, DEC);
  Serial.print("spriteBgCacheStartY: ");
  Serial.println(_startY, DEC);
  Serial.print("spriteBgCacheEndY: ");
  Serial.println(_endY, DEC);
#endif
  _w = _endX - _startX + 1;
  _h = _endY - _startY + 1;
  _size = _w * _h;
#ifdef verbose
  Serial.print("spriteBgCacheW: ");
  Serial.println(_w, DEC);
  Serial.print("spriteBgCacheH: ");
  Serial.println(_w, DEC);
  Serial.print("spriteBgCacheSize: ");
  Serial.println(_size, DEC);
#endif
  byte spriteBg[_size];
  
#ifdef verbose
  Serial.println("Saving to bg cache from LcdCache");
#endif
  int spriteBgIndex = 0;
  for (byte j = _startY; j <= _endY; j++) {
    for (byte i = _startX; i <= _endX; i++) {
      spriteBg[spriteBgIndex] = LcdCache[i + (j * LCD_X_RES)];
#ifdef verbose
  Serial.print(i + (j * LCD_X_RES), DEC);
  Serial.print(":");
  Serial.print(LcdCache[i + (j * LCD_X_RES)], HEX);
  Serial.print(", ");
#endif
      spriteBgIndex++;
    }
  }

#ifdef verbose
  Serial.println();
#endif
  _data = (byte *) malloc (spriteBgIndex);
  for (int z = 0; z < spriteBgIndex; z++) {
    _data[z] = spriteBg[z];
  }
#ifdef verbose
  Serial.println("Saved data: ");
  for (int q = 0; q < spriteBgIndex; q++) {
    Serial.print(q, DEC);
    Serial.print(":");
    Serial.print(_data[q], HEX);
    Serial.print(", ");
  }
  Serial.println();
#endif
  _hasData = true;
}

void SpriteBgCache::restore(void) {
  if (spriteMode == PIXEL_XOR) {
    drawSpriteAt(_x, _y, PIXEL_XOR);
    return;
  }
  
#ifdef verbose
  Serial.println("Restoring from bg cache to LcdCache");
  Serial.print("sBCSY:");
  Serial.print(_startY, DEC);
  Serial.print(", sBCEY:");
  Serial.print(_endY, DEC);
  Serial.print(", sBCSX:");
  Serial.print(_startX, DEC);
  Serial.print(", sBCEX:");
  Serial.println(_endX, DEC);
  Serial.println("Data to restore: ");
  for (int q = 0; q < _size; q++) {
    Serial.print(q, DEC);
    Serial.print(":");
    Serial.print(_data[q], HEX);
    Serial.print(", ");
  }
  Serial.println();
#endif
  int spriteBgIndex = 0;
  for (byte j = _startY; j <= _endY; j++) {
    for (byte i = _startX; i <= _endX; i++) {
      LcdCache[i + (j * LCD_X_RES)] = _data[spriteBgIndex];
#ifdef verbose
  Serial.print(i + (j * LCD_X_RES), DEC);
  Serial.print(":");
  Serial.print(_data[spriteBgIndex], HEX);
  Serial.print(", ");
#endif
      spriteBgIndex++;
    }
  }
#ifdef verbose
  Serial.println();
#endif

  free(_data);
  _data = 0;
  _x = 0;
  _y = 0;
  _w = 0;
  _h = 0;
  _startX = 0;
  _endX = 0;
  _startY = 0;
  _endY = 0;
  _size = 0;
  _hasData = false;
}

void drawSpriteAt(byte x, byte y, int mode) {
  spriteMode = mode;

  bgCache.save(x, y, 5, 8);
  char buffer;

  for (byte i = 0; i < 5; i++) {
    for (byte j = 0; j < 8; j++) {
      if ((x + i) > (LCD_X_RES - 1)) {
        break;
      }
      if ((y + j) > (LCD_Y_RES - 1)) {
        break;
      }
      buffer = pgm_read_byte(&(CursorIcon[i]));
      if (buffer & 1 << j) {
        LcdPixel(x + i, y + j, mode);
      }
    }
  }

  int mask_mode = PIXEL_OFF;
  if (mode == PIXEL_XOR) {
    return;
  } else if (mode == PIXEL_OFF) {
    mask_mode = PIXEL_ON;
  }

  for (byte i = 0; i < 5; i++) {
    for (byte j = 0; j < 8; j++) {
      if ((x + i) > (LCD_X_RES - 1)) {
        break;
      }
      if ((y + j) > (LCD_Y_RES - 1)) {
        break;
      }
      buffer = pgm_read_byte(&(CursorMask[i]));
      if (buffer & 1 << j) {
        LcdPixel(x + i, y + j, mask_mode);
      }
    }
  }
}

void hideSprite(void) {
  bgCache.restore();
}

void moveSpriteTo(byte x, byte y) {
  hideSprite();
  drawSpriteAt(x, y, spriteMode);
  cursorX = x;
  cursorY = y;
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdLine
 
 Description  :  Draws a line between two points on the display.
 
 Argument(s)  :  x1, y1 -> Absolute pixel coordinates for line origin.
 x2, y2 -> Absolute pixel coordinates for line end.
 mode   -> Off, On or Xor. See enum.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdLine(byte x1, byte y1, byte x2, byte y2, int mode) {
  int dx, dy, stepx, stepy, fraction;

  dy = y2 - y1;
  dx = x2 - x1;

  if (dy < 0) {
    dy = -dy;
    stepy = -1;
  } 
  else {
    stepy = 1;
  }

  if (dx < 0) {
    dx = -dx;
    stepx = -1;
  } 
  else {
    stepx = 1;
  }

  dx <<= 1;
  dy <<= 1;

  LcdPixel(x1, y1, mode);

  if (dx > dy) {
    fraction = dy - (dx >> 1);
    while (x1 != x2) {
      if (fraction >= 0) {
        y1 += stepy;
        fraction -= dx;
      }
      x1 += stepx;
      fraction += dy;
      LcdPixel(x1, y1, mode);
    }
  } 
  else {
    fraction = dx - (dy >> 1);
    while (y1 != y2) {
      if (fraction >= 0) {
        x1 += stepx;
        fraction -= dy;
      }
      y1 += stepy;
      fraction += dx;
      LcdPixel(x1, y1, mode);
    }
  }

  UpdateLcd = true;
}

/*--------------------------------------------------------------------------------------------------
 
 Name	   :  LcdUpdate
 
 Description  :  Copies the LCD cache into the device RAM.
 
 Argument(s)  :  None.
 
 Return value :  None.
 
 --------------------------------------------------------------------------------------------------*/
void LcdUpdate(void) {
  int i;

  //TEMP: always update everything for the moment
  LoWaterMark = 0;
  HiWaterMark = LCD_CACHE_SIZE - 1;

  if (LoWaterMark < 0) {
    LoWaterMark = 0;
  } 
  else if (LoWaterMark >= LCD_CACHE_SIZE) {
    LoWaterMark = LCD_CACHE_SIZE - 1;
  }

  if (HiWaterMark < 0) {
    HiWaterMark = 0;
  } 
  else if (HiWaterMark >= LCD_CACHE_SIZE) {
    HiWaterMark = LCD_CACHE_SIZE - 1;
  }

  //  Set base address according to LoWaterMark.
  LcdSendCmd(0x80 | (LoWaterMark % LCD_X_RES), LCD_CMD);
  LcdSendCmd(0x40 | (LoWaterMark / LCD_X_RES), LCD_CMD);

  //  Serialize the video buffer.
  for (i = LoWaterMark; i <= HiWaterMark; i++) {
    LcdSendCmd(LcdCache[i], LCD_DATA);
  }

  //  Reset watermark pointers.
  LoWaterMark = LCD_CACHE_SIZE - 1;
  HiWaterMark = 0;

  UpdateLcd = false;
}

/*
 * initialize the mouse. Reset it, and place it into remote
 * mode, so we can get the encoder data on demand.
 */
void mouse_init() {
  mouse.write(0xff);  // reset
  mouse.read();  // ack byte
  mouse.read();  // blank */
  mouse.read();  // blank */
  mouse.write(0xf0);  // remote mode
  mouse.read();  // ack
  delayMicroseconds(100);
  mouse.write(0xe8);  // set resolution
  mouse.read();  // ack
  mouse.write(0x00);  // 1 count per mm from default 4 counts per mm
  mouse.read();  // ack
  delayMicroseconds(100);
}

char IM_mouse_flag = 0;

void setup() {
  Serial.begin(9600);
  //power control led
  pinMode(CONTROL_LED,OUTPUT);

  pinMode(LCD_RST_PIN,OUTPUT);
  pinMode(LCD_DC_PIN ,OUTPUT);
  pinMode(LCD_CE_PIN ,OUTPUT);
  pinMode(SPI_MOSI_PIN ,OUTPUT);
  pinMode(SPI_CLK_PIN,OUTPUT);

  //power the display
  //not used in this sketch. the display was powered permanently
  // pinMode(LCD_POWER_PIN,OUTPUT);

  //start
  digitalWrite(CONTROL_LED, HIGH);
  LcdInit();
  LcdContrast(0x19);

  LcdStr(FONT_1X, "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\040\041");
  drawSpriteAt(cursorX, cursorY, PIXEL_ON);
  LcdUpdate();

  mouse_init();
}

void loop() {
  char mstat;
  char mx;
  char my;

  /* get a reading from the mouse */
  mouse.write(0xeb);  // give me data!
  mouse.read();      // ignore ack
  mstat = mouse.read();
  mx = mouse.read();
  my = mouse.read();
  if (mx > 1) mx >>= 1;
  if (my > 1) my >>= 1;
  
  int newX = cursorX + mx;
  int newY = cursorY + my;

  if (newX >= LCD_X_RES) {
    newX = LCD_X_RES - 1;
  } 
  else if (newX < 0) {
    newX = 0;
  }

  if (newY >= LCD_Y_RES) {
    newY = LCD_Y_RES - 1;
  } 
  else if (newY < 0) {
    newY = 0;
  }

  if (newX != cursorX || newY != cursorY) {
    moveSpriteTo(newX, newY);
    LcdUpdate();
  }
}


