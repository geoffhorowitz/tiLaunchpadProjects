
//#includes
#include "driverlib.h"
#include "msp.h"
#include "stdio.h"
#include "ledhelper.h"

//additional #defines
#define MAIN_1_SECTOR_31 0x0003F000 //to identify sector 31 of bank 1 main flash memory

//declare variables
int totalMeasurements = 30;
int numchars = 0;
int newLine = 10; //New Line value
int carriageReturn = 13; // Carriage Return value

volatile float temp = 0;
volatile float tempout = 0;
volatile float allMeas[120] = {};
volatile float output_from_flash_float[120];
char string_from_flash[120]={}; // array of characters (ASCII)
char toprint; //to transmit each character independently
unsigned int dcoFrequency = 0;

int but1 = 0;	//used to assign the input value of switch 1
int but2 = 0;	//used to assign the input value of switch 2
int inputcount = 0; //counts number of input measurements
int outputcount = 0; //counts number of output measurements
int i=0; //garbage varable for loop
int j=0; //garbage varable for loop

// Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_64,         // increments counter every 64 clock cycles
        187500,		                           // to produce an interrupt every second (3M/64 = 46875) (12M/64 = 187500)
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

// Set configure UART communication
const eUSCI_UART_Config UART_init = {
		EUSCI_A_UART_CLOCKSOURCE_SMCLK,	//SMCLK Clock Source
		19,								//clockPrescalar
		8,								//FirstModReg
		85,								//SecondModReg
		EUSCI_A_UART_NO_PARITY,			//No Parity
		EUSCI_A_UART_LSB_FIRST,			//LSB First -> to have normal ASCII output
		EUSCI_A_UART_ONE_STOP_BIT,		//One Stop Bit
		EUSCI_A_UART_MODE,				//UART Mode
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,	//Oversampling
};

//baud rate 			9600	38400	9600
//clock rate 			3MHz	3MHz	12MHz
//clockPrescalar = 		19		4		78
//firstModReg = 		8		14		2
//secondModReg=			85		0		0
//overSampling=			1		1		1


