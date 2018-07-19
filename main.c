#include "config.h"
#include "lcd.h"
#include "led.h"
#include "Mem.h"
#include "ssd.c"
#include "utils.h"
#include "rgbled.h"
#include "btn.h"
#include <xc.h>  
#include <sys/attribs.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
     
//==================================================
//              Global Configuration
//==================================================
// Device Config Bits in  DEVCFG1:	
#pragma config FNOSC =	FRCPLL
#pragma config FSOSCEN =	OFF
#pragma config POSCMOD =	EC
#pragma config OSCIOFNC =	ON
#pragma config FPBDIV =     DIV_1
#pragma config JTAGEN = OFF
#pragma config FWDTEN = OFF

// Device Config Bits in  DEVCFG2:	
#pragma config FPLLIDIV =	DIV_2
#pragma config FPLLMUL =	MUL_20
#pragma config FPLLODIV =	DIV_1
//Definitions for our projects.
#define _XTAL_FREQ 8000000 //freq of BASYS MX3
#define REG_SIZE 10 // the size of one register
#define MEM_SIZE 2048 // the size of the memory each program has
#define MEM_LENGTH 65536 // the maximum value can be stored in a register


//=============================================================================
// Initialization of variables. we used all the variables in this project 
// as global variables in order to make it easier to read,
// without any pointers included.
//=============================================================================
//general
int pause = 0; // pause has 3 states. 0 = execute, 1 = dont execute, 2= single execution
int INST;
int reg[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int IORegister[3] = {0,0,0};
int pc = 0;
long currentTime=0;
int sw7;
int temp;
int temp2=85;

//flags
int pressed_flag[5] = {0,0,0,0,0};
int btn_flag[5] = {0,0,0,0,0};
int swt_flag[8] = {0,0,0,0,0,0,0,0};
int run;
int func_flag; //shows which program is running. 0=Fib, 1= LEDs
int imm_flag=0;
int prog=0;

//counters
float offset62 = 0.0;
//float offset1000 = 0.0;
int offset1=0;
float counter = 0;

//strings
char str[80];
char line1[80];
char line2[16];
char str2[80];

//indexes
int mem_ind = 0;
int reg_ind = 2;
int reg_sec=3;
int ind=0;

//=============================================================================
// END OF VARIABLES INITIALIZATION
//=============================================================================
void Timer4Setup()
{
    static int fTimerInitialised = 0;
    if(!fTimerInitialised)
    {        
                                          //  setup peripheral
        PR4 = 10000;                        //             set period register, generates one interrupt every 1 ms
        TMR4 = 0;                           //             initialize count to 0
        T4CONbits.TCKPS = 3;                //            1:256 prescale value
        T4CONbits.TGATE = 0;                //             not gated input (the default)
        T4CONbits.TCS = 0;                  //             PCBLK input (the default)
        T4CONbits.ON = 1;                   //             turn on Timer1
        IPC4bits.T4IP = 2;                  //             priority
        IPC4bits.T4IS = 0;                  //             subpriority
        IFS0bits.T4IF = 0;                  //             clear interrupt flag
        IEC0bits.T4IE = 1;                  //             enable interrupt
        fTimerInitialised = 1;
        macro_enable_interrupts();
    }
}



//==============================================================================
//Interrupt Handler- handled every 1msec
//==============================================================================
void __ISR(_TIMER_4_VECTOR, ipl2auto) Timer4SR(void)
{currentTime++;

if (pressed(1)) // the function pressed definition is below.
    if (pause==1) pause = 0; // pause explanation at initialization of the variable
    else pause = 1;
if (pressed(3) && pause==1) pause=2; //single step
if (pressed(2)) IORegister[2]++; // This register counts the number of pressed on BTNC
if (pressed(0)) pressed_flag[0]=1; 
if (pressed(4)) pressed_flag[4]=1;
for (ind=0;ind<5;ind++){btn_flag[ind]=BTN_GetValue(ind);}

if ((currentTime-offset62)>62.5){
    // if 62.5 msecs has passed, increase the counters, and turn on the running
    // flag, so the main function will execute the next assembly line.
        offset62+=62.95;
        IORegister[0]=(IORegister[0]+1)%0xFFFFFFFF;
        run=1;}

    
IFS0bits.T4IF = 0;                  // clear interrupt flag
    }
int ctoi(char c) { //char to integer
	if (c <= '9') { return (c - '0'); }
	else if (c >= 'a') { return (c - 'a' + 10); }
	else {
		return (c - 'A' + 10);
	}
}
int function_executer(){  
	/*this function is the core of the simulator, translating the hexadecimal
      to actual actions and values.
      execution is very similiar to how we did it in the first project.*/
    signed int rd, rs, rt;
	char imm[REG_SIZE];
    counter++; //counts the number of operations done since last reset.
    strcpy(str,MEM[pc+(func_flag*2048)]);
    if (pause==2) pause=1; // if we are on single step mode, pause for next iteration
	rd = ctoi(str[1]);
	rs = ctoi(str[2]);
	rt = ctoi(str[3]);
    reg[1] = 0;
    reg[0] = 0;
	if ((rd == 1) || (rs == 1) || (rt == 1)) {
		strcpy(imm, MEM[pc+1+(func_flag*2048)]);
		reg[1] = strtol(imm, NULL, 16);
		if (imm[0] > '7') 
            reg[1] -= MEM_LENGTH; 
		pc++;
        imm_flag=1;
        
	}
    else imm_flag=0;
	if (str[0] == '0') { reg[rd] = reg[rs] + reg[rt]; }
	else if (str[0] == '1') { reg[rd] = reg[rs] - reg[rt]; }
	else if (str[0] == '2') { reg[rd] = reg[rs] & reg[rt]; }
	else if (str[0] == '3') { reg[rd] = reg[rs] | reg[rt]; }
	else if (str[0] == '4') { reg[rd] = reg[rs] << reg[rt]; }
	else if (str[0] == '5') { reg[rd] = reg[rs] >> reg[rt]; }
	else if (str[0] == '6') { if (reg[rt]==0) {reg[rd]=IORegister[reg[rs]];} 
                              else {    if (rs!=0) 
                                            IORegister[reg[rs]]=reg[rd];
                                    }
                            }
	else if (str[0] == '7') { if (reg[rs] == reg[rt]) { pc = reg[rd] - 1; } }
	else if (str[0] == '8') {
        if (reg[rs] > reg[rt]) { pc = reg[rd] -     1 ; } }
	else if (str[0] == '9') { if (reg[rs] <= reg[rt]) { pc = reg[rd] - 1; } }
	else if (str[0] == 'A' || str[0] == 'a') { if (reg[rs] != reg[rt]) { pc = reg[rd] - 1; } }
	else if (str[0] == 'B' || str[0] == 'b') { reg[15] = (pc+1);  pc = reg[rd] - 1; }
	else if (str[0] == 'C' || str[0] == 'c') {
		reg[rd] = strtol(MEM[((reg[rs] + reg[rt]) & 0x000007FF)+(func_flag*2048)], NULL, 16); 
		if (reg[rd] > 0x7FFF) reg[rd] -= MEM_LENGTH;
	}
	else if (str[0] == 'D' || str[0] == 'd') {
		// making sure words that are stored in the memory are with size of 4 hexadecimal digits.
		sprintf(imm, "%.4X",reg[rd] & 0x0000FFFF);
		strcpy(MEM[((reg[rs] + reg[rt]) & 0x000007FF)+(func_flag*2048)], imm); 
	}
	else if (str[0] == 'E' || str[0] == 'e') {
		reg[rd] = (reg[rd] & 0x0000FFFF) + pow(2, 16)*(reg[rs]);
	}
	else if (str[0] == 'F' || str[0] == 'f') { pause = 1; pc=-1;}
	return 0;
}
int pressed(int i)
{
    /* This function is used in order to make sure every pressing on buttons 
     * will be counted as one press, and will happen while releasing the button.
     * we did it with the following idea:
     * BTN_GetValue(i)        btn_flag      return
     *      0                     1             1    button has been released. count
     *      1                     0             0    button has been pressed.
     *      0                     0             0    button is not pressed and was not released since last iteration.
     *      1                     1             0    button is pressed but was pressed in last iteration as well.
     */
    if (!BTN_GetValue(i)&&btn_flag[i]) return 1;
    return 0;}


int switched(int i)
{/* This function is used to make the usage of switch as a change of mode, not as a constant mode. 
     * we did it with the following idea:
     * SWT_GetValue(i)        swt_flag      return
     *      0                     1             -1   switch changed from up to down.
     *      1                     0             1    switch changed from down to up
     *      0                     0             0    switch remained down.
     *      1                     1             0    switch remained up.
     */
    if (SWT_GetValue(i)&&!swt_flag[i]) {swt_flag[i]=1;return 1;}
else if ((!SWT_GetValue(i))&&swt_flag[i]){swt_flag[i]=0; return -1;}

return 0;}
//==============================================================================
//this function initializes the needed components
//==============================================================================
void init(){
    LCD_Init(); 
    LED_Init();
    BTN_Init();
    SWT_Init();
    SSD_Init();
    //RGBLED_Init();
    Timer4Setup();
}

void main(){
    init(); //initialize all needed components
    while(1){
        LED_SetGroupValue(IORegister[1]&0x000000FF);
        if (run&&(pause!=1)) {
            // run function executer only if:
            // 1. 62.5 msec has passed since last execute (run flag controls it)
            // 2. we're not in pause. execute when running or single step mode.
            function_executer();
            pc++;
            run = 0;
        }

   if (!(SWT_GetValue(0)) && !(SWT_GetValue(1))&&!prog)
        { // calculates the INST and showing on LCD screen
       INST = strtol(MEM[pc+(func_flag*2048)], NULL, 16);
       INST=INST&0xFFFF;
        if (imm_flag == 1) 
            {temp = strtol(MEM[pc+1+(func_flag*2048)],NULL,16);
            if (temp>0x7FFF) 
                temp-=MEM_LENGTH;
            INST += temp * pow(2, 16);}
            sprintf(line1,"INST:                 ");
            sprintf(line2,"%.8X                  ",INST);
            LCD_WriteStringAtPos(line2,1,0);
            LCD_WriteStringAtPos(line1, 0 , 0);
            pressed_flag[0]=0;
            pressed_flag[4]=0;
        }
    if (SWT_GetValue(0) && !(SWT_GetValue(1))&&!prog){
        // We wanted this function to be more friendly, so we used both lines
        // to show two items of reg array. the following lines are basically 
        // to make this "feature" more humane and intuitive.
            if(pressed_flag[0])
                {reg_ind--; 
                reg_sec--;
                if (reg_ind<0) {reg_ind=14; reg_sec=15;}
                else if (reg_sec < 0) {reg_sec=15;}
                 pressed_flag[0]=0;}
            
            if (pressed_flag[4]) 
                {reg_ind++;
                reg_sec++;
                if(reg_ind>15) {
                    reg_ind=0;
                    reg_sec=1;
                }
                else if (reg_sec>15) 
                    reg_sec = 0;
                pressed_flag[4]=0;}
            sprintf(line1,"R%.2d=%.8X             ",reg_ind, reg[reg_ind]);
            sprintf(line2,"R%.2d=%.8X             ",reg_sec, reg[reg_sec]);
            LCD_WriteStringAtPos(line2,1,0);
            LCD_WriteStringAtPos(line1,0,0);
    }

    if ((SWT_GetValue(1))&&(SWT_GetValue(0))&&!prog)
        {   sprintf(line2,"%.0f                  ",counter);
        LCD_WriteStringAtPos(line2,1,0);
        LCD_WriteStringAtPos("counter:             ",0,0); 
        pressed_flag[0]=0;
        pressed_flag[4]=0;
    }
    if ((SWT_GetValue(1)) && !(SWT_GetValue(0))&&!prog){
        // show the MEM items, with different starting points.
        // also print the Register stack pointer.
        if (switched(5)!=0 || switched(6)!=0)
           if (!SWT_GetValue(6)){ 
               if (SWT_GetValue(5)) 
                   mem_ind = 1024;
               else  
                   mem_ind = 0;
               swt_flag[5] = 0; 
               swt_flag[6] = 0;
           }
           else 
               mem_ind = 2047;
           if(pressed_flag[0])
                {mem_ind--; 
                if (mem_ind==-1) mem_ind=2047;
                 pressed_flag[0]=0;}
            if (pressed_flag[4]) 
                {mem_ind++;
                if(mem_ind==2048) mem_ind=0;
                pressed_flag[4]=0;}
           swt_flag[6] = SWT_GetValue(6);
           swt_flag[5] = SWT_GetValue(5);
           //print to LCD screen the right MEM, from the right function
           sprintf(line1,"MEM[%.4X]=%s         ",mem_ind,MEM[mem_ind+func_flag*2048]); 
           LCD_WriteStringAtPos(line1, 0, 0);
           sprintf(line2,"RSP=%.8x                 ",reg[13]); 
           LCD_WriteStringAtPos(line2,1,0); 
    }
    if (SWT_GetValue(3)&& pause==1){
        /*we have added an interactive functionality, which allows us to program
         specific places in the memory, in order to calculate other fibonnaci 
         items, or making the light go in different velocity.
         Edits the value of the current program paused.*/
         prog =1;
        if(pressed_flag[0]){ 
            // add one (calculate higher items of fibbonaci, or make the lights go slower)
            temp = 1+strtol(MEM[1024+func_flag*(2048-1014)], NULL, 16); 
            // dont calculate values higher than 10, dont go slower than 1 light change per 10 seconds
            if (temp==11+func_flag*127) temp = 10+func_flag*127; 
            pressed_flag[0]=0;
            sprintf(MEM[1024+func_flag*(2048-1014)], "%.4X", temp & 0x0000FFFF);
        }
        if(pressed_flag[4]){ 
            // subtract one (calculate lower item of fibbonaci, or make the lights go faster)
            temp = -1+strtol(MEM[1024+func_flag*(2048-1014)], NULL, 16);
            // dont calculate values lower than 0 to fibonnaci
            if (temp==-1) temp =0;
            pressed_flag[4]=0;
            sprintf(MEM[1024+func_flag*(2048-1014)], "%.4X", temp & 0x0000FFFF);}
         sprintf(line2,"MEM[%.4X]=%s         ",1024+func_flag*(-1014),MEM[1024+func_flag*(2048-1014)]);
         strcpy(line1,"**Programming**    " );
         LCD_WriteStringAtPos(line2, 1, 0);
         LCD_WriteStringAtPos(line1, 0, 0);
         // writes Prog on 7-Segment screen 
         // we added the letters p,r to the array which saves all the hexadecimal numbers and letters.
         // in the indexes 17,18 respectively.
         SSD_WriteDigits(9,0,18,17,0,0,0,0);     
    }
    else
        prog=0;

    sw7 = switched(7);
    if (sw7 == 1)  // switch to led program
    {func_flag=1;
      pc=0;
    }
    if (sw7 == -1)
    {func_flag=0; pc=0; IORegister[1]=0;} // switch to fibbonaci program, turn of LEDs  
    if (prog==0) 
        SSD_WriteDigitsGrouped(pc,0); // use 7segment display with pc printed on it.
    }
}






