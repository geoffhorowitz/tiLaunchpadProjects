
// Include appropriate headers
#include "msp.h"
#include "stdio.h"
#include "driverlib.h"

// Declare volatile variables for ISRs
volatile char receiveBuffer[250]={}; // array of characters (ASCII)

//declare variables
int count = -1; //counting variable
int i=0; //garbage varable for loop
int newLine = 48; //new line value (though not ASCII value = 10??)
int carriageReturn = 176; // Carriage Return value (though not ASCII value = 13??)

// Set configure UART communication
const eUSCI_UART_Config UART_init = {
		EUSCI_A_UART_CLOCKSOURCE_SMCLK,	//SMCLK Clock Source
		78,								//clockPrescalar
		2,								//FirstModReg
		0,								//SecondModReg
		EUSCI_A_UART_NO_PARITY,			//No Parity
		EUSCI_A_UART_MSB_FIRST,			//MSB First
		EUSCI_A_UART_ONE_STOP_BIT,		//One Stop Bit
		EUSCI_A_UART_MODE,				//UART Mode
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,	//Oversampling
};

//baud rate 9600
//clock rate 3MHz
//clockPrescalar = 19
//firstModReg = 8
//secondModReg=85
//overSampling=1

//baud rate 38400
//clock rate 3MHz
//clockPrescalar = 4
//firstModReg = 14
//secondModReg=0
//overSampling=1


void main(void){
	
	//hold watchdog timer
    WDT_A_holdTimer();

    //clock frequency configuration
    unsigned int dcoFrequency = 12E+6; //3E+6;
    MAP_CS_setDCOFrequency(dcoFrequency);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

	// Define GPIOs and UART TX/RX
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
	//////////receive is P1.2, transmit is P1.3

	// Initialize UART x
	MAP_UART_initModule(EUSCI_A0_MODULE, &UART_init); //configure the module
	MAP_UART_enableModule(EUSCI_A0_MODULE); //enable the module
	
	// Enable UART receive interrupts
	MAP_Interrupt_disableMaster();
	MAP_UART_enableInterrupt(EUSCI_A0_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
	MAP_Interrupt_enableMaster();

	//set priority
	//Interrupt_setPriority (uart_isr,0)

	while(1){
		if(count > 0){
			if (receiveBuffer[count] == carriageReturn){  //determines if the carriage return was pressed
				for(i=0; i<=count; i++){
					MAP_UART_transmitData(EUSCI_A0_MODULE,receiveBuffer[i]); //send each data byte
					while((UCA0IFG & 0x02) == 0){} ; // Ensure transmission is done
				}//end of for loop
				
				MAP_UART_transmitData(EUSCI_A0_MODULE, newLine); //send a new line character
				while((UCA0IFG & 0x02) == 0){} ; //ensure transmission is done
				
				count = -1; //reset counter
			}
		}
	} // end while
} // end main


// Code for ISR
void uart_isr(void){
	//MAP_UART_clearInterruptFlag(EUSCI_A0_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_UART_clearInterruptFlag(EUSCI_A0_MODULE, MAP_UART_getEnabledInterruptStatus(EUSCI_A0_MODULE)); //clear the interrupt flag
	++count; //add to the counter
	receiveBuffer[count] = MAP_UART_receiveData(EUSCI_A0_MODULE); //store the input byte value
}
