#ifndef _bsp_H_
#define _bsp_H_

#include  <msp430g2553.h>          // MSP430x2xx
//#include  <msp430xG46x.h>  // MSP430x4xx


#define   debounceVal      250
#define   LEDs_SHOW_RATE   0xFFFF  // 62_5ms

// UDS abstraction
#define UDSDir            P1DIR
#define UDSSel            P2SEL
#define UDSSelP1          P1SEL


// UART abstraction
#define UARTSel           P1SEL
#define UARTSel2          P1SEL2
#define UARTDir           P1DIR
#define UARTOut           P1OUT
#define TXD               BIT2
#define RXD               BIT1


// SERVO abstraction
#define SERVODir            P2DIR
#define SERVOSel            P2SEL


// PushButtons abstraction
#define PBsArrPort2    P2IN

#define PBsArrIntPend2     P2IFG
#define PBsArrIntEn2       P2IE
#define PBsArrIntEdgeSel2  P2IES
#define PBsArrPortSel2     P2SEL 
#define PBsArrPortDir2     P2DIR

#define PB0                0x01



// #define CHECKBUSY    1  // using this define, only if we want to read from LCD

#ifdef CHECKBUSY
    #define LCD_WAIT lcd_check_busy()
#else
    #define LCD_WAIT DelayMs(5)
#endif

#define FOURBIT_MODE    0x0
#define EIGHTBIT_MODE   0x1
#define LCD_MODE        FOURBIT_MODE
   
#define OUTPUT_PIN      1
#define INPUT_PIN       0
#define OUTPUT_DATA     (LCD_MODE ? 0xFF : (0x0F << LCD_DATA_OFFSET))
#define INPUT_DATA      0x00

#define LCD_STROBE_READ(value)  LCD_EN(1), \
                asm("nop"), asm("nop"), \
                value=LCD_DATA_READ, \
                LCD_EN(0)

#define lcd_cursor(x)       lcd_cmd(((x)&0x7F)|0x80)
#define lcd_clear()     lcd_cmd(0x01)
#define lcd_putchar(x)      lcd_data(x)
#define lcd_goto(x)     lcd_cmd(0x80+(x))
#define lcd_cursor_right()  lcd_cmd(0x14)
#define lcd_cursor_left()   lcd_cmd(0x10)
#define lcd_display_shift() lcd_cmd(0x1C)
#define lcd_home()      lcd_cmd(0x02)
#define cursor_off              lcd_cmd(0x0C)
#define cursor_on               lcd_cmd(0x0F) 
#define lcd_function_set        lcd_cmd(0x3C) // 8bit,two lines,5x10 dots 
#define lcd_new_line            lcd_cmd(0xC0)                                  

#define LCD_EN(a)   (!a ? (P1OUT&=~0X10) : (P1OUT|=0X10)) // P1.4 is lcd enable pin
#define LCD_EN_DIR(a)   (!a ? (P1DIR&=~0X10) : (P1DIR|=0X10)) // P1.4 pin direction

#define LCD_RS(a)   (!a ? (P1OUT&=~0X20) : (P1OUT|=0X20)) // P1.5 is lcd RS pin
#define LCD_RS_DIR(a)   (!a ? (P1DIR&=~0X20) : (P1DIR|=0X20)) // P1.5 pin direction
  
#define LCD_RW(a)   (!a ? (P1OUT&=~0X80) : (P1OUT|=0X80)) // P1.7 is lcd RW pin
#define LCD_RW_DIR(a)   (!a ? (P1DIR&=~0X80) : (P1DIR|=0X80)) // P1.7 pin direction

#define LCD_DATA_OFFSET 0x04 //data pin selection offset for 4 bit mode, variable range is 0-4, default 0 - Px.0-3, no offset
   
#define LCD_DATA_WRITE  P2OUT
#define LCD_DATA_DIR    P2DIR
#define LCD_DATA_READ   P2IN

extern void GPIOconfig(void);
extern void TIMERconfig(void);
extern void ADCconfig(void);
extern void UDSconfig(void);
extern void UARTconfig(void);
extern void SERVOconfig(void);

#endif



