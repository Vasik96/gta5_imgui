#define IMGUI_DEFINE_MATH_OPERATORS
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <thread>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <urlmon.h>
#include <filesystem>
#include <commdlg.h>
#include <atomic>
#include <mutex>
#include <fstream>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <powrprof.h>


#include "json.hpp"

#include "gui.h"
#include "ProcessHandler.h"
#include "Calculations.h"
#include "FormattedInfo.h"
#include "TicTacToe.h"
#include "Logging.h"
#include "globals.h"
#include "ExternalLogger.h"
#include "LoggingServer.h"

#include "System.h"

#include "stb_image.h"
#include "imgui/imgui_internal.h"
#include "imgui_styleGTA5/Include.hpp"

#include "IconsFontAwesome6.h"

#include "FPSTracker.h"
#include "DXHandler.h"
#include "GTAOnline.h"

#include "hwisenssm2.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "PowrProf.lib")

using Microsoft::WRL::ComPtr;

AlertNotification g_alert_notification;

static C_ImMMenu Menu;

static gui::GuiTabs current_tab = gui::GuiTabs::TAB_MAIN;

std::string titleText = "Home";

char filePathBuffer[256] = "";

std::string fps;

static const char* popupTitle = "Desync from the current GTA:O session?";
static std::string mainWindowTitle = ICON_FA_SCREWDRIVER_WRENCH " GTA 5 Menu [External]";

char currentPlayerMoneyBuffer[64] = "";
char moneyToReceiveBuffer[64] = "";
char ceoBonusBuffer[64] = "";
char totalMoneyText[128] = "";

int currentSelection = 0;
int clickedTimes = 0;

bool noActivateFlag = true;
bool showImGuiDebugWindow = false;
bool gta5NotRunning = false;
bool fivemNotRunning = false;
bool loggedStart = false;
bool scrollToBottom = false;
bool p_open = true;
bool _network__no_save_exploit = false;
bool isLosSantosSelected = true;
bool externalLogsWindowActive = false;
bool showOtherWindow = true;
bool showGTA5styleWindow = false;
bool isDesyncInProgress = false;
bool has_resolution_changed_for_ssaa = false;

static bool show_processes_window = false;
static bool showMetricsOverlay = false;
static bool showGta5MapWindow = false;

//for process list:
DWORD aProcesses[1024];
DWORD cbNeeded;
static std::vector<std::string> process_names_list;
static std::vector<const char*> items;

ImVec4 title_color(0.0f, 0.5f, 1.0f, 1.0f);
ImVec4 subtitle_color(0.14f, 0.87f, 0.42f, 1.f);


ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw

std::vector<std::string> availableResolutions = System::GetAvailableResolutions();
static std::vector<const char*> resolutionOptions;

static const char* power_options[] = { "Shutdown", "Restart", "Sleep" };
static int currentPowerOption = 0;

static int currentResolution = 0;
std::chrono::steady_clock::time_point startTime;

ID3D11ShaderResourceView* lsMapTexture = nullptr;
ID3D11ShaderResourceView* cayoMapTexture = nullptr;

static float progress = 0.0f;
static float progress_dir = 1.0f;
static float progress_duration = 10.0f;
static float progress_step = 0.0f;



extern double g_totalVRAMGB;

// variables for global settings
nlohmann::json global_settings;
std::string config_filename = "config.json";

void LoadSettings(nlohmann::json& settings, const std::string& filename);
void ResetSettings(nlohmann::json& settings, const std::string& filename);
void SaveSettings(const nlohmann::json& settings, const std::string& filename);

void SaveSettings(const nlohmann::json& settings, const std::string& filename)
{
    // Create JSON object
    nlohmann::json j = settings;


    j["showGTA5styleWindow"] = showGTA5styleWindow;
    j["showGta5MapWindow"] = showGta5MapWindow;
    j["externalLogsWindowActive"] = externalLogsWindowActive;
    j["noActivateFlag"] = noActivateFlag;
    j["showImGuiDebugWindow"] = showImGuiDebugWindow;
    j["showOtherWindow"] = showOtherWindow;
    j["globals::overlayToggled"] = globals::overlayToggled;
    j["showMetricsOverlay"] = showMetricsOverlay;
    j["globals::real_alt_f4_enabled"] = globals::real_alt_f4_enabled;


    // Write to file
    std::ofstream file(filename);
    file << j.dump(4);
}

void ResetSettings(nlohmann::json& settings, const std::string& filename)
{
    // Reset all settings to default values
    settings["showGTA5styleWindow"] = false;
    settings["showGta5MapWindow"] = false;
    settings["externalLogsWindowActive"] = false;
    settings["noActivateFlag"] = true;
    settings["showImGuiDebugWindow"] = false;
    settings["showOtherWindow"] = true;
    settings["globals::overlayToggled"] = true;
    settings["showMetricsOverlay"] = false;
    settings["globals::real_alt_f4_enabled"] = false;

    // Manually update the actual values as well
    showGTA5styleWindow = false;
    showGta5MapWindow = false;
    externalLogsWindowActive = false;
    noActivateFlag = true;
    showImGuiDebugWindow = false;
    showOtherWindow = true;
    globals::overlayToggled = true;
    showMetricsOverlay = false;
    globals::real_alt_f4_enabled = false;

    // Save the default settings to file
    SaveSettings(settings, filename);
}

void LoadSettings(nlohmann::json& settings, const std::string& filename)
{
    // Open the file
    std::ifstream file(filename);

    try
    {
        // Check if the file exists and is not empty
        if (file.good())
        {
            // Attempt to parse the JSON from the file
            nlohmann::json j;
            file >> j;

            if (j.is_null() || j.empty())
            {
                throw std::runtime_error("Settings file is empty or malformed");
            }

            // Load the entire settings JSON into the settings object
            settings = j;

            // Check if each key exists, and if not, add it with a default value
            if (!settings.contains("showGTA5styleWindow")) {
                settings["showGTA5styleWindow"] = false;
            }
            if (!settings.contains("showGta5MapWindow")) {
                settings["showGta5MapWindow"] = false;
            }
            if (!settings.contains("externalLogsWindowActive")) {
                settings["externalLogsWindowActive"] = false;
            }
            if (!settings.contains("noActivateFlag")) {
                settings["noActivateFlag"] = true;
            }
            if (!settings.contains("showImGuiDebugWindow")) {
                settings["showImGuiDebugWindow"] = false;
            }
            if (!settings.contains("showOtherWindow")) {
                settings["showOtherWindow"] = true;
            }
            if (!settings.contains("globals::overlayToggled")) {
                settings["globals::overlayToggled"] = true;
            }
            if (!settings.contains("showMetricsOverlay")) {
                settings["showMetricsOverlay"] = false;
            }
            if (!settings.contains("globals::real_alt_f4_enabled")) {
                settings["globals::real_alt_f4_enabled"] = false;
            }

            // Manually assign to variables
            showGTA5styleWindow = settings["showGTA5styleWindow"].get<bool>();
            showGta5MapWindow = settings["showGta5MapWindow"].get<bool>();
            externalLogsWindowActive = settings["externalLogsWindowActive"].get<bool>();
            noActivateFlag = settings["noActivateFlag"].get<bool>();
            showImGuiDebugWindow = settings["showImGuiDebugWindow"].get<bool>();
            showOtherWindow = settings["showOtherWindow"].get<bool>();
            globals::overlayToggled = settings["globals::overlayToggled"].get<bool>();
            showMetricsOverlay = settings["showMetricsOverlay"].get<bool>();
            globals::real_alt_f4_enabled = settings["globals::real_alt_f4_enabled"].get<bool>();
        }
        else
        {
            // If the file doesn't exist or is empty, throw an error
            throw std::runtime_error("Settings file is missing or empty");
        }
    }
    catch (const nlohmann::json::parse_error& e)
    {
        // Specific catch for JSON parsing errors
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        // General error handling
        std::cerr << "Error loading settings: " << e.what() << std::endl;

        // Delete the corrupted file if an error occurs
        if (std::filesystem::exists(filename))
        {
            std::filesystem::remove(filename);
        }

        // Reset the settings to default values
        ResetSettings(settings, filename);
    }
}



static HANDLE hMapFile = nullptr;
static PHWiNFO_SENSORS_SHARED_MEM2 pInfo = nullptr;

bool InitHWiNFOMemory()
{
    hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, L"Global\\HWiNFO_SENS_SM2");
    if (!hMapFile) {
        printf("Failed to open HWiNFO shared memory. Is HWiNFO running with Shared Memory enabled?\n");
        return false;
    }
    printf("Opened HWiNFO shared memory mapping successfully.\n");

    pInfo = (PHWiNFO_SENSORS_SHARED_MEM2)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (!pInfo) {
        printf("Failed to map view of HWiNFO shared memory.\n");
        CloseHandle(hMapFile);
        hMapFile = nullptr;
        return false;
    }
    printf("Mapped view of HWiNFO shared memory successfully.\n");



    const DWORD HWiNFOSignature = 0x53695748; // value printed by your program
    if (pInfo->dwSignature != HWiNFOSignature) {
        printf("HWiNFO shared memory signature mismatch. Signature = 0x%08X\n", pInfo->dwSignature);
        UnmapViewOfFile(pInfo);
        CloseHandle(hMapFile);
        pInfo = nullptr;
        hMapFile = nullptr;
        return false;
    }

    printf("HWiNFO shared memory initialized and verified successfully.\n");
    return true;
}



void ShutdownHWiNFOMemory()
{
    if (pInfo) UnmapViewOfFile(pInfo);
    if (hMapFile) CloseHandle(hMapFile);
    pInfo = nullptr;
    hMapFile = nullptr;
}

