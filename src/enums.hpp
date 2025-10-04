enum Piece {
    EMPTY = 0,
    W_PAWN = 1, // white pawn
    B_PAWN = 2, // black pawn
    KNIGHT = 3,
    BISHOP = 4,
    ROOK = 5,
    QUEEN = 6,
    KING = 7,
};

enum Colour {
    WHITE,
    BLACK,
};

constexpr Colour operator!(const Colour &a) {
    if (a == WHITE) return BLACK;
    return WHITE;
}