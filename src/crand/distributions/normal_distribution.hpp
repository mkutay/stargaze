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

#ifndef CONSTEXPR_RANDOM_NORMAL_DISTRIBUTION_HPP
#define CONSTEXPR_RANDOM_NORMAL_DISTRIBUTION_HPP

#include "crand/concepts/uniform_random_bit_generator.hpp"
#include "crand/distributions/uniform_real_distribution.hpp"

#include <concepts>
#include <limits>
#include <optional>

#include <cmath>

namespace crand
{
/// Produces normal-distributed random numbers.
///
/// # Notes
/// - `normal_distribution` satisfies `random_number_distribution`.
template<std::floating_point RealType = double>
class normal_distribution
{
  public:
    using result_type = RealType;

    /// Constructs a normal distribution with mean 0 and standard deviation 1
    constexpr normal_distribution() noexcept
        : normal_distribution(0.0)
    {
    }
    /// Constructs a normal distribution
    ///
    /// # Parameters
    /// - `mean`
    ///     The mean value of the normal distribution
    /// - `stddev`
    ///     The standard deviation of the normal distribution
    constexpr explicit normal_distribution(RealType mean, RealType stddev = 1.0) noexcept
        : m_mean(mean)
        , m_stddev(stddev)
    {
    }

    /// Generates random numbers according to `mean` and `stddev`
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
    constexpr auto operator()(G& g) -> result_type
    {
        // Marsaglia polar method
        if (m_cache)
        {
            result_type const r = m_cache.value();
            m_cache.reset();
            return r;
        }

        result_type u, v, s;
        do
        {
            u = s_dist(g);
            v = s_dist(g);
            s = u * u + v * v;
        } while (s == 0 || s >= 1);
        s       = std::sqrt(-2 * std::log(s) / s);
        m_cache = v * s * m_stddev + m_mean;
        return u * s * m_stddev + m_mean;
    }

    /// Returns the `mean` parameter the distribution was constructed with.
    constexpr auto mean() const noexcept -> result_type { return m_mean; }
    /// Returns the `stddev` parameter the distribution was constructed with.
    constexpr auto stddev() const noexcept -> result_type { return m_stddev; }

    /// Returns the minimum potentially generated value
    constexpr auto min() const noexcept -> result_type { return std::numeric_limits<result_type>::lowest(); }
    /// Returns the maximum potentially generated value
    constexpr auto max() const noexcept -> result_type { return std::numeric_limits<result_type>::max(); }

    /// Compares two distribution objects by their internal state.
    ///
    /// # Notes
    /// Not visible to ordinary unqualified or qualified lookup, can only found via ADL.
    friend constexpr auto operator==(normal_distribution const& lhs, normal_distribution const& rhs) noexcept
        -> bool = default;

  private:
    static constexpr uniform_real_distribution<result_type> s_dist{exclusive<result_type>{-1.},
                                                                   exclusive<result_type>{1.}};

    result_type                m_mean;
    result_type                m_stddev;
    std::optional<result_type> m_cache;
};
} // namespace crand

#endif // CONSTEXPR_RANDOM_NORMAL_DISTRIBUTION_HPP
