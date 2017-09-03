/*
 * ledhelper.h
 *
 *  Created on: Sep 16, 2016
 *      Author: Geoffrey
 */

#ifndef LEDHELPER_H_
#define LEDHELPER_H_

#define PORT1 		GPIO_PORT_P1
#define PORT2 		GPIO_PORT_P2
#define S1			GPIO_PIN1
#define S2			GPIO_PIN4
#define LED1 		GPIO_PIN0
#define LED2RED		GPIO_PIN0
#define LED2GRE		GPIO_PIN1
#define LED2BLU		GPIO_PIN2

setLEDred(){
	MAP_GPIO_setAsOutputPin(PORT2, LED2RED);
	MAP_GPIO_setOutputHighOnPin(PORT2, LED2RED);
}

setLEDgreen(){
	MAP_GPIO_setAsOutputPin(PORT2, LED2GRE);
	MAP_GPIO_setOutputHighOnPin(PORT2, LED2GRE);
}

setLEDblue(){
	MAP_GPIO_setAsOutputPin(PORT2, LED2BLU);
	MAP_GPIO_setOutputHighOnPin(PORT2, LED2BLU);
}

#endif /* LEDHELPER_H_ */
