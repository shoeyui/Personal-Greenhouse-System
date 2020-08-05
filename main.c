/*********************************************
 * Connection Information

Format: TIVA PIN - ROW/COL NUM - PIN

ROW
PE
PE1-1-2
PE2-2-7
PE3-3-6
PE4-4-4

COL
PC
PC4-1-3
PC5-2-1
PC6-3-5

LCD
PA
PA2-SCK
PA3-/CS
PA4-SDO
PA5-SDI

HEATING PAD
PD3

FAN
PD6

LED PANEL
PD2
*********************************************/

/*********************************************
    INCLUDE SECTION
*********************************************/
#include <stdint.h>
#include <stdio.h>

/*********************************************
    CUSTOM LIBRARIES SECTION
*********************************************/
#include "inc/tm4c123gh6pm.h"

/*********************************************
    DEFINE && GLOBAL SECTION
*********************************************/
#define  padRows  4
#define  padCols  3
#define PORTFDAT (*((volatile unsigned int*)0x400253FC)) // PORTF Data Register Setup
#define PORTFDIR (*((volatile unsigned int*)0x40025400)) // Data Direction Register
#define PORTFDEN (*((volatile unsigned int*)0x4002551C)) // Digital Enable Register
#define RCGCGPIO (*((volatile unsigned int*)0x400FE608)) // Clock Gating Register
#define SCB_CPAC (*((volatile unsigned int*)0xE000ED88)) // Access Control Register
unsigned int ncols = 0;
unsigned int nrows = 0;
unsigned const char symbol[padRows][padCols] = {{ '1', '2',  '3'},
                                                { '4', '5',  '6'},
                                                { '7', '8',  '9'},
                                                { '*', '0',  '#'}};
int fanState = 0;
int panelState = 0;
int heatState = 0;


/*********************************************
    MAIN CODE SECTION
*********************************************/

void delay(int n){
    int x,y;
    for (x = 0; x < n; x++)
    for (y = 0; y <1460; y++)
    {}
}

void spi_master_ini(void){
    SYSCTL_RCGCSSI_R|=0x7; //selecting SSI0 module
    SYSCTL_RCGC2_R|=0x1F;   //providing clock to PORTA
    GPIO_PORTA_AFSEL_R|= 0x3C;//selecting alternative functions
    GPIO_PORTA_PCTL_R=0x00222200;//selecting SSI as alternative function
    GPIO_PORTA_DEN_R|= 0x3C;//enabling digital mode for PORTA 2,3,4,5
    SSI0_CR1_R=0;          //disabling SSI module for settings
    SSI0_CC_R=0;           //using main system clock
    SSI0_CPSR_R=64;        //selecting divisor 64 for SSI clock
    SSI0_CR0_R=0x7;        //freescale mode, 8 bit data, steady clock low
    SSI0_CR1_R|=(0x2);    //enabling SSI
}

void portd_ini(void){

       SYSCTL_RCGCGPIO_R = 0x08; //0b00001000 (enables port D)
       delay(100);
       GPIO_PORTD_DIR_R = 0x4C;//Sets PD6 and PD3 to Output
       GPIO_PORTD_DEN_R = 0x4C;//Enables Digital Capabilities on PD6 and PD3
}

void send_byte(char data){
    SSI0_DR_R=data; //putting the byte to send from SSI
    while((SSI0_SR_R&(1<<0))==0); //waiting for transmission to be done
}

void send_num(int data){
    SSI0_DR_R=data; //putting the byte to send from SSI
    while((SSI0_SR_R&(1<<0))==0); //waiting for transmission to be done
}

void send_str(char *buffer){ //function for sending each byte of string one by one
    while(*buffer!=0){
    send_byte(*buffer);
    delay(5); //added to ensure the data gets sent to the LCD properly (provides less glitches)
        buffer++;
    }
}

void clr_screen(void){ //function for clearing the LCD screen

    send_num(0x7C); //opens settings
    send_num(0x2D);// clear screen and set cursor to begining

}

void str_pos (int row, int position){ //function that sets the position and row of the cursor
    switch(row) {

        case 1  :
        row = 0;
            break;

        case 2  :

        row = 64;
            break;
        case 3  :

                row = 20;
                    break;
        case 4  :

                row = 84;
                    break;

        default :
        row = 0;
    }

    send_num(254);
    send_num(128+ row + position);
}
void delay_us(int time)
{
    int i, j;
    for(i = 0 ; i < time; i++)
        for(j = 0; j < 3; j++){}
}
void delay_ms(int n){
    int x,y;
    for (x=0;x<n;x++){
        for (y=0;y<2666;y++){
        }
    }
}
void keypad_Init(void)
{
  SYSCTL_RCGCGPIO_R |= 0x14;            //enable clc for port C & E
  while ((SYSCTL_RCGCGPIO_R&0x14)==0);  //wait for clock to be enabled
  GPIO_PORTC_CR_R  |= 0xF0;             //allow changes to all the bits in port C
  GPIO_PORTE_CR_R  |= 0x1E;             //allow changes to all the bits in port E
  GPIO_PORTC_DIR_R |= 0xF0;             //set directions cols are o/ps
  GPIO_PORTE_DIR_R |= 0x00;             //set directions raws are i/ps
  GPIO_PORTE_PDR_R |= 0x1E;             //pull down resistor on Raws
  GPIO_PORTC_DEN_R |= 0xF0;             //digital enable pins in port C
  GPIO_PORTE_DEN_R |= 0x1E;             //digital enable pins in port E
}

