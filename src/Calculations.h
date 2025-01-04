#pragma once
#ifndef CALCULATIONS_H
#define PCALCULATIONS_H

#include <string>



namespace Calculations {
	std::string CalculateTotalMoney(int currentMoney, int moneyToReceive, int ceoBonus);
	int ParseInteger(const char* str);
}

#endif
