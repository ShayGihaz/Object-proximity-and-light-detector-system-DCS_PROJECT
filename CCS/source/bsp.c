#include  "../header/bsp.h"    // private library - BSP layer

//-----------------------------------------------------------------------------  
//           GPIO configuration
//-----------------------------------------------------------------------------
void GPIOconfig(void){
 // volatile unsigned int i; // in case of while loop usage

  WDTCTL = WDTHOLD | WDTPW;     // Stop WDT

    
  // PushButtons Setup - port 2
  PBsArrPortSel2 &= ~0xFF;           // bit clear
  PBsArrPortDir2 |= 0xF0;             //first 4 legs to input and the others to output
  PBsArrIntEdgeSel2 |= 0x01;         // pull-up mode
  PBsArrIntEn2 &= ~ 0xFF;            // reset interrupt
  PBsArrIntEn2 |= 0x01;              // interrrupt enable
  PBsArrIntPend2 &= ~0xFF;           // bit clear flags


  
  _BIS_SR(GIE);                     // enable interrupts globally
}



//------------------------------------------------------------------------------------- 
//                   UDS configuration
//-------------------------------------------------------------------------------------

void UDSconfig(){
    TACTL = TACLR;
    BCSCTL1= CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    UDSDir |= 0x40; // p1.6 TRIGGER to output
    UDSSel |= 0x02; // p2.1 (ECHO)  to timer\clock setting
    UDSSelP1 |= 0x40; // p1.6 (TRIGGER)  to timer\clock setting
    TA0CTL = TASSEL_2;  // SMCLK --> to start change to MC_1
    TA0CCR0 = 0xFFFF;  // count up to 65535 (period for PWM), one cycle one usec (clock is at 1MHz)
    TA0CCR1 = 0x000A;  // duty cycle A=10 (usec) for trigger
    TA0CCTL1 = OUTMOD_7;
    TA1CTL = TASSEL_2 ; // SMCLK  -->  to start change to MC_2
    TA1CCTL1 = CAP + CCIE + CCIS_0 + CM_3 + SCS ; // CCIE capture mode for getting the time values, interrupt enable, capture input selected,capture in rising and falling edge , sync

   // _BIS_SR(GIE);                     // enable interrupts globally

}



//-------------------------------------------------------------------------------------
//                   UART configuration
//-------------------------------------------------------------------------------------
void UARTconfig(){
    UARTSel |= BIT1 + BIT2;
    UARTSel2 |= BIT1 + BIT2;
    UCA0CTL1 |= UCSSEL_2;                     // CLK = SMCLK
    UCA0BR0 = 104;                           //
    UCA0BR1 = 0x00;                           //
    UCA0MCTL = UCBRS0;               //
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
   // _BIS_SR(GIE);                     // enable interrupts globally

}



//-------------------------------------------------------------------------------------
//                   SERVO configuration
//-------------------------------------------------------------------------------------
void SERVOconfig(){
    SERVODir |= 0x04;
    SERVOSel |= 0x04;
    _BIS_SR(GIE);                     // enable interrupts globally

}


//-------------------------------------------------------------------------------------
//                    ADC configuration
//-------------------------------------------------------------------------------------
void ADCconfig(){
    ADC10CTL0 = ADC10ON + ADC10IE + ADC10SHT_2 + SREF_0;  // enable interrupt, turn on ADC, CLK*16 (like HW2), V+=VDD V-=0
}




             
             
            
  

