
#include "driverlib.h"
#include "msp.h"
#include "stdio.h"

#define A1 	GPIO_PIN4 	//P2.4
#define B1 	GPIO_PIN5	//P2.5
#define A2 	GPIO_PIN6	//P2.6
#define B2	GPIO_PIN7	//P2.7

//declare variables
unsigned int dcoFrequency_uart = 3E6;
unsigned int dcoFrequency_motor = 3E6;
int newLine = 10; 			//New Line value
int carriageReturn = 13; 	//Carriage Return value
int uart_count=0;			//Counts the number of inputs by uart
int program_stage = 0;		//stage 1 = turning CCW (initial rotation), stage 2 = turning CW (going "home")
int rotation = 0;			//rotation determined by user, 0-60 equating to 0-360 degrees
int degree_count = 0;
int stator_count = 0;		//determines the order of stators to activate to make motor turn appropriately
int timer = 0; 				//determines when to change which coils are energized
int i=0;					//garbage variable for loops
int j=0;					//garbage variable for loops

volatile char receiveBuffer[10]={}; 	// array of characters (ASCII)
char toprint; 						//to transmit each character independently
char first_message[50];
char message_out[10];

// Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_64,         // increments counter every 64 clock cycles
        46875,		                           // to produce an interrupt every second (3M/64 = 46875) (12M/64 = 187500) (2M/64 = 31250)
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
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

//baud rate 			9600	38400	9600	9600
//clock rate 			3MHz	3MHz	12MHz	2MHz
//clockPrescalar = 		19		4		78		13
//firstModReg = 		8		14		2		0
//secondModReg=			85		0		0		0
//overSampling=			1		1		1		1


