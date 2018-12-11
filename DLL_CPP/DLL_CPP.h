#pragma once
#include "stdafx.h"
#include "../mainProgram/Constatnts.h"
//void RgbToHsl(); //todo parameters -> zamiana rgb na hsl
const int RANGE = 14;
//void HslToRgb(); //todo parameters -> zamiana rgb na hsl
void ProcedeSCPP(int *hues, float *saturations, int H, int ammountToCalculate); //todo parameters -> operacja na HSL
//parametry:
//hue ->szukane hue do zwiêkszenia saturacji b¹dz jej ustawieniu na 0 
//adres do pierwszego elementu
//ilosc pixeli do przerobienia