void DrawVRAMUsage()
{
    if (!pInfo) {
        ImGui::Text("Other: [HWiNFO not running]");
        return;
    }

    auto* sensorBase = (BYTE*)pInfo + pInfo->dwOffsetOfSensorSection;
    auto* readingBase = (BYTE*)pInfo + pInfo->dwOffsetOfReadingSection;

    PHWiNFO_SENSORS_SENSOR_ELEMENT gpuSensor = nullptr;

    for (DWORD i = 0; i < pInfo->dwNumSensorElements; i++)
    {
        auto* sensor = (PHWiNFO_SENSORS_SENSOR_ELEMENT)(sensorBase + i * pInfo->dwSizeOfSensorElement);
        // look for GPU sensors; you can match by utfSensorNameUser or szSensorNameUser
        std::string name(reinterpret_cast<char*>(sensor->utfSensorNameUser));
        if (name.find("GPU") != std::string::npos)
        {
            gpuSensor = sensor;
            break; // first GPU
        }
    }

    if (!gpuSensor)
    {
        ImGui::Text("GPU: N/A");
        return;
    }

    double vramUsagePercent = -1.0;

    for (DWORD j = 0; j < pInfo->dwNumReadingElements; j++)
    {
        auto* reading = (PHWiNFO_SENSORS_READING_ELEMENT)(readingBase + j * pInfo->dwSizeOfReadingElement);

        if (reading->dwSensorIndex == (DWORD)(gpuSensor - (PHWiNFO_SENSORS_SENSOR_ELEMENT)sensorBase))
        {
            std::string label(reinterpret_cast<char*>(reading->szLabelOrig));
            if (label.find("GPU Memory Controller Utilization") != std::string::npos)
            {
                vramUsagePercent = reading->Value;
                break;
            }
        }
    }


    if (vramUsagePercent >= 0)
    {
        double vramUsedGB = (vramUsagePercent / 100.0) * g_totalVRAMGB;
        ImGui::Text("VRAM: %.1f/%.1f GB", vramUsedGB, g_totalVRAMGB);
    }
    else
    {
        ImGui::Text("VRAM: N/A");
    }
}


void DrawGPUUsage()
{
    if (!pInfo) {
        return;
    }

    auto* sensorBase = (BYTE*)pInfo + pInfo->dwOffsetOfSensorSection;
    auto* readingBase = (BYTE*)pInfo + pInfo->dwOffsetOfReadingSection;

    PHWiNFO_SENSORS_SENSOR_ELEMENT gpuSensor = nullptr;

    // Find first GPU sensor
    for (DWORD i = 0; i < pInfo->dwNumSensorElements; i++)
    {
        auto* sensor = (PHWiNFO_SENSORS_SENSOR_ELEMENT)(sensorBase + i * pInfo->dwSizeOfSensorElement);
        std::string name(reinterpret_cast<char*>(sensor->utfSensorNameUser));
        if (name.find("GPU") != std::string::npos)
        {
            gpuSensor = sensor;
            break; // first GPU
        }
    }

    if (!gpuSensor)
    {
        ImGui::Text("GPU: N/A");
        return;
    }

    double gpuUsagePercent = -1.0;

    for (DWORD j = 0; j < pInfo->dwNumReadingElements; j++)
    {
        auto* reading = (PHWiNFO_SENSORS_READING_ELEMENT)(readingBase + j * pInfo->dwSizeOfReadingElement);

        if (reading->dwSensorIndex == (DWORD)(gpuSensor - (PHWiNFO_SENSORS_SENSOR_ELEMENT)sensorBase))
        {
            if (reading->tReading == SENSOR_TYPE_USAGE)
            {
                std::string label(reinterpret_cast<char*>(reading->szLabelOrig)); // English string
                if (label.find("GPU Utilization") != std::string::npos)
                {
                    gpuUsagePercent = reading->Value;
                    break;
                }
            }
        }
    }


    if (gpuUsagePercent >= 0)
        ImGui::Text("GPU: %d%%", (int)gpuUsagePercent);
    else
        ImGui::Text("GPU: N/A");
}

void DrawCPUTemperature()
{
    if (!pInfo)
        return;

    auto* sensorBase = (BYTE*)pInfo + pInfo->dwOffsetOfSensorSection;
    auto* readingBase = (BYTE*)pInfo + pInfo->dwOffsetOfReadingSection;

    double cpuTemp = -1.0;

    for (DWORD j = 0; j < pInfo->dwNumReadingElements; j++)
    {
        auto* reading = (PHWiNFO_SENSORS_READING_ELEMENT)(readingBase + j * pInfo->dwSizeOfReadingElement);

        if (reading->tReading == SENSOR_TYPE_TEMP)
        {
            std::string label(reinterpret_cast<char*>(reading->szLabelOrig));

            if (label.find("CPU") != std::string::npos)
            {
                cpuTemp = reading->Value;
                break;
            }
        }
    }

    if (cpuTemp >= 0.0)
    {
        ImGui::Text("CPU Temp: %d *C", static_cast<int>(cpuTemp));
    }
    else
    {
        ImGui::Text("CPU Temp: N/A");
    }
}




void UpdateProcessList() {
    std::memset(aProcesses, 0, sizeof(aProcesses)); // clear array
    process_names_list.clear();
    items.clear();
    
    
    EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded);

    DWORD count = cbNeeded / sizeof(DWORD); // PID = 4 bytes (DWORD), cbNeeded is bytes, not a count
                                            // for example: 250 processes running, each PID is DWORD which is 4 bytes
                                            // therefore we divide by sizeof(DWORD) to get the process count.

    for (DWORD i = 0; i < count; ++i) {
        std::string process_name = ProcessHandler::GetProcessNameFromPID(aProcesses[i]);

        if (!process_name.empty()) {
            process_names_list.push_back(process_name);
            std::cout << process_name << std::endl;
        }
    }

    std::sort(process_names_list.begin(), process_names_list.end());
}


void LaunchLS()
{

    std::thread([]()
        {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = L"C:\\Steam\\steamapps\\common\\Lossless Scaling\\LosslessScaling.exe";
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteEx(&sei);
    }).detach();
}

/*
void LaunchModRemoveTool()
{

    std::thread([]()
        {
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = L"C:\\Users\\leman\\Desktop\\Soubory\\gta5_mod_remove_tool.exe";
            sei.nShow = SW_SHOWNORMAL;
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            ShellExecuteEx(&sei);
        }).detach();
}
*/




DWORD versionInfoSize;
std::string gameVersionStr;

void RetrieveGameVersion(const std::wstring& gameExePath)
{
    DWORD versionInfoSize = GetFileVersionInfoSize(gameExePath.c_str(), nullptr);
    if (versionInfoSize == 0)
    {
        std::cerr << "Error: Could not get file version info size." << std::endl;
        return;
    }
    BYTE* versionInfo = new BYTE[versionInfoSize];

    if (!GetFileVersionInfo(gameExePath.c_str(), 0, versionInfoSize, versionInfo))
    {
        std::cerr << "Error: Could not get file version info." << std::endl;
        delete[] versionInfo;
        return;
    }

    LPVOID versionData;
    UINT versionDataLen;
    if (!VerQueryValue(versionInfo, L"\\StringFileInfo\\040904B0\\ProductVersion", &versionData, &versionDataLen))
    {
        std::cerr << "Error: Could not query product version." << std::endl;
        delete[] versionInfo;
        return;
    }

    std::wstring productVersionWstr((wchar_t*)versionData);
    std::string productVersionStr;
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, productVersionWstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed > 0) 
    {
        productVersionStr.resize(size_needed - 1); // Resize string to hold the UTF-8 output
        WideCharToMultiByte(CP_UTF8, 0, productVersionWstr.c_str(), -1, &productVersionStr[0], size_needed, nullptr, nullptr);
    }


    // Parse the version string
    std::stringstream ss(productVersionStr);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(ss, token, '.'))
    {
        parts.push_back(token);
    }

    // Check if we have enough parts
    if (parts.size() >= 3)
    {
        gameVersionStr = parts[2]; // Extract the third part (index 2)
    }
    else
    {
        gameVersionStr = "Version Parse Error"; // Handle error
    }

    delete[] versionInfo;
}



void guiLoadImage(ID3D11Device* device, const char* imagePath, ID3D11ShaderResourceView** textureView)
{
    Logging::Log("loading gta5 map", 1); //so i know its being called

    // Load the image using stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(imagePath, &width, &height, &channels, 4); // Load as RGBA (4 channels)
    if (!imageData) 
    {
        printf("Failed to load image: %s\n", imagePath);
        return;
    }

    // Create a texture description
    D3D11_TEXTURE2D_DESC textureDesc = { };
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;

    // Create the texture
    D3D11_SUBRESOURCE_DATA initData = { };
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4; // 4 bytes per pixel (RGBA)
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&textureDesc, &initData, &texture);
    stbi_image_free(imageData); // Free image data after creating texture

    if (FAILED(hr)) 
    {
        printf("Failed to create texture from image\n");
        return;
    }

    // Create Shader Resource View (SRV)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(texture, &srvDesc, textureView);
    texture->Release(); // Release the texture (we only need the SRV)
    if (FAILED(hr))
    {
        printf("Failed to create shader resource view for texture\n");
    }
}





//download font from github repo
bool DownloadFont(const std::string& url, const std::string& outputPath)
{
    // URLDownloadToFile will download the font file to the given path
    HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), outputPath.c_str(), 0, nullptr);
    return (hr == S_OK);
}





//log
std::vector<std::string> logBuffer;

