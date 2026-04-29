#include "gameBoard.h"
#include <cstring>
#include <string>

GameBoard::GameBoard(const char* board) {
	numPiles = 0;
	piles = nullptr;
	createPiles(board);
}


GameBoard::~GameBoard() {
	delete[] piles;
}


void GameBoard::createPiles(const char* board) {
	if ((board[0] - '0') >= 3 && (board[0] - '0') <= 9)  {
		numPiles = board[0] - '0';
		piles = new int[numPiles];

		int index = 0;
		for (int i = 1; i + 1 < strlen(board) && index < numPiles; i = i + 2) {
			if ((board[i] - '0') * 10 + (board[i + 1] - '0') <= 20 && (board[i] - '0') * 10 + (board[i + 1] - '0') > 0) {
				piles[index++] = ((board[i] - '0') * 10 + (board[i + 1] - '0'));
			}
			else {
				delete[] piles;
				piles = nullptr;
				numPiles = 0;
				return;
			}
		}
	}
}

int GameBoard::makeMove(const char* move) {
	int fromPile = (move[0] - '0') - 1;

	if (strlen(move) != 3) {
		return -1; // Invalid move: Win by default
	}

	if(fromPile < 0 || fromPile > numPiles) {
		return -1; // Invalid move: Win by default
	}

	int numToRemove = (move[1] - '0') * 10 + (move[2] - '0');
	if (numToRemove > piles[fromPile]) {
		return -1; // Invalid move: Win by default
	}

	piles[fromPile] -= numToRemove;
	return 0; // Valid move: Update the game board
}

bool GameBoard::checkWin() const {
	for (int i = 0; i < numPiles; i++) {
		if (piles[i] > 0) {
			return false; // Game continues
		}
	}
	return true; // All piles are empty: Current player wins
}
	

