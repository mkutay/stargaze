//
// MIT License
//
// Copyright (c) 2022 Jan Möller
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef CONSTEXPR_RANDOM_XOSHIRO256_STARSTAR_DETAILS_HPP
#define CONSTEXPR_RANDOM_XOSHIRO256_STARSTAR_DETAILS_HPP

#include "tiny_splitmix64.hpp"

#include <algorithm>
#include <array>

#include <cstdint>

namespace crand::detail::xoshiro256_starstar
{
constexpr auto seed(std::uint64_t s) noexcept -> std::array<std::uint64_t, 4>
{
    return {tiny_splitmix64(&s), tiny_splitmix64(&s), tiny_splitmix64(&s), tiny_splitmix64(&s)};
}

constexpr auto advance_state(std::array<std::uint64_t, 4>& state) noexcept -> std::uint64_t
{
    constexpr auto rol64 = [](std::uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    };
    std::uint64_t const result = rol64(state[1] * 5, 7) * 9;
    std::uint64_t const t      = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;
    state[3] = rol64(state[3], 45);

    return result;
}
template<std::array<std::uint64_t, 4> JumpTable>
constexpr auto generate_forwarded_state(std::array<std::uint64_t, 4>& state) noexcept -> std::array<std::uint64_t, 4>
{
    std::array<std::uint64_t, 4> s{0, 0, 0, 0};
    for (auto magic : JumpTable)
        for (int shift = 0; shift < 64; ++shift)
        {
            if (magic & std::uint64_t(1) << shift)
                std::ranges::transform(s, state, s.begin(), [](std::uint64_t a, std::uint64_t b) { return a ^ b; });
            advance_state(state);
        }
    return s;
}
} // namespace crand::detail::xoshiro256_starstar

#endif // CONSTEXPR_RANDOM_XOSHIRO256_STARSTAR_DETAILS_HPP
