/*
 * heaterController.h
 *
 *  Created on: Jan 25, 2026
 *      Author: ryuyoonmin
 */

#ifndef INC_HEATERCONTROLLER_H_
#define INC_HEATERCONTROLLER_H_

#include "main.h"
enum{
	t_OFF = 0,
	t_ON = 1
};
uint8_t getHeaterState();
void heaterController(uint8_t onOff);

#endif /* INC_HEATERCONTROLLER_H_ */
