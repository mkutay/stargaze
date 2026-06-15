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

#ifndef CONSTEXPR_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP
#define CONSTEXPR_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP

#include "crand/concepts/uniform_random_bit_generator.hpp"
#include "crand/distributions/detail/uniform_real_distribution_details.hpp"
#include "crand/distributions/distribution_limits.hpp"
#include "crand/distributions/uniform_int_distribution.hpp"

#include <concepts>

#include <cassert>
#include <cmath>

namespace crand
{
/// Produces uniformly distributed random floating point numbers.
///
/// Mathematically, the probability of a number in range [`x`, `x+d`] being generated is `d/(b-a)`, and the probability
/// of a specific floating-point number is `0`. However, due to the way the IEEE-754 floating point representation
/// works, the same is not true for this distribution.
/// This distribution will only generate a relatively small subset of the values in the requested range - this subset is
/// the largest subset of uniformly distributed representable values in that range.
///
/// # Notes
/// - `uniform_int_distribution` satisfies `random_number_distribution`.
/// - As its `operator()` is `const`, creating `constexpr` variables of this type can make sense.
template<std::floating_point RealType = double>
class uniform_real_distribution
{
  public:
    using result_type = RealType;

    /// Constructs a distribution that generates numbers in [a, b]
    ///
    /// # Parameters
    /// - a
    ///     lowest potentially generated value
    /// - b
    ///     largest potentially generated value
    ///
    /// # Preconditions
    /// - Behavior is undefined if `a > b`.
    /// - Behavior is undefined if `b - a > std::numeric_limits<RealType>::max()`.
    ///
    /// # Notes
    /// If `a == b`, the distribution will always produce the same value.
    constexpr uniform_real_distribution(inclusive<RealType> a, inclusive<RealType> b) noexcept
        : m_a(a.value)
        , m_b(b.value)
        , m_g(detail::uniform_real_distribution::compute_gamma(m_a, m_b))
        , m_hi(m_g > 0 ? detail::uniform_real_distribution::ceilint(m_a, m_b, m_g) : 0)
        , m_min(m_a)
        , m_max(m_b)
        , m_int_dist(inclusive{std::size_t{0}}, inclusive{m_hi})
        , m_gn(std::abs(m_a) <= std::abs(m_b)
                   ? detail::uniform_real_distribution::gen_inclusive_inclusive_loe<RealType>
                   : detail::uniform_real_distribution::gen_inclusive_inclusive_nloe<RealType>)
    {
        assert(m_b - m_a <= std::numeric_limits<RealType>::max());
        assert(m_a <= m_b);
    }

    /// Constructs a distribution that generates numbers in (a, b]
    ///
    /// # Parameters
    /// - a
    ///     lower bound on the potentially generated values
    /// - b
    ///     largest potentially generated value
    ///
    /// # Preconditions
    /// - Behavior is undefined if `a >= b`.
    /// - Behavior is undefined if `b - a > std::numeric_limits<RealType>::max()`.
    ///
    /// # Notes
    /// Due to the way IEEE-754 floating point representation works, there may be values larger than `a` (but lower than
    /// `b`) that are never generated.
    constexpr uniform_real_distribution(exclusive<RealType> a, inclusive<RealType> b) noexcept
        : m_a(a.value)
        , m_b(b.value)
        , m_g(detail::uniform_real_distribution::compute_gamma(m_a, m_b))
        , m_hi(detail::uniform_real_distribution::ceilint(m_a, m_b, m_g))
        , m_min(std::abs(m_a) <= std::abs(m_b) ? m_b - (m_hi - 1) * m_g : m_a + m_g)
        , m_max(m_b)
        , m_int_dist(inclusive{std::size_t{0}}, inclusive{m_hi - 1})
        , m_gn(std::abs(m_a) <= std::abs(m_b)
                   ? detail::uniform_real_distribution::gen_exclusive_inclusive_loe<RealType>
                   : detail::uniform_real_distribution::gen_exclusive_inclusive_nloe<RealType>)
    {
        assert(m_b - m_a <= std::numeric_limits<RealType>::max());
        assert(m_a < m_b);
    }

