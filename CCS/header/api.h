#ifndef _api_H_
#define _api_H_

#include  "../header/halGPIO.h"     // private library - HAL layer

extern void object_prox();
extern void SemiCircle();
extern void setDegrees(int deg);
extern void Telemeter();
extern void calibration();
extern void light();
extern void script();
extern void light_and_objects();
extern int hex_to_int(char* hex);
extern int degree_to_PWM(int degree);
extern void read_script(int script);


#endif








