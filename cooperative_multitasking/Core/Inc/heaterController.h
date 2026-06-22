/*
 * heaterController.h
 *
 *  Created on: Jan 25, 2026
 *      Author: ryuyoonmin
 */

#ifndef INC_HEATERCONTROLLER_H_
#define INC_HEATERCONTROLLER_H_
#define t_OFF 0
#define t_ON  1

#include "main.h"

uint8_t getHeaterState();
void heaterController(uint8_t onOff);

#endif /* INC_HEATERCONTROLLER_H_ */