namespace gui
{
    void LogImGui(const std::string& message, int errorLevel)
    {

        if (!ImGui::GetCurrentContext())
        {
            Logging::Log("No context!", 3);
            return;  // Avoid logging if ImGui context is not initialized
        }


        // Validate error level and set default if invalid
        std::string prefix;
        ImVec4 color;

        switch (errorLevel)
        {
        case 1: // Info
            prefix = "[info]  ";
            color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); // Aqua
            break;
        case 2: // Warning
            prefix = "[warning]  ";
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            break;
        case 3: // Error
            prefix = "[error]  ";
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
            break;
        case 4:
            prefix = "[external]  ";
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
            break;
        default: // Default to Info
            prefix = "[info]  ";
            color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); // Aqua
            break;
        }

        // Push the formatted message to the log buffer
        logBuffer.push_back(prefix + message);


        scrollToBottom = true;
    }
}








void SetImGuiStyle() 
{
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text] = ImVec4(0.93f, 0.93f, 0.93f, 1.00f); // Light text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f); // Dark background
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.23f, 0.23f, 0.23f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.25f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.23f, 0.20f, 0.78f); // More noticeable hover effect
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.25f, 0.22f, 0.94f); // Lighter active frame background
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.14f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.26f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.16f, 0.14f, 0.61f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.12f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.21f, 0.31f, 0.26f, 1.00f); // Cold green for scrollbar
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.23f, 0.33f, 0.28f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.35f, 0.30f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.34f, 0.29f, 1.00f); // Lighter slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.38f, 0.32f, 1.00f); // Stronger active slider grab
    colors[ImGuiCol_Button] = ImVec4(0.30f, 0.42f, 0.36f, 0.74f); // Lighter button
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.47f, 0.41f, 0.85f); // More noticeable hover effect
    colors[ImGuiCol_ButtonActive] = ImVec4(0.38f, 0.50f, 0.44f, 1.00f); // Stronger active effect
    colors[ImGuiCol_Header] = ImVec4(0.28f, 0.38f, 0.32f, 0.31f); // Lighter header
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.40f, 0.34f, 0.50f); // More noticeable header hover
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.42f, 0.36f, 0.51f); // Stronger active header
    colors[ImGuiCol_Separator] = ImVec4(0.22f, 0.32f, 0.27f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.35f, 0.29f, 0.50f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.28f, 0.38f, 0.32f, 0.64f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.22f, 0.32f, 0.27f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.36f, 0.31f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.29f, 0.39f, 0.34f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.30f, 0.42f, 0.36f, 0.70f); // Default tab color
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.53f, 0.46f, 0.80f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.40f, 0.52f, 0.46f, 1.00f); // Lighter when selected (clicked)
    colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.52f, 0.46f, 1.00f); // Lighter when active (clicked)
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.30f, 0.42f, 0.36f, 0.74f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.35f, 0.47f, 0.41f, 0.85f); // Lighter when clicked but unfocused
    colors[ImGuiCol_PlotLines] = ImVec4(0.55f, 0.70f, 0.60f, 1.00f); // Cold green plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.45f, 0.75f, 0.65f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.55f, 0.65f, 0.60f, 1.00f); // Green histogram
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.60f, 0.70f, 0.65f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.21f, 0.59f, 0.29f, 0.35f); // Subtle green selected text background
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.29f, 1.00f); // Green navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

}


/**
 **************************   Initialize ImGui ***************************************
**/

static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };


void InitializeImGui(HWND window, ID3D11Device* device, ID3D11DeviceContext* device_context)
{
    InitHWiNFOMemory();

    Logging::Log("Initializing settings..", 1);
    LoadSettings(global_settings, config_filename);


    Logging::Log("Getting game patch...", 1);
    RetrieveGameVersion(L"C:\\Steam\\steamapps\\common\\Grand Theft Auto V Enhanced\\GTA5_Enhanced.exe");

    Logging::Log("Initializing ImGui...", 1);


    //assets

    guiLoadImage(device, "mLS.png", &lsMapTexture);
    guiLoadImage(device, "mCayo.png", &cayoMapTexture);


    const char* fontPath = "font.ttf";
    const std::string url = "https://github.com/Vasik96/gta5_imgui/raw/refs/heads/master/font.ttf";


    if (!std::filesystem::exists(fontPath))
    {
        if (DownloadFont(url, fontPath))
        {
            Logging::Log("Font downloaded successfully.", 1);
        }
        else
        {
            Logging::Log("failed to download font.", 3);
            return;
        }
    }


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    /*
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 1.0f;
    style.GrabRounding = 4.0f;*/



    //SetImGuiStyle();
    //ApplyReshadeUIStyle();

    ImGui::StyleColorsClassic();
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;


    //ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.95f;
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.30f);
    colors[ImGuiCol_TabHovered] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TabSelected], 0.5f);
    colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_WindowBg], 0.60f);
    colors[ImGuiCol_TabDimmedSelected] = ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_WindowBg], 0.40f);
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_WindowBg], 0.50f);
    colors[ImGuiCol_TabHovered] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TabSelected], 0.4f);
    colors[ImGuiCol_TabSelected] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 1.1f);


    // io & fonts
    ImGuiIO& io = ImGui::GetIO();

    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(fontPath, 14.0f);

    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;

    ImFont* iconFont = io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 14.0f, &icons_config, icons_ranges);

    io.Fonts->Build();
    
    
    
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    Menu.Initialize();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);


    for (const auto& res : availableResolutions)
    {
        resolutionOptions.push_back(res.c_str());
    }
    currentResolution = System::GetCurrentResolutionIndex(availableResolutions);
    Logging::Log("ImGui initialized", 1);


    SetFocus(NULL);
}





bool DrawSpinner(const ImVec2& position, float radius, int thickness, const ImU32& color)
{
    // Get the draw list for the current window
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (!draw_list) 
    {
        return false; // If for some reason the draw list is invalid
    }

    const int num_segments = 30;

    // Dynamic calculation for the animation effect
    int start = abs((int)(ImSin((float)(ImGui::GetTime() * 1.8f)) * (num_segments - 5)));

    // Angle range for the arc (this defines the arc to be drawn)
    const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

    // The center of the spinner (adjusted for the provided position)
    const ImVec2 center = ImVec2(position.x + radius, position.y + radius);

    // Clear the path before we start drawing
    draw_list->PathClear();

    // Loop through each segment of the spinner
    for (int i = 0; i < num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);

        // Calculate the line position for this segment
        const ImVec2 segment_position = ImVec2(
            center.x + (float)ImCos((float)(a + ImGui::GetTime() * 8)) * radius,
            center.y + (float)ImSin((float)(a + ImGui::GetTime() * 8)) * radius
        );


        // Add this position to the path
        draw_list->PathLineTo(segment_position);
    }

    // Render the spinner (stroke the path)
    draw_list->PathStroke(color, false, (float)(thickness));

    return true;
}


void EnableShutdownPrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
        CloseHandle(hToken);
    }
}


void MoveFiles(const std::vector<std::filesystem::path>& files, const std::filesystem::path& targetDir) {
    std::filesystem::create_directories(targetDir);
    for (const auto& file : files) {
        try {
            std::filesystem::rename(file, targetDir / file.filename());
        }
        catch (...) {
            Logging::Log("error while moving file", 3);
        }
    }
}

namespace MainWindow
{
    /***************************************************************/
    /**************************  SECTIONS  *************************/
    /***************************************************************/

    /*********  MAIN  **********/
    void RenderMainTab()
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::SeparatorText(ICON_FA_HAMMER " General");

        ImGui::Text("Overlay FPS: %.0f", io.Framerate);


