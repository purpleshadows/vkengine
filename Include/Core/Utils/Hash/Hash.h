// core/hash.hpp
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <functional>

namespace Core::Hash {

// 64-bit FNV-1a (deterministic across platforms)
constexpr uint64_t fnv1a(const void* data, size_t len) noexcept {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

inline uint64_t fnv1a(const std::string& s) noexcept {
    return fnv1a(s.data(), s.size());
}

// Boost-style hash combine on size_t
constexpr inline void combine(std::size_t& seed, std::size_t value) noexcept {
    seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
}

constexpr inline uint64_t combine64(uint64_t seed, uint64_t value) noexcept {
    seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
    return seed;
}

// Range helper (e.g., for vectors of strings/ints)
template <class It>
inline std::size_t combine_range(It first, It last) noexcept {
    std::size_t seed = 0;
    for (; first != last; ++first) {
        combine(seed, std::hash<std::decay_t<decltype(*first)>>{}(*first));
    }
    return seed;
}

// Tuple helper
template <class Tuple, size_t... I>
inline std::size_t tuple_impl(const Tuple& t, std::index_sequence<I...>) noexcept {
    std::size_t seed = 0;
    (combine(seed, std::hash<std::tuple_element_t<I, Tuple>>{}(std::get<I>(t))), ...);
    return seed;
}

template <class... Ts>
inline std::size_t tuple(const std::tuple<Ts...>& t) noexcept {
    return tuple_impl(t, std::index_sequence_for<Ts...>{});
}

} // namespace core::hash
