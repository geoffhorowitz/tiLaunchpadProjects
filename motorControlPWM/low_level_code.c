
#include "driverlib.h"
#include "msp.h"
#include "stdio.h"

#define LED1_port 		GPIO_PORT_P1
#define LED1_pin 		GPIO_PIN0
#define LED2_port		GPIO_PORT_P2
#define LED2BLU_pin		GPIO_PIN2
#define button1_port 	GPIO_PORT_P1
#define button1_pin		GPIO_PIN1
#define button2_port 	GPIO_PORT_P1
#define button2_pin		GPIO_PIN4

#define ADC_port		GPIO_PORT_P5
#define ADC_pin			GPIO_PIN5
#define PWM1_port		GPIO_PORT_P2	//TA0.1
#define PWM1_pin		GPIO_PIN4		//TA0.1
#define PWM2_port		GPIO_PORT_P5	//TA2.1
#define PWM2_pin		GPIO_PIN6		//TA2.1

//declare variables
unsigned int dcoFrequency = 3E6;
volatile int voltage = 0;
int speed = 0;
int duty = 0;
int forward = 0;
int reverse = 0;
int button1=0;
int button2=0;


int main(void){
	//hold watchdog timer
    WDT_A_holdTimer();

	////////////////////////////////////////////////////////////Initializations
	//clock frequency configuration
	MAP_CS_setDCOFrequency(dcoFrequency);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	///////////////////////////////////////////////////  Button/LED configurations
	MAP_GPIO_setAsOutputPin(LED1_port, LED1_pin);			//configures LED1 as an output
	MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin);  		//turn LED 1 off
	MAP_GPIO_setAsOutputPin(LED2_port, LED2BLU_pin);			//configures LED2 BLUE as an output
	MAP_GPIO_setOutputLowOnPin(LED2_port, LED2BLU_pin);  		//turn LED2 BLUE off

	MAP_GPIO_setAsInputPinWithPullUpResistor(button1_port, button1_pin);		//configures switch 1 as input
	MAP_GPIO_setAsInputPinWithPullUpResistor(button2_port, button2_pin);		//configures switch 2 as input


	////////////////////////////////////////////////  PWM configurations -> PWM1
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

	////////////////////////////////////////////////  PWM configurations -> PWM2
	P5SEL0 |= 0x40 ; // Set bit 6 of P2SEL0 to enable TA1.1 functionality on P5.6
	P5SEL1 &= ~0x40 ; // Clear bit 4 of P2SEL1 to enable TA1.1 functionality on P5.6
	P5DIR |= 0x40 ; // Set pin 5.6 as an output pin
    // Set Timer A period (PWM signal period)
    TA2CCR0 = 3000 ;
    // Set Duty cycle
    TA2CCR1 = 0 ;
    // Set output mode to Reset/Set
    TA2CCTL1 = OUTMOD_7 ; // Macro which is equal to 0x00e0, defined in msp432p401r.h
    // Initialize Timer A
    TA2CTL = TASSEL__SMCLK | MC__UP | TACLR ; // Tie Timer A to SMCLK, use Up mode, and clear TA0R

	/////////////////////////////////////////////////	ADC configuration:
	// Initializing ADC ----> ADC A0
	MAP_ADC14_enableModule();  //Enables ADC
	ADC14_setResolution(ADC_10BIT); // Sets number of bits
	MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_4, 0); // Sets clock source and clock rate

	// Configuring GPIOs
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(ADC_port, ADC_pin, GPIO_TERTIARY_MODULE_FUNCTION);

	// Configuring ADC Memory
	MAP_ADC14_configureSingleSampleMode(ADC_MEM0, false); // Configures conversion mode
	MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, ADC_NONDIFFERENTIAL_INPUTS); // Allocates register to hold results

	// Configuring Sample Timer
	MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION); // Configures manual vs automatic triggering

	// Enabling Conversion
	MAP_ADC14_enableConversion();  // Enables conversion

	//////////////////////////////////////////////////	Primary program loop:

    while(1){

    	///////////////////////////////////////////////// ADC Control
        MAP_ADC14_toggleConversionTrigger();  // Initiates a single conversion (trigger) - note, this trigger gets cleared automatically
        while(ADC14_isBusy()==1){}
        voltage = ADC14_getResult(ADC_MEM0); //data between 0-1023 corresponding to 0V-3.3V (3.3 by default)
        speed = voltage * 3000;
        duty = speed / 1023;

    	button1 = MAP_GPIO_getInputPinValue(button1_port, button1_pin);		//reads the value from button 1
    	if(button1==0){
    		forward++;
    	}
		if(forward==1){
			MAP_GPIO_setOutputHighOnPin(LED1_port, LED1_pin);
			TA1CCR1 = 0 ;  // ensure reverse is off
			TA0CCR1 = duty;	//turn forward on
		}else if(forward==2){
			forward=0;
			MAP_GPIO_setOutputLowOnPin(LED1_port, LED1_pin);
			TA0CCR1 = 0;  //turn forward off
		}

    	button2 = MAP_GPIO_getInputPinValue(button2_port, button2_pin);		//reads the value from button 2
    	if(button2==0){
    		reverse++;
    	}
    	if(reverse==1){
			MAP_GPIO_setOutputHighOnPin(LED2_port, LED2BLU_pin);
			TA0CCR1 = 0;  	// ensure forward is off
			TA2CCR1 = duty;	//turn reverse on
		}else if(reverse==2){
			reverse=0;
			MAP_GPIO_setOutputLowOnPin(LED2_port, LED2BLU_pin);
			TA2CCR1 = 0;  //turn reverse off
		}


    }//end while
}//end main