    /// Constructs a distribution that generates numbers in [a, b)
    ///
    /// # Parameters
    /// - a
    ///     lowest potentially generated value
    /// - b
    ///     upper bound on the potentially generated values
    ///
    /// # Preconditions
    /// - Behavior is undefined if `a >= b`.
    /// - Behavior is undefined if `b - a > std::numeric_limits<RealType>::max()`.
    ///
    /// # Notes
    /// Due to the way IEEE-754 floating point representation works, there may be values less than `b` (but greater than
    /// `a`) that are never generated.
    constexpr uniform_real_distribution(inclusive<RealType> a, exclusive<RealType> b) noexcept
        : m_a(a.value)
        , m_b(b.value)
        , m_g(detail::uniform_real_distribution::compute_gamma(m_a, m_b))
        , m_hi(detail::uniform_real_distribution::ceilint(m_a, m_b, m_g))
        , m_min(m_a)
        , m_max(std::abs(m_a) <= std::abs(m_b) ? m_b - m_g : m_a + (m_hi - 1) * m_g)
        , m_int_dist(inclusive{std::size_t{1}}, inclusive{m_hi})
        , m_gn(std::abs(m_a) <= std::abs(m_b)
                   ? detail::uniform_real_distribution::gen_inclusive_exclusive_loe<RealType>
                   : detail::uniform_real_distribution::gen_inclusive_exclusive_nloe<RealType>)
    {
        assert(m_b - m_a <= std::numeric_limits<RealType>::max());
        assert(m_a < m_b);
    }

    /// Constructs a distribution that generates numbers in (a, b)
    ///
    /// # Parameters
    /// - a
    ///     lower bound on the potentially generated values
    /// - b
    ///     upper bound on the potentially generated values
    ///
    /// # Preconditions
    /// - Behavior is undefined if `a >= b`.
    /// - Behavior is undefined if `std::nexttoward(a, b) == b`.
    /// - Behavior is undefined if `b - a > std::numeric_limits<RealType>::max()`.
    ///
    /// # Notes
    /// Due to the way IEEE-754 floating point representation works, there may be values larger than `a` (but lower than
    /// `b`) that are never generated, and there may be values less than `b` (but greater than `a`) that are never
    /// generated.
    constexpr uniform_real_distribution(exclusive<RealType> a, exclusive<RealType> b) noexcept
        : m_a(a.value)
        , m_b(b.value)
        , m_g(detail::uniform_real_distribution::compute_gamma(m_a, m_b))
        , m_hi(detail::uniform_real_distribution::ceilint(m_a, m_b, m_g))
        , m_min(std::abs(m_a) <= std::abs(m_b) ? m_b - (m_hi - 1) * m_g : m_a + m_g)
        , m_max(std::abs(m_a) <= std::abs(m_b) ? m_b - m_g : m_a + (m_hi - 1) * m_g)
        , m_int_dist(inclusive{std::size_t{1}}, inclusive{m_hi - 1})
        , m_gn(std::abs(m_a) <= std::abs(m_b)
                   ? detail::uniform_real_distribution::gen_exclusive_exclusive_loe<RealType>
                   : detail::uniform_real_distribution::gen_exclusive_exclusive_nloe<RealType>)
    {
        assert(m_b - m_a <= std::numeric_limits<RealType>::max());
        assert(m_a < m_b);
//        assert(std::nexttoward(m_a, m_b) != m_b); // TODO: check why it doesn't compile
    }

    /// Generates random numbers in the desired range
    ///
    /// # Parameters
    /// - g
    ///     An object satisfying `uniform_random_bit_generator`
    ///
    /// # Return Value
    ///     The generated random number.
    ///
    /// # Complexity
    ///     Amortized constant number of invocations of `g()`.
    template<uniform_random_bit_generator G>
    constexpr auto operator()(G& g) const -> result_type
    {
        auto const k = m_int_dist(g);
        return m_gn(m_a, m_b, m_g, m_hi, k);
    }

    /// Returns the `a` parameter the distribution was constructed with.
    [[nodiscard]] constexpr auto a() const noexcept -> result_type { return m_a; }
    /// Returns the `b` parameter the distribution was constructed with.
    [[nodiscard]] constexpr auto b() const noexcept -> result_type { return m_b; }
    /// Returns the minimum potentially generated value.
    [[nodiscard]] constexpr auto min() const noexcept -> result_type { return m_min; }
    /// Returns the maximum potentially generated value.
    [[nodiscard]] constexpr auto max() const noexcept -> result_type { return m_max; }
    /// Returns the smallest difference two generated values may have.
    [[nodiscard]] constexpr auto gamma() const noexcept -> result_type { return m_g; }
    /// Returns the amount of values that can be generated.
    [[nodiscard]] constexpr auto num_unique_values() const noexcept -> std::size_t
    {
        return m_int_dist.max() - m_int_dist.min();
    }

    /// Compares two distribution objects by their internal state.
    ///
    /// # Notes
    /// Not visible to ordinary unqualified or qualified lookup, can only found via ADL.
    friend constexpr auto operator==(uniform_real_distribution const& lhs,
                                     uniform_real_distribution const& rhs) noexcept -> bool = default;

  private:
    RealType                              m_a;
    RealType                              m_b;
    RealType                              m_g;
    std::size_t                           m_hi;
    RealType                              m_min;
    RealType                              m_max;
    uniform_int_distribution<std::size_t> m_int_dist;
    RealType (*m_gn)(RealType a, RealType b, RealType g, RealType hi, std::size_t k);
};
} // namespace crand

#endif // CONSTEXPR_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP
