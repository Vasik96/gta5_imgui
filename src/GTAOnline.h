#pragma once
#include <iostream>

struct WeatherState {
	const char* name;
	const char* emoji;
};

struct WeatherChange { int hour; const char* name; };

static WeatherChange weather_states[] = {
    {0, "Partly Cloudy"},
    {4, "Misty"},
    {7, "Mostly Cloudy"},
    {11, "Clear"},
    {14, "Misty"},
    {16, "Clear"},
    {28, "Misty"},
    {31, "Clear"},
    {41, "Hazy"},
    {45, "Partly Cloudy"},
    {52, "Misty"},
    {55, "Cloudy"},
    {62, "Foggy"},
    {66, "Cloudy"},
    {72, "Partly Cloudy"},
    {78, "Foggy"},
    {82, "Cloudy"},
    {92, "Mostly Clear"},
    {104, "Partly Cloudy"},
    {105, "Drizzling"},
    {108, "Partly Cloudy"},
    {125, "Misty"}, 
    {128, "Partly Cloudy"},
    {131, "Raining"},
    {134, "Drizzling"}, 
    {137, "Cloudy"},
    {148, "Misty"}, 
    {151, "Mostly Cloudy"},
    {155, "Foggy"},
    {159, "Clear"}, 
    {176, "Mostly Clear"}, 
    {196, "Foggy"},
    {201, "Partly Cloudy"},
    {220, "Misty"},
    {222, "Mostly Clear"},
    {244, "Misty"}, 
    {246, "Mostly Clear"}, 
    {247, "Raining"},
    {250, "Drizzling"}, 
    {252, "Partly Cloudy"},
    {268, "Misty"},
    {270, "Partly Cloudy"},
    {272, "Cloudy"}, 
    {277, "Partly Cloudy"},
    {292, "Misty"}, 
    {295, "Partly Cloudy"},
    {300, "Mostly Cloudy"},
    {306, "Partly Cloudy"},
    {318, "Mostly Cloudy"},
    {330, "Partly Cloudy"},
    {337, "Clear"}, 
    {367, "Partly Cloudy"},
    {369, "Raining"},
    {376, "Drizzling"}, 
    {377, "Partly Cloudy"}
};

constexpr double GAME_HOUR_LENGTH = 120.0; // 1 in-game hour = 120s
constexpr double WEATHER_PERIOD = 384.0; // full cycle length [gta hours]


namespace gta5_online {
	std::string get_current_weather();
	std::string get_next_weather();
    std::string get_next_weather_with_eta();
    std::string get_ingame_time();
}