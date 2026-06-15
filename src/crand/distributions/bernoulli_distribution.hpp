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

#ifndef CONSTEXPR_RANDOM_BERNOULLI_DISTRIBUTION_HPP
#define CONSTEXPR_RANDOM_BERNOULLI_DISTRIBUTION_HPP

#include "crand/concepts/uniform_random_bit_generator.hpp"
#include "crand/distributions/uniform_real_distribution.hpp"

namespace crand
{
/// Produces bernoulli-distributed random boolean values.
///
/// The probability of `true` being returned is `p`. Consequently, the probability of `false` being returned is `1-p`.
///
/// # Notes
/// - `bernoulli_distribution` satisfies `random_number_distribution`.
/// - As its `operator()` is `const`, creating `constexpr` variables of this type can make sense.
class bernoulli_distribution
{
  public:
    using result_type = bool;

    /// Constructs a distribution that returns `true` 50% of the time, otherwise false.
    constexpr bernoulli_distribution() noexcept
        : bernoulli_distribution(0.5)
    {
    }

    /// Constructs a distribution that returns `true` according to `p`.
    ///
    /// # Parameters
    /// - p
    ///     A floating point value in the range [`0`, `1`], where `0` means this distribution never returns true, and
    ///     `1` means it never returns false
    ///
    /// # Preconditions
    /// Behavior is undefined if `0 <= p <= 1` doesn't hold true
    constexpr explicit bernoulli_distribution(double p) noexcept
        : m_p(p)
    {
        assert(p >= 0 && p <= 1);
    }

    /// Generates random booleans according to `p`
    ///
    /// # Parameters
    /// - g
    ///     An object satisfying `uniform_random_bit_generator`
    ///
    /// # Return Value
    ///     The generated random boolean.
    ///
    /// # Complexity
    ///     Amortized constant number of invocations of `g()`.
    template<uniform_random_bit_generator G>
    constexpr auto operator()(G& g) const -> result_type
    {
        return s_dist(g) < m_p;
    }

    /// Returns the `p` parameter the distribution was constructed with.
    [[nodiscard]] constexpr auto p() const noexcept -> double { return m_p; }

    /// Returns `false`
    [[nodiscard]] constexpr auto min() const noexcept -> bool { return false; }
    /// Returns `true`
    [[nodiscard]] constexpr auto max() const noexcept -> bool { return true; }

    /// Compares two distribution objects by their internal state.
    ///
    /// # Notes
    /// Not visible to ordinary unqualified or qualified lookup, can only found via ADL.
    friend constexpr auto operator==(bernoulli_distribution const& lhs, bernoulli_distribution const& rhs)
        -> bool = default;

  private:
    constexpr static uniform_real_distribution<double> s_dist{inclusive{0.}, exclusive{1.}};

    double m_p;
};
} // namespace crand

#endif // CONSTEXPR_RANDOM_BERNOULLI_DISTRIBUTION_HPP
