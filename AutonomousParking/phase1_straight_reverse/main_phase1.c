
#include "driverlib.h"
#include "stdio.h"

#define sensor1_port 	GPIO_PORT_P6
#define sensor1_pin		GPIO_PIN4
#define button1_port 	GPIO_PORT_P1
#define button1_pin		GPIO_PIN1
#define LED1_port 		GPIO_PORT_P1
#define LED1_pin		GPIO_PIN0

#define PWM1f_port		GPIO_PORT_P2	//TA0.1
#define PWM1f_pin		GPIO_PIN4		//TA0.1
#define PWM1r_port		GPIO_PORT_P5	//TA2.1
#define PWM1r_pin		GPIO_PIN6		//TA2.1

#define PWM2f_port		GPIO_PORT_P2	//TA0.2
#define PWM2f_pin		GPIO_PIN5		//TA0.2
#define PWM2r_port		GPIO_PORT_P5	//TA2.1
#define PWM2r_pin		GPIO_PIN7		//TA2.2

//1. assuming we start straight, turn on motors, back up till IR sensor dings, stop motor.
//program starts by turning motors (slowly) in reverse.

// Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_64,         // increments counter every 64 clock cycles
        46875,		                           // to produce an interrupt every second (3M/64 = 46875)
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

int time = 0;

int main(void){
    //hold watchdog timer
	WDT_A_holdTimer();

    //declare variables
    unsigned int dcoFrequency = 3E6;
    int sensor1 = 0;
    int button1 = 0;
    int duty = 1500;		//75% duty cycle
    int firsttime=0;

	//////////////////////////////////////////////////// Clock Frequency Initializations
	//clock frequency configuration
	MAP_CS_setDCOFrequency(dcoFrequency);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	//////////////////////////////////////////////////  Timer A configurations
    // Configuring Timer_A1 for Up Mode
    MAP_Timer_A_configureUpMode(TIMER_A1_MODULE, &upConfig);

    // Enabling interrupts and starting the timer
    MAP_Interrupt_enableInterrupt(INT_TA1_0); //enables the timer A interrupt

	///////////////////////////////////////////////////  Button/LED configurations
    //configure the buttons and LEDs
    MAP_GPIO_setAsOutputPin(LED1_port, LED1_pin);			//configures LED1 as an output
    MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin);  					//turn LED 1 off

    MAP_GPIO_setAsInputPinWithPullUpResistor(button1_port, button1_pin);		//configures switch 1 as input

    ///////////////////////////////////////////////////  Sensor Configurations
    MAP_GPIO_setAsInputPinWithPullUpResistor(sensor1_port, sensor1_pin);		//configures sensor 1 as input

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



	//main while loop
    while(1){
    	button1 = MAP_GPIO_getInputPinValue(button1_port, button1_pin);		//reads the value from button

    	while (button1==0){
			MAP_GPIO_setOutputHighOnPin(LED1_port, LED1_pin); 					//turn on LED 1 to indicate program start

    		if (firsttime==0){
    			MAP_GPIO_setOutputHighOnPin(LED1_port, LED1_pin); 					//turn on LED 1 to indicate program start
				MAP_Timer_A_startCounter(TIMER_A1_MODULE, TIMER_A_UP_MODE); //starts timer A
				while (time<3){}	//waits 3 seconds before beginning to move
				MAP_Interrupt_disableInterrupt(INT_TA1_0);
				time = 0;
				firsttime = 1;
			}


    		sensor1 = MAP_GPIO_getInputPinValue(sensor1_port, sensor1_pin);		//reads the value from sensor - note that default value is high for infinite distance.


    		if(sensor1==1){
    			//motor 1
    			TA2CCR1 = 0;	//turn reverse on
    			TA0CCR1 = duty;  	//ensure forward is off

    			//motor 2
    			TA2CCR2 = 0;	//turn reverse on
    			TA0CCR2 = duty;  	//ensure forward is off
    		}else{
    			TA2CCR1 = 0;	//turn reverse off -> motor 1
    			TA2CCR2 = 0;	//turn reverse off --> motor 2


    			MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin); 					//turn off LED 1 to indicate program start
    		}

    	}//end while
    }//end while
}//end main

// Timer A ISR
void timer_a_1_isr(void){
	time++;
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //resets CC counter
}