        if (!p_gta5NotRunning || !p_fivemNotRunning)
        {
            ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());
            ImGui::Text("Current patch: %s", gameVersionStr.c_str());

        }
        else
        {
            ProcessHandler::ResetSession();
            //ImGui::Text("No active GTA 5 or FiveM session");
        }

        ImGui::Text("Last Timer Result: %s", globals::lastTimerResult.c_str());

        if (p_gta5NotRunning && p_fivemNotRunning)
        {

            ImGui::SetNextItemWidth(128.0f);

            // Create the combo box with three selections
            static const char* comboItems[] = { "GTA 5", "GTA 5 (Modded)", "FiveM" };
            if (ImGui::BeginCombo("##GameCombo", comboItems[currentSelection]))
            {
                for (int i = 0; i < IM_ARRAYSIZE(comboItems); i++)
                {
                    bool isSelected = (currentSelection == i);
                    if (ImGui::Selectable(comboItems[i], isSelected))
                    {
                        currentSelection = i;  // Update the current selection based on user input
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();  // Keep the default selection focused
                    }
                }
                ImGui::EndCombo();
            }


            if (ImGui::Button(ICON_FA_CHECK " Launch##Main"))
            {
                switch (currentSelection) //online launch removed as of enhanced
                {
                case 0:
                    // Launch GTA 5 normally
                    ProcessHandler::LaunchGTA5(true);
                    break;
                case 1:
                    // Launch GTA 5 without anticheat (used for modded game)
                    ProcessHandler::LaunchGTA5(false);
                    break;
                case 2:
                    // Launch FiveM
                    ProcessHandler::LaunchFiveM();
                    break;
                default:
                    ImGui::Text("[Error] Invalid selection");
                    break;
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Launches the selected game.");
            }



            ImGui::SameLine();
        }


        if (globals::is_gta5_running || globals::is_fivem_running)
        {
            if (ImGui::Button(ICON_FA_XMARK " Exit Game"))
            {
                gta5NotRunning = !ProcessHandler::TerminateGTA5();
                fivemNotRunning = !ProcessHandler::TerminateGTA5();
            }


            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Terminates the GTA 5 or FiveM process.");
            }

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_DESKTOP " SSAA")) {
                if (!has_resolution_changed_for_ssaa) {
                    has_resolution_changed_for_ssaa = true;
                    System::ChangeResolution(2560, 1440); //2880x1620, 2560x1440
                }
                else { // revert resolution
                    has_resolution_changed_for_ssaa = false;

                    DEVMODE devMode = {};
                    devMode.dmSize = sizeof(DEVMODE);
                    if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devMode)) {
                        int width = devMode.dmPelsWidth;
                        int height = devMode.dmPelsHeight;

                        System::ChangeResolution(width, height);
                        Logging::Log("Successfully reset resolution to the native value", 1);
                    }
                    else {
                        System::ChangeResolution(1920, 1080);
                        Logging::Log("Fallback to hardcoded resolution reset", 2);
                    }

                }

            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Applies supersampling by increasing resolution. (toggle)\n - This assumes you have DSR or some custom resolution added, otherwise do not apply this, doing so might result in an error or broken resolution");
            }

            ImGui::SameLine();
        }


        if (ImGui::Button(ICON_FA_EXPAND " Launch LS"))
        {
            LaunchLS();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Launches Lossless Scaling");
        }

        /*
        if (!globals::is_gta5_running && !globals::is_fivem_running)
        {
            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_GAMEPAD " Manage Mods"))
            {
                LaunchModRemoveTool();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Launches GTA 5 Mod Remove Tool");
            }
        }*/


        if (gta5NotRunning)
        {
            ImGui::Text("GTA5.exe isn't running.");
        }
        else if (fivemNotRunning)
        {
            ImGui::Text("FiveM.exe isn't running.");
        }





        ImGui::SeparatorText(ICON_FA_GEAR " Settings");

        ImVec2 childSize = ImVec2(0, ImGui::GetContentRegionAvail().y * 0.95f);
        if (ImGui::BeginChild("SettingsChild", childSize, true))
        {
            if (ImGui::CollapsingHeader(ICON_FA_LIST_CHECK " Options", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("GTA 5 Menu", &showGTA5styleWindow);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Toggles the GTA 5 interaction menu style window.");
                }

                ImGui::Checkbox("GTA 5 Map", &showGta5MapWindow);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Toggles the map window.");
                }

                ImGui::Checkbox("Overlay", &globals::overlayToggled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("If disabled, overlay will not show.");
                }

                ImGui::Checkbox("Performance Overlay", &showMetricsOverlay);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Overlay needs to be enabled, this shows the game framerate and frametime.");
                }


                ImGui::Checkbox("BG Interaction", &noActivateFlag);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("If enabled, the window will behave more like an overlay. (Reopen the menu to apply changes) [WS_EX_NOACTIVATE]");
                }


                ImGui::Checkbox("Demo Window", &showImGuiDebugWindow);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Toggles the ImGui demo window.");
                }

                if (ImGui::Checkbox("This Window", &showOtherWindow))
                {
                    showGTA5styleWindow = true;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hides this window, and shows the GTA 5 style window.");
                }

                ImGui::Checkbox("Real ALT+F4", &globals::real_alt_f4_enabled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("When enabled, ALT+F4 will instantly close GTA 5, without confirmation.");
                }
            }

            if (ImGui::CollapsingHeader(ICON_FA_FOLDER " Save & Load"))
            {
                if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save")) { SaveSettings(global_settings, config_filename); }
                if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load")) { LoadSettings(global_settings, config_filename); }
                if (ImGui::Button(ICON_FA_ROTATE_LEFT " Reset")) { ResetSettings(global_settings, config_filename); }
            }
        }
        ImGui::EndChild();

    }

    /*******  UTILITY  ********/
    void RenderUtilityTab()
    {
        ImGui::SeparatorText(ICON_FA_DOLLAR_SIGN " Money Calculations");
        ImGui::Spacing();
        ImGui::BeginChild("moneyTextFields", ImVec2(128, 105), true);
        ImGui::PushItemWidth(112.0f);
        ImGui::TextColored(title_color, "Input details...");

        // GUI input and calculation handling
        if (ImGui::InputTextWithHint("##currentMoney", "Current money", currentPlayerMoneyBuffer, sizeof(currentPlayerMoneyBuffer), ImGuiInputTextFlags_CharsDecimal))
        {
            int currentPlayerMoney = Calculations::ParseInteger(currentPlayerMoneyBuffer);
            if (currentPlayerMoney == -1)
            {
                currentPlayerMoneyBuffer[0] = '\0'; // Make the text field blank
            }
            else
            {
                snprintf(currentPlayerMoneyBuffer, sizeof(currentPlayerMoneyBuffer), "%d", currentPlayerMoney);
            }
        }

        // Money to receive input
        if (ImGui::InputTextWithHint("##moneyToReceive", "Received money", moneyToReceiveBuffer, sizeof(moneyToReceiveBuffer), ImGuiInputTextFlags_CharsDecimal))
        {
            int moneyToReceive = Calculations::ParseInteger(moneyToReceiveBuffer);
            if (moneyToReceive == -1)
            {
                moneyToReceiveBuffer[0] = '\0'; // Make the text field blank
            }
            else
            {
                snprintf(moneyToReceiveBuffer, sizeof(moneyToReceiveBuffer), "%d", moneyToReceive);
            }
        }

        // CEO bonus input
        if (ImGui::InputTextWithHint("##ceoBonus", "CEO's in lobby", ceoBonusBuffer, sizeof(ceoBonusBuffer), ImGuiInputTextFlags_CharsDecimal))
        {
            int ceoBonus = Calculations::ParseInteger(ceoBonusBuffer);
            if (ceoBonus == -1)
            {
                ceoBonusBuffer[0] = '\0'; // Make the text field blank
            }
            else
            {
                snprintf(ceoBonusBuffer, sizeof(ceoBonusBuffer), "%d", ceoBonus);
            }
        }

        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();

        //convert to int
        int currentPlayerMoney = Calculations::ParseInteger(currentPlayerMoneyBuffer);
        int moneyToReceive = Calculations::ParseInteger(moneyToReceiveBuffer);
        int ceoBonus = Calculations::ParseInteger(ceoBonusBuffer);

        if (ImGui::Button(ICON_FA_CALCULATOR " Calculate Total Money"))
        {
            std::string result = Calculations::CalculateTotalMoney(currentPlayerMoney, moneyToReceive, ceoBonus);
            strncpy_s(totalMoneyText, sizeof(totalMoneyText), result.c_str(), sizeof(totalMoneyText) - 1);
        }
        ImGui::Text("Total Money: %s", totalMoneyText); //calculated money text

        ImGui::EndGroup();


        ImGui::SeparatorText(ICON_FA_GAUGE_HIGH " Performance Monitoring");

        std::string name; // keep this alive
        const char* process_name = "<not active>";

        if (g_tracked_pid != 0) {
            name = ProcessHandler::GetProcessNameFromPID(g_tracked_pid);
            if (!name.empty()) {
                process_name = name.c_str(); // safe, 'name' is alive
            }
        }

        ImGui::Text("Current process: %s", process_name);



        
        if (ImGui::Button("Select process")) {
            show_processes_window = true;

            UpdateProcessList();
        }

        if (show_processes_window) {
            static int current_item = 0;

            if (ImGui::Begin("Select process from list", &show_processes_window, ImGuiWindowFlags_NoScrollbar)) {
                if (ImGui::BeginListBox("##process_list", ImVec2(ImGui::GetWindowSize().x * 0.95f, ImGui::GetWindowSize().y * 0.8f))) {
                    

                    for (int i = 0; i < process_names_list.size(); ++i) {
                        ImGui::PushID(i); // guaranteed unique
                        bool is_selected = (current_item == i);
                        if (ImGui::Selectable(process_names_list[i].c_str(), is_selected))
                            current_item = i;
                        ImGui::PopID();
                    }
                }
                ImGui::EndListBox();

                if (ImGui::Button(ICON_FA_ROTATE_LEFT " Update")) {
                    UpdateProcessList();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_CHECK " Confirm")) {
                    std::string selected_name = process_names_list[current_item];
                    std::wstring selected_name_w = std::wstring(selected_name.begin(), selected_name.end());
                    g_tracked_pid = ProcessHandler::GetPIDByProcessName(selected_name_w.c_str());

                    show_processes_window = false;
                }
            }
            ImGui::End();
        }
    }

    /*******  NETWORK  ********/
    void RenderNetworkTab()
    {
        ImGui::SeparatorText(ICON_FA_WIFI " Network");

        if (!globals::is_gta5_running)
        {
            ImGui::Text("GTA 5 is not running.");
        }
        else
        {
            _network__no_save_exploit ?
                ImGui::TextColored(ImVec4(0, 255, 0, 255), "Status: Active") : ImGui::TextColored(ImVec4(255, 0, 0, 255), "Status: Disabled");

            std::string button_text = _network__no_save_exploit ? "Disable firewall rule" : "Enable firewall rule";
            if (ImGui::Button(button_text.c_str()))
            {
                _network__no_save_exploit = !_network__no_save_exploit;
                _network__no_save_exploit ? globals::NO_SAVE__AddFirewallRule() : globals::NO_SAVE__RemoveFirewallRule();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Use this for heist replays");
            }
            if (ImGui::Button("Desync from GTA:O"))
            {

                progress = 0.0f;
                progress_dir = 1.0f;
                progress_duration = 10.0f;
                progress_step = 0.0f;
                isDesyncInProgress = false;

                if (!globals::is_gta5_running)
                {
                    gta5NotRunning = true;
                }
                else
                {
                    ImGui::OpenPopup(popupTitle);
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Suspends the GTA5.exe process for 10 seconds, which results in you desyncing from the current GTA Online session.");
            }

            //desync confirmation popup
            if (ImGui::BeginPopupModal(popupTitle, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
            {
                ImGui::Text("This will desync you from your current GTA Online session.\nThis operation cannot be undone!");
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "WARNING: DO NOT PRESS ANYTHING UNTIL THE GAME IS UNFREEZED!");
                ImGui::Separator();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                ImGui::PopStyleVar();

                if (progress_duration > 0.0f)
                {
                    progress_step = (1.0f / progress_duration) * ImGui::GetIO().DeltaTime;
                }

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    isDesyncInProgress = true;

                    std::thread desyncThread(ProcessHandler::DesyncFromGTAOnline);
                    desyncThread.detach();
                }

                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                    isDesyncInProgress = false;
                }

                ImVec2 current_window_size = ImGui::GetWindowSize();
                ImVec2 new_window_size = ImVec2(current_window_size.x, current_window_size.y + 950);
                ImGui::SetWindowSize(new_window_size);
                ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                ImVec2 spinner_position = ImVec2(cursor_pos.x + 10, cursor_pos.y + 20);
                DrawSpinner(spinner_position, 15, 6, IM_COL32(0, 75, 255, 255));
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                if (isDesyncInProgress)
                {
                    progress += progress_step * progress_dir;

                    if (progress >= 1.0f)
                    {
                        progress = 1.0f;
                        progress_dir = 0.0f;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::Text("Desyncing, please wait...");
                    ImGui::Text(" ");
                    ImGui::Text("  ");
                }

                ImGui::EndPopup();
            }
        }
    }

    /*******  SYSTEM  *********/
    void RenderSystemTab()
    {
        ImGui::SeparatorText(ICON_FA_CLOCK " Date & Time");
        ImGui::Text("Time: %s", FormattedInfo::GetFormattedTime().c_str());
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Text("Date: %s", FormattedInfo::GetFormattedDate().c_str());
        ImGui::Spacing();

        ImGui::SeparatorText(ICON_FA_GAUGE_HIGH " System Usage");
        ImGui::Text("CPU Usage: %s", FormattedInfo::GetFormattedCPUUsage().c_str());
        ImGui::SameLine();
        ImGui::Text("  Running processes: %s", FormattedInfo::GetFormattedRP().c_str());

        static float cpuUsageHistory[100] = { 0 };
        static int historyIndex = 0;
        static float lastCPUUsage = -1.0f;
        float currentCPUUsage = std::stof(FormattedInfo::GetFormattedCPUUsage());

        cpuUsageHistory[historyIndex] = currentCPUUsage;
        historyIndex = (historyIndex + 1) % IM_ARRAYSIZE(cpuUsageHistory);
        lastCPUUsage = currentCPUUsage;

        float average = 0.0f;
        for (int n = 0; n < IM_ARRAYSIZE(cpuUsageHistory); n++)
            average += cpuUsageHistory[n];
        average /= (float)IM_ARRAYSIZE(cpuUsageHistory);

        char overlay[32];
        sprintf_s(overlay, "Avg: %.1f%%", average);

        ImGui::PlotLines(
            "##CPUUsageGraph",
            cpuUsageHistory,
            IM_ARRAYSIZE(cpuUsageHistory),
            historyIndex,
            overlay,
            0.0f,
            100.0f,
            ImVec2(230, 60)
        );
        ImGui::Text("RAM Usage: %s", FormattedInfo::GetFormattedRAMUsage().c_str());

        ImGui::SeparatorText(ICON_FA_DESKTOP " Screen");
        ImGui::PushItemWidth(110.f);

        if (ImGui::BeginCombo("Resolution##Screen", resolutionOptions[currentResolution]))
        {
            for (int i = 0; i < resolutionOptions.size(); i++)
            {
                bool isSelected = (currentResolution == i);
                if (ImGui::Selectable(resolutionOptions[i], isSelected))
                {
                    currentResolution = i;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();

        if (ImGui::Button(ICON_FA_CHECK " Apply"))
        {
            int width = 0, height = 0;
            sscanf_s(resolutionOptions[currentResolution], "%dx%d", &width, &height);
            System::ChangeResolution(width, height);

            
        }

        ImGui::SeparatorText(ICON_FA_POWER_OFF " Power");
        ImGui::PushItemWidth(110.f);

        if (ImGui::BeginCombo("Select##Power", power_options[currentPowerOption]))
        {
            for (int i = 0; i < IM_ARRAYSIZE(power_options); ++i)
            {
                bool is_selected = (currentPowerOption == i);
                if (ImGui::Selectable(power_options[i], is_selected))
                    currentPowerOption = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if (ImGui::Button(ICON_FA_POWER_OFF " Execute"))
        {
            EnableShutdownPrivilege();

            switch (currentPowerOption)
            {
            case 0: // Shutdown
                InitiateSystemShutdownExW(
                    NULL,
                    NULL,
                    0,
                    TRUE,
                    FALSE,
                    SHTDN_REASON_FLAG_USER_DEFINED
                );
                break;

            case 1: // Restart
                InitiateSystemShutdownExW(
                    NULL,
                    NULL,
                    0,
                    TRUE,
                    TRUE,
                    SHTDN_REASON_FLAG_USER_DEFINED
                );
                break;

            case 2: // Sleep
                SetSuspendState(FALSE, TRUE, FALSE);
                break;
            }
        }
    }

    /*********  INFO  *********/
    void RenderInfoTab()
    {
        ImGui::SeparatorText(ICON_FA_KEYBOARD " Keybinds");
        ImGui::Text("[F5] Start/Stop a timer. (Shows in the overlay)");
        ImGui::Text("[Insert] Switch between window modes.");
        ImGui::Text("[End] Exit the program.");

        ImGui::Spacing();
        ImGui::SeparatorText(ICON_FA_GLOBE " Websites");
        ImGui::TextLinkOpenURL("GTA:O Map", "https://gtaweb.eu/gtao-map/ls/");
        ImGui::TextLinkOpenURL("R* Activity Feed", "https://socialclub.rockstargames.com/");
        ImGui::TextLinkOpenURL("R* Newswire", "https://www.rockstargames.com/newswire/");
        ImGui::TextLinkOpenURL("R* Community Races", "https://socialclub.rockstargames.com/jobs/?dateRangeCreated=any&platform=pc&sort=likes&title=gtav/");
    }

    /*******  EXTRAS  *********/
    void RenderExtrasTab()
    {
        ImGui::SeparatorText(ICON_FA_COOKIE " Cookie Clicker");
        if (ImGui::Button(ICON_FA_ARROW_POINTER " Click for points"))
        {
            clickedTimes++;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Click to add a point.\nCurrent points: %d", clickedTimes);
        }

        ImGui::Text("Points: %d", clickedTimes);
        ImGui::Spacing();
        ImGui::SeparatorText(ICON_FA_TABLE_CELLS " Tic-Tac-Toe");

        //tic tac toe
        ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
        ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
        ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw
        TicTacToe::DrawTicTacToeInsideWindow(playerColor, aiColor, drawColor);
    }

    /*********  LOGS  *********/
    void RenderLogsTab()
    {
        ImGui::SeparatorText(ICON_FA_CLIPBOARD_LIST " Logs");

        if (ImGui::Button(ICON_FA_ERASER " Clear"))
        {
            logBuffer.clear();
            gui::LogImGui("Logs cleared", 1);
        }
        ImGui::SameLine();
        ImGui::Checkbox("External Logs Window", &externalLogsWindowActive);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

        ImGui::BeginChild("LogChild", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& log : logBuffer)
        {
            ImVec4 textColor;

            if (log.find("[info]") == 0)
            {
                textColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
            }
            else if (log.find("[warning]") == 0)
            {
                textColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            }
            else if (log.find("[error]") == 0)
            {
                textColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); 
            }
            else
            {
                textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            }

            ImGui::TextColored(textColor, "%s", log.c_str());
        }
        if (scrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void RenderModsTab()
    {
        static constexpr const char* GTA_PATH = "C:\\Steam\\steamapps\\common\\Grand Theft Auto V Enhanced";
        static const std::vector<const char*> LEGIT_FILES = {
            "amd_ags_x64.dll",
            "amd_fidelityfx_dx12.dll",
            "bink2w64.dll",
            "dstorage.dll",
            "dstoragecore.dll",
            "fvad.dll",
            "GFSDK_Aftermath_Lib.x64.dll",
            "libcurl.dll",
            "libtox.dll",
            "nvngx_dlss.dll",
            "oo2core_5_win64.dll",
            "opus.dll",
            "opusenc.dll",
            "sl.common.dll",
            "sl.dlss.dll",
            "sl.interposer.dll",
            "sl.pcl.dll",
            "sl.reflex.dll",
            "steam_api64.dll",
            "XCurl.dll",
            "zlib1.dll",
            "nvngx_dlssg.dll",
            "sl.dlss_g.dll",

            "installscript.vdf",
            "installscript_sdk.vdf",
            "title.rgl",
            "rpf.cache",

            "GTA5.exe",
            "GTA5_Enhanced.exe",
            "GTA5_Enhanced_BE.exe",
            "PlayGTAV.exe",

            "common.rpf",
            "x64a.rpf",
            "x64b.rpf",
            "x64c.rpf",
            "x64d.rpf",
            "x64e.rpf",
            "x64f.rpf",
            "x64g.rpf",
            "x64h.rpf",
            "x64i.rpf",
            "x64j.rpf",
            "x64k.rpf",
            "x64l.rpf",
            "x64m.rpf",
            "x64n.rpf",
            "x64o.rpf",
            "x64p.rpf",
            "x64q.rpf",
            "x64r.rpf",
            "x64s.rpf",
            "x64t.rpf",
            "x64u.rpf",
            "x64v.rpf",
            "x64w.rpf"
        };

        static std::vector<std::filesystem::path> modFiles;

        ImGui::SeparatorText(ICON_FA_GAMEPAD " Mods");
        if (globals::is_gta5_running) {
            ImGui::Text("GTA 5 is running, cannot manage mods.");
            return;
        }

        modFiles.clear();
        const std::filesystem::path gtaPath(GTA_PATH);

        wchar_t exePathBuffer[MAX_PATH];
        GetModuleFileNameW(nullptr, exePathBuffer, MAX_PATH);
        const std::filesystem::path exePath = std::filesystem::path(exePathBuffer).parent_path();

        const std::filesystem::path whitelistPath = exePath / "gta5_mod_whitelist";

        for (const auto& entry : std::filesystem::directory_iterator(gtaPath)) {
            if (!entry.is_regular_file()) continue;

            auto filename = entry.path().filename().string();
            if (filename == "desktop.ini") continue;

            bool isLegit = std::ranges::any_of(LEGIT_FILES, [&](const char* legit) {
                return _stricmp(filename.c_str(), legit) == 0; // Case-insensitive compare
                });

            if (!isLegit) {
                modFiles.push_back(entry.path());
            }
        }

        ImGui::Text("Mods in GTA 5 Folder");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::BeginChild("ModListChild", ImVec2(0, 280), true);
        for (const auto& mod : modFiles) {
            ImGui::Text("%s", mod.filename().string().c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (ImGui::Button("Move mods to whitelist")) {
            MoveFiles(modFiles, whitelistPath);
        }

        if (ImGui::Button("Move mods back to GTA V folder")) {
            std::vector<std::filesystem::path> returnMods;
            for (const auto& entry : std::filesystem::directory_iterator(whitelistPath)) {
                if (entry.is_regular_file()) {
                    returnMods.push_back(entry.path());
                }
            }
            MoveFiles(returnMods, gtaPath);
        }
    }

    /***************************************************************/
    /**************************  TABS  *****************************/
    /***************************************************************/

    void RenderTablist()
    {
        using gui::GuiTabs;

        ImGui::BeginChild("Tablist", ImVec2(ImGui::GetWindowSize().x / 4, 0), true);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.4f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6)); // 8 - default horizontal, 4 - default vertical

        auto render_tab_button = [](const char* label, GuiTabs tab_id, GuiTabs& current_tab) {
            bool selected = (current_tab == tab_id);

            if (!selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
            }

            if (ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x, 40.f))) {
                current_tab = tab_id;
            }

            if (!selected) {
                ImGui::PopStyleColor(3);
            }
        };

        render_tab_button("Main", GuiTabs::TAB_MAIN, current_tab);
        render_tab_button("Utility", GuiTabs::TAB_UTILITY, current_tab);
        render_tab_button("Network", GuiTabs::TAB_NETWORK, current_tab);
        render_tab_button("Mods", GuiTabs::TAB_MODS, current_tab);
        render_tab_button("System", GuiTabs::TAB_SYSTEM, current_tab);
        render_tab_button("Info", GuiTabs::TAB_INFO, current_tab);
        render_tab_button("Extras", GuiTabs::TAB_EXTRAS, current_tab);
        render_tab_button("Logs", GuiTabs::TAB_LOGS, current_tab);

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }



}



void RenderMain(bool p_gta5NotRunning, bool p_fivemNotRunning)
{
    using gui::GuiTabs;

    ImGuiIO& io = ImGui::GetIO();


    ImGui::Begin(mainWindowTitle.c_str(), &globals::pForceHideWindow);  // Main window

    MainWindow::RenderTablist();
    ImGui::SameLine();
    ImGui::BeginChild("ContentMain", ImVec2(0, 0), true);

    switch (current_tab)
    {

        case GuiTabs::TAB_MAIN: {
            MainWindow::RenderMainTab();
            break;
        }
        case GuiTabs::TAB_UTILITY: {
            MainWindow::RenderUtilityTab();
            break;
        }
        case GuiTabs::TAB_NETWORK: {
            MainWindow::RenderNetworkTab();
            break;
        }
        case GuiTabs::TAB_MODS: {
            MainWindow::RenderModsTab();
            break;
        }
        case GuiTabs::TAB_SYSTEM: {
            MainWindow::RenderSystemTab();
            break;
        }
        case GuiTabs::TAB_INFO: {
            MainWindow::RenderInfoTab();
            break;
        }
        case GuiTabs::TAB_EXTRAS: {
            MainWindow::RenderExtrasTab();
            break;
        }
        case GuiTabs::TAB_LOGS: {
            MainWindow::RenderLogsTab();
            break;
        }
        default: {
            MainWindow::RenderMainTab();
            break;
        }

    }
    ImGui::EndChild();

    ImGui::End();
}



bool isFileDialogOpen = false;

void OpenFileExplorer()
{
    // Initialize OPENFILENAME structure
    OPENFILENAME ofn;       // common dialog box structure
    wchar_t szFile[260];    // buffer for file name (wide char)

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = L'\0';  // Initialize with empty string
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Text Files\0*.TXT\0Log Files\0*.LOG\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Select a File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the file dialog box
    if (GetOpenFileName(&ofn) == TRUE)
    {
        // Convert the wide string (wchar_t) to multibyte string (char)
        WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, filePathBuffer, sizeof(filePathBuffer), NULL, NULL);
    }
    // Set the flag to false after the file dialog is closed
    isFileDialogOpen = false;
}




void RenderExternalLogWindow() 
{
    static float previousScrollY = 0.0f; // Track the previous scroll position
    static bool userScrolling = false;  // Whether the user is actively scrolling

    if (ImGui::Begin(ICON_FA_CLIPBOARD_LIST " External Logs", &externalLogsWindowActive))
    {
        // Text input for file path
        ImGui::InputTextWithHint("##filePathField", "Enter file path here...", filePathBuffer, sizeof(filePathBuffer));

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open File Explorer"))
        {
            if (!isFileDialogOpen)
            {
                isFileDialogOpen = true;
                std::thread fileDialogThread(OpenFileExplorer);
                fileDialogThread.detach();
            }
        }

        // Log viewer child window
        ImGui::BeginChild("LogChild", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);

        float scrollY = ImGui::GetScrollY();            // Current scroll position
        float scrollMaxY = ImGui::GetScrollMaxY();      // Maximum scroll position

        // Detect user scrolling by checking if the scroll position changed
        userScrolling = (scrollY != previousScrollY);
        previousScrollY = scrollY;

        // Fetch logs and split them into lines
        std::vector<std::string> logLines;
        if (strlen(filePathBuffer) > 0)
        {
            std::string logContents = FormattedInfo::GetLogFromFile(filePathBuffer);

            // Split the log contents into lines for rendering
            std::istringstream logStream(logContents);
            std::string line;
            while (std::getline(logStream, line))
            {
                logLines.push_back(line);
            }
        }

        // Use the ImGuiListClipper for efficient rendering of log lines
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(logLines.size())); // Number of log lines
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) 
            {
                ImGui::TextUnformatted(logLines[i].c_str());
            }
        }
        clipper.End();

        // Automatically scroll down if the user is not scrolling
        if (!userScrolling && scrollY < scrollMaxY)
        {
            ImGui::SetScrollY(scrollMaxY); // Scroll to the bottom
        }

        ImGui::EndChild();
    }
    ImGui::End();
}


