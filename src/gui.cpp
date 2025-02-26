#define IMGUI_DEFINE_MATH_OPERATORS

#include <iostream>
#include <thread>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <urlmon.h>
#include <filesystem>
#include <commdlg.h>

#include "gui.h"
#include "ProcessHandler.h"
#include "Calculations.h"
#include "FormattedInfo.h"
#include "TicTacToe.h"
#include "Logging.h"
#include "globals.h"
#include "ExternalLogger.h"

#include "System.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui/imgui_internal.h"
#include "imgui_styleGTA5/Include.hpp"


#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "urlmon.lib")





static C_ImMMenu MMenu;



std::string titleText = "Home";






char filePathBuffer[256] = "";


std::string fps;

static const char* popupTitle = "Desync from the current GTA:O session?";
static std::string mainWindowTitle = "GTA 5 ImGui [External]";

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

bool isLosSantosSelected = true;

bool externalLogsWindowActive = false;

static bool showGta5MapWindow = false;

bool showOtherWindow = true;
bool showGTA5styleWindow = false;


bool isDesyncInProgress = false;


ImVec4 subTitleColor(0.0f, 0.5f, 1.0f, 1.0f);
ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw




std::vector<std::string> availableResolutions = System::GetAvailableResolutions();
static std::vector<const char*> resolutionOptions;

static int currentResolution = 0;


std::chrono::steady_clock::time_point startTime;


ID3D11ShaderResourceView* lsMapTexture = nullptr;
ID3D11ShaderResourceView* cayoMapTexture = nullptr;



static float progress = 0.0f;
static float progress_dir = 1.0f;
static float progress_duration = 10.0f;
static float progress_step = 0.0f;



void DesyncOperation() {
    ProcessHandler::DesyncFromGTAOnline();
}


void LaunchLS() {

    std::thread([]() {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = L"C:\\Steam\\steamapps\\common\\Lossless Scaling\\LosslessScaling.exe";
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteEx(&sei);
    }).detach();
}





