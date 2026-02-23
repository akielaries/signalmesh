/*
 * gpio.h
 *
 *  Created on: Jan 25, 2026
 *      Author: akiel
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include "GOWIN_M1.h"

// Define pin states for clarity
typedef enum {
    PIN_STATE_LOW = 0,
    PIN_STATE_HIGH = 1
} Pin_State_t;

// Structure to map a board pin number to a GPIO port and pin mask
typedef struct {
    uint8_t board_pin_number;
    GPIO_TypeDef* port;
    uint32_t pin_mask;
} Board_Pin_Map_t;

void gpio_init(void);

void GPIO_ToggleBit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);

void Board_Pin_Write(uint8_t board_pin_number, Pin_State_t state);
void Board_Pin_Toggle(uint8_t board_pin_number);


#endif /* GPIO_H_ */
