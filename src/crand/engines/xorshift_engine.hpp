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

#ifndef CONSTEXPR_RANDOM_XORSHIFT_ENGINE_HPP
#define CONSTEXPR_RANDOM_XORSHIFT_ENGINE_HPP

#include "detail/xorshift_engine_details.hpp"

#include <concepts>

#include <cstdint>

namespace crand
{
/// Random number engine based on the xorshift algorithm.
///
/// # Notes
/// The `xorshift32` and `xorshift64` typedefs define the engine with common parameter sets.
template<std::unsigned_integral T, std::uint8_t a, std::uint8_t b, std::uint8_t c>
class xorshift_engine
{
  public:
    using result_type = T;

    static constexpr result_type default_seed = 1;

    /// Constructs the engine with a default seed
    constexpr xorshift_engine() noexcept
        : xorshift_engine(default_seed)
    {
    }

    /// Constructs the engine
    ///
    /// # Parameters
    /// - seed
    ///     Value used to seed the engine
    constexpr explicit xorshift_engine(result_type value) noexcept
        : m_state(detail::xorshift_engine::seed(value))
    {
    }

    /// Re-seeds the engine
    constexpr void seed(result_type value = default_seed) noexcept { m_state = detail::xorshift_engine::seed(value); }

    /// Generates a pseudo-random value. The engine state is advanced by one (the next call to this
    /// function will return the next number in the sequence).
    ///
    /// # Return Value
    /// A pseudo-random number in [`min`, `max`].
    ///
    /// # Complexity
    /// Constant.
    constexpr auto operator()() noexcept -> result_type
    {
        m_state ^= m_state << a;
        m_state ^= m_state >> b;
        m_state ^= m_state << c;
        return m_state;
    }

    /// Advances the state by z.
    ///
    /// # Parameters
    /// - z
    ///     The number of times to advance the internal state
    ///
    /// # Complexity
    /// Linear in `z`.
    ///
    /// # Notes
    /// Functionally equivalent to calling `operator()` `z` times.
    constexpr void discard(unsigned long long z) noexcept
    {
        for (unsigned long long i = 0; i < z; ++i)
            operator()();
    }

    /// Returns the minimum potentially generated value.
    static constexpr auto min() noexcept -> result_type { return 0; }
    /// Returns the maximum potentially generated value.
    static constexpr auto max() noexcept -> result_type { return -1; }

    /// Compares two engine objects by their internal state.
    ///
    /// # Notes
    /// Not visible to ordinary unqualified or qualified lookup, can only found via ADL.
    friend constexpr auto operator==(xorshift_engine const& lhs, xorshift_engine const& rhs) -> bool = default;

  private:
    T m_state;
};

/// Defines the xorshift32 engine with the recommended parameter set from
/// "Xorshift RNGs" by Marsaglia, 2003.
using xorshift32 = xorshift_engine<std::uint32_t, 13, 17, 5>;
/// Defines the xorshift64 engine with the recommended parameter set from
/// "Xorshift RNGs" by Marsaglia, 2003.
using xorshift64 = xorshift_engine<std::uint64_t, 13, 7, 17>;
} // namespace crand

#endif // CONSTEXPR_RANDOM_XORSHIFT_ENGINE_HPP
