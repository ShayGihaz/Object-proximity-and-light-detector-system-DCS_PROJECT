#include  "../header/halGPIO.h"     // private library - HAL layer


#define NUM_OF_SCANS        61

int ldr1[10];
int ldr2[10];
unsigned int ldr_idx = 0;
unsigned int str_ldr_idx = 0;
unsigned int ADC_idx = 0;
char timerwait_flag = '0';
char read = '0';
char ldr = '1';
char str_ldr[10];
unsigned int distance_list[NUM_OF_SCANS];
unsigned int ADC_list[NUM_OF_SCANS];
unsigned int distance_idx = 0;
char done_flag = '0';
unsigned int i = 0;
unsigned int j = 0;
unsigned int diff;
int flag = 0;
unsigned int c = 0;
unsigned int angle_idx = 0;
char str_angle[5];
int angle;
char str_dis[7];
unsigned int dis_idx = 0;
unsigned int temp[2];
char calibration_flag = '1';  // 1 not ready, 0 is calibrated!!!!!! 4 is cancel from PC
unsigned int Xdelay = 0x5555;  // halfsec
unsigned int Xdelay_counter = 0;
const unsigned int delay_10ms = 0x01B6;  //0x01b4
int got_script = 0;
int temp_script_size = 0;
char *flash_ptrB;
char *flash_ptrC;
char *flash_ptrD;
int flash_B_flag = 0;
int flash_C_flag = 0;
int flash_D_flag = 0;
char prev_calibrate_state = '1';


struct script_segment{
    int script_counter;
    char *script_name[3];// pointer array to scripts names    ??
    char *ptr_arr[3];   // pointer array to the start of the script
    int script_size[3];  // every file size
};

struct script_segment scripts;
//--------------------------------------------------------------------
//             System Configuration  
//--------------------------------------------------------------------
void sysConfig(void){ 
    GPIOconfig();
   // flag = 1;
    UDSconfig();
   // flag = 2;
    UARTconfig();
    SERVOconfig();
    ADCconfig();
    scripts.script_counter = 0;
    scripts.script_name[0] = "1";
    scripts.script_name[1] = "2";
    scripts.script_name[2] = "3";
    scripts.ptr_arr[0] = (char *)0x1080;  // segment B
    scripts.ptr_arr[1] = (char *)0x1040;  // segment C
    scripts.ptr_arr[2] = (char *)0x1000;  // segment D
    set_flash();

}

void set_flash(){
    erase_flash();
    *scripts.ptr_arr[0] = 0;  // segment B
    stop_write_flash();
    erase_flash();
    *scripts.ptr_arr[1] = 0;  // segment C
    stop_write_flash();
    erase_flash();
    *scripts.ptr_arr[2] = 0;  // segment D
    stop_write_flash();


}

//---------------------------------------------------------------------
//            Polling based Delay function
//---------------------------------------------------------------------
void delay(unsigned int t){  // t[msec]
    volatile unsigned int i;

    for(i=t; i>0; i--);
}

//******************************************************************
// LCD
//******************************************************************

