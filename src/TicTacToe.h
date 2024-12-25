#pragma once
#ifndef TICTACTOE_H
#define TICTACTOE_H

#include <string>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

// Namespace for TicTacToe game
namespace TicTacToe {
    extern char board[3][3];
    extern bool playerTurn;
    extern const char* result;
    extern bool gameFinished;

    void ResetTicTacToe();
    void BotMove();
    std::string CheckWinner();
    int Minimax(int depth, bool isMaximizingPlayer);
    void DrawTicTacToeInsideWindow(ImVec4 playerColor, ImVec4 aiColor, ImVec4 drawColor);
}

#endif // TICTACTOE_H