void Overlay_AddSection(const char* title, float windowWidth)
{
    float text_width = ImGui::CalcTextSize(title).x;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::SetCursorPosX((windowWidth - text_width) / 2.0f);
    ImGui::TextColored(subtitle_color, "%s", title);
    ImGui::Separator();
    ImGui::Spacing();
}



void RenderOverlays(bool p_gta5NotRunning, bool p_fivemNotRunning)
{
    // Overlay flags (from imgui demo)
    static int location = 1;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMouseInputs |
        ImGuiWindowFlags_NoNavInputs |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMove;

    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
    window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
    window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
    window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("InfoOverlay", &p_open, window_flags))
    {
        // Center the "Common info" text
        const char* title = "Common Info";
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) / 2.0f);
        ImGui::TextColored(title_color, "%s", title); // subtitle_2 = subtitle, subtitle = title

        ImGui::Spacing();
        ImGui::Separator();

        Overlay_AddSection("System", windowWidth);

        ImGui::Text("Time: %s", FormattedInfo::GetFormattedTime().c_str());
        ImGui::Text("Date: %s", FormattedInfo::GetFormattedDate().c_str());

        ImGui::Text("CPU: %s", FormattedInfo::GetFormattedCPUUsage().c_str());
        ImGui::Text("RAM: %s", FormattedInfo::GetFormattedRAMUsage().c_str());
        DrawGPUUsage();
        DrawVRAMUsage();
        DrawCPUTemperature();
        

        if (!p_gta5NotRunning || !p_fivemNotRunning) {
            Overlay_AddSection("Game", windowWidth);


            if (!p_gta5NotRunning)
            {
                ImGui::Text("Currently playing GTA 5");
                //ImGui::Text("FPS: %s", FormattedInfo::GetFormattedGTA5FPS());
            }
            else if (!p_fivemNotRunning)
            {
                ImGui::Text("Currently playing FiveM");
            }

            
            ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());
            

            if (!p_gta5NotRunning) {
                ImGui::Text("Next weather: %s", gta5_online::get_next_weather_with_eta().c_str());
                ImGui::Text("Time: %s", gta5_online::get_ingame_time().c_str());

                if (_network__no_save_exploit) {
                    ImGui::Text("R* cloud saves:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(255, 0, 0, 255), "disabled");
                }
            }


        }


        Overlay_AddSection("Menu", windowWidth);
       
        ImGui::Text("Overlay FPS: %.0f", io.Framerate);
        ImGui::Text("Menu running: %s", FormattedInfo::GetFormattedMenuUptime().c_str());
        if (globals::isTimerRunning)
        {
            ImGui::Text("Timer: %s", FormattedInfo::GetFormattedTimer().c_str());
        }

    }
    ImGui::End();
}