char keypad_getkey(void)
{
    int i=0;
    int j=0;
    while(1)
    {

    for(i = 0; i < 3; i++){    // Going through each col
        GPIO_PORTC_DATA_R = (1U << i+4);
        delay_us(2);
        for(j = 0; j < 4; j++)   // Going through each row
    {
    if((GPIO_PORTE_DATA_R & 0x1E) & (1U << j+1))
        return symbol[j][i];
    }
    }
    }
}

void portf_init(void){
    RCGCGPIO |= 0x20;                 // Enabled clock for CPIOF at the clock gating register
    GPIO_PORTF_LOCK_R = 0x4C4F434B;   // Unlock PORTF (PF0 must be manually unlocked)
    GPIO_PORTF_CR_R = 0x1F;           // Enable changes for pins PF0-PF4
    GPIO_PORTF_PCTL_R = 0x00000000;   // Parallel control GPIO on PF0-PF4
    PORTFDIR=0x1F;                    // PF0-PF4 are OUTPUT pins
    PORTFDEN=0x3F;                    // Enable PORTF pins 0-4
}

void ledpanel_on (){

    if (fanState == 1 && heatState == 1){

             GPIO_PORTD_DATA_R = 0x4C; //All Peripherals ON
         }

        if (fanState == 1 && heatState==0){

                   GPIO_PORTD_DATA_R = 0x44; //Keeps Heating Pad Off, Everything else ON
               }
        if (fanState == 0 && heatState==1){

                   GPIO_PORTD_DATA_R = 0x0C; ////Keeps FAN Off, Everything else ON
               }

        if (fanState == 0 && heatState==0) {

         GPIO_PORTD_DATA_R = 0x04;//LED On Only

         }
        panelState=1;
}

void ledpanel_off (){

    if (fanState == 1 && heatState == 1){

             GPIO_PORTD_DATA_R = 0x48; //All Peripherals ON
         }

        if (fanState == 1 && heatState==0){

                   GPIO_PORTD_DATA_R = 0x40; //Turns LED PANEL Off, Everything else ON
               }
        if (fanState == 0 && heatState==1){

                   GPIO_PORTD_DATA_R = 0x08; ////Turn LED PANEL OFF and keeps FAN Off, Everything else ON
               }

        if (fanState == 0 && heatState==0) {

         GPIO_PORTD_DATA_R = 0x00;//All OFF
         }
        panelState=0;
}

void heat_on (){


    if (fanState == 1 && panelState == 1){

             GPIO_PORTD_DATA_R = 0x4C; //All Peripherals ON
         }

        if (fanState == 1 && panelState==0){

                   GPIO_PORTD_DATA_R = 0x48; //Turns Heating ON, LED OFF, Everything else ON
               }
        if (fanState == 0 && panelState==1){

                   GPIO_PORTD_DATA_R = 0x0C; //Turns Heating ON, FAN OFF, LED ON
               }

        if (fanState == 0 && panelState==0) {

         GPIO_PORTD_DATA_R = 0x08; //Heating ON Only
         }
        heatState=1;
}

void heat_off (){

    if (fanState == 1 && panelState == 1){

              GPIO_PORTD_DATA_R = 0x44; //All Peripherals ON except Heating
          }

         if (fanState == 1 && panelState==0){

                    GPIO_PORTD_DATA_R = 0x40; //Turns Heating OFF, LED OFF, Everything else ON
                }
         if (fanState == 0 && panelState==1){

                    GPIO_PORTD_DATA_R = 0x04; //Turns Heating OFF, FAN OFF, LED ON
                }

         if (fanState == 0 && panelState==0) {

          GPIO_PORTD_DATA_R = 0x00; //Heating ON Only
          }
         heatState=0;
}

void fan_on(){

        if (panelState == 1 && heatState == 1){

             GPIO_PORTD_DATA_R = 0x4C; //All Peripherals ON
         }

        if (panelState == 1 && heatState==0){

                   GPIO_PORTD_DATA_R = 0x44; //Keeps Heating Pad Off, Everything else ON
               }
        if (panelState == 0 && heatState==1){

                   GPIO_PORTD_DATA_R = 0x48; ////Keeps LED Panel Off, Everything else ON
               }

         if (panelState == 0 && heatState==0) {

         GPIO_PORTD_DATA_R = 0x40;//Fan On Only

         }
        fanState=1;
}

