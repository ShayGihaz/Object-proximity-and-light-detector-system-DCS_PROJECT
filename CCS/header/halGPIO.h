#ifndef _halGPIO_H_
#define _halGPIO_H_

#include  "../header/bsp.h"         // private library - BSP layer
#include  "../header/app.h"         // private library - APP layer
#include  <stdio.h>



extern enum FSMstate state;   // global variable
extern enum SYSmode lpm_mode; // global variable





extern void sysConfig(void);
extern void set_flash();

extern void delay(unsigned int);
extern void enterLPM(unsigned char);

extern void enable_interrupts();
extern void disable_interrupts();

extern void lcd_init();
extern void lcd_cmd(unsigned char cmd);
extern void lcd_data(unsigned char data);
extern void TimerWait(int number);
extern void lcd_puts(const char * s);

extern void start_object_timers();
extern void start_SemiCircle_timer();
extern void set_SemiCircle_timer(int num);
extern void stopPWM();

extern int get_angle();
extern void send_distance();
extern void stop_USDscan();

extern void send_distance_list();
extern void send_done();
extern void distance_append();
extern void set_calibration_flag(char oldstate);


extern void startADC_ldr1();
extern void startADC_ldr2();
extern void stopADC();
extern void sendADC_value(int value);
extern void sendNewlineREQ();
extern void ADC_append();
extern void ADC_idx_reset();
extern void send_ADC_list();
extern char calibrated();
extern void send_scan_error();
extern int return_ldr_avarage();




extern void count_down_LCD(int num);
extern void count_up_LCD(int num);
extern void set_delay(int new_delay);
extern void pixel_char_moving(char c);
extern void script_delay();
extern void servo_deg(int degree);
extern void send_angle(int degree);
extern void send_distance_list_script(int scan_counter);
extern void servo_scan_script(int left, int right);
extern void stop_write_flash();
extern void erase_flash();
extern int get_script_size(int script);
extern void send_char(char c);

extern void disable_PB0_INT();
extern void enable_PB0_INT();

extern struct script_segment scripts;
extern int got_script;
extern char *flash_ptrB;
extern char *flash_ptrC;
extern char *flash_ptrD;
extern int flash_B_flag;
extern int flash_C_flag;
extern int flash_D_flag;
extern char prev_calibrate_state;

extern __interrupt void Timer_A(void);
extern __interrupt void DMA_ISR(void);
extern __interrupt void USCI0RX_ISR(void);
extern __interrupt void USCI0TX_ISR(void);
extern __interrupt void timer_a01(void);



#endif


