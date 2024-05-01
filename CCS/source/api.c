#include  "../header/api.h"    		// private library - API layer
#include  "../header/halGPIO.h"     // private library - HAL layer

int current_angle = 0;
const unsigned int halfsec = 0x5555;
const unsigned int quatersec = 0x2AAA;
const unsigned int eightsec = 0x1FFF;
const unsigned int sec = 0xAAAA;
const unsigned int thirdquatersec = 0x7FFF;




void change_degree(){

    setDegrees(current_angle);
    TimerWait(sec); //  need to check if can do less!
    stopPWM();
}


void setDegrees(int deg){
    set_SemiCircle_timer(deg);
    TimerWait(0xAAAA);
    stopPWM();
    enable_interrupts();

}


// ------------------------------------------------------------------
//                          STATE 1
// ------------------------------------------------------------------

void object_prox(){
    //  engine start
    //start_object_timers();
    //enable_interrupts();
    stopPWM();
    unsigned int k;
    enable_interrupts();
    for(k=490; k<=2290; k = k + 30){ // IF CHANGING ANGLES, CHANGE THE 60!
        stopPWM();
        //if (k == 440){
        //    set_SemiCircle_timer(490);
        //}
       // else{
            set_SemiCircle_timer(k);
        //}
        TimerWait(quatersec);
        //TimerWait(eightsec);
        //TimerWait(thirdhalfsec);
        stopPWM();
        UDSconfig();
        start_object_timers();
        enterLPM(lpm_mode);
        distance_append();
        if (state != state1){
            stopPWM();
            break;
        }
    }
    stopPWM();
    send_distance_list();
    //enterLPM(lpm_mode);
}

// ------------------------------------------------------------------
//                  STATE 2 - Telemeter
// ------------------------------------------------------------------

void Telemeter(int angle){
    int new_angle;
    //TimerWait(halfsec); //  need to check if can do less!
    stopPWM();
    UDSconfig();
    current_angle = angle;
    while(1){
        new_angle = get_angle();
        if (new_angle != current_angle){
            current_angle = new_angle;
            //stopPWM();
            //TimerWait(halfsec);
            //change_degree();
            setDegrees(current_angle);
            UDSconfig();
            start_object_timers();
        }
        if (state != state2){
            stop_USDscan();
            break;
        }
        enable_interrupts();
        start_object_timers();
        enterLPM(lpm_mode);
        if (state != state2){
            break;
        }
        send_distance();
        TimerWait(quatersec);
        //start_object_timers();

    }

}

// ------------------------------------------------------------------
//                          STATE 3
// ------------------------------------------------------------------


void calibration(){
    int line, avarage_ldr, last_scan = 0;
    for (line = 10; line >= 0; line--){
        if ((state != state3 && state != state4 && state != state6) || calibrated() == '4'){
            set_calibration_flag(prev_calibrate_state);
            return;
        }
        enable_interrupts();
        startADC_ldr1();
        enterLPM(lpm_mode);
        stopADC();
        startADC_ldr2();
        enterLPM(lpm_mode);
        stopADC();
        avarage_ldr = return_ldr_avarage();

        while ((line > 3 && avarage_ldr > 1019) ||  avarage_ldr < last_scan - 100){
            if (line <=3 && avarage_ldr > 1019){
                break;
            }
            if ((state != state3 && state != state4 && state != state6) || calibrated() == '4'){
                set_calibration_flag(prev_calibrate_state);
                return;
            }
            send_scan_error();
            if ((state != state3 && state != state4 && state != state6) || calibrated() == '4'){
                set_calibration_flag(prev_calibrate_state);
                return;
            }
            enterLPM(lpm_mode);
            startADC_ldr1();
            enterLPM(lpm_mode);
            stopADC();
            startADC_ldr2();
            enterLPM(lpm_mode);
            stopADC();
            avarage_ldr = return_ldr_avarage();

        }
        last_scan = avarage_ldr;
        if ((state != state3 && state != state4 && state != state6) || calibrated() == '4'){
            set_calibration_flag(prev_calibrate_state);
            return;
        }
        sendADC_value(avarage_ldr);
        enterLPM(lpm_mode);
        if (!line){
            break;
        }
        sendNewlineREQ();
        enterLPM(lpm_mode);  // wait until ready signal from PC
    }
    send_done();
}