void RenderPerformanceOverlay(bool p_gta5NotRunning, bool p_fivemNotRunning)
{
    static int location = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMouseInputs |
        ImGuiWindowFlags_NoNavInputs |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMove;

    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
    window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
    window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
    window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("FPSOverlay", nullptr, window_flags))
    {
        const char* title = "Performance";
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) / 2.0f);
        ImGui::TextColored(title_color, "%s", title);

        ImGui::Spacing();
        ImGui::Separator();

        
        double frameTimeMs = GTA::GetCurrentFrametimeMs();
        int fps = GTA::GetCurrentFPS();

        if (fps == 0) {
            ImGui::Text("Process not selected, please select a process in the 'Utility' tab");
            ImGui::End();
            return;
        }


        ImGui::Text("FPS: %d", fps);
        //ImGui::Text("Frametime: %.2f ms", frameTimeMs);

        static std::vector<float> fps_samples;
        if (fps > 0) {
            fps_samples.push_back(static_cast<float>(fps));
            if (fps_samples.size() > 300) // last ~15s if updating ~20Hz
                fps_samples.erase(fps_samples.begin());

            std::vector<float> sorted = fps_samples;
            std::sort(sorted.begin(), sorted.end());
            size_t idx = max(static_cast<size_t>(1), static_cast<size_t>(sorted.size() * 0.01f)) - 1;
            float fps_1percent_low = sorted[idx];
            ImGui::Text("1%% Low: %.0f FPS", fps_1percent_low);
        }

        static float frameTimeHistory[240] = {};
        static int historyIndex = 0;

        constexpr float plot_fps = 90.f;
        static double lastUpdate = ImGui::GetTime();
        double currentTime = ImGui::GetTime();
        const double updateInterval = 1.0 / plot_fps;

        static char overlay[32] = "Frametime: 0.00 ms";
        if ((currentTime - lastUpdate) >= updateInterval)
        {
            lastUpdate = currentTime;

            frameTimeHistory[historyIndex] = static_cast<float>(frameTimeMs);
            historyIndex = (historyIndex + 1) % IM_ARRAYSIZE(frameTimeHistory);

            float avgFrametime = 0.0f;
            for (float ft : frameTimeHistory)
                avgFrametime += ft;
            avgFrametime /= static_cast<float>(IM_ARRAYSIZE(frameTimeHistory));

            sprintf_s(overlay, "Frametime: %.2f ms", avgFrametime);
        }

        ImGui::PlotLines("##FrametimeGraph", frameTimeHistory, IM_ARRAYSIZE(frameTimeHistory),
            historyIndex, overlay, 0.0f, 50.0f, ImVec2(230, 60));
        
    }
    ImGui::End();
}