int main(void) 
{
    WDT_A_holdTimer();

	//clock frequency configuration
	dcoFrequency = 3E+6; //12E+6;
	MAP_CS_setDCOFrequency(dcoFrequency);
	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    //enable floating point module
    FPU_enableModule();

	//Disable master interrupts
	MAP_Interrupt_disableMaster();

    ///////////////////////////////////////////////////  Button/LED configurations
    //configure the buttons and LEDs
    MAP_GPIO_setAsInputPinWithPullUpResistor(PORT1, S1);		//configures switch 1 as input
    MAP_GPIO_setAsOutputPin(PORT1, LED1);						//configures LED1 as an output
    MAP_GPIO_setAsInputPinWithPullUpResistor(PORT1, S2);		//configures switch 2 as input

    MAP_GPIO_setOutputLowOnPin(PORT1, LED1);  					//turn LED 1 off
    ///////////////////////////////////////////////////  UART Initializations
	// Define GPIOs and UART TX/RX
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
	//////////receive is P1.2, transmit is P1.3

	// Initialize UART x
	MAP_UART_initModule(EUSCI_A0_MODULE, &UART_init); //configure the module
	MAP_UART_enableModule(EUSCI_A0_MODULE); //enable the module


    ///////////////////////////////////////////////////  ADC Configurations
	// Initializing ADC
	MAP_ADC14_enableModule();  //Enables ADC
	ADC14_setResolution(ADC_10BIT); // Sets number of bits
	MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_4, 0); // Sets clock source and clock rate

	// Configuring GPIOs (5.5 A0)
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION); //P5.5 is ADC A0

	// Configuring ADC Memory
	MAP_ADC14_configureSingleSampleMode(ADC_MEM0, false); // Configures conversion mode
	MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, ADC_NONDIFFERENTIAL_INPUTS); // Allocates register to hold results

	// Configuring Sample Timer
	MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION); // Configures manual vs automatic triggering

	// Enabling/Toggling Conversion
	MAP_ADC14_enableConversion();  // Enables conversion


	///////////////////////////////////////////////////  Timer A configurations
    // Configuring Timer_A1 for Up Mode
    MAP_Timer_A_configureUpMode(TIMER_A1_MODULE, &upConfig);

    // Enabling interrupts and starting the timer
    MAP_Interrupt_enableInterrupt(INT_TA1_0); //enables the timer A interrupt


    //////////////////////////////////////////////////	Primary program loop:

    //Enable master interrupts
    //MAP_Interrupt_enableMaster();

    while(1){
    	//Button 1 Control:
    		but1 = MAP_GPIO_getInputPinValue(PORT1, S1);				//reads the value from button 1

    		if(but1 == 0){
    			//data acquisition routine
    				/*//clock frequency configuration
    				dcoFrequency = 3E+6;
    				MAP_CS_setDCOFrequency(dcoFrequency);
    				MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
					*/
    				inputcount = 0;

    			    //Enable master interrupts
    			    MAP_Interrupt_enableMaster();

    			    //MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //resets CC counter

    				//starts timer A
    				MAP_Timer_A_startCounter(TIMER_A1_MODULE, TIMER_A_UP_MODE);

    		}//end of button 1 routine

    	//Button 2 Control:
    		but2 = GPIO_getInputPinValue(PORT1, S2);					//reads the value from button 2

    		if (but2 == 0){

    			MAP_GPIO_setOutputLowOnPin(PORT1, LED1);	//when button 2 is pressed, turn off LED 1 to indicate begining of data output routine

    			//data output routine:
    		    	/*//clock frequency configuration
    		    	dcoFrequency = 12E+6; //3E+6;
    		    	MAP_CS_setDCOFrequency(dcoFrequency);
    		    	MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    		    	*/

    				uint8_t s = {'s'};
    				MAP_UART_transmitData(EUSCI_A0_MODULE, s);
    				while((UCA0IFG & 0x02) == 0){} ;
    				MAP_UART_transmitData(EUSCI_A0_MODULE, 65);
    				while((UCA0IFG & 0x02) == 0){} ;



   					for(i=0; i< 30; i++){

   						//convert to ascii and store in array:
   						int num = sprintf(string_from_flash,"%.1f", *((float*)MAIN_1_SECTOR_31 + i));

   				    	for(j=0;j<num;j++){
   				    		toprint = string_from_flash[j];

   				    		MAP_UART_transmitData(EUSCI_A0_MODULE, toprint); //send each data byte
   				    		while((UCA0IFG & 0x02) == 0){} ;
   				    	}

				    		MAP_UART_transmitData(EUSCI_A0_MODULE, carriageReturn); //send a new line character
				    		while((UCA0IFG & 0x02) == 0){} ;
				    		MAP_UART_transmitData(EUSCI_A0_MODULE, newLine); //send a new line character
				    		while((UCA0IFG & 0x02) == 0){} ;

   				    	numchars = numchars + 4 ;		//each temp value will contain 4 chars ("XX.X")

    					}//end of for loop
    		}//end of button 2 routine
    }//end of while
}//end of main


// Timer A ISR
void timer_a_0_isr(void)
{
	if (inputcount<totalMeasurements){
		MAP_ADC14_toggleConversionTrigger();  // Initiates a single conversion (trigger) - note, this trigger gets cleared automatically

		while(ADC14_isBusy()==1){}

		temp = ADC14_getResult(ADC_MEM0); //data between 0-255 corresponding to 0V-3.3V (3.3 by default)
		tempout = temp / 1023; //gets value between 0 & 3.3 V by resolution formula --> (vmax-vmin)/(2^N-1) = (3.3-0)/(1024-1) -> see below for the 3.3V conversion
		tempout = tempout * 198; // convert data to temp 0-2.5 V -> 0-150 F.  Therefore *(150/2.5)= *60*3.3V = 198

		allMeas[inputcount]=tempout; //store value to array
		printf("%f \n", tempout);
		inputcount++;

		MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //resets CC counter
	}
	else {
		// Unprotecting Main Bank 1, Sector 31
		MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);
		// Trying to erase the sector. Within this function, the API will automatically try to
		// erase the maximum number of tries. If it fails, notify user.
		if(!MAP_FlashCtl_eraseSector(MAIN_1_SECTOR_31)){
			printf("Erase failed\r\n");
		}
		// Trying to program the memory. Within this function, the API will automatically try
		// to program the maximum number of tries. If it fails, notify user.
		if(!MAP_FlashCtl_programMemory(allMeas, (void*) MAIN_1_SECTOR_31, 120)){
			printf("Write failed\r\n") ;
		}
		/* Setting the sector back to protected */
		MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);

		MAP_GPIO_setOutputHighOnPin(PORT1, LED1);  //turn on LED 1 to indicate end of data acquisition

		//Disable master interrupts
		MAP_Interrupt_disableMaster();
	}
}


