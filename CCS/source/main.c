#include  "../header/api.h"         // private library - API layer
#include  "../header/app.h"         // private library - APP layer

enum FSMstate state;
enum SYSmode lpm_mode;
char main_menu_return;
void main(void){
  

  state = state0;
  lpm_mode = mode0;     
  sysConfig();
  lcd_init();
  while(1){
    switch(state){
      case state0:
          enable_PB0_INT();
          stopPWM();
          setDegrees(490);
          lcd_clear();
          lcd_home();
          lcd_puts("state0");
          enterLPM(lpm_mode);
          break;
         
      case state1:
          disable_PB0_INT();
          lcd_clear();
          lcd_puts("state1");

          while(1){
              enterLPM(lpm_mode);
              if (state != state1){
                  break;
              }
              object_prox();
              if (state != state1){
                  break;
              }
          }
         break;

      case state2:
          disable_PB0_INT();
          lcd_clear();
          lcd_puts("state2");
          enterLPM(lpm_mode); // ANGLE RECIVED
          setDegrees(get_angle());
          Telemeter(get_angle());
          break;

      case state3:
          enable_PB0_INT();
          if (calibrated() == '1'){
            setDegrees(1050);  // make it more comfortable to calibrate
            enterLPM(lpm_mode);
            if (state != state3){
                break;
            }
            lcd_clear();
            lcd_puts("state3");
            lcd_new_line;
            lcd_puts("calibration");
            calibration();
            if (calibrated() == '1'){
                state = state0;
            }
            main_menu_return = '0';
        }
        lcd_clear();
        lcd_puts("state3");
        lcd_new_line;
        lcd_puts("scanning..");
        if (state != state3){
            break;
        }
        while(1){
            if (main_menu_return == '0'){
                enterLPM(lpm_mode);  // if goes to main menu and return to state3, we are asleep here!
            }
            if (state != state3){
                main_menu_return = '1';
                break;
            }
            if (calibrated() == '1'){
                setDegrees(1050);
                enterLPM(lpm_mode);
                if (state != state3){
                    break;
                }
                lcd_clear();
                lcd_puts("state3");
                lcd_new_line;
                lcd_puts("calibration");
                calibration();
                if (state != state3){
                    break;
                }
            }
            light();
            if (main_menu_return == '1'){
                enterLPM(lpm_mode);
            }
            if (state != state3){
                main_menu_return = '1';
                break;
                }
        }
        break;

      case state4:
          enable_PB0_INT();
          if (calibrated() == '1'){
              lcd_clear();
              lcd_puts("state4");
              lcd_new_line;
              lcd_puts("calibration");
              setDegrees(1050);  // make it more comfortable to calibrate
              enterLPM(lpm_mode);
              if (state != state4){
                  break;
              }
              calibration();
              main_menu_return = '0';
          }
          lcd_clear();
          lcd_puts("state4");
          lcd_new_line;
          lcd_puts("scanning..");
          if (state != state4){
              break;
          }
          while(1){
              if (main_menu_return == '0'){
                  enterLPM(lpm_mode);  // if goes to main menu and return to state3, we are asleep here!
              }
              if (state != state4){
                  main_menu_return = '1';
                  break;
              }
              if (calibrated() == '1'){
                  enterLPM(lpm_mode);
                  if (state != state4){
                      break;
                  }
                  lcd_clear();
                  lcd_puts("state4");
                  lcd_new_line;
                  lcd_puts("calibration");
                  setDegrees(1050);  // make it more comfortable to calibrate
                  calibration();
                  if (state != state4){
                      break;
                  }
              }
              light_and_objects();
              if (main_menu_return == '1'){
                  enterLPM(lpm_mode);
              }
              if (state != state4){
                  main_menu_return = '1';
                  break;
                  }
          }

           break;

      case state5:
          disable_PB0_INT();
          lcd_clear();
          lcd_puts("script mode");
          script();

          break;

      case state6:  // CALIBRATION STATE!
          enable_PB0_INT();
          lcd_clear();
          lcd_puts("push button 0");
          lcd_new_line;
          lcd_puts("calibration");
          setDegrees(1050);
          enterLPM(lpm_mode);
          if (calibrated() == '4'){
              set_calibration_flag(prev_calibrate_state);
              state = state0;
              break;
          }
          calibration();

          if (calibrated() == '0')
              {
              main_menu_return = '1';
              }
          state = state0;

        break;



    }
  }
}
  

  
  
  
