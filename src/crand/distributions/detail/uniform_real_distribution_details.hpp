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

#ifndef CONSTEXPR_RANDOM_UNIFORM_REAL_DISTRIBUTION_DETAILS_HPP
#define CONSTEXPR_RANDOM_UNIFORM_REAL_DISTRIBUTION_DETAILS_HPP

#include <cmath>

namespace crand::detail::uniform_real_distribution
{
template<typename T>
constexpr auto ceilint(T a, T b, T g) noexcept -> T
{
    auto const s   = b / g - a / g;
    auto const eps = (std::abs(a) <= std::abs(b)) ? -a / g - (s - b / g) : b / g - (s + a / g);
    auto const si  = std::ceil(s);
    return (s != si) ? si : si + (eps > 0);
}
template<typename T>
constexpr auto compute_gamma(T a, T b) noexcept -> T
{
    return std::abs(a) <= std::abs(b) ? b - std::nexttoward(b, a) : std::nexttoward(a, b) - a;
}
template<typename T>
constexpr auto gen_inclusive_inclusive_loe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return (k == hi) ? a : b - k * g;
}
template<typename T>
constexpr auto gen_inclusive_inclusive_nloe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return (k == hi) ? b : a + k * g;
}
template<typename T>
constexpr auto gen_exclusive_inclusive_loe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return b - k * g;
}
template<typename T>
constexpr auto gen_exclusive_inclusive_nloe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return (k == hi - 1) ? b : a + (k + 1) * g;
}
template<typename T>
constexpr auto gen_inclusive_exclusive_loe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return (k == hi) ? a : b - k * g;
}
template<typename T>
constexpr auto gen_inclusive_exclusive_nloe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return a + (k - 1) * g;
}
template<typename T>
constexpr auto gen_exclusive_exclusive_loe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return b - k * g;
}
template<typename T>
constexpr auto gen_exclusive_exclusive_nloe(T a, T b, T g, T hi, std::size_t k) noexcept -> T
{
    return a + k * g;
}
} // namespace crand::detail::uniform_real_distribution
#endif // CONSTEXPR_RANDOM_UNIFORM_REAL_DISTRIBUTION_DETAILS_HPP