int main(void)
{
	//hold watchdog timer
    WDT_A_holdTimer();

		////////////////////////////////////////////////////////////Initializations
		//clock frequency configuration
		MAP_CS_setDCOFrequency(dcoFrequency_uart);
		MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	    //enable floating point module
	    FPU_enableModule();

		//Disable master interrupts
		MAP_Interrupt_disableMaster();

		///////////////////////////////////////////////////  Button/LED configurations
	    //configure the buttons and LEDs
	    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);			//configures LED1 as an output

	    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);  					//turn LED 1 off


	    ///////////////////////////////////////////////////  Motor output configurations
	    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, A1);
	    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, B1);
		MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, A2);
		MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, B2);

		//ensure all pins start on low:
		MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A1);
	    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A2);
	    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, B1);
	    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, B2);


	    ///////////////////////////////////////////////////  UART Initializations
		// Define GPIOs and UART TX/RX
		MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);  //receive is P1.2, transmit is P1.3

		// Initialize UART x
		MAP_UART_initModule(EUSCI_A0_MODULE, &UART_init); //configure the module
		MAP_UART_enableModule(EUSCI_A0_MODULE); //enable the module

		// Enable UART receive interrupts
		MAP_UART_enableInterrupt(EUSCI_A0_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
		MAP_Interrupt_enableInterrupt(INT_EUSCIA0);

		//////////////////////////////////////////////////  Timer A configurations
	    // Configuring Timer_A1 for Up Mode
	    MAP_Timer_A_configureUpMode(TIMER_A1_MODULE, &upConfig);

	    // Enabling interrupts and starting the timer
	    MAP_Interrupt_enableInterrupt(INT_TA1_0); //enables the timer A interrupt


	    //////////////////////////////////////////////////	Primary program loop:

	    //set priority
		Interrupt_setPriority (INT_EUSCIA0,0);
	    Interrupt_setPriority (INT_TA1_0,1);

	    //Enable master interrupts
	    MAP_Interrupt_enableMaster();

		int num1 = sprintf(first_message,"\n\rProgram ready...\n\rEnter an integer 0-60: ");
		for(j=0;j<num1;j++){
			toprint = first_message[j];

			MAP_UART_transmitData(EUSCI_A0_MODULE, toprint); //send each data byte
			while((UCA0IFG & 0x02) == 0){} ;
		}

   	//main while loop
    while(1){

    	//user input a value 0-60, begins CCW rotation
    	if(program_stage == 1){

    		MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);  //turn on LED 1 to indicate working

    		//pull value from uart input
    		if(uart_count==1){
    			rotation = receiveBuffer[0]-48;
    		}else if(uart_count==2){
    			rotation = receiveBuffer[0]-48;
    			rotation = rotation * 10;
    			rotation = rotation + receiveBuffer[1] - 48;
    		}else{
    			printf("\nerror: too many inputs entered.");
    			rotation = 0;
    		}

       		uart_count = 0;

       		MAP_Timer_A_enableCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
       		MAP_Timer_A_startCounter(TIMER_A1_MODULE, TIMER_A_UP_MODE); //starts timer A
       		MAP_CS_setDCOFrequency(dcoFrequency_motor);
       		MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

       		//rotation protocol:
       		while (degree_count<rotation){

       			while(timer == 0){};//Timer A determines when to energize each coil

       			if (stator_count==0){
       				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A2|B1|B2);
       				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, A1);
       				stator_count++;
       			}
       			else if(stator_count==1) {
       				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A1|A2|B2);
       				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, B1);
       				stator_count++;
       			}
       			else if(stator_count==2) {
       				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A1|B1|B2);
       				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, A2);
       				stator_count++;
       			}
       			else if(stator_count==3) {
       				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A1|A2|B1);
       				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, B2);
       				stator_count = 0;
       			}
       			else {
       				UART_transmitData(EUSCI_A0_MODULE,0x07);
       			}
       			degree_count++;
       			timer = 0;
       		}//end while

       		Timer_A_disableCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //turn off Timer A/counter
       		MAP_CS_setDCOFrequency(dcoFrequency_uart);
       		MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);


       		int num2 = sprintf(message_out, "\n\rStart? ");
       	   	for(j=0;j<num2;j++){
       	   		toprint = message_out[j];

       	   		MAP_UART_transmitData(EUSCI_A0_MODULE, toprint);
       	   		while((UCA0IFG & 0x02) == 0){} ;
       	   	}//end for

       	   	program_stage++; //stage 2 is waiting for user to indicate to start returning home
       	   	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);  //turn off LED 1 to indicate waiting
       	}//end if stage 1

    	//user ready for CW rotation
       	if(program_stage == 3){
       	   		program_stage=0;

       	   		/*char ok[4] = ['o','k', newLine, carriageReturn];
           	   	for(j=0;j<4;j++){
           	   		toprint = ok[j];
           	   		MAP_UART_transmitData(EUSCI_A0_MODULE, toprint);
           	   		while((UCA0IFG & 0x02) == 0){} ;
           	   	}//end for
           	   	*/

       	   		MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);  //turn on LED 1 to indicate working

       	   		MAP_Timer_A_enableCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
       	   		MAP_Timer_A_startCounter(TIMER_A1_MODULE, TIMER_A_UP_MODE); //starts timer A
       	   		MAP_CS_setDCOFrequency(dcoFrequency_motor*4);
       	   		MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

           		//reverse rotation protocol:
       	   		stator_count=stator_count - 2;
           		while (degree_count>0){

           			while(timer == 0){};//Timer A determines when to energize each coil

           			if (stator_count==0){
           		    	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, B1);
           				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, A1);
           				stator_count=3;
           			}
           			else if(stator_count==1) {
           				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A2);
           				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, B1);
           				stator_count--;
           			}
           			else if(stator_count==2) {
           				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, B2);
           				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, A2);
           				stator_count--;
           			}
           			else if(stator_count==3) {
           				MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, A1);
           				MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, B2);
           				stator_count--;
           			}
           			degree_count--;
           			timer = 0;
           		}//end while

           		Timer_A_disableCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //turn off Timer A
           		MAP_CS_setDCOFrequency(dcoFrequency_uart);
           		MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);


           	   	for(j=0;j<num1;j++){
           	   		toprint = first_message[j]; //sets up the restart of the program

           	   		MAP_UART_transmitData(EUSCI_A0_MODULE, toprint);
           	   		while((UCA0IFG & 0x02) == 0){} ;
           	   	}//end for

           	 MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);  //turn off LED 1 to indicate waiting
       	}//end if stage 3
   }//end while
}//end main



//UART ISR
void uart_isr(void){
   	receiveBuffer[uart_count] = MAP_UART_receiveData(EUSCI_A0_MODULE); //store the input byte value
   	if (receiveBuffer[uart_count] == carriageReturn){
   		program_stage++;
   	}else{
	MAP_UART_transmitData(EUSCI_A0_MODULE, receiveBuffer[uart_count]); //send each data byte
	while((UCA0IFG & 0x02) == 0){} ;
   	uart_count++; //add to the counter
   	}

	//MAP_UART_clearInterruptFlag(EUSCI_A0_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
   	MAP_UART_clearInterruptFlag(EUSCI_A0_MODULE, MAP_UART_getEnabledInterruptStatus(EUSCI_A0_MODULE)); //clear the interrupt flag

}

// Timer A ISR
void timer_a_1_isr(void){
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_MODULE, TIMER_A_CAPTURECOMPARE_REGISTER_0); //resets CC counter
	timer = 1;
}