//******************************************************************
// Delay usec functions
//******************************************************************
void DelayUs(unsigned int cnt){
  
    unsigned char i;
        for(i=cnt ; i>0 ; i--) asm("nOp"); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
// Delay msec functions
//******************************************************************
void DelayMs(unsigned int cnt){
  
    unsigned char i;
        for(i=cnt ; i>0 ; i--) DelayUs(1000); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
// lcd strobe functions
//******************************************************************
void lcd_strobe(){
  LCD_EN(1);
  asm("noP");
  asm("Nop");
  LCD_EN(0);
}

void lcd_cmd(unsigned char c){
  
    LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

    if (LCD_MODE == FOURBIT_MODE)
    {
        LCD_DATA_WRITE &= ~OUTPUT_DATA;// clear bits before new write
                LCD_DATA_WRITE |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
        lcd_strobe();
                LCD_DATA_WRITE &= ~OUTPUT_DATA;
            LCD_DATA_WRITE |= (c & (0x0F)) << LCD_DATA_OFFSET;
        lcd_strobe();
    }
    else
    {
        LCD_DATA_WRITE = c;
        lcd_strobe();
    }
}
//******************************************************************
// send data to the LCD
//******************************************************************
void lcd_data(unsigned char c){
        
    LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

    LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_RS(1);
    if (LCD_MODE == FOURBIT_MODE)
    {
            LCD_DATA_WRITE &= ~OUTPUT_DATA;
                LCD_DATA_WRITE |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;  
        lcd_strobe();
                LCD_DATA_WRITE &= (0xF0 << LCD_DATA_OFFSET) | (0xF0 >> 8 - LCD_DATA_OFFSET);
                LCD_DATA_WRITE &= ~OUTPUT_DATA;
        LCD_DATA_WRITE |= (c & 0x0F) << LCD_DATA_OFFSET;
        lcd_strobe();
    }
    else
    {
        LCD_DATA_WRITE = c;
        lcd_strobe();
    }
          
    LCD_RS(0);
}
//******************************************************************
// write a string of chars to the LCD
//******************************************************************
void lcd_puts(const char * s){
  
    while(*s)
        lcd_data(*s++);
}
//******************************************************************
// initialize the LCD
//******************************************************************
void lcd_init(){
  
    char init_value;

    if (LCD_MODE == FOURBIT_MODE) init_value = 0x3 << LCD_DATA_OFFSET;
        else init_value = 0x3F;

    LCD_RS_DIR(OUTPUT_PIN);
    LCD_EN_DIR(OUTPUT_PIN);
    LCD_RW_DIR(OUTPUT_PIN);
        LCD_DATA_DIR |= OUTPUT_DATA;
        LCD_RS(0);
    LCD_EN(0);
    LCD_RW(0);
        
    DelayMs(15);
        LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_DATA_WRITE |= init_value;
    lcd_strobe();
    DelayMs(5);
        LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_DATA_WRITE |= init_value;
    lcd_strobe();
    DelayUs(200);
        LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_DATA_WRITE |= init_value;
    lcd_strobe();

    if (LCD_MODE == FOURBIT_MODE){
        LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h
                LCD_DATA_WRITE &= ~OUTPUT_DATA;
        LCD_DATA_WRITE |= 0x2 << LCD_DATA_OFFSET; // Set 4-bit mode
        lcd_strobe();
        lcd_cmd(0x28); // Function Set
    }
        else lcd_cmd(0x3C); // 8bit,two lines,5x10 dots 

    lcd_cmd(0xF); //Display On, Cursor On, Cursor Blink
    lcd_cmd(0x1); //Display Clear
    lcd_cmd(0x6); //Entry Mode
    lcd_cmd(0x80); //Initialize DDRAM address to zero
}

//---------------------------------------------------------------------
//            Enable interrupts
//---------------------------------------------------------------------
void enable_interrupts(){
  _BIS_SR(GIE);
}
//---------------------------------------------------------------------
//            Disable interrupts
//---------------------------------------------------------------------
void disable_interrupts(){
  _BIC_SR(GIE);
}


// ------------------------------------------------------------------
//                  STATE 1
// ------------------------------------------------------------------

void start_object_timers(){
    i = 0;
    TA0CTL |= MC_1;
    TA1CTL |= MC_2;

}

void distance_append(){
    if (distance_idx > NUM_OF_SCANS - 1){
        distance_idx = 0;
    }
    distance_list[distance_idx] = diff;
    distance_idx ++;
}


void send_distance_list(){
    for (distance_idx = 0; distance_idx < NUM_OF_SCANS; distance_idx++){
        sprintf(str_dis, "%d", distance_list[distance_idx]);
        dis_idx = 0;
        IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
        enterLPM(lpm_mode);
        if ((state != state1) && (state != state4)){
            IE2 &= ~UCA0TXIE;
            break;
        }
    }
    if (state == state4){
        done_flag = '0';
    }
}



// ------------------------------------------------------------------
//                  STATE 2
//WHEN TIMER_A1 COUNTER IS LESS THEN TA1CCR1 VALUE --> PWM WITH HIGH PULSE
////WHEN TIMER_A1 COUNTER IS LESS THEN TA1CCR1 VALUE --> PWM WITH LOW PULSE
// ------------------------------------------------------------------
void set_SemiCircle_timer(int num){
    flag = 2;
    TA1CCR0 = 20000;           //PWM period
    TA1CCR1 = num;             //CCR1 PWM Duty Cycle  !min 350 max 2350 angle 200, //350 2350-200 degrees
    TA1CCTL1 = OUTMOD_7;       //CCR1 selection reset-set
    TA1CTL = TASSEL_2 + MC_1;

}




void stopPWM(){
    TA1CTL = TACLR + MC_0;
    TA1CTL &= 0x00;
    TA1CCTL0 &= 0x0000;
    TA1CCTL1 &= 0x0000;
    TA1CCTL2 &= 0x0000;
    TA1CCR1 = 0;
    TA1CCR0 = 0;
    TA0CTL = TACLR + MC_0;
    TA0CTL &= 0x00;
    TA0CCTL0 &= 0x0000;
    TA0CCTL1 &= 0x0000;
    TA0CCTL2 &= 0x0000;
    TA0CCR0 = 0;



}


char read1(){
    return read;
}

// ------------------------------------------------------------------
//                  STATE 2 - Telemeter
// ------------------------------------------------------------------
int get_angle(){
    angle = 0;
    for (angle_idx = 0; str_angle[angle_idx] != '\0' && str_angle[angle_idx] != '\n'; angle_idx++) {        // SAME IMPLANTATION AS ATOI() - FOR SOME REASON SSCANF() DIDNT WORK
            angle = angle * 10 + (str_angle[angle_idx] - 48);
    }
    angle_idx = 0;
    return angle;
}


void send_distance(){

    sprintf(str_dis, "%d", diff);
    dis_idx = 0;
    IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
}


void stop_USDscan(){
    TA0CTL &= ~MC_3;
    TA1CTL &= ~MC_3;
}




// ------------------------------------------------------------------
//                       STATE 3
// ------------------------------------------------------------------
void set_calibration_flag(char oldstate){
    ldr_idx = 0;
    calibration_flag = oldstate;  // 1 not ready, 0 is calibrated!!!!!! 4 is cancel from PC
}

void startADC_ldr1(){
    ldr = '1';
    ADC10CTL1 = INCH_0 + ADC10SSEL_3;     // CHANNALE 0 (1.0 <-> A0) , SMCLK
    ADC10CTL0 |= ENC + ADC10SC + SHS_0; //start the sample
}


void startADC_ldr2(){
    ldr = '2';
    ADC10CTL1 = INCH_3 + ADC10SSEL_3;     // CHANNALE 3 (1.3 <-> A3) , SMCLK
    ADC10CTL0 |= ENC + ADC10SC + SHS_0; //start the sample
}


void stopADC(){
    ADC10CTL0 &= ~ADC10ON;  //turn off ADC
}

void sendADC_value(int value){
    sprintf(str_ldr, "%d", value);
    ldr_idx++;
    str_ldr_idx = 0;
    IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
    //enterLPM(lpm_mode);
}


int return_ldr_avarage(){
    return ((ldr1[ldr_idx] + ldr2[ldr_idx]) >> 1);
}

void send_scan_error(){
    UCA0TXBUF = 'E';
    while (!(IFG2&UCA0TXIFG));
}


void sendNewlineREQ(){
    UCA0TXBUF = 'n';
    while (!(IFG2&UCA0TXIFG));
}

void send_done(){
    UCA0TXBUF = 'd';
    while (!(IFG2&UCA0TXIFG));
    calibration_flag = '0';
}

void ADC_idx_reset(){
    ADC_idx = 0;
}

void ADC_append(){
    if (ADC_idx > NUM_OF_SCANS - 1){
        ADC_idx = 0;
    }
    ADC_list[ADC_idx] = (ldr1[ldr_idx] + ldr2[ldr_idx]) >> 1;
    ADC_idx ++;
}

void send_ADC_list(){
    for (ADC_idx = 0; ADC_idx < NUM_OF_SCANS; ADC_idx++){
            sprintf(str_ldr, "%d", ADC_list[ADC_idx]);
            str_ldr_idx = 0;
            IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
            //enterLPM(lpm_mode);
            if ((state != state3) && (state != state4)){
                IE2 &= ~UCA0TXIE;
                break;
            }
        }
    if (state == state4){
        done_flag = '1';
    }
}


char calibrated(){
    return calibration_flag;
}




// ------------------------------------------------------------------
//                       STATE 5
// ------------------------------------------------------------------

void count_up_LCD(int num){
    int temp_got_script, temp_script_size1, temp_flagB, temp_flagC, temp_flagD, temp_cliab_flag, temp_xDel, temp_xcounter;
    temp_got_script = got_script;
    temp_script_size1 = temp_script_size;
    temp_flagB = flash_B_flag;
    temp_flagC = flash_C_flag;
    temp_flagD = flash_D_flag;
    temp_cliab_flag = calibration_flag;
    temp_xDel = Xdelay;
    temp_xcounter = Xdelay_counter;
    // for some reason, the sprintf() overwrite these variables in RAM.
    // we are 12 hours before submission, therefore the solution is easy and dumb.
    char counter_str[4];
    int counter_idx;
    for (counter_idx = 0; counter_idx <= num; counter_idx ++){
        if (state != state5){
            break;
        }
        sprintf(counter_str, "%d", counter_idx);
        got_script = temp_got_script;
        temp_script_size = temp_script_size1;
        flash_B_flag = temp_flagB;
        flash_C_flag = temp_flagC;
        flash_D_flag = temp_flagD;
        calibration_flag = temp_cliab_flag;
        Xdelay = temp_xDel;
        Xdelay_counter = temp_xcounter;
        // for some reason, the sprintf() overwrite these variables in RAM.
        // we are 12 hours before submission, therefore the solution is easy and dumb.
        lcd_clear();
        lcd_puts(counter_str);
        script_delay();
    }
}


void count_down_LCD(int num){
    int temp_got_script, temp_script_size1, temp_flagB, temp_flagC, temp_flagD, temp_cliab_flag, temp_xDel, temp_xcounter;
    temp_got_script = got_script;
    temp_script_size1 = temp_script_size;
    temp_flagB = flash_B_flag;
    temp_flagC = flash_C_flag;
    temp_flagD = flash_D_flag;
    temp_cliab_flag = calibration_flag;
    temp_xDel = Xdelay;
    temp_xcounter = Xdelay_counter;
    // for some reason, the sprintf() overwrite these variables in RAM.
    // we are 12 hours before submission, therefore the solution is easy and dumb.
    char counter_str[4];
    int counter_idx;
    sprintf(counter_str, "%d", num);
    for (counter_idx = num; counter_idx >= 0; counter_idx --){
        if (state != state5){
            break;
        }
        sprintf(counter_str, "%d", counter_idx);
        got_script = temp_got_script;
        temp_script_size = temp_script_size1;
        flash_B_flag = temp_flagB;
        flash_C_flag = temp_flagC;
        flash_D_flag = temp_flagD;
        calibration_flag = temp_cliab_flag;
        Xdelay = temp_xDel;
        Xdelay_counter = temp_xcounter;
        // for some reason, the sprintf() overwrite these variables in RAM.
        // we are 12 hours before submission, therefore the solution is easy and dumb.
        lcd_clear();
        lcd_puts(counter_str);
        script_delay();
    }
}

void script_delay(){
    if (Xdelay_counter > 0){
            TimerWait(0xFFFE);
    }
    if (Xdelay > 0){
        TimerWait(Xdelay);
    }
}

void set_delay(int new_delay){
    int idx;
    Xdelay_counter = 0;
    while(new_delay > 150){
        Xdelay_counter ++;
        new_delay = new_delay - 150;
    }
    Xdelay = 0;
    for(idx = 0; idx < new_delay; idx++){  // no multiple in MSP so for loop of adders
        Xdelay = Xdelay + delay_10ms;
    }
}

void pixel_char_moving(char c){
    int idx;
    for (idx = 0; idx < 16; idx++)
    {
        lcd_clear();
        lcd_goto(idx);
        lcd_data(c);
        script_delay();
    }
    lcd_clear();
    lcd_new_line;
    for (idx = 0; idx < 16; idx++){
        lcd_data(c);
        script_delay();
        lcd_cursor_left();
        lcd_data(' ');
    }

}


void servo_deg(int degree){
    set_SemiCircle_timer(degree);
    TimerWait(0xFFFF);
    stopPWM();
    enable_interrupts();
    UDSconfig();
    start_object_timers();
    enterLPM(lpm_mode);
    if (state != state5){
        return;
    }
    stopPWM();
    send_distance();
    enterLPM(lpm_mode);
    //done_flag = '1';
    //send_angle(degree);
    //done_flag = '0';
}




void servo_scan_script(int left, int right){
    unsigned int k;
    distance_idx = 0;
    int scan_counter = 0;
    enable_interrupts();
    for(k=left; k<=right; k = k + 30){ // IF CHANGING ANGLES, CHANGE THE 60!
        //if (k == left + 90){
         //   set_SemiCircle_timer(left+140);
       // }
       //else{
            set_SemiCircle_timer(k);
       // }
        TimerWait(0x2AAA);  // quatersec
        stopPWM();
        UDSconfig();
        start_object_timers();
        enterLPM(lpm_mode);
        distance_append();
        scan_counter ++;
        if (state != state5){
            stopPWM();
            break;
        }
    }
    stopPWM();
    send_distance_list_script(scan_counter);
    //done_flag = '1';
    //send_angle(left);  WE FIRST OF ALL SEND THE DEGREES AND THEN START TO SCAN
    //send_angle(right); THIS IS DONE PREVIOUSLLY IN THE READ_FALSH FUNCTION
    //done_flag = '0';
}


void send_distance_list_script(int scan_counter){
    int temp_got_script, temp_script_size1, temp_flagB, temp_flagC, temp_flagD, temp_cliab_flag, temp_xDel, temp_xcounter;
    temp_got_script = got_script;
    temp_script_size1 = temp_script_size;
    temp_flagB = flash_B_flag;
    temp_flagC = flash_C_flag;
    temp_flagD = flash_D_flag;
    temp_cliab_flag = calibration_flag;
    temp_xDel = Xdelay;
    temp_xcounter = Xdelay_counter;
    // for some reason, the sprintf() overwrite these variables in RAM.
    // we are 12 hours before submission, therefore the solution is easy and dumb.
    for (distance_idx = 0; distance_idx < scan_counter; distance_idx++){
        sprintf(str_dis, "%d", distance_list[distance_idx]);
        got_script = temp_got_script;
        temp_script_size = temp_script_size1;
        flash_B_flag = temp_flagB;
        flash_C_flag = temp_flagC;
        flash_D_flag = temp_flagD;
        calibration_flag = temp_cliab_flag;
        Xdelay = temp_xDel;
        Xdelay_counter = temp_xcounter;
        // for some reason, the sprintf() overwrite these variables in RAM.
        // we are 12 hours before submission, therefore the solution is easy and dumb.
        dis_idx = 0;
        done_flag = '1';
        IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
        enterLPM(lpm_mode);

        if (state != state5){
            IE2 &= ~UCA0TXIE;
            break;
        }
    }
}

void send_angle(int degree){
    int temp_got_script, temp_script_size1, temp_flagB, temp_flagC, temp_flagD, temp_cliab_flag, temp_xDel, temp_xcounter;
    temp_got_script = got_script;
    temp_script_size1 = temp_script_size;
    temp_flagB = flash_B_flag;
    temp_flagC = flash_C_flag;
    temp_flagD = flash_D_flag;
    temp_cliab_flag = calibration_flag;
    temp_xDel = Xdelay;
    temp_xcounter = Xdelay_counter;
    // for some reason, the sprintf() overwrite these variables in RAM.
    // we are 12 hours before submission, therefore the solution is easy and dumb.
    sprintf(str_angle, "%d", degree);
    got_script = temp_got_script;
    temp_script_size = temp_script_size1;
    flash_B_flag = temp_flagB;
    flash_C_flag = temp_flagC;
    flash_D_flag = temp_flagD;
    calibration_flag = temp_cliab_flag;
    Xdelay = temp_xDel;
    Xdelay_counter = temp_xcounter;
    // for some reason, the sprintf() overwrite these variables in RAM.
    // we are 12 hours before submission, therefore the solution is easy and dumb.
    angle_idx = 0;
    done_flag = '0';
    IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
    //enterLPM(lpm_mode);
}


void erase_flash(){
    FCTL2 = FWKEY + FSSEL0 + FN1;             // set key 0xA5, setting clock ACLK (as in example), divider 2
    FCTL1 = FWKEY + ERASE;                    // Set Erase bit
    FCTL3 = FWKEY;                            // Clear Lock bit

    // need to set write bit. in the script case, we set it up in the TX interrupt vector
}

void stop_write_flash(){
    FCTL1 = FWKEY;                            // Clear WRT bit
    FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}

int get_script_size(int script){
    return scripts.script_size[script-1];
}

void send_char(char c){
    UCA0TXBUF = c;
    while (!(IFG2&UCA0TXIFG));
}

//---------------------------------------------------------------------
//            Timer Wait time with timer A with SMclk --> 1sec
// TA0 works in 2^20 Hz plus divider 8 = 2^17. in this timer we work in up\down
// mode until 0xFFFF --> 
//                    (0xFFFF + 1) * SMCLK/16
// Because of the up-down mode, we will do it twice, so we will get a 1sec wait
//---------------------------------------------------------------------
void TimerWait(int number){  // CCR0 get interrupt only at TACCR0 value. therefore up-down + up equal  3 * CCR0. so if we want 1 sec then --> (TACCR0 * 2)/3, TACCR0 --> 0xAAAA
  
  TACCTL0 = CCIE; // INTERRUP ENABLE
  TACCR0 =number;
  TACTL = TASSEL_2 + MC_3 + ID_3;  // SMCLK, UP\DOWN MODE, DIV 8
  
  enable_interrupts();
  enterLPM(lpm_mode);
  enterLPM(lpm_mode);
  //TACCTL0 &=~ CCIE;
  //TACTL &= ~MC_3;
}



void disable_PB0_INT(){
    PBsArrIntEn2 &= 0xFE;              // interrrupt disable PB0
}

void enable_PB0_INT(){
    PBsArrIntPend2 &= ~PB0;
    PBsArrIntEn2 |= 0x01;              // interrrupt enable PB0
}


//*********************************************************************
//              UART Interrupt Service Rotine - RX

//        Legend:
//        -- 's', 'r' --> exit LPM0 and continue program
//        -- 'p'      --> exit LPM0 and play script
//        -- 'Z'      --> end of program and return to state0
//        -- 'c'      --> LDR calibration, calibration_flag = 1
//        -- 'd'      -->LDR calibration, calibration_flag = 0
// * calibration_flag = 0 mean LDR is calibrated, calibration_flag = 1 LDR is need to be calibrated!
//*********************************************************************

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){
    UCA0RXBUF = UCA0RXBUF;
    IE2 &= ~UCA0TXIE;
    if (state == state1){
        if (UCA0RXBUF == 'Z'){
            state = state0;

            LPM0_EXIT;
        }
        else if ( UCA0RXBUF == 's'){
            state = state1;
            LPM0_EXIT;
        }
    }

    else if (state == state2){
        if (UCA0RXBUF == 'Z')
            state = state0;
            // MAYBE NEED HERE LPM0_EXIT

        else{
            str_angle[angle_idx] = UCA0RXBUF;
            angle_idx++;
            if (str_angle[angle_idx - 1] == '\n')
            {
                angle_idx = 0;
                LPM0_EXIT;
            }

        }
    }

    else if (state == state3){
        if (UCA0RXBUF == 'Z'){
            state = state0;
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'r'){
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'c'){
            prev_calibrate_state = calibration_flag;
            calibration_flag = '1';
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'd'){
            //calibration_flag = '0';
            calibration_flag = '4';
            LPM0_EXIT;
        }
    }

    else if (state == state4){
        if (UCA0RXBUF == 'Z'){
            state = state0;
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'r'){
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'c'){
            prev_calibrate_state = calibration_flag;
            calibration_flag = '1';
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'd'){
            calibration_flag = '4';  // was interrupted during the calibration process
            LPM0_EXIT;
        }
    }

    else if (state == state5){

        if (UCA0RXBUF == 'Z'){
            state = state0;
            LPM0_EXIT;
        }
        else if (UCA0RXBUF == '1' && got_script == 0){
            temp_script_size = 0;
            got_script = 1;
            LPM0_EXIT;
        }
        else if (UCA0RXBUF == '2' && got_script == 0){
            temp_script_size = 0;
            got_script = 2;
            LPM0_EXIT;
        }
        else if (UCA0RXBUF == '3' && got_script == 0){
            temp_script_size = 0;
            got_script = 3;
            LPM0_EXIT;
        }

        else if (got_script == 1){
            if (scripts.script_size[0] > 0 && UCA0RXBUF == 'p'){
                // script is in memory - activate!
                LPM0_EXIT;
            }
            else {
                // receive script and save in memory
                if (UCA0RXBUF == 'r'){
                    if (scripts.script_size[0] > 0){
                        scripts.script_counter--;
                    }
                    scripts.script_size[0] = 0;
                    LPM0_EXIT;
                }
                else if (UCA0RXBUF == '!'){
                    scripts.script_size[0] = temp_script_size;
                    flash_B_flag = 1;
                    got_script = 0;
                    UCA0TXBUF = 'd';
                    while (!(IFG2&UCA0TXIFG));
                    scripts.script_counter++;
                    LPM0_EXIT;
                }
                else{
                    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
                    *flash_ptrB = UCA0RXBUF;
                    flash_ptrB ++;
                    temp_script_size++;
                }
            }
        }

        else if (got_script == 2){
            if (scripts.script_size[1] > 0 && UCA0RXBUF == 'p'){
                // script is in memory - activate!
                LPM0_EXIT;
            }
            else {
                // receive script and save in memory
                if (UCA0RXBUF == 'r'){
                    if (scripts.script_size[1] > 0){
                        scripts.script_counter--;
                    }
                    scripts.script_size[1] = 0;

                    LPM0_EXIT;
                }
                else if (UCA0RXBUF == '!'){
                    scripts.script_size[1] = temp_script_size;
                    flash_C_flag = 1;
                    got_script = 0;
                    UCA0TXBUF = 'd';
                    while (!(IFG2&UCA0TXIFG));
                    scripts.script_counter++;
                    LPM0_EXIT;
                }
                else{
                    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
                    *flash_ptrC = UCA0RXBUF;
                    flash_ptrC ++;
                    temp_script_size++;
                }
            }
        }

        else if (got_script == 3){
            if (scripts.script_size[2] > 0 && UCA0RXBUF == 'p'){
                // script is in memory - activate!
                LPM0_EXIT;
            }
            else {
                // receive script and save in memory
                if (UCA0RXBUF == 'r'){
                    if (scripts.script_size[2] > 0){
                        scripts.script_counter--;
                    }
                    scripts.script_size[2] = 0;
                    LPM0_EXIT;
                }
                else if (UCA0RXBUF == '!'){
                    scripts.script_size[2] = temp_script_size;
                    flash_D_flag = 1;
                    got_script = 0;
                    UCA0TXBUF = 'd';
                    while (!(IFG2&UCA0TXIFG));
                    scripts.script_counter++;
                    LPM0_EXIT;
                }
                else{
                    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
                    *flash_ptrD = UCA0RXBUF;
                    flash_ptrD ++;
                    temp_script_size++;
                }
            }
        }
    }

    else if (state == state6){
        if(UCA0RXBUF == 'r'){
            LPM0_EXIT;
        }
        else if(UCA0RXBUF == 'd'){
            calibration_flag = '4';  // was interrupted during the calibration process
            LPM0_EXIT;
        }
    }

    else if (state == state0){
        if (UCA0RXBUF == '1'){  // objects scan
            state = state1;
            LPM0_EXIT;
            }
        else if (UCA0RXBUF == '2'){  // telemeter
            state = state2;
            LPM0_EXIT;
        }
        else if (UCA0RXBUF == '3'){  // ldr scan
            state = state3;
            LPM0_EXIT;
        }
        else if (UCA0RXBUF == '4'){ // bonus
            state = state4;
            LPM0_EXIT;
        }
        else if (UCA0RXBUF == '5'){  // script
            state = state5;
            LPM0_EXIT;
        }
    }

            // END OF STATES DEFINITION

}


//---------------------------------------------------------------------
//            Enter from LPM0 mode
//---------------------------------------------------------------------
void enterLPM(unsigned char LPM_level){
    if (LPM_level == 0x00)
      _BIS_SR(LPM0_bits);     /* Enter Low Power Mode 0 */
        else if(LPM_level == 0x01)
      _BIS_SR(LPM1_bits);     /* Enter Low Power Mode 1 */
        else if(LPM_level == 0x02)
      _BIS_SR(LPM2_bits);     /* Enter Low Power Mode 2 */
    else if(LPM_level == 0x03)
      _BIS_SR(LPM3_bits);     /* Enter Low Power Mode 3 */
        else if(LPM_level == 0x04)
      _BIS_SR(LPM4_bits);     /* Enter Low Power Mode 4 */
    }


//*********************************************************************
//            UART Interrupt Service Rotine - TX
//********************************************************************
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    if (state == state0){
        IE2 &= ~UCA0TXIE;
        LPM0_EXIT;
    }
    if (state == state1){
            UCA0TXBUF = str_dis[dis_idx];
            dis_idx ++;
            if (!str_dis[dis_idx-1]){                                                     // TX over?
                UCA0TXBUF = '\n';
                IE2 &= ~UCA0TXIE;
                LPM0_EXIT;
            }                                                                           // Disable USCI_A0 TX interrupt

    }

    else if (state == state2)
    {
        UCA0TXBUF = str_dis[dis_idx];
        dis_idx ++;
        if (!str_dis[dis_idx-1]){                                                     // TX over?
            UCA0TXBUF = '\n';
            IE2 &= ~UCA0TXIE;
        }                                                                           // Disable USCI_A0 TX interrupt
    }
    else if (state == state3 || state == state6){
        UCA0TXBUF = str_ldr[str_ldr_idx];
        str_ldr_idx ++;
        while (!(IFG2&UCA0TXIFG));
        if (!str_ldr[str_ldr_idx]){                                              // TX over?
            UCA0TXBUF = '\n';
            while (!(IFG2&UCA0TXIFG));
            IE2 &= ~UCA0TXIE;                                                         // Disable USCI_A0 TX interrupt       WILL THAT BULLSHIT WORK?
            LPM0_EXIT;
        }
    }

    else if (state == state4){
        if (done_flag == '0'){
            UCA0TXBUF = str_ldr[str_ldr_idx];
            str_ldr_idx ++;
            while (!(IFG2&UCA0TXIFG));
            if (!str_ldr[str_ldr_idx]){                                              // TX over?
                UCA0TXBUF = '\n';
                while (!(IFG2&UCA0TXIFG));
                IE2 &= ~UCA0TXIE;                                                         // Disable USCI_A0 TX interrupt
                LPM0_EXIT;
            }                                                                         // Disable USCI_A0 TX interrupt
        }
        else{
            UCA0TXBUF = str_dis[dis_idx];
            dis_idx ++;
            if (!str_dis[dis_idx-1]){                                                     // TX over?
                UCA0TXBUF = '\n';
                while (!(IFG2&UCA0TXIFG));
                IE2 &= ~UCA0TXIE;
                LPM0_EXIT;
            }
        }

    }

    else if (state == state5){
        if (done_flag == '0'){
            UCA0TXBUF = str_angle[angle_idx];
            angle_idx ++;
            while (!(IFG2&UCA0TXIFG));
            if (!str_angle[angle_idx]){                                              // TX over?
                UCA0TXBUF = '\n';
                while (!(IFG2&UCA0TXIFG));
                IE2 &= ~UCA0TXIE;                                                         // Disable USCI_A0 TX interrupt
                done_flag = '1';
                //LPM0_EXIT;
            }                                                                           // Disable USCI_A0 TX interrupt
        }

        else if(done_flag == '1')
           {
            UCA0TXBUF = str_dis[dis_idx];
            dis_idx ++;
            if (!str_dis[dis_idx-1]){                                                     // TX over?
                UCA0TXBUF = '\n';
                IE2 &= ~UCA0TXIE;
                done_flag = '0';
                LPM0_EXIT;
               }
           }

    }
}



