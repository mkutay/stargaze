class Move {
private:
  unsigned short m_move; // 16 bits (6 for from and to, 4 for type)
public:
  Move(int from, int to, int flags);
  unsigned short get_from();
  unsigned short get_to();
  unsigned short get_flags();
  bool is_promotion();
  bool is_capture();
  int get_promotion_piece();
};