public Class GameBoard {

	GameBoard(char[]& board) {
		createPiles(board);
	}

	int numPiles = 0;
	int piles[];

	void createPiles(char[]& board) {
		if (stoi(board[0]) >= 3 && stoi(board[0]) <= 9)  {
			numPiles = stoi(board[0]);
			for (int i = 1; i < char[].length; i = i + 2) {
				if (stoi(strcat(board[i], board[i + 1])) <= 20 && stoi(strcat(board[i], board[i + 1])) > 0) {
					piles.push(stoi(strcat(board[i], board[i + 1])));
				}
				else {
					numPiles = 0;
				}
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

	char checkWin() {
		for (int i = 0; i < piles.length; i++) {
			if (piles[i] > 0) {
				return 'C'
			}
			else {
				return 'W';
			}
		}
	}
	

}