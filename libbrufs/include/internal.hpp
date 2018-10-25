/*
 * Copyright (c) 2017-2018 Luc Everse <luc@cmpsb.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "types.hpp"
#include "Seed.hpp"

#define UNUSED __attribute__((unused))

template<typename N>
static inline N updiv(N divident, N divisor) {
    return (divident + divisor - 1) / divisor;
}

namespace Brufs {

static const Hash CHECKSUM_SEED = HASH_SEED;

static const Size BLOCK_SIZE = 512;

static const Address NULL_BLOCK = 0;

static constexpr int MAX_COLLISIONS = 32;

static constexpr inline bool is_valid_size(Size size) {
    return (size & (BLOCK_SIZE - 1)) == 0;
}

template <typename T>
static constexpr inline bool is_power_of_two(T v) {
    // https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
    return v && !(v & (v - 1));
}

template <typename T>
static constexpr inline T previous_power_of_two(T x) {
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    return x - (x >> 1);
}

template <typename T>
static constexpr inline T next_power_of_two(T x) {
    --x;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    ++x;
    return x;
}

template <typename T>
static constexpr inline T previous_multiple_of(T multiple, T base) {
    return ((multiple - base + 1) / base) * base;
}

template <typename T>
static constexpr inline T next_multiple_of(T multiple, T base) {
    return ((multiple + base - 1) / base) * base;
}

template<typename T>
static inline constexpr T min(T etaoin, T shrdlu) {
    return (etaoin < shrdlu) ? etaoin : shrdlu;
}

template<typename T>
static inline constexpr T max(T etaoin, T shrdlu) {
    return (etaoin < shrdlu) ? shrdlu : etaoin;
}

}
