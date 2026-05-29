#ifndef SOKOBAN_ZOBRIST_H_
#define SOKOBAN_ZOBRIST_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace sokoban {

constexpr uint64_t SPLIT64_S1 = 30;
constexpr uint64_t SPLIT64_S2 = 27;
constexpr uint64_t SPLIT64_S3 = 31;
constexpr uint64_t SPLIT64_C1 = 0x9E3779B97F4A7C15ULL;
constexpr uint64_t SPLIT64_C2 = 0xBF58476D1CE4E5B9ULL;
constexpr uint64_t SPLIT64_C3 = 0x94D049BB133111EBULL;

constexpr auto splitmix64(uint64_t seed) noexcept -> uint64_t {
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}

template <std::size_t Words>
struct ZobristKey {
    static_assert(Words >= 1, "ZobristKey must have at least one 64-bit word.");

    std::array<uint64_t, Words> word{};

    constexpr auto operator^=(const ZobristKey& other) noexcept -> ZobristKey& {
        for (std::size_t i = 0; i < Words; ++i) {
            word[i] ^= other.word[i];
        }
        return *this;
    }

    friend constexpr auto operator^(ZobristKey lhs, const ZobristKey& rhs) noexcept -> ZobristKey {
        lhs ^= rhs;
        return lhs;
    }

    friend constexpr auto operator==(const ZobristKey& lhs, const ZobristKey& rhs) noexcept -> bool {
        return lhs.word == rhs.word;
    }

    [[nodiscard]] constexpr auto low64() const noexcept -> uint64_t {
        return word[0];
    }
};

using Zobrist64 = ZobristKey<1>;
using Zobrist128 = ZobristKey<2>;
using Zobrist256 = ZobristKey<4>;

template <std::size_t Words, typename Element>
constexpr auto to_local_hash(int flat_size, Element el, int offset) noexcept -> ZobristKey<Words> {
    ZobristKey<Words> out{};
    const auto element_id = static_cast<uint64_t>(static_cast<int>(el));
    const uint64_t base = static_cast<uint64_t>(flat_size) * element_id + static_cast<uint64_t>(offset);
    for (std::size_t lane = 0; lane < Words; ++lane) {
        const uint64_t lane_seed = base * static_cast<uint64_t>(Words) + static_cast<uint64_t>(lane);
        out.word[lane] = splitmix64(lane_seed);
    }
    return out;
}

}    // namespace sokoban

namespace std {
template <std::size_t Words>
struct hash<sokoban::ZobristKey<Words>> {
    auto operator()(const sokoban::ZobristKey<Words>& key) const noexcept -> std::size_t {
        uint64_t h = 0;
        for (std::size_t i = 0; i < Words; ++i) {
            h ^= sokoban::splitmix64(key.word[i] + sokoban::SPLIT64_C1 * static_cast<uint64_t>(i + 1));
        }
        return static_cast<std::size_t>(h);
    }
};
}    // namespace std

#endif    // SOKOBAN_ZOBRIST_H_