void guiLoadImage(ID3D11Device* device, const char* imagePath, ID3D11ShaderResourceView** textureView) {
    Logging::Log("loading gta5 map", 1); //so i know its being called

    // Load the image using stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(imagePath, &width, &height, &channels, 4); // Load as RGBA (4 channels)
    if (!imageData) {
        printf("Failed to load image: %s\n", imagePath);
        return;
    }

    // Create a texture description
    D3D11_TEXTURE2D_DESC textureDesc = {};
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
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4; // 4 bytes per pixel (RGBA)
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&textureDesc, &initData, &texture);
    stbi_image_free(imageData); // Free image data after creating texture

    if (FAILED(hr)) {
        printf("Failed to create texture from image\n");
        return;
    }

    // Create Shader Resource View (SRV)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(texture, &srvDesc, textureView);
    texture->Release(); // Release the texture (we only need the SRV)
    if (FAILED(hr)) {
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

namespace gui {
    void LogImGui(const std::string& message, int errorLevel) {

        if (!ImGui::GetCurrentContext()) {
            Logging::Log("No context!", 3);
            return;  // Avoid logging if ImGui context is not initialized
        }


        // Validate error level and set default if invalid
        std::string prefix;
        ImVec4 color;

        switch (errorLevel) {
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











/**
 **************************   Initialize ImGui ***************************************
**/

void InitializeImGui(HWND window, ID3D11Device* device, ID3D11DeviceContext* device_context) {
    Logging::Log("Initializing ImGui...", 1);

    //assets

    guiLoadImage(device, "mLS.png", &lsMapTexture);
    guiLoadImage(device, "mCayo.png", &cayoMapTexture);


    const char* fontPath = "roboto_medium.ttf";
    const std::string url = "https://github.com/Vasik96/gta5_imgui/raw/refs/heads/master/roboto_medium.ttf";


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

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 2.0f;
    style.GrabRounding = 4.0f;



    ImGui::StyleColorsClassic();

    //ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.7f;

    // io & fonts
    ImGuiIO& io = ImGui::GetIO();


    io.Fonts->AddFontFromFileTTF(fontPath, 14.0f);
    io.Fonts->Build();
    
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    MMenu.Initialize();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);


    for (const auto& res : availableResolutions) {
        resolutionOptions.push_back(res.c_str());
    }
    currentResolution = System::GetCurrentResolutionIndex(availableResolutions);
    Logging::Log("ImGui initialized", 1);

    SetFocus(NULL);
}










void RenderMain(bool p_gta5NotRunning, bool p_fivemNotRunning) {

    ImGuiIO& io = ImGui::GetIO();


    ImGui::Begin(mainWindowTitle.c_str(), &globals::running);  // Main window



    if (ImGui::BeginTabBar("tab_bar1")) {



        // Main tab
        if (ImGui::BeginTabItem("Main")) {

            ImGui::SeparatorText("General");



            if (!p_gta5NotRunning || !p_fivemNotRunning) {
                ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());

            }
            else {
                ProcessHandler::ResetSession();
                //ImGui::Text("No active GTA 5 or FiveM session");
            }

            ImGui::Text("Last Timer Result: %s", globals::lastTimerResult.c_str());

            if (p_gta5NotRunning && p_fivemNotRunning) {

                ImGui::SetNextItemWidth(128.0f);

                // Create the combo box with three selections
                const char* comboItems[] = { "GTA 5", "GTA 5 (Online)", "GTA 5 (Modded)", "FiveM" };
                if (ImGui::BeginCombo("##GameCombo", comboItems[currentSelection])) {
                    for (int i = 0; i < IM_ARRAYSIZE(comboItems); i++) {
                        bool isSelected = (currentSelection == i);
                        if (ImGui::Selectable(comboItems[i], isSelected)) {
                            currentSelection = i;  // Update the current selection based on user input
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();  // Keep the default selection focused
                        }
                    }
                    ImGui::EndCombo();
                }


                if (ImGui::Button("Launch##Main")) {
                    switch (currentSelection) {
                    case 0:
                        // Launch GTA 5 normally
                        ProcessHandler::LaunchGTA5(true, false);
                        break;
                    case 1:
                        // Launch GTA 5 straight into online
                        ProcessHandler::LaunchGTA5(true, true);
                        break;
                    case 2:
                        // Launch GTA 5 without anticheat (used for modded game)
                        ProcessHandler::LaunchGTA5(false, false);
                        break;
                    case 3:
                        // Launch FiveM
                        ProcessHandler::LaunchFiveM();
                        break;
                    default:
                        ImGui::Text("[Error] Invalid selection");
                        break;
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Launches the selected game.");
                }



                ImGui::SameLine();
            }


            if (ImGui::Button("Exit Game")) {
                gta5NotRunning = !ProcessHandler::TerminateGTA5();
                fivemNotRunning = !ProcessHandler::TerminateGTA5();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Terminates the GTA 5 or FiveM process.");
            }

            ImGui::SameLine();
            if (ImGui::Button("Desync from GTA:O")) {

                progress = 0.0f;
                progress_dir = 1.0f;
                progress_duration = 10.0f;
                progress_step = 0.0f;

                isDesyncInProgress = false;


                if (!ProcessHandler::IsProcessRunning(L"GTA5.exe")) {
                    gta5NotRunning = true;
                }
                else {
                    ImGui::OpenPopup(popupTitle);
                }
            }
            if (ImGui::IsItemHovered()) {
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

                if (progress_duration > 0.0f) {
                    progress_step = (1.0f / progress_duration) * ImGui::GetIO().DeltaTime; // How much progress to add per frame
                }

                // If "OK" button clicked, start desync operation and set progress flag
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    isDesyncInProgress = true;

                    std::thread desyncThread(DesyncOperation);
                    desyncThread.detach();
                }

                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();

                // Close popup if user clicks "Cancel"
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    isDesyncInProgress = false; // Cancel progress
                }

                // If desync is in progress, show the progress bar
                if (isDesyncInProgress) {
                    // Increment progress based on the frame time and direction
                    progress += progress_step * progress_dir;

                    // Ensure progress stays within the 0.0f to 1.0f range
                    if (progress >= 1.0f) {
                        progress = 1.0f;
                        progress_dir = 0.0f; // Stop the progress after reaching 100%
                        ImGui::CloseCurrentPopup(); // Close the popup after reaching 100%
                    }

                    ImGui::Text("Desyncing, please wait...");
                    ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
                }

                ImGui::EndPopup();
            }







            if (gta5NotRunning) {
                ImGui::Text("GTA5.exe isn't running.");
            }
            else if (fivemNotRunning) {
                ImGui::Text("FiveM.exe isn't running.");
            }



            if (ImGui::Button("Launch LS")) {
                LaunchLS();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Launches Lossless Scaling");
            }



            ImGui::SeparatorText("Settings");

            ImGui::Checkbox("GTA 5 Menu", &showGTA5styleWindow);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggles the GTA 5 interaction menu style window.");
            }

            ImGui::Checkbox("GTA 5 Map", &showGta5MapWindow);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggles the map window.");
            }

            ImGui::Checkbox("Overlay", &globals::overlayToggled);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If disabled, overlay will not show.");
            }


            ImGui::Checkbox("Background Interaction", &noActivateFlag);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If enabled, the window will behave more like an overlay, but keyboard input (text fields) will not work. (Reopen the menu to apply changes)");
            }


            ImGui::Checkbox("Demo Window", &showImGuiDebugWindow);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggles the ImGui demo window.");
            }









            ImGui::EndTabItem();
        }



        if (ImGui::BeginTabItem("Utility")) {
            ImGui::SeparatorText("Money Calculations");
            ImGui::Spacing();

            ImGui::BeginChild("moneyTextFields", ImVec2(128, 105), true);

            ImGui::PushItemWidth(112.0f);



            
            ImGui::TextColored(subTitleColor, "Input details...");

            // GUI input and calculation handling
            if (ImGui::InputTextWithHint("##currentMoney", "Current money", currentPlayerMoneyBuffer, sizeof(currentPlayerMoneyBuffer), ImGuiInputTextFlags_CharsDecimal)) {
                int currentPlayerMoney = Calculations::ParseInteger(currentPlayerMoneyBuffer);
                if (currentPlayerMoney == -1) {
                    currentPlayerMoneyBuffer[0] = '\0'; // Make the text field blank
                }
                else {
                    snprintf(currentPlayerMoneyBuffer, sizeof(currentPlayerMoneyBuffer), "%d", currentPlayerMoney);
                }
            }

            


            // Money to receive input
            if (ImGui::InputTextWithHint("##moneyToReceive", "Received money", moneyToReceiveBuffer, sizeof(moneyToReceiveBuffer), ImGuiInputTextFlags_CharsDecimal)) {
                int moneyToReceive = Calculations::ParseInteger(moneyToReceiveBuffer);
                if (moneyToReceive == -1) {
                    moneyToReceiveBuffer[0] = '\0'; // Make the text field blank
                }
                else {
                    snprintf(moneyToReceiveBuffer, sizeof(moneyToReceiveBuffer), "%d", moneyToReceive);
                }
            }

            //ImGui::SameLine();

            // CEO bonus input
            if (ImGui::InputTextWithHint("##ceoBonus", "CEO's in lobby", ceoBonusBuffer, sizeof(ceoBonusBuffer), ImGuiInputTextFlags_CharsDecimal)) {
                int ceoBonus = Calculations::ParseInteger(ceoBonusBuffer);
                if (ceoBonus == -1) {
                    ceoBonusBuffer[0] = '\0'; // Make the text field blank
                }
                else {
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

            if (ImGui::Button("Calculate Total Money")) {
                std::string result = Calculations::CalculateTotalMoney(currentPlayerMoney, moneyToReceive, ceoBonus);
                strncpy_s(totalMoneyText, sizeof(totalMoneyText), result.c_str(), sizeof(totalMoneyText) - 1);
            }

            

            ImGui::Text("Total Money: %s", totalMoneyText); //calculated money text


            ImGui::EndGroup();




            ImGui::EndTabItem();
        }


            // System tab
        if (ImGui::BeginTabItem("System")) {

            ImGui::SeparatorText("Date & Time");



            ImGui::Text("Time: %s", FormattedInfo::GetFormattedTime().c_str());
            ImGui::Text("Date: %s", FormattedInfo::GetFormattedDate().c_str());



            ImGui::Spacing();
            ImGui::SeparatorText("System Usage");


            ImGui::Text("CPU Usage: %s", FormattedInfo::GetFormattedCPUUsage().c_str());
            ImGui::SameLine();
            ImGui::Text("  Running processes: %s", FormattedInfo::GetFormattedRP().c_str());



            static float cpuUsageHistory[100] = { 0 };
            static int historyIndex = 0;
            static float lastCPUUsage = -1.0f;

            // Fetch CPU usage
            float currentCPUUsage = std::stof(FormattedInfo::GetFormattedCPUUsage());

            // Store the new value only if it has changed
            cpuUsageHistory[historyIndex] = currentCPUUsage;
            historyIndex = (historyIndex + 1) % IM_ARRAYSIZE(cpuUsageHistory);
            lastCPUUsage = currentCPUUsage;

            // Compute rolling average for smoother movement
            float average = 0.0f;
            for (int n = 0; n < IM_ARRAYSIZE(cpuUsageHistory); n++)
                average += cpuUsageHistory[n];
            average /= (float)IM_ARRAYSIZE(cpuUsageHistory);

            // Create an overlay text for average CPU usage
            char overlay[32];
            sprintf_s(overlay, "Avg: %.1f%%", average);

            // Render the smoothed graph
            ImGui::PlotLines(
                "##CPUUsageGraph",
                cpuUsageHistory,
                IM_ARRAYSIZE(cpuUsageHistory),
                historyIndex,
                overlay,  // Show the average usage in the overlay
                0.0f,
                100.0f,
                ImVec2(230, 60)
            );






            ImGui::Text("RAM Usage: %s", FormattedInfo::GetFormattedRAMUsage().c_str());

            ImGui::SeparatorText("Screen");







            ImGui::PushItemWidth(110.f);

            if (ImGui::BeginCombo("Resolution##Screen", resolutionOptions[currentResolution])) {
                for (int i = 0; i < resolutionOptions.size(); i++) {
                    bool isSelected = (currentResolution == i);
                    if (ImGui::Selectable(resolutionOptions[i], isSelected)) {
                        currentResolution = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();

            // Apply Button
            if (ImGui::Button("Apply")) {
                int width = 0, height = 0;
                sscanf_s(resolutionOptions[currentResolution], "%dx%d", &width, &height);
                System::ChangeResolution(width, height);
            }




            ImGui::EndTabItem();

        }
        





        // Info tab
        if (ImGui::BeginTabItem("Info")) {

            ImGui::SeparatorText("Keybinds");

            //F keybinds
            ImGui::Text("[F5] Start/Stop a timer. (Shows in the overlay)");




            ImGui::Text("[Insert] Switch between window modes.");
            ImGui::Text("[End] Exit the program.");



            ImGui::Spacing();
            ImGui::SeparatorText("Other");


            ImGui::TextLinkOpenURL("GTA:O Map", "https://gtaweb.eu/gtao-map/ls/");
           /* ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();*/
            ImGui::TextLinkOpenURL("R* Activity Feed", "https://socialclub.rockstargames.com/");


            ImGui::TextLinkOpenURL("R* Newswire", "https://www.rockstargames.com/newswire/");
          /*  ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();*/
            ImGui::TextLinkOpenURL("R* Community Races", "https://socialclub.rockstargames.com/jobs/?dateRangeCreated=any&platform=pc&sort=likes&title=gtav/");

            ImGui::EndTabItem();
        }









        // Extras tab
        if (ImGui::BeginTabItem("Extras")) {


            ImGui::SeparatorText("Cookie Clicker");


            if (ImGui::Button("Click for points")) {
                clickedTimes++;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to add a point.\nCurrent points: %d", clickedTimes);
            }

            ImGui::Text("Points: %d", clickedTimes);


            ImGui::Spacing();
            ImGui::SeparatorText("Tic-Tac-Toe");


            //tic tac toe
            ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
            ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
            ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw
            TicTacToe::DrawTicTacToeInsideWindow(playerColor, aiColor, drawColor);




            ImGui::EndTabItem();
        }
 
        

        // Debug tab
        if (ImGui::BeginTabItem("Debug")) {


            ImGui::SeparatorText("Logs");



            if (ImGui::Button("Clear")) {
                //clear logs
                logBuffer.clear();
                gui::LogImGui("Logs cleared", 1);
            }
            ImGui::SameLine();
            ImGui::Checkbox("External Logs Window", &externalLogsWindowActive);



            ImGui::BeginChild("LogChild", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);


            for (const auto& log : logBuffer) {
                ImVec4 textColor;

                if (log.find("[info]") == 0) {
                    textColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // Aqua
                }
                else if (log.find("[warning]") == 0) {
                    textColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                }
                else if (log.find("[error]") == 0) {
                    textColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                }
                else {
                    textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white
                }

                // Render the log with the determined color
                ImGui::TextColored(textColor, "%s", log.c_str());
            }

            if (scrollToBottom) {
                ImGui::SetScrollHereY(1.0f); // Scroll to the bottom
                scrollToBottom = false; // Reset the flag to prevent excessive scrolling
            }

            ImGui::EndChild(); // End of the scrollable log section

            // End the Tab Item
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

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




void RenderExternalLogWindow() {
    static float previousScrollY = 0.0f; // Track the previous scroll position
    static bool userScrolling = false;  // Whether the user is actively scrolling

    if (ImGui::Begin("External Logs", &externalLogsWindowActive)) {
        // Text input for file path
        ImGui::InputTextWithHint("##filePathField", "Enter file path here...", filePathBuffer, sizeof(filePathBuffer));

        ImGui::SameLine();

        if (ImGui::Button("Open File Explorer"))
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
        if (strlen(filePathBuffer) > 0) { // Ensure file path is not empty
            std::string logContents = FormattedInfo::GetLogFromFile(filePathBuffer);

            // Split the log contents into lines for rendering
            std::istringstream logStream(logContents);
            std::string line;
            while (std::getline(logStream, line)) {
                logLines.push_back(line);
            }
        }

        // Use the ImGuiListClipper for efficient rendering of log lines
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(logLines.size())); // Number of log lines
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                ImGui::TextUnformatted(logLines[i].c_str());
            }
        }
        clipper.End();

        // Automatically scroll down if the user is not scrolling
        if (!userScrolling && scrollY < scrollMaxY) {
            ImGui::SetScrollY(scrollMaxY); // Scroll to the bottom
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

















void RenderOverlays(bool p_gta5NotRunning, bool p_fivemNotRunning) {
    // Overlay flags (from imgui demo)
    static int location = 1;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
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
    if (ImGui::Begin("InfoOverlay", &p_open, window_flags)) {
        // Center the "Common info" text
        const char* title = "Common Info";
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) / 2.0f);
        ImGui::TextColored(subTitleColor, "%s", title);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Time: %s", FormattedInfo::GetFormattedTime().c_str());
        ImGui::Text("Date: %s", FormattedInfo::GetFormattedDate().c_str());

        ImGui::Text("CPU: %s", FormattedInfo::GetFormattedCPUUsage().c_str());
        

        if (!p_gta5NotRunning) {
            ImGui::Text("Currently playing GTA 5");
            //ImGui::Text("FPS: %s", FormattedInfo::GetFormattedGTA5FPS());
        }
        else if (!p_fivemNotRunning) {
            ImGui::Text("Currently playing FiveM");
        }

        if (!p_gta5NotRunning || !p_fivemNotRunning) {
            ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());
        }
        if (globals::isTimerRunning) {
            ImGui::Text("Timer: %s", FormattedInfo::GetFormattedTimer().c_str());
        }
        


    }
    ImGui::End();
}


void RenderMapWindow() {
    static ImVec2 blueDotPos = ImVec2(-1, -1); // Initial position of the blue dot (off-screen)
    const ImVec2 imageSize = ImVec2(550, 550); // Size of the image

    // Choose the correct texture based on the selection
    ImTextureID currentMapTexture = isLosSantosSelected ? (ImTextureID)lsMapTexture : (ImTextureID)cayoMapTexture;

    if (currentMapTexture) {
        ImGui::Begin("GTA 5 Map");

        ImGui::TextColored(subTitleColor, "Interactive Map");
        ImGui::Spacing();

        // Radio buttons to switch between Los Santos and Cayo Perico maps
        if (ImGui::RadioButton("Los Santos", isLosSantosSelected))
        {
            isLosSantosSelected = true;
        }
        if (ImGui::RadioButton("Cayo Perico", !isLosSantosSelected))
        {
            isLosSantosSelected = false;
        }

        if (ImGui::BeginChild("MapChild", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar)) {

            // Get the current position of the image in the ImGui window
            ImVec2 imageStartPos = ImGui::GetCursorScreenPos();

            // Render the correct image based on the selection
            ImGui::Image(currentMapTexture, imageSize);

            // Check if the image was clicked
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Get the mouse position
                ImVec2 mousePos = ImGui::GetMousePos();

                // Calculate the relative position of the mouse within the image
                blueDotPos.x = mousePos.x - imageStartPos.x;
                blueDotPos.y = mousePos.y - imageStartPos.y;

                blueDotPos.x = std::clamp(blueDotPos.x, 0.0f, imageSize.x);
                blueDotPos.y = std::clamp(blueDotPos.y, 0.0f, imageSize.y);
            }

            // Draw the blue dot if it's on-screen
            if (blueDotPos.x >= 0 && blueDotPos.y >= 0) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 dotScreenPos = ImVec2(imageStartPos.x + blueDotPos.x, imageStartPos.y + blueDotPos.y);
                drawList->AddCircleFilled(dotScreenPos, 5.0f, IM_COL32(0, 0, 255, 255)); // Blue dot
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }
}









void RenderImGui(
    bool p_gta5NotRunning,
    bool p_fivemNotRunning,
    HWND window) {


    bool shouldShowCursor = showGta5MapWindow || showOtherWindow || showImGuiDebugWindow;

    ImGui::GetIO().MouseDrawCursor = shouldShowCursor;


    if (visibilityStatus == HIDDEN) {
        return; // Do not render anything if hidden
    }


    if (visibilityStatus == VISIBLE) {
        //toggle flag and update
        if (noActivateFlag) {
            // Add WS_EX_NOACTIVATE flag
            SetWindowLongPtr(window, GWL_EXSTYLE, GetWindowLongPtr(window, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
        }
        else {
            // Remove WS_EX_NOACTIVATE flag
            SetWindowLongPtr(window, GWL_EXSTYLE, GetWindowLongPtr(window, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
        }
    }
    else {
    }




    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    if (showGTA5styleWindow) {
        //render main stuff here (windows, and other interactive stuff)
        if (visibilityStatus == VISIBLE) {
            MMenu.SetHeaderText("GTA 5 Menu");

            enum MenuState
            {
                MENU_MAIN,
                SECTION_SYSTEM,
                SECTION_INFO,
                SECTION_CONFIRM_DESYNC,
                SECTION_MONEY

            };

            static MenuState currentMenu = MENU_MAIN;

            if (MMenu.Begin())
            {
                MMenu.SetTitleText(titleText);
                MMenu.SetFooterText("v1.1");

                if (currentMenu == MENU_MAIN)
                {

                    MMenu.SetTitleText(titleText);

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


                    if (!p_gta5NotRunning || !p_fivemNotRunning) {
                        std::string sessionText = "Current session: " + ProcessHandler::GetCurrentSession();
                        MMenu.Item.AddTextUnselectable(sessionText.c_str());
                    }

                    std::string timerText = "Last Timer Result: " + globals::lastTimerResult;
                    MMenu.Item.AddTextUnselectable(timerText.c_str());

                    MMenu.Item.AddSeparator("General");


                    static std::vector<std::string> m_ComboList = {
                         "GTA 5",
                         "GTA Online",
                         "GTA 5 (Modded)",
                         "FiveM"
                    };

                    static int current_selection = 0;

                    MMenu.SetTitleText(titleText);

                    // Ensure the combo box is functional
                    if (MMenu.Item.AddCombo("Launch GTA 5", &current_selection, m_ComboList)) {
                        // launch the selected game
                        switch (current_selection) {
                        case 0:
                            // Launch GTA 5 normally
                            ProcessHandler::LaunchGTA5(true, false);
                            break;
                        case 1:
                            // Launch GTA 5 straight into online
                            ProcessHandler::LaunchGTA5(true, true);
                            break;
                        case 2:
                            // Launch GTA 5 without anticheat (used for modded game)
                            ProcessHandler::LaunchGTA5(false, false);
                            break;
                        case 3:
                            // Launch FiveM
                            ProcessHandler::LaunchFiveM();
                            break;
                        default:
                            ImGui::Text("[Error] Invalid selection");
                            break;
                        }
                    }

                    if (MMenu.Item.AddText("Launch LS")) {
                        LaunchLS();
                    }


                    MMenu.Item.AddCheckbox("GTA 5 Map", &showGta5MapWindow);
                    MMenu.Item.AddCheckbox("Overlay", &globals::overlayToggled);
                    MMenu.Item.AddCheckbox("Background Interaction", &noActivateFlag);
                    MMenu.Item.AddCheckbox("Demo Window", &showImGuiDebugWindow);
                    MMenu.Item.AddCheckbox("Other", &showOtherWindow);

                    if (MMenu.Item.AddText("Exit Game")) {
                        gta5NotRunning = !ProcessHandler::TerminateGTA5();
                        fivemNotRunning = !ProcessHandler::TerminateGTA5();
                    }
                    if (MMenu.Item.AddText("Exit Menu")) {
                        globals::running = false;
                    }

                    if (MMenu.Item.AddSection("Desync from GTA:O")) {
                        if (ProcessHandler::IsProcessRunning(L"GTA5.exe")) {
                            currentMenu = SECTION_CONFIRM_DESYNC;
                        }
                    }

                    MMenu.Item.AddSeparator("Utility");
                    if (MMenu.Item.AddSection("Money Calculations")) {
                        currentMenu = SECTION_MONEY;
                        titleText = "Money Calculations";
                    }

                }

                else if (currentMenu == SECTION_CONFIRM_DESYNC)
                {
                    MMenu.Item.AddTextUnselectable("{FF0000}WARNING: {FFFFFF}GTA 5 will freeze after confirming!");
                    MMenu.Item.AddTextUnselectable("Wait 10s after confirming.");
                    MMenu.Item.AddSeparator("");


                    if (MMenu.Item.AddText("Yes")) {
                        std::thread desyncThread(DesyncOperation);
                        desyncThread.detach();

                        currentMenu = MENU_MAIN;
                    }
                    if (MMenu.Item.AddText("No")) {
                        currentMenu = MENU_MAIN;
                    }
                }




                else if (currentMenu == SECTION_MONEY) {
                    if (MMenu.Item.AddSection("Back")) {
                        titleText = "Main";
                        MMenu.SetTitleText(titleText);
                        currentMenu = MENU_MAIN;
                    }

                    MMenu.Item.AddTextInput("Enter current money ->", "Current money", currentPlayerMoneyBuffer, sizeof(currentPlayerMoneyBuffer), ImGuiInputTextFlags_CharsDecimal);
                    MMenu.Item.AddTextInput("Enter received money ->", "Received money", moneyToReceiveBuffer, sizeof(moneyToReceiveBuffer), ImGuiInputTextFlags_CharsDecimal);
                    MMenu.Item.AddTextInput("Enter number of CEO's in lobby ->", "CEO's in lobby", ceoBonusBuffer, sizeof(ceoBonusBuffer), ImGuiInputTextFlags_CharsDecimal);


                    int currentPlayerMoney = Calculations::ParseInteger(currentPlayerMoneyBuffer);
                    int moneyToReceive = Calculations::ParseInteger(moneyToReceiveBuffer);
                    int ceoBonus = Calculations::ParseInteger(ceoBonusBuffer);

                    if (MMenu.Item.AddText("{FFF300}Calculate Money")) {
                        std::string result = Calculations::CalculateTotalMoney(currentPlayerMoney, moneyToReceive, ceoBonus);
                        strncpy_s(totalMoneyText, sizeof(totalMoneyText), result.c_str(), sizeof(totalMoneyText) - 1);
                    }

                    std::string totalMoneyTextStr = "Total Money: " + std::string(totalMoneyText);
                    MMenu.Item.AddTextUnselectable(totalMoneyTextStr.c_str());

                }



                MMenu.End();




            }
        }

    }

    if (showGta5MapWindow && visibilityStatus == VISIBLE) {
        RenderMapWindow();
    }


    if (showOtherWindow && visibilityStatus == VISIBLE) {
        RenderMain(p_gta5NotRunning, p_fivemNotRunning);
    }


    if (showImGuiDebugWindow && visibilityStatus == VISIBLE) {
        ImGui::ShowDemoWindow();
    }


    if ((visibilityStatus == VISIBLE || visibilityStatus == OVERLAY) && globals::overlayToggled) {
        RenderOverlays(p_gta5NotRunning, p_fivemNotRunning);
    }



    if (externalLogsWindowActive) {
        RenderExternalLogWindow();
    }




    if (!loggedStart) {
        Logging::InitializeImGuiLogging();
        ExternalLogger::InitializeExternalLogger();
        Logging::Log("Initialization finished", 1);
        loggedStart = true;

    }


    ImGui::Render();

    

}