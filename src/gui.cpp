#include <iostream>
#include <thread>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "gui.h"
#include "ProcessHandler.h"
#include "Calculations.h"
#include "FormattedInfo.h"
#include "TicTacToe.h"
#include "Logging.h"
#include "globals.h"
#include "ExternalLogger.h"


#pragma comment(lib, "Ws2_32.lib")


std::string fps;

static const char* popupTitle = "Desync from the current GTA:O session?";
static std::string mainWindowTitle = "GTA 5 ImGui [External]";

char currentPlayerMoneyBuffer[64] = "";
char moneyToReceiveBuffer[64] = "";
char ceoBonusBuffer[64] = "";
char totalMoneyText[128] = "";

int currentSelection = 0;
int clickedTimes = 0;

bool noActivateFlag = false;
bool showImGuiDebugWindow = false;
bool gta5NotRunning = false;
bool fivemNotRunning = false;
bool loggedStart = false;
bool scrollToBottom = false;
bool p_open = true;

ImVec4 subTitleColor(0.0f, 0.5f, 1.0f, 1.0f);
ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw




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





void RenderMain(bool p_gta5NotRunning, bool p_fivemNotRunning) {

    ImGui::Begin(mainWindowTitle.c_str(), &globals::running);  // Main window



    if (ImGui::BeginTabBar("tab_bar1")) {


        // Main tab
        if (ImGui::BeginTabItem("Main")) {
            if (!p_gta5NotRunning || !p_fivemNotRunning) {
                ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());

            }
            else {
                ProcessHandler::ResetSession();
                ImGui::Text("No active GTA 5 or FiveM session");
            }


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


            if (ImGui::Button("Exit GTA 5")) {
                gta5NotRunning = !ProcessHandler::TerminateGTA5();
                fivemNotRunning = !ProcessHandler::TerminateGTA5();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Terminates the GTA 5 or FiveM process.");
            }

            ImGui::SameLine();
            if (ImGui::Button("Desync from GTA:O")) {
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

                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    ProcessHandler::DesyncFromGTAOnline();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();

                //close popup
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }


            if (gta5NotRunning) {
                ImGui::Text("GTA5.exe isn't running.");
            }
            else if (fivemNotRunning) {
                ImGui::Text("FiveM.exe isn't running.");
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


            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(subTitleColor, "Money Calculations");
            ImGui::Spacing();

            ImGui::PushItemWidth(112.0f);


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

            ImGui::SameLine();

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


            //convert to int
            int currentPlayerMoney = Calculations::ParseInteger(currentPlayerMoneyBuffer);
            int moneyToReceive = Calculations::ParseInteger(moneyToReceiveBuffer);
            int ceoBonus = Calculations::ParseInteger(ceoBonusBuffer);

            if (ImGui::Button("Calculate Total Money")) {
                std::string result = Calculations::CalculateTotalMoney(currentPlayerMoney, moneyToReceive, ceoBonus);
                strncpy_s(totalMoneyText, sizeof(totalMoneyText), result.c_str(), sizeof(totalMoneyText) - 1);
            }

            ImGui::SameLine();
            ImGui::Text("Total Money: %s", totalMoneyText); //calculated money text
            


            ImGui::EndTabItem();
        }

        // System tab
        if (ImGui::BeginTabItem("System")) {

            ImGui::TextColored(subTitleColor, "Date & Time");
            ImGui::Spacing();


            ImGui::Text("Time: %s", FormattedInfo::GetFormattedTime().c_str());
            ImGui::Text("Date: %s", FormattedInfo::GetFormattedDate().c_str());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(subTitleColor, "System Usage");
            ImGui::Spacing();


            ImGui::Text("CPU Usage: %s", FormattedInfo::GetFormattedCPUUsage().c_str());
            ImGui::SameLine();
            ImGui::Text("  Running processes: %s", FormattedInfo::GetFormattedRP().c_str());

            ImGui::Text("RAM Usage: %s", FormattedInfo::GetFormattedRAMUsage().c_str());




            ImGui::EndTabItem();
        }



        // Info tab
        if (ImGui::BeginTabItem("Info")) {

            ImGui::TextColored(subTitleColor, "Keybinds");
            ImGui::Spacing();
            ImGui::Text("[Insert] Switch between window modes.");
            ImGui::Text("[End] Exit the program.");



            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();


            ImGui::TextColored(subTitleColor, "Other");
            ImGui::Spacing();
            ImGui::TextLinkOpenURL("GTA:O Map", "https://gtaweb.eu/gtao-map/ls/");
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("R* Activity Feed", "https://socialclub.rockstargames.com/");


            ImGui::TextLinkOpenURL("R* Newswire", "https://www.rockstargames.com/newswire/");
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("R* Community Races", "https://socialclub.rockstargames.com/jobs/?dateRangeCreated=any&platform=pc&sort=likes&title=gtav/");

            ImGui::EndTabItem();
        }









        // Extras tab
        if (ImGui::BeginTabItem("Extras")) {

            ImGui::TextColored(subTitleColor, "Cookie Clicker");
            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::Button("Click for points")) {
                clickedTimes++;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to add a point.\nCurrent points: %d", clickedTimes);
            }

            ImGui::Text("Points: %d", clickedTimes);


            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            //tic tac toe
            ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
            ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
            ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw
            TicTacToe::DrawTicTacToeInsideWindow(playerColor, aiColor, drawColor);




            ImGui::EndTabItem();
        }
 
        

        // Logs tab
        if (ImGui::BeginTabItem("Logs")) {


            ImGui::TextColored(subTitleColor, "Log"); // Subtitle with color
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                //clear logs
                logBuffer.clear();
                gui::LogImGui("Logs cleared", 1);
            }



            // Create a scrollable child window for the log area
            ImGui::BeginChild("LogChild", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);

            // Display all logs in the buffer
            for (const auto& log : logBuffer) {
                // Check the prefix to determine the color
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


    }
    ImGui::End();
}









