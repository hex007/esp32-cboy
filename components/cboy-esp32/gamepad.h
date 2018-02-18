#include <stdint.h>

#ifndef GAMEPAD_H
#define GAMEPAD_H

void gamepad_init();

uint8_t gp_get_buttons();
uint8_t gp_get_directions();

#endif
