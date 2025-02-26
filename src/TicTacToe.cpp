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


// imgui
#include <imgui/imgui.h>



namespace TicTacToe {
    char board[3][3] = { {' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '} }; // Define the board
    bool playerTurn = true;  // Define the player's turn
    const char* result = nullptr; // Define the result
    bool gameFinished = false;

    void ResetTicTacToe();
    void BotMove();
    std::string CheckWinner();
    int Minimax(int depth, bool isMaximizingPlayer);
    void DrawTicTacToeInsideWindow(ImVec4 playerColor, ImVec4 aiColor, ImVec4 drawColor);
}



char board[3][3]; // The game board
bool playerTurn = true; // True for player, false for AI
const char* result = nullptr; // "Draw", "Win", or "Lose"
bool gameFinished = false;

std::string TicTacToe::CheckWinner() {
    // Check rows and columns
    for (int i = 0; i < 3; ++i) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return (board[i][0] == 'X') ? "Win" : "Lose";
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return (board[0][i] == 'X') ? "Win" : "Lose";
    }

    // Check diagonals
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return (board[0][0] == 'X') ? "Win" : "Lose";
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return (board[0][2] == 'X') ? "Win" : "Lose";

    // No winner yet
    return "";
}



void TicTacToe::ResetTicTacToe() {
    memset(board, ' ', sizeof(board));
    playerTurn = true;
    result = nullptr;
    gameFinished = false;
}



int TicTacToe::Minimax(int depth, bool isMaximizingPlayer) {
    std::string winner = CheckWinner();

    // Return the evaluation score if game is over
    if (winner == "Win") {
        return -10 + depth; // Player wins
    }
    if (winner == "Lose") {
        return 10 - depth; // Bot wins
    }
    if (winner == "Draw") {
        return 0; // Draw
    }

    if (isMaximizingPlayer) {
        int best = -1000;

        // Try every possible move
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == ' ') {
                    board[i][j] = 'O'; // Bot's move
                    best = max(best, Minimax(depth + 1, false));
                    board[i][j] = ' '; // Undo move
                }
            }
        }
        return best;
    }
    else {
        int best = 1000;

        // Try every possible move
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == ' ') {
                    board[i][j] = 'X'; // Player's move
                    best = min(best, Minimax(depth + 1, true));
                    board[i][j] = ' '; // Undo move
                }
            }
        }
        return best;
    }
}




void TicTacToe::BotMove() {
    if (gameFinished || result) return;

    int bestVal = -1000;
    int bestRow = -1, bestCol = -1;

    // First, check if the bot can win in the next move
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == ' ') {
                board[i][j] = 'O'; // Bot's move
                if (CheckWinner() == "Win") {
                    board[i][j] = 'O'; // Make the move
                    playerTurn = true;  // Switch turn after the bot moves
                    return;
                }
                board[i][j] = ' '; // Undo move
            }
        }
    }

    // Check if the player can win and block them
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == ' ') {
                board[i][j] = 'X'; // Pretend it's the player's turn
                if (CheckWinner() == "Lose") {
                    board[i][j] = 'O'; // Block the player's winning move
                    playerTurn = true;  // Switch turn after the bot moves
                    return;
                }
                board[i][j] = ' '; // Undo move
            }
        }
    }

    // If no immediate threats or opportunities, pick the best move (Minimax)
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == ' ') {
                board[i][j] = 'O'; // Make the move
                int moveVal = Minimax(0, false); // Evaluate the move
                board[i][j] = ' '; // Undo the move

                if (moveVal > bestVal) {
                    bestRow = i;
                    bestCol = j;
                    bestVal = moveVal;
                }
            }
        }
    }

    // Make the best move
    if (bestRow != -1 && bestCol != -1) {
        board[bestRow][bestCol] = 'O';
        playerTurn = true; // Switch turn after the bot moves
    }
}






void TicTacToe::DrawTicTacToeInsideWindow(ImVec4 playerColor, ImVec4 aiColor, ImVec4 drawColor) {

    ImGui::Spacing();

    if (result) {
        ImVec4 color;
        if (strcmp(result, "Draw") == 0) {
            color = drawColor;
        }
        else if (strcmp(result, "Win") == 0) {
            color = playerColor;
        }
        else {
            color = aiColor;
        }
        ImGui::TextColored(color, "%s", result);
    }
    else {
        ImGui::Text("Your turn: %s", playerTurn ? "X" : "O");
    }

    ImGui::SameLine();

    // Restart button
    if (ImGui::Button("Restart")) {
        ResetTicTacToe();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Restarts the Tic-Tac-Toe game.");
    }

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            ImGui::PushID(i * 3 + j);
            if (ImGui::Button(std::string(1, board[i][j]).c_str(), ImVec2(50, 50)) && board[i][j] == ' ' && playerTurn && !result) {
                board[i][j] = 'X';
                playerTurn = false;
                // After the player's move, let the bot take its turn
                BotMove();
            }
            ImGui::PopID();
            if (j < 2) ImGui::SameLine();
        }
    }

    // Check for winner
    for (int i = 0; i < 3; ++i) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            result = (board[i][0] == 'X') ? "Win" : "Lose";
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            result = (board[0][i] == 'X') ? "Win" : "Lose";
    }
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        result = (board[0][0] == 'X') ? "Win" : "Lose";
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        result = (board[0][2] == 'X') ? "Win" : "Lose";

    // Check for draw
    if (!result) {
        bool draw = true;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == ' ') {
                    draw = false;
                    break;
                }
            }
            if (!draw) break;
        }
        if (draw) result = "Draw";
    }

    if (result) gameFinished = true;

   
}