struct Marker
{
    ImVec2 position;  // Position of the marker
    std::string label;  // Label of the marker
};

// Function to load markers from a JSON file
std::vector<Marker> LoadMarkers(const std::string& fileName)
{
    std::vector<Marker> markers;
    std::ifstream file(fileName);  // Open the file
    if (file.is_open()) 
    {
        try 
        {
            nlohmann::json json;
            file >> json;  // Parse the JSON content

            // Iterate through the JSON array and load markers
            for (auto& item : json)
            {
                markers.push_back({ ImVec2(item["x"], item["y"]), item["label"] });
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading markers: " << e.what() << std::endl;
        }
        file.close();
    }
    return markers;
}

// Function to save markers to a JSON file
void SaveMarkers(const std::string& fileName, const std::vector<Marker>& markers) 
{
    try
    {
        nlohmann::json json;
        for (const auto& marker : markers)
        {
            json.push_back({ {"x", marker.position.x}, {"y", marker.position.y}, {"label", marker.label} });
        }

        std::ofstream file(fileName);  // Open the file
        if (file.is_open()) 
        {
            file << json.dump(4);  // Pretty print the JSON data with an indent of 4 spaces
            file.close();
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error saving markers: " << e.what() << std::endl;
    }
}


void RenderMapWindow()
{
    static ImVec2 blueDotPos = ImVec2(-1, -1); // Initial position of the blue dot (off-screen)
    static float zoom = 1.0f;  // Zoom level (default 1.0)
    static ImVec2 panOffset = ImVec2(0, 0); // Offset for panning
    const float minZoom = 1.0f, maxZoom = 10.0f; // Zoom limits

    // Choose the correct texture based on the selection
    ImTextureID currentMapTexture = isLosSantosSelected ? (ImTextureID)lsMapTexture : (ImTextureID)cayoMapTexture;

    // Load markers for the selected map
    std::vector<Marker> markers = isLosSantosSelected ? LoadMarkers("mainMarkers.json") : LoadMarkers("cayoMarkers.json");

    // Variable to store clicked position for marker placement
    static ImVec2 clickedPosition = ImVec2(-1, -1); // Initially set to an invalid position

    if (currentMapTexture) 
    {
        ImGui::Begin(ICON_FA_COMPASS " GTA 5 Map");

        // Radio buttons to switch between Los Santos and Cayo Perico maps
        if (ImGui::RadioButton("Los Santos", isLosSantosSelected))
        {
            isLosSantosSelected = true;
        }
        if (ImGui::RadioButton("Cayo Perico", !isLosSantosSelected))
        {
            isLosSantosSelected = false;
        }

        if (ImGui::BeginChild("MapChild",
            ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()),
            true, 
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {


            // Get the current position of the image in the ImGui window
            ImVec2 imageStartPos = ImGui::GetCursorScreenPos();

            // Handle zooming centered on the mouse cursor
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 mouseRelativeToImage = ImVec2((mousePos.x - imageStartPos.x - panOffset.x) / zoom,
                (mousePos.y - imageStartPos.y - panOffset.y) / zoom);
            // Handle zooming using the scroll wheel and block normal scrolling
            float mouseWheel = ImGui::GetIO().MouseWheel;
            if (mouseWheel != 0.0f)
            {
                float prevZoom = zoom;
                // Apply a multiplier to the zoom speed:
                if (mouseWheel > 0.0f)
                {
                    // Zooming in (2x faster)
                    zoom += mouseWheel * 0.1f * 1.5f;  // 1.5x speed
                }
                else
                {
                    // Zooming out (4x faster)
                    zoom += mouseWheel * 0.1f * 2.5f;  // 2.5x speed
                }

                zoom = std::clamp(zoom, minZoom, maxZoom);

                // Get the current window size (the available space)
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                ImVec2 windowCenter = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f);

                // Calculate the image's current center position
                ImVec2 imageCenterBefore = ImVec2(imageStartPos.x + panOffset.x + (550 * prevZoom) * 0.5f,
                    imageStartPos.y + panOffset.y + (550 * prevZoom) * 0.5f);

                if (zoom < prevZoom)
                {  // Zooming out
                    // Find the shift factor for centering when zooming out
                    float zoomOutFactor = (1.0f - (zoom / prevZoom));

                    // Adjust panOffset based on zoom-out factor
                    panOffset.x += (windowCenter.x - imageCenterBefore.x) * zoomOutFactor;
                    panOffset.y += (windowCenter.y - imageCenterBefore.y) * zoomOutFactor;

                    // Ensure the map doesn't go off-screen when zooming out
                    ImVec2 imageSize = ImVec2(550 * zoom, 550 * zoom);
                    panOffset.x = std::clamp(panOffset.x, -(imageSize.x - windowSize.x), 0.0f);
                    panOffset.y = std::clamp(panOffset.y, -(imageSize.y - windowSize.y), 0.0f);

                }
                else 
                {  // Zooming in (center on cursor)
                    ImVec2 mousePos = ImGui::GetMousePos();
                    ImVec2 mouseRelativeToImage = ImVec2((mousePos.x - imageStartPos.x - panOffset.x) / prevZoom,
                        (mousePos.y - imageStartPos.y - panOffset.y) / prevZoom);

                    panOffset.x += (mouseRelativeToImage.x * (prevZoom - zoom));
                    panOffset.y += (mouseRelativeToImage.y * (prevZoom - zoom));
                }
            }








            ImVec2 imageSize = ImVec2(550 * zoom, 550 * zoom); // Adjusted map size

            // Render the image at the adjusted position
            ImGui::SetCursorScreenPos(imageStartPos + panOffset);
            ImGui::Image(currentMapTexture, imageSize);

            // Check if the image was clicked (Left-click to place marker)
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                clickedPosition = ImVec2((mousePos.x - imageStartPos.x - panOffset.x) / zoom,
                    (mousePos.y - imageStartPos.y - panOffset.y) / zoom);

                clickedPosition.x = std::clamp(clickedPosition.x, 0.0f, 550.0f);
                clickedPosition.y = std::clamp(clickedPosition.y, 0.0f, 550.0f);

                ImGui::OpenPopup("MarkerPopup");
            }

            if (ImGui::BeginPopup("MarkerPopup", ImGuiWindowFlags_NoMove))
            {
                static char label[128] = "";

                ImGui::PushItemWidth(100.f);
                ImGui::InputTextWithHint("##MarkerLabel", "Marker label", label, IM_ARRAYSIZE(label));
                ImGui::PopItemWidth();

                if (ImGui::Button("Confirm"))
                {
                    if (strlen(label) > 0)
                    {
                        markers.push_back({ clickedPosition, std::string(label) });
                        SaveMarkers(isLosSantosSelected ? "mainMarkers.json" : "cayoMarkers.json", markers);
                    }
                    memset(label, 0, sizeof(label));
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // Right-click to delete a marker
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImVec2 relativePos = ImVec2((mousePos.x - imageStartPos.x - panOffset.x) / zoom,
                    (mousePos.y - imageStartPos.y - panOffset.y) / zoom);

                for (auto it = markers.begin(); it != markers.end(); )
                {
                    ImVec2 markerPos = it->position;
                    double distance = sqrt(pow(relativePos.x - markerPos.x, 2) + pow(relativePos.y - markerPos.y, 2));
                    if (distance < 10.0f)
                    {
                        it = markers.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }

                SaveMarkers(isLosSantosSelected ? "mainMarkers.json" : "cayoMarkers.json", markers);
            }

            // Draw all the markers
            for (const auto& marker : markers)
            {
                ImVec2 markerPos = ImVec2(imageStartPos.x + panOffset.x + marker.position.x * zoom,
                    imageStartPos.y + panOffset.y + marker.position.y * zoom);
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddCircleFilled(markerPos, 3.6f, IM_COL32(255, 0, 0, 255));

                if (ImGui::IsMouseHoveringRect(markerPos - ImVec2(5, 5), markerPos + ImVec2(5, 5)))
                {
                    ImGui::SetTooltip("%s", marker.label.c_str());
                }
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }
}


enum MenuState
{
    MENU_MAIN,
    SECTION_SYSTEM,
    SECTION_INFO,
    SECTION_CONFIRM_DESYNC,
    SECTION_MONEY,
    SECTION_CONFIG
};

static MenuState currentMenu = MENU_MAIN;

void RenderGTA5Menu(
    bool p_gta5NotRunning,
    bool p_fivemNotRunning
)
{
    //render main stuff here (windows, and other interactive stuff)
    if (visibilityStatus == VISIBLE)
    {
        Menu.SetHeaderText("GTA 5 Menu");

        

        if (Menu.Begin())
        {
            Menu.SetTitleText(titleText);
            Menu.SetFooterText("v1.1");

            if (currentMenu == MENU_MAIN)
            {

                Menu.SetTitleText(titleText);

                /*
                  if (!p_gta5NotRunning || !p_fivemNotRunning) {
                ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());

            }
            else {
                ProcessHandler::ResetSession();
                //ImGui::Text("No active GTA 5 or FiveM session");
            }

            ImGui::Text("Last Timer Result: %s", globals::lastTimerResult.c_str());
                */


                if (!p_gta5NotRunning || !p_fivemNotRunning)
                {
                    std::string sessionText = "Current session: " + ProcessHandler::GetCurrentSession();
                    Menu.Item.AddTextUnselectable(sessionText.c_str());
                }

                std::string timerText = "Last Timer Result: " + globals::lastTimerResult;
                Menu.Item.AddTextUnselectable(timerText.c_str());

                Menu.Item.AddSeparator("General");


                static std::vector<std::string> m_ComboList =
                {
                     "GTA 5",
                     "GTA 5 (Modded)",
                     "FiveM"
                };

                static int current_selection = 0;

                Menu.SetTitleText(titleText);

                // Ensure the combo box is functional
                if (Menu.Item.AddCombo("Launch GTA 5", &current_selection, m_ComboList))
                {
                    // launch the selected game
                    switch (current_selection) 
                    {
                    case 0:
                        // Launch GTA 5 normally
                        ProcessHandler::LaunchGTA5(true);
                        break;
                    case 1:
                        // Launch GTA 5 without anticheat (used for modded game)
                        ProcessHandler::LaunchGTA5(false);
                        break;
                    case 2:
                        // Launch FiveM
                        ProcessHandler::LaunchFiveM();
                        break;
                    default:
                        ImGui::Text("[Error] Invalid selection");
                        break;
                    }
                }



                if (Menu.Item.AddText("Launch LS"))
                {
                    LaunchLS();
                }


                Menu.Item.AddSeparator("Settings");

                Menu.Item.AddCheckbox("GTA 5 Map", &showGta5MapWindow);
                Menu.Item.AddCheckbox("Overlay", &globals::overlayToggled);
                Menu.Item.AddCheckbox("Background Interaction", &noActivateFlag);
                Menu.Item.AddCheckbox("Demo Window", &showImGuiDebugWindow);
                Menu.Item.AddCheckbox("External Logs Window", &externalLogsWindowActive);
                Menu.Item.AddCheckbox("Other", &showOtherWindow);

                

                if (Menu.Item.AddSection("Desync from GTA:O"))
                {
                    if (globals::is_gta5_running)
                    {
                        currentMenu = SECTION_CONFIRM_DESYNC;
                    }
                }


                
                if (Menu.Item.AddSection("Config"))
                {
                    currentMenu = SECTION_CONFIG;
                    titleText = "Config";
                }


                if (Menu.Item.AddText("{FF0000}Exit Game"))
                {
                    gta5NotRunning = !ProcessHandler::TerminateGTA5();
                    fivemNotRunning = !ProcessHandler::TerminateGTA5();
                }
                if (Menu.Item.AddText("{FF5127}Exit Menu"))
                {
                    globals::running = false;
                }


                Menu.Item.AddSeparator("Utility");
                if (Menu.Item.AddSection("Money Calculations")) 
                {
                    currentMenu = SECTION_MONEY;
                    titleText = "Money Calculations";
                }

            }

            else if (currentMenu == SECTION_CONFIRM_DESYNC)
            {
                Menu.Item.AddTextUnselectable("{FF0000}WARNING: {FFFFFF}GTA 5 will freeze after confirming!");
                Menu.Item.AddTextUnselectable("Wait 10s after confirming.");
                Menu.Item.AddSeparator("");


                if (Menu.Item.AddText("Yes"))
                {
                    std::thread desyncThread(ProcessHandler::DesyncFromGTAOnline);
                    desyncThread.detach();

                    currentMenu = MENU_MAIN;
                }
                if (Menu.Item.AddText("No")) 
                {
                    currentMenu = MENU_MAIN;
                }
            }




            else if (currentMenu == SECTION_MONEY)
            {
                if (Menu.Item.AddSection("Back"))
                {
                    titleText = "Main";
                    Menu.SetTitleText(titleText);
                    currentMenu = MENU_MAIN;
                }

                Menu.Item.AddTextInput("Enter current money ->", "Current money", currentPlayerMoneyBuffer, sizeof(currentPlayerMoneyBuffer), ImGuiInputTextFlags_CharsDecimal);
                Menu.Item.AddTextInput("Enter received money ->", "Received money", moneyToReceiveBuffer, sizeof(moneyToReceiveBuffer), ImGuiInputTextFlags_CharsDecimal);
                Menu.Item.AddTextInput("Enter number of CEO's in lobby ->", "CEO's in lobby", ceoBonusBuffer, sizeof(ceoBonusBuffer), ImGuiInputTextFlags_CharsDecimal);


                int currentPlayerMoney = Calculations::ParseInteger(currentPlayerMoneyBuffer);
                int moneyToReceive = Calculations::ParseInteger(moneyToReceiveBuffer);
                int ceoBonus = Calculations::ParseInteger(ceoBonusBuffer);

                if (Menu.Item.AddText("{FFF300}Calculate Money")) 
                {
                    std::string result = Calculations::CalculateTotalMoney(currentPlayerMoney, moneyToReceive, ceoBonus);
                    strncpy_s(totalMoneyText, sizeof(totalMoneyText), result.c_str(), sizeof(totalMoneyText) - 1);
                }

                std::string totalMoneyTextStr = "Total Money: " + std::string(totalMoneyText);
                Menu.Item.AddTextUnselectable(totalMoneyTextStr.c_str());

            }

            else if (currentMenu == SECTION_CONFIG)
            {
                if (Menu.Item.AddSection("Back")) 
                {
                    titleText = "Main";
                    Menu.SetTitleText(titleText);
                    currentMenu = MENU_MAIN;
                }
                
                if (Menu.Item.AddText("Save"))
                {
                    SaveSettings(global_settings, config_filename);
                }

                if (Menu.Item.AddText("Load"))
                {
                    LoadSettings(global_settings, config_filename);
                }

                if (Menu.Item.AddText("Reset")) 
                {
                    ResetSettings(global_settings, config_filename);
                }
                

                

            }




            Menu.End();




        }
    }
}

void RenderImGui(
    bool p_gta5NotRunning,
    bool p_fivemNotRunning,
    HWND window
)
{


    std::this_thread::yield();

    if (visibilityStatus == HIDDEN)
    {
        return; // Do not render anything if hidden
    }

    


    bool shouldShowCursor = (showGta5MapWindow || showOtherWindow || showImGuiDebugWindow || externalLogsWindowActive) && visibilityStatus == VISIBLE;

    ImGui::GetIO().MouseDrawCursor = shouldShowCursor;

    if (visibilityStatus == VISIBLE)
    {
        //toggle flag and update
        if (noActivateFlag)
        {
            // Add WS_EX_NOACTIVATE flag
            SetWindowLongPtr(window, GWL_EXSTYLE, GetWindowLongPtr(window, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
        }
        else 
        {
            // Remove WS_EX_NOACTIVATE flag
            SetWindowLongPtr(window, GWL_EXSTYLE, GetWindowLongPtr(window, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    g_alert_notification.Draw();

    if (showGTA5styleWindow)
    {
        RenderGTA5Menu(p_gta5NotRunning, p_fivemNotRunning);
    }
    if (showGta5MapWindow && visibilityStatus == VISIBLE) 
    {
        RenderMapWindow();
    }
    if (showOtherWindow && visibilityStatus == VISIBLE)
    {
        RenderMain(p_gta5NotRunning, p_fivemNotRunning);
    }
    if (showImGuiDebugWindow && visibilityStatus == VISIBLE)
    {
        ImGui::ShowDemoWindow();
    }
    if ((visibilityStatus == VISIBLE || visibilityStatus == OVERLAY) && globals::overlayToggled)
    {
        RenderOverlays(p_gta5NotRunning, p_fivemNotRunning);
        if (showMetricsOverlay)
            RenderPerformanceOverlay(p_gta5NotRunning, p_fivemNotRunning);
    }

    if (externalLogsWindowActive)
    {
        RenderExternalLogWindow();
    }

    if (_network__no_save_exploit) {
        g_alert_notification.CreateNotification("WARNING: R* cloud saves are blocked!");
    }

    //executes once in the first frame
    if (!loggedStart) 
    {
        Logging::InitializeImGuiLogging();

        //ExternalLogger::InitializeExternalLogger();
        std::thread logServerThread(LoggingServer::StartLogServer);
        logServerThread.detach();


        Logging::Log("Initialization finished", 1);
        loggedStart = true;
    }

    ImGui::EndFrame();

    ImGui::Render();

    

}