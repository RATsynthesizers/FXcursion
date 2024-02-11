/*
 * Buttons.cpp
 *
 *  Created on: Jun 16, 2022
 *      Author: romte
 */

#include "Button.hpp"
#include "Encoder.hpp"
#include "Led.hpp"

// TODO: place all btns into array for cyclic update
Button btnN		(&UIadapterReg,	7, ACTIVE_LOW);
Button btnY		(&UIadapterReg,	0, ACTIVE_LOW);
Button btn1		(&UIadapterReg,	8, ACTIVE_LOW);
Button btn2		(&UIadapterReg,	15, ACTIVE_LOW);
Button btnREC	(&UIadapterReg,	22, ACTIVE_LOW);
Button btnSTP	(&UIadapterReg,	20, ACTIVE_LOW);
Button btnPLY	(&UIadapterReg,	21, ACTIVE_LOW);
Button enc1btn	(&UIadapterReg,	3, ACTIVE_LOW);
Button enc2btn	(&UIadapterReg,	4, ACTIVE_LOW);
Button enc3btn	(&UIadapterReg,	11, ACTIVE_LOW);
Button enc4btn	(&UIadapterReg,	12, ACTIVE_LOW);
Button enc5btn	(&UIadapterReg,	19, ACTIVE_LOW);

Button* buttons[BUTTONS_NUM] = {&btnN, &btnY, &btnREC, &btnSTP, &btnPLY, &btn1, &btn2, &enc1btn, &enc2btn, &enc3btn, &enc4btn, &enc5btn};

Led noLed(&UIadapterReg, 2, 5, 2, 6, 2, 7, ACTIVE_HIGH);
Led yesLed(&UIadapterReg, 2, 1, 2, 2, 2, 3, ACTIVE_HIGH);
Led recLed(&UIadapterReg, 1, 0, 1, 1, 1, 2, ACTIVE_HIGH);
Led stpLed(&UIadapterReg, 1, 6, 1, 7, 2, 0, ACTIVE_HIGH);
Led plyLed(&UIadapterReg, 1, 3, 1, 4, 1, 5, ACTIVE_HIGH);

Led* leds[LEDS_NUM] = {&noLed, &yesLed, &recLed, &stpLed, &plyLed};

Encoder enc1(&UIadapterReg, 0, 1, 0, 2, ACTIVE_LOW);
Encoder enc2(&UIadapterReg, 0, 6, 0, 5, ACTIVE_LOW);
Encoder enc3(&UIadapterReg, 1, 1, 1, 2, ACTIVE_LOW);
Encoder enc4(&UIadapterReg, 1, 6, 1, 5, ACTIVE_LOW);
Encoder enc5(&UIadapterReg, 2, 1, 2, 2, ACTIVE_LOW);

Encoder* encs[ENCS_NUM] = {&enc1, &enc2, &enc3, &enc4, &enc5};