void fan_off(){
    if (panelState == 1 && heatState == 1){

             GPIO_PORTD_DATA_R = 0x0C; //All Peripherals ON
         }

        if (panelState == 1 && heatState==0){

                   GPIO_PORTD_DATA_R = 0x04; //Turns Fan Off, Everything else ON
               }
        if (panelState == 0 && heatState==1){

                   GPIO_PORTD_DATA_R = 0x08; ////Turns Fan Off, Everything else ON
               }

        if (panelState == 0 && heatState==0){

         GPIO_PORTD_DATA_R = 0x00;//ALL Peripherals OFF
         }

        fanState=0;
}

int main(){
    keypad_Init();
    portf_init();
    portd_ini();
    char a='3';
    int firsttime=1;

    while(1){

        /*
         * a=keypad_getkey();
        if (a=='5'){ // Blink Red
            PORTFDAT=0x02;
        }
        if (a=='2'){ // Blink Blue
            PORTFDAT=0x04;
        }
        if (a=='6'){ // Blink Green
            PORTFDAT=0x08;
        }
        */
        if (firsttime==1){
                spi_master_ini(); //initializing controller as master
                clr_screen();
                delay(1000);
                str_pos(2, 4);
                send_str("Welcome to");
                delay(50);
                str_pos(3,4);
                send_str("Seed2Weed PGS");
                delay(3000);
                firsttime=2;
        }
        delay(100);
        clr_screen();

        str_pos(2, 4);
        delay(100);
        if (a=='1'){
            send_str("Temp is: ");
            delay(100);
            str_pos(4, 4);
            send_str("3 - Main Menu");
        }
        else if (a=='2'){
            send_str("Humidity is: ");
            delay(100);
            str_pos(4, 4);
            send_str("3 - Main Menu");
        }
        else if (a=='4'){
           switch (fanState){
           case 0 :
               str_pos(2, 3);
               send_str("Turning Fan On...");
               fan_on();
               delay(100);
               clr_screen();
               delay(100);
               str_pos(2, 4);
               send_str("Fan is On!");
               delay(500);
               str_pos(4, 4);
               send_str("3 - Main Menu");
               break;

           case 1 :

             str_pos(2, 3);
             send_str("Turning Fan Off...");
             fan_off();
             delay(100);
             clr_screen();
             str_pos(2, 4);
             send_str("Fan is Off!");
             delay(500);
             str_pos(4, 4);
             send_str("3 - Main Menu");
             break;
            }
        }
           else if (a=='5'){
                    switch (panelState){
                    case 0 :
                        str_pos(2, 3);
                        send_str("Turning LED's On...");
                        ledpanel_on();
                        delay(100);
                        clr_screen();
                        delay(100);
                        str_pos(2, 4);
                        send_str("LED's are On!");
                        delay(500);
                        str_pos(4, 4);
                        send_str("3 - Main Menu");
                        break;

                    case 1 :

                      str_pos(2, 3);
                      send_str("Turning LED's Off...");
                      ledpanel_off();
                      delay(100);
                      clr_screen();
                      str_pos(2, 4);
                      send_str("LED's are Off!");
                      delay(500);
                      str_pos(4, 4);
                      send_str("3 - Main Menu");
                      break;
                     }
           }
                    else if (a=='6'){
                        switch (heatState){
                                    case 0 :
                                        str_pos(2, 3);
                                        send_str("Turning Heat On...");
                                       heat_on();
                                        delay(100);
                                        clr_screen();
                                        delay(100);
                                        str_pos(2, 4);
                                        send_str("Heat is On!");
                                        delay(500);
                                        str_pos(4, 4);
                                        send_str("3 - Main Menu");
                                        break;

                                    case 1 :

                                      str_pos(2, 3);
                                      send_str("Turning Heat Off...");
                                      heat_off();
                                      delay(100);
                                      clr_screen();
                                      str_pos(2, 4);
                                      send_str("Heat is Off!");
                                      delay(500);
                                      str_pos(4, 4);
                                      send_str("3 - Main Menu");
                                      break;
                                     }

        }
         else if (a=='3'){
            delay(1000);
            str_pos(1, 2);
            send_str("1 - Temp");
            delay(100);
            str_pos(2, 2);
            send_str("2 - Humidity");
            delay(100);
            str_pos(3, 2);

            if(fanState==0){
            send_str("4 - Turn Fan On");
            }

            if(fanState==1){
            send_str("4 - Turn Fan Off");
            }
            str_pos(4, 2);

            if(panelState==0){
             send_str("5 - Turn LED's On");
                      }

             if(panelState==1){
                      send_str("5 - Turn LED's Off");
                      }

                }

        a='z';
        while (a=='z'){
            delay(50);
            a=keypad_getkey();
        }
    }
    return 0;
}








