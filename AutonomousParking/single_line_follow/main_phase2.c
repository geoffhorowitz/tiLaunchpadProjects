#include "driverlib.h"
#include "stdio.h"

/*
 * Set-up Notes:
 *
 * Looking down on chassis with single wheel in back.
 * Motor 1 is on left
 * Motor 2 is on right
 * Line sensor 1 is on left
 * Line sensor 2 is in center
 * Line sensor 3 is on right
 *
 */

//inputs
#define IR1_port 		GPIO_PORT_P6		//interrupt associated with this port.  Try not to put anything else to it.
#define IR1_pin			GPIO_PIN4

#define Line1_port 		GPIO_PORT_P3
#define Line1_pin		GPIO_PIN0
#define Line2_port 		GPIO_PORT_P3
#define Line2_pin		GPIO_PIN2
#define Line3_port 		GPIO_PORT_P3
#define Line3_pin		GPIO_PIN3

#define button1_port 	GPIO_PORT_P1
#define button1_pin		GPIO_PIN1
#define button2_port 	GPIO_PORT_P1
#define button2_pin		GPIO_PIN4

//outputs
#define LED1_port 		GPIO_PORT_P1
#define LED1_pin		GPIO_PIN0
#define LED2_port		GPIO_PORT_P2
#define LED2BLU_pin		GPIO_PIN2
#define LED2GRE_pin		GPIO_PIN1

#define motor1for		TA0CCR1			//P2.4	//TA0.1 - DON'T MESS WITH THESE
#define motor1rev		TA2CCR1			//P5.6	//TA2.1 - DON'T MESS WITH THESE

#define motor2for		TA0CCR2			//P2.5	//TA0.2 - DON'T MESS WITH THESE
#define motor2rev		TA2CCR2			//P5.7	//TA2.2 - DON'T MESS WITH THESE

//1. use line following sensors to go in straight line, turning each wheel independently as needed to correct for off-angle

// Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_64,         // increments counter every 64 clock cycles
        9375,		                           // to produce an interrupt every second (3M/64 = 46875) every 5th of a second (.2s)
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

//declare variables
unsigned int dcoFrequency = 3E6;
int duty = 1500;		//50% duty cycle

int ir1 = 0;
int line1 = 0;
int line1_flag = 0;
int line2 = 0;
int line2_flag = 0;
int line3 = 0;
int line3_flag = 0;
int button1 = 0;
int button2 = 0;
int count = 0;
int time = 0;
int white = 0;
int black = 1;

int main(void){
    //hold watchdog timer
	WDT_A_holdTimer();

	//Disable master interrupts
	MAP_Interrupt_disableMaster();

	//////////////////////////////////////////////////// Clock Frequency Initializations
	//clock frequency configuration
	MAP_CS_setDCOFrequency(dcoFrequency);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	///////////////////////////////////////////////////  Button/LED configurations
    //configure the buttons and LEDs
    MAP_GPIO_setAsOutputPin(LED1_port, LED1_pin);
    MAP_GPIO_setAsOutputPin(LED2_port, LED2BLU_pin);
    MAP_GPIO_setAsOutputPin(LED2_port, LED2GRE_pin);
    MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin);
    MAP_GPIO_setOutputLowOnPin(LED2_port, LED2BLU_pin);
    MAP_GPIO_setOutputLowOnPin(LED2_port, LED2GRE_pin);

    MAP_GPIO_setAsInputPinWithPullUpResistor(button1_port, button1_pin);		//configures switch 1 as input
    MAP_GPIO_setAsInputPinWithPullUpResistor(button2_port, button2_pin);		//configures switch 2 as input

	//////////////////////////////////////////////////  Timer A configurations
    // Configuring Timer_A2 for Up Mode
    MAP_Timer_A_configureUpMode(TIMER_A1_MODULE, &upConfig);

    // Enabling interrupts and starting the timer
    MAP_Interrupt_enableInterrupt(INT_TA1_0); //enables the timer A interrupt

    ///////////////////////////////////////////////////  Sensor Configurations
    MAP_GPIO_setAsInputPinWithPullUpResistor(IR1_port, IR1_pin);
    MAP_GPIO_setAsInputPinWithPullUpResistor(Line1_port, Line1_pin);
    MAP_GPIO_setAsInputPinWithPullUpResistor(Line2_port, Line2_pin);
    MAP_GPIO_setAsInputPinWithPullUpResistor(Line3_port, Line3_pin);

    //sensor interrupts
    MAP_GPIO_clearInterruptFlag(IR1_port, IR1_pin);
    MAP_GPIO_enableInterrupt(IR1_port, IR1_pin);
    MAP_Interrupt_enableInterrupt(INT_PORT6);

	//////////////////////////////////////////////////  PWM configurations -> motor 2 forward
	P2SEL0 |= 0x10 ; // Set bit 4 of P2SEL0 to enable TA0.1 functionality on P2.4
	P2SEL1 &= ~0x10 ; // Clear bit 4 of P2SEL1 to enable TA0.1 functionality on P2.4
	P2DIR |= 0x10 ; // Set pin 2.4 as an output pin
    // Set Timer A period (PWM signal period)
    TA0CCR0 = 3000 ;
    // Set Duty cycle
    TA0CCR1 = 0 ;
    // Set output mode to Reset/Set
    TA0CCTL1 = OUTMOD_7 ; // Macro which is equal to 0x00e0, defined in msp432p401r.h
    // Initialize Timer A
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR ; // Tie Timer A to SMCLK, use Up mode, and clear TA0R

    //////////////////////////////////////////////////  PWM configurations -> motor 1 reverse
	P5SEL0 |= 0x40 ; // Set bit 6 of P2SEL0 to enable TA1.1 functionality on P5.6
	P5SEL1 &= ~0x40 ; // Clear bit 6 of P2SEL1 to enable TA1.1 functionality on P5.6
	P5DIR |= 0x40 ; // Set pin 5.6 as an output pin
    // Set Timer A period (PWM signal period)
    TA2CCR0 = 3000 ;
    // Set Duty cycle
    TA2CCR1 = 0 ;
    // Set output mode to Reset/Set
    TA2CCTL1 = OUTMOD_7 ; // Macro which is equal to 0x00e0, defined in msp432p401r.h
    // Initialize Timer A
    TA2CTL = TASSEL__SMCLK | MC__UP | TACLR ; // Tie Timer A to SMCLK, use Up mode, and clear TA0R

	//////////////////////////////////////////////////  PWM configurations -> motor 2 forward
	P2SEL0 |= 0x20 ; // Set bit 5 of P2SEL0 to enable TA0.2 functionality on P2.5
	P2SEL1 &= ~0x20 ; // Clear bit 5 of P2SEL1 to enable TA0.2 functionality on P2.5
	P2DIR |= 0x20 ; // Set pin 2.5 as an output pin
    // Set Timer A period (PWM signal period)
    TA0CCR0 = 3000 ;
    // Set Duty cycle
    TA0CCR2 = 0 ;
    // Set output mode to Reset/Set
    TA0CCTL2 = OUTMOD_7 ; // Macro which is equal to 0x00e0, defined in msp432p401r.h
    // Initialize Timer A
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR ; // Tie Timer A to SMCLK, use Up mode, and clear TA0R

	//////////////////////////////////////////////////  PWM configurations -> motor 2 reverse
	P5SEL0 |= 0x80 ; // Set bit 7 of P2SEL0 to enable TA1.2 functionality on P5.7
	P5SEL1 &= ~0x80 ; // Clear bit 7 of P2SEL1 to enable TA1.2 functionality on P5.7
	P5DIR |= 0x80 ; // Set pin 5.2 as an output pin
    // Set Timer A period (PWM signal period)
    TA2CCR0 = 3000 ;
    // Set Duty cycle
    TA2CCR2 = 0 ;
    // Set output mode to Reset/Set
    TA2CCTL2 = OUTMOD_7 ; // Macro which is equal to 0x00e0, defined in msp432p401r.h
    // Initialize Timer A
    TA2CTL = TASSEL__SMCLK | MC__UP | TACLR ; // Tie Timer A to SMCLK, use Up mode, and clear TA0R

    /////////////////////////////////////////////////	Enable Master Interrupt
    //Interrupt_setPriority (INT_PORT6,0);
    //Interrupt_setPriority (INT_TA1_0,1);
    MAP_Interrupt_enableMaster();


	//main while loop
    while(1){
    	button1 = MAP_GPIO_getInputPinValue(button1_port, button1_pin);		//reads the value from button
    	if(button1==0){
    		MAP_GPIO_setOutputHighOnPin(LED1_port, LED1_pin); 					//turn on LED 1 to indicate program start
    	    MAP_Timer_A_startCounter(TIMER_A1_MODULE, TIMER_A_UP_MODE); //starts timer A
    	    while (time<2){}	//waits 2 seconds before beginning to move

    	    //have car start moving using 70% duty cycle, then slow down to pre-defined duty cycle
    		motor1for = 2100;
    		motor2for = 2100;
    		time = 0;
    	    while (time<1){}
    		motor1for = duty;
    		motor2for = duty;
    	}

    	while (button1==0){
    		button2 = MAP_GPIO_getInputPinValue(button2_port, button2_pin);
    		ir1 = MAP_GPIO_getInputPinValue(IR1_port, IR1_pin);					//This value is high when infinite distance -> can change with pull down resistor if desired
    		line1 = MAP_GPIO_getInputPinValue(Line1_port, Line1_pin);
    		line3 = MAP_GPIO_getInputPinValue(Line3_port, Line3_pin);


			if(line1==black){//off line
				MAP_GPIO_setOutputHighOnPin(LED2_port, LED2BLU_pin);
				motor2for = 0;
				time = 0;
				while (time<1){};
				motor2for = duty;
				time = 0;
				while (time<1){};
			}else{
				MAP_GPIO_setOutputLowOnPin(LED2_port, LED2BLU_pin);
			}//end if line1

			if(line3==black){//off line
				MAP_GPIO_setOutputHighOnPin(LED2_port, LED2BLU_pin);
				motor1for = 0;
				time = 0;
				while (time<1){};
				motor1for = duty;
				time = 0;
				while (time<1){};
			}else{
				MAP_GPIO_setOutputLowOnPin(LED2_port, LED2BLU_pin);
			}//end if line1

    		if (button2==0){
    			motor1for = 0;
    			motor2for = 0;
    			button1 = 0;
    			MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin);
    		}

    	}//end while button 1
    }//end main while
}//end main

// Timer A ISR
void timer_a_1_isr(void){
	time++;
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //resets CC counter
}

void port6_isr(void)
{
    uint32_t status = MAP_GPIO_getEnabledInterruptStatus(IR1_port);

    if (status & IR1_pin){
		motor1for = 0;
		motor2for = 0;
		button1 = 0;
		MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin); 				//turn off LED 1 to indicate program end
    }

	button1 = MAP_GPIO_getInputPinValue(button1_port, button1_pin);		//reads the value from button
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, status);
}

