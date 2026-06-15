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

#ifndef CONSTEXPR_RANDOM_TINY_SPLITMIX64_HPP
#define CONSTEXPR_RANDOM_TINY_SPLITMIX64_HPP

#include <cstdint>

namespace crand::detail
{
constexpr auto tiny_splitmix64(std::uint64_t* state) noexcept -> std::uint64_t
{
    std::uint64_t result = *state += 0x9e3779b97f4a7c15;
    result               = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
    result               = (result ^ (result >> 27)) * 0x94d049bb133111eb;
    return result ^ (result >> 31);
};
} // namespace crand::detail

#endif // CONSTEXPR_RANDOM_TINY_SPLITMIX64_HPP
