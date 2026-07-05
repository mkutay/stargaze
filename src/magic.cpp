#include "magic.hpp"

namespace Magic {

constexpr MagicKeys keys{};

constexpr std::array<std::array<BitBoard, 64>, 64> RAY_BETWEEN =
    detail::generate_ray_between();

static_assert(rook_attacks(SQ::D5, BitBoard{}) == Mask::ROOK_MASKS.at(SQ::D5));
static_assert(bishop_attacks(SQ::D5, BitBoard{}) ==
              Mask::BISHOP_MASKS.at(SQ::D5));

} // namespace Magic
