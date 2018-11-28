// DLL_CPP.cpp : Definiuje eksportowane funkcje dla aplikacji DLL.
//

#include "stdafx.h"
#include "DLL_CPP.h"
void ProcedeSCPP(int* hues, float* saturations, int H, int ammountToCalculate)
{
	for (int i = 0; i < ammountToCalculate; ++i)
	{
		if(hues[i] <= H + RANGE && hues[i] >= H - RANGE)
		{
			saturations[i] += INCREASE_S;
		}
		else
		{
			saturations[i] = MIN_S;
		}
	}
}