//*********************************************************************
//            Port2 Interrupt Service Rotine
//*********************************************************************
#pragma vector=PORT2_VECTOR
  __interrupt void PBs_handler2(void){
   
    delay(debounceVal);
//---------------------------------------------------------------------
//            selector of transition between states
//---------------------------------------------------------------------
    if(PBsArrIntPend2 & PB0){
        if (state == state0){
            UCA0TXBUF = 'c';
            while (!(IFG2&UCA0TXIFG));
            state = state6;
            PBsArrIntPend2 &= ~PB0;
            LPM0_EXIT;
        }
        else if (state == state3 || state == state4 || state == state6){
            if(calibration_flag == '1'){
            UCA0TXBUF = 'o'; // little 'o', send to continue calibrating from push button 0
            while (!(IFG2&UCA0TXIFG));
            }
            else{
                prev_calibrate_state = calibration_flag;
                calibration_flag = '1';  // start calibration!
                UCA0TXBUF = 'c';
                set_SemiCircle_timer(490);
                TimerWait(0xAAAA);
                stopPWM();
                enable_interrupts();

                while (!(IFG2&UCA0TXIFG));
            }
        }
      PBsArrIntPend2 &= ~PB0;
    }

        
}

//*********************************************************************
//            TIMER1 A1 Interrupt Service Rotine
//*********************************************************************

#pragma vector = TIMER1_A1_VECTOR
__interrupt void Timer_A(void){


            temp[i] = TA1CCR1;
            i += 1;
            TA1CCTL1 &= ~CCIFG ;
            if (i==2) {
                diff = temp[i-1]-temp[i-2]; //UCA0TXBUF
                i=0;
                if (diff > 0){

                    LPM0_EXIT;
                }
        }
}

//*********************************************************************
//            TimerA0 Interrupt Service Rotine
/*
TA1 works in 2^20 Hz. in this timer we capture the input wave and capture
the rising edges and counting it.
TA0 works in 2^20 Hz plus diveder 8 = 2^17. in this timer wer work in up\down
mode until 0xFFFF --> thus 2^16 in each inteval. the ratio between the timers
is 1/2^4 --> 0.0625.
*/
//*********************************************************************

#pragma vector=TIMER0_A0_VECTOR
  __interrupt void timer_a0(void) {
/*
      if (state == state1 || state == state2 || state == state3 || state == state5){
              done_flag = '1';
          }

      else{
          LPM0_EXIT;
      }
}
*/
      LPM0_EXIT;
  }


//*********************************************************************
//            ADC10 Interrupt Service Rotine
//*********************************************************************

#pragma vector=ADC10_VECTOR
__interrupt void ADC10(void) {
    if (ldr == '1'){
        ldr1[ldr_idx] = ADC10MEM;
        LPM0_EXIT;
    }
    else{
        ldr = '1';
        ldr2[ldr_idx] = ADC10MEM;
        LPM0_EXIT;
    }


}
