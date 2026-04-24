public Class GameBoard {
private:
	int numPiles = 0;
	int piles[];

public :
	GameBoard(char[]& board) {
		createPiles(board);
	}

	void createPiles(char[]& board) {
		if (stoi(board[0]) >= 3 && stoi(board[0]) <= 9)  {
			numPiles = stoi(board[0]);
			for (int i = 1; i < char[].length; i = i + 2) {
				piles.push(stoi(strcat(board[i], board[i + 1])));
			}
		}
	}

	int makeMove(char[] & move) {
		if(move[0] > numPiles) {
			return -1; // Invalid move: Win by default
		}
		int fromPile = move[0];

		int numToRemove = stoi(strcat(move[1], move[2]));
		if (numToRemove > piles[fromPile]) {
			return -1; // Invalid move: Win by default
		}

		piles[fromPile] -= numToRemove;
		return 0; // Valid move: Update the game board
	}
	

}