/**
 **************************   Initialize ImGui ***************************************
**/

void InitializeImGui(HWND window, ID3D11Device* device, ID3D11DeviceContext* device_context) {
    Logging::Log("Initializing ImGui...", 1);


    const char* fontPath = "C:\\Users\\leman\\Desktop\\Soubory\\.projects\\gta5_imgui\\build\\roboto_medium.ttf";


    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 3.0f;
    style.GrabRounding = 4.0f;


    ImGui::StyleColorsClassic();

    //ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.7f;

    // io & fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(fontPath, 14.0f);




    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);


    Logging::Log("ImGui initialized", 1);
}



void RenderImGui(
    bool p_gta5NotRunning,
    bool p_fivemNotRunning, 
    HWND window, 
    ID3D11DeviceContext* device_context, 
    ID3D11RenderTargetView* render_target_view, 
    IDXGISwapChain* swap_chain) {


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

    



    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    if (showImGuiDebugWindow) {
        ImGui::ShowDemoWindow();
    }



    if ((visibilityStatus == VISIBLE || visibilityStatus == OVERLAY) && globals::overlayToggled) {
        RenderOverlays(p_gta5NotRunning, p_fivemNotRunning);
    }


    if (visibilityStatus == VISIBLE) {
        RenderMain(p_gta5NotRunning, p_fivemNotRunning);
    }




    if (!loggedStart) {
        Logging::InitializeImGuiLogging();
        ExternalLogger::InitializeExternalLogger();
        Logging::Log("Initialization finished", 1);
        loggedStart = true;

    }





    ImGui::Render();


    constexpr float color[4] = { 0.f, 0.f, 0.f, 0.f };
    device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
    if (render_target_view != nullptr) {
        device_context->ClearRenderTargetView(render_target_view, color);
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); //this has to be here, otherwise the window will not appear

    swap_chain->Present(1U, 0U);

    

}



void ShutdownImGui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}