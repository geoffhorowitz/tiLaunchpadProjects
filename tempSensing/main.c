#include "driverlib.h"
#include "stdio.h"

#define ADC_port		GPIO_PORT_P5
#define ADC_pin			GPIO_PIN5
#define PWM1_port		GPIO_PORT_P2	//TA0.1
#define PWM1_pin		GPIO_PIN4		//TA0.1
#define button2_port 	GPIO_PORT_P1
#define button2_pin		GPIO_PIN4

//declare variables
unsigned int dcoFrequency = 3E6;
volatile float voltage = 0;
float temp = 0;
float tempout = 0;
int button2 = 0;
int duty = 0;
int perc = 0;

// Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_64,         // increments counter every 64 clock cycles
        46875,		                           // to produce an interrupt every second (3M/64 = 46875)
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

int main(void) {
	//hold watchdog timer
    WDT_A_holdTimer();

    //enable floating point module
    FPU_enableModule();

	//Disable master interrupts
	MAP_Interrupt_disableMaster();

	////////////////////////////////////////////////////////////Initializations
	//clock frequency configuration
	MAP_CS_setDCOFrequency(dcoFrequency);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	//button
    MAP_GPIO_setAsInputPinWithPullUpResistor(button2_port, button2_pin);		//configures switch 1 as input


	//////////////////////////////////////////////////  Timer A configurations
    // Configuring Timer_A2 for Up Mode
    MAP_Timer_A_configureUpMode(TIMER_A2_MODULE, &upConfig);

    // Enabling interrupts and starting the timer
    MAP_Interrupt_enableInterrupt(INT_TA2_0); //enables the timer A interrupt
    MAP_Timer_A_startCounter(TIMER_A2_MODULE, TIMER_A_UP_MODE); //starts timer A

	////////////////////////////////////////////////  PWM configurations -> PWM1
	P2SEL0 |= 0x10 ; // Set bit 4 of P2SEL0 to enable TA0.1 functionality on P2.4
	P2SEL1 &= ~0x10 ; // Clear bit 4 of P2SEL1 to enable TA0.1 functionality on P2.4
	P2DIR |= 0x10 ; // Set pin 2.4 as an output pin
    // Set Timer A period (PWM signal period)
    TA0CCR0 = 3000 ;
    // Set Duty cycle
    TA0CCR1 = 600 ;  		 				// start at a 20% duty cycle
    // Set output mode to Reset/Set
    TA0CCTL1 = OUTMOD_7 ; // Macro which is equal to 0x00e0, defined in msp432p401r.h
    // Initialize Timer A
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR ; // Tie Timer A to SMCLK, use Up mode, and clear TA0R

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

    //Enable master interrupts
    MAP_Interrupt_enableMaster();

    while(1){
    	button2 = MAP_GPIO_getInputPinValue(button2_port, button2_pin);
    	if(button2 == 0){
    		duty = duty + 300;
			if(duty==2400){
				duty = 600;
			}
			TA0CCR1 = duty;		//change duty cycle
			perc = (duty*100)/3000;
			printf("\nduty cycle = %i%%\n",perc);

    	}
    }//end while
}//end main


// Timer A ISR
void timer_a_2_isr(void){

	MAP_ADC14_toggleConversionTrigger();  // Initiates a single conversion (trigger) - note, this trigger gets cleared automatically
	while(ADC14_isBusy()==1){}

	voltage = ADC14_getResult(ADC_MEM0); //data between 0-255 corresponding to 0V-3.3V (3.3 by default)
	temp = voltage / 1023; //gets value between 0 & 3.3 V by resolution formula --> (vmax-vmin)/(2^N-1) = (3.3-0)/(1024-1) -> see below for the 3.3V conversion
	tempout = temp * 330; // convert data to temp 0-2.5 V -> 0-250 F. Therefore (250/2.5)= 100*3.3V = 330

	printf("%f \n", tempout);

	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A2_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //resets CC counter
}