void light(){
    //stopPWM();
    ADC_idx_reset();
    unsigned int k;
    enable_interrupts();
    for(k=490; k<=2290; k = k + 30){ // IF CHANGING ANGLES, CHANGE THE 30!
            //if (k == 440){
            //    set_SemiCircle_timer(490);
            //}
            //else{
                set_SemiCircle_timer(k);
            //}
        TimerWait(quatersec);
        //TimerWait(thirdhalfsec);
        //stopPWM();
        startADC_ldr1();
        enterLPM(lpm_mode);
        stopADC();
        startADC_ldr2();
        enterLPM(lpm_mode);
        stopADC();
        ADC_append();
        if (state != state3){
            stopPWM();
            break;
        }
    }
    stopPWM();
    send_ADC_list();

}

// ------------------------------------------------------------------
//                          STATE 4
// ------------------------------------------------------------------

void light_and_objects(){
    ADC_idx_reset();
    unsigned int k;
    enable_interrupts();
    for(k=490; k<=2290; k = k + 30){ // IF CHANGING ANGLES, CHANGE THE 60!
        stopPWM();
        set_SemiCircle_timer(k);
        TimerWait(quatersec);
        //TimerWait(thirdhalfsec);
        //stopPWM();
        startADC_ldr1();
        enterLPM(lpm_mode);
        stopADC();
        startADC_ldr2();
        enterLPM(lpm_mode);
        stopADC();
        // end of ldr scan

        // start object scan
        stopPWM();
        UDSconfig();
        start_object_timers();
        enterLPM(lpm_mode);
        // end of object scan

        // start of appending data
        distance_append();
        ADC_append();
        if (state != state4){
            stopPWM();
            break;
        }
    }
    stopPWM();
    send_ADC_list();
    enterLPM(lpm_mode);  // waiting for acknowledge bit from pc
    send_distance_list();
}




// ------------------------------------------------------------------
//                          STATE 5
// ------------------------------------------------------------------

void script(){
    while(1){
        flash_ptrB = (char *) 0x1080;             // Initialize Flash segment B pointer
        flash_ptrC = (char *) 0x1040;             // Initialize Flash segment C pointer
        flash_ptrD = (char *) 0x1000;             // Initialize Flash segment D pointer
        if (state != state5){
            got_script = 0;
            break;
        }
        if (got_script == 0){
            enterLPM(lpm_mode);  // MCU wake up after getting the script num
        }
        if (got_script == 1){
            enterLPM(lpm_mode);  // MCU wake up after getting 'r'
            if (get_script_size(1) != 0){
                lcd_clear();
                //lcd_puts("script1");
                read_script(1);
                got_script = 0;
                continue;
            }
            else{
                erase_flash();
                *flash_ptrB = 0;                           // Dummy write to erase flash segmentB
                enterLPM(lpm_mode);
                // should exit in the end of receiving script
                stop_write_flash();
                }
        }

        else if (got_script == 2){
            enterLPM(lpm_mode);  // MCU wake up after getting 'r'
            if (get_script_size(2) != 0){
                lcd_clear();
                //lcd_puts("script2");
                read_script(2);
                got_script = 0;
                continue;
            }
            else{
                erase_flash();
                *flash_ptrC = 0;                           // Dummy write to erase flash segmentC
                enterLPM(lpm_mode);
                // should exit in the end of receiving script
                stop_write_flash();
            }
        }

        else if (got_script == 3){
            enterLPM(lpm_mode);  // MCU wake up after getting 'r'
            if (get_script_size(3) != 0){
                lcd_clear();
                //lcd_puts("script3");
                read_script(3);
                got_script = 0;
                continue;
            }
            else{
                erase_flash();
                *flash_ptrD = 0;                           // Dummy write to erase flash segmentD
                enterLPM(lpm_mode);
                // should exit in the end of receiving script
                stop_write_flash();
                }
        }
    }
}




