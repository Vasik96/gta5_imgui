#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <string>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>

#include "Calculations.h"


namespace Calculations {
}



// Function to calculate total money with proper CEO bonus calculation
std::string Calculations::CalculateTotalMoney(int currentMoney, int moneyToReceive, int ceoBonus) {
    // Start with the current money and money to receive
    float total = static_cast<float>(currentMoney) + static_cast<float>(moneyToReceive);

    // If the ceoBonus is greater than 0, calculate the bonus based on the number of CEOs
    if (ceoBonus > 0) {
        // 2.5% bonus per CEO/MC President
        float bonusMultiplier = 1.0f + (ceoBonus * 2.5f / 100.0f);
        // Calculate the bonus amount
        float bonusAmount = moneyToReceive * (ceoBonus * 2.5f / 100.0f);
        // Add bonus amount to total
        total += bonusAmount;
    }

    // Round to the nearest integer and convert to string
    int roundedTotal = static_cast<int>(std::round(total));

    return "$" + std::to_string(roundedTotal); // Return the result as a string with the dollar sign
}



// Function to parse integers and ensure it's a valid number
int Calculations::ParseInteger(const char* str) {
    try {
        return max(0, std::stoi(str)); // Ensure non-negative values
    }
    catch (...) {
        return -1;
    }
}