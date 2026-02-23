#include "gpio.h"
#include "GOWIN_M1.h"


void gpio_init(void) {
  GPIO_InitTypeDef GPIO_InitType;

  //Initializes GPIO pin 0-4 as output
  GPIO_InitType.GPIO_Pin = GPIO_Pin_0 |
		  	  	  	  	       GPIO_Pin_1 |
                           GPIO_Pin_2 /*|
                           GPIO_Pin_3 |
                           GPIO_Pin_4 |
                           GPIO_Pin_5 |
						   GPIO_Pin_6*/;

  GPIO_InitType.GPIO_Mode = GPIO_Mode_OUT;  //As output
  GPIO_InitType.GPIO_Int = GPIO_Int_Disable;
  GPIO_Init(GPIO0,&GPIO_InitType);          //Initialized


  //Initializes output value
  //on:0; 1:off on some boards
  //on:1; 0:off on some boards
/*
  GPIO_SetBit(GPIO0, GPIO_Pin_0 |
		  	  	  	 GPIO_Pin_1 |
					 GPIO_Pin_2 |
					 GPIO_Pin_3 |
					 GPIO_Pin_4 |
					 GPIO_Pin_5 |
					 GPIO_Pin_6);
*/
}

void GPIO_ToggleBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin) {
  if (GPIOx->DATAOUT & GPIO_Pin) {
    GPIOx->DATAOUT &= ~GPIO_Pin;
  }
  else {
    GPIOx->DATAOUT |= GPIO_Pin;
  }
}