int hex_to_int(char* hex){
    int result = 0;
    if(hex[0] >= 48 && hex[0] <= 57){  // is hex[0] a number?
        result = ((int)hex[0] - 48) << 4;
        if (hex[1] >= 48 && hex[1] <= 57){  // hex[0] a number. is hex[1] a number?
            result += (int)hex[1] - 48;
        }
        else{
            result += ((int)hex[1] - 55);  // hex[0] a number. hex[1] is not a number.
        }
    }
    else{  // hex[0] is not a number.
        result = ((int)hex[0] - 55) << 4;
        if (hex[1] >= 48 && hex[1] <= 57){  // hex[0] is not a number. is hex[1] a number?
            result += (int)hex[1] - 48;
        }
        else{
            result += ((int)hex[1] - 55);  // hex[0] is not a number. hex[1] is not a number.
        }
    }
    return result;
}


int degree_to_PWM(int degree){  // (degree * 10) + 490
    int idx;
    int iteration = degree;
    for (idx = 9; idx > 0; idx--){
        degree = degree + iteration;
    }
    return (degree + 490);
}







void read_script(int script){
    int temp_address = 0x1080;
    int idx, script_bytes, left, right, new_delay, angle, PWM_deg, angle_l, angle_r;
    char* address;
    char opcode[2], operand1[2], operand2[2];
    for (idx = 0; idx < script-1; idx ++){  // 0x1080 - (script - 1) * 0x40
        temp_address = temp_address - 0x40;
    }
    if (got_script != script || state != state5){
        return;
    }
    address = (char*) temp_address;
    script_bytes = get_script_size(script);
    while (script_bytes > 0){
        if (got_script != script || state != state5){
            return;
        }
        opcode[0] = *address++;
        opcode[1] = *address++;
        if (opcode[1] == '8'){  // sleep
            script_bytes -= 2;
            if (script_bytes == 0){
                send_char('f');
                got_script = 0;
                return;
            }
            enterLPM(lpm_mode);
            continue;
        }
        else if (opcode[1] == '5'){ // clear lcd
            lcd_clear();
            script_bytes -= 2;
            continue;
        }
        else if (opcode[1] == '7'){
            operand1[0] = *address++;
            operand1[1] = *address++;
            operand2[0] = *address++;
            operand2[1] = *address++;
            left = hex_to_int(operand1);
            right = hex_to_int(operand2);
            send_char('s');
            send_angle(left);
            enterLPM(lpm_mode);  // wake up by 'p'
            send_angle(right);
            enterLPM(lpm_mode);  // wake up by 'p'
            angle_l = degree_to_PWM(left);
            angle_r = degree_to_PWM(right);
            servo_scan_script(angle_l, angle_r);
            enterLPM(lpm_mode);  // wake up by 'p'
            script_bytes -= 6;
        }
        else{
            operand1[0] = *address++;
            operand1[1] = *address++;
            if (opcode[1] == '1'){       // count up on LCD from until the number provided with Xdelay
                count_up_LCD(hex_to_int(operand1));
                script_bytes -= 4;
                continue;
            }
            else if (opcode[1] == '2'){  // count down on LCD from the number provided to zero with Xdelay
                count_down_LCD(hex_to_int(operand1));
                script_bytes -= 4;
                continue;
            }
            else if (opcode[1] == '3'){  // Rotate right onto LCD from pixel index 0 to pixel index 31 the provided char with Xdelay
                pixel_char_moving((char)hex_to_int(operand1));
                script_bytes -= 4;
                continue;
            }
            else if (opcode[1] == '4'){
                new_delay = hex_to_int(operand1);
                set_delay(new_delay);
                script_bytes -= 4;
                continue;
            }
            else if (opcode[1] == '6'){
                // servo degree --> need to calculate the right PWM high level time
                send_char('d');
                angle = hex_to_int(operand1);
                send_angle(angle);
                enterLPM(lpm_mode);  // wake up by 'p'
                PWM_deg = degree_to_PWM(angle);
                //enterLPM(lpm_mode);  // wake up by 'p'
                servo_deg(PWM_deg);
                enterLPM(lpm_mode);  // wake up by 'p'
                script_bytes -= 4;
                continue;
            }
        } // end of file!
    }
    send_char('f');
}







