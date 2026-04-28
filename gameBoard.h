#ifndef GAMEBOARD_H
#define GAMEBOARD_H

class GameBoard {
private:
    int numPiles;
    int* piles;

    void createPiles(const char* board);

public:
    GameBoard(const char* board);
    ~GameBoard();

    int makeMove(const char* move);
    bool checkWin() const;
};

#endif