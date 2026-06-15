# crand

C++23 constexpr pseudo-random number generation.

## Engines

- splitmix64
- xorshift32, xorshift64
- xoshiro256**

## Distributions

- bernoulli
- uniform (int / real)
- normal

## Why Is This C++23?

Because this needs `constexpr` math. And `<cmath>` only became (partially)
`constexpr` in C++23. Everything else is C++20.