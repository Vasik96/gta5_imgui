#include "GTAOnline.h"
#include <chrono>
#include <cmath>

using namespace std::chrono;


/*
* ========================= GTA:O WEATHER =========================
   1 in-game hour = 120 real seconds (2 minutes).
   The full weather cycle = 384 in-game hours (about 16 real hours).
   We use current UTC time -> convert to in-game hours -> mod 384 -> find current and next weather from the schedule.
   This reproduces Rockstar's synced online weather exactly without network access.
*/


static double get_weather_period_time()
{
    auto now = system_clock::now();
    auto timestamp_int = duration_cast<seconds>(now.time_since_epoch()).count();
    double timestamp = static_cast<double>(timestamp_int);
    double total_gta_hours = timestamp / GAME_HOUR_LENGTH;
    return std::fmod(total_gta_hours, WEATHER_PERIOD);
}

std::string gta5_online::get_current_weather()
{
    double weather_period_time = get_weather_period_time();

    std::string current_weather = "Unknown";
    const int num_states = sizeof(weather_states) / sizeof(weather_states[0]);

    for (int i = 1; i < num_states; i++)
    {
        if (weather_states[i].hour > weather_period_time)
        {
            current_weather = weather_states[i - 1].name;
            break;
        }
    }

    if (current_weather == "Unknown")
        current_weather = weather_states[num_states - 1].name;

    return current_weather;
}

std::string gta5_online::get_next_weather()
{
    double weather_period_time = get_weather_period_time();

    std::string next_weather = "Unknown";
    const int num_states = sizeof(weather_states) / sizeof(weather_states[0]);

    for (int i = 1; i < num_states; i++)
    {
        if (weather_states[i].hour > weather_period_time)
        {
            next_weather = weather_states[i].name;
            break;
        }
    }

    if (next_weather == "Unknown")
        next_weather = weather_states[0].name;

    return next_weather;
}


std::string gta5_online::get_next_weather_with_eta()
{
    double weather_period_time = get_weather_period_time();

    const int num_states = sizeof(weather_states) / sizeof(weather_states[0]);
    int next_index = -1;

    for (int i = 0; i < num_states; i++)
    {
        if (weather_states[i].hour > weather_period_time)
        {
            next_index = i;
            break;
        }
    }

    if (next_index == -1)
        next_index = 0;

    double gta_hours_until_next = weather_states[next_index].hour - weather_period_time;
    if (gta_hours_until_next < 0)
        gta_hours_until_next += WEATHER_PERIOD;

    double real_seconds_until_next = gta_hours_until_next * GAME_HOUR_LENGTH;
    int minutes = static_cast<int>(real_seconds_until_next / 60.0 + 0.5);

    std::ostringstream oss;
    oss << weather_states[next_index].name << " (" << minutes << "min)";
    return oss.str();
}


std::string gta5_online::get_ingame_time()
{
    auto now = system_clock::now();
    auto timestamp_int = duration_cast<seconds>(now.time_since_epoch()).count();
    double timestamp = static_cast<double>(timestamp_int);

    // convert to gta hours
    double total_gta_hours = timestamp / GAME_HOUR_LENGTH;
    int hour = static_cast<int>(fmod(total_gta_hours, 24.0));
    int minute = static_cast<int>(fmod(total_gta_hours * 60.0, 60.0));

    char buffer[16];
    sprintf_s(buffer, "%02d:%02d", hour, minute);
    return std::string(buffer);
}
