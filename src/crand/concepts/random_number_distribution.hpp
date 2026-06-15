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

#ifndef CONSTEXPR_RANDOM_RANDOM_NUMBER_DISTRIBUTION_HPP
#define CONSTEXPR_RANDOM_RANDOM_NUMBER_DISTRIBUTION_HPP

#include "crand/concepts/uniform_random_bit_generator.hpp"

#include <concepts>

namespace crand
{
/// A random number distribution is a function object returning random numbers according to a probability density
/// function or a discrete probability distribution.
///
/// # Semantic Requirements
/// `random_number_distribution<D>` is modeled only if, given any object `d` of type `D` and any object `g` modelling
/// `uniform_random_bit_generator`:
/// - `d(g)` is in the range [`d.min()`, `d.max()`]
/// - `d(g)` has amortized constant complexity
///
/// # Notes
/// The function call operator need not be const, but some implementations may provide a const operator.
template<typename D>
concept random_number_distribution // clang-format off
    =  std::copy_constructible<D>
    && std::assignable_from<D&, D>
    && std::equality_comparable<D>
    && std::is_arithmetic_v<typename D::result_type>
    && requires (D& m, D const& c, detail::uniform_random_bit_generator::uniform_random_bit_generator_archetype& g) {
        { m(g) } -> std::same_as<typename D::result_type>;
        { c.min() } -> std::same_as<typename D::result_type>;
        { c.max() } -> std::same_as<typename D::result_type>;
    }; // clang-format on
} // namespace crand
#endif // CONSTEXPR_RANDOM_RANDOM_NUMBER_DISTRIBUTION_HPP
