#ifndef SDR_CONSTANTS_H_
#define SDR_CONSTANTS_H_

#include <sparsehash/dense_hash_set>
#include <unordered_set>
#include <utility>

namespace sdr
{

typedef std::size_t position_t;
typedef std::size_t width_t;

constexpr std::uint32_t F_PREFIX  { 0x5D };
constexpr std::uint32_t F_VERSION { 0x01 };

template <typename T>
#ifdef USE_STL_HASH
using hash_set = std::unordered_set<T>;
#else
using hash_set = google::dense_hash_set<T, std::hash<T>>;
#endif

void hash_set_init(hash_set<std::uint32_t> & hset)
{
#ifndef USE_STL_HASH
    hset.set_empty_key(0xFFFFFFFF);
#endif
}

void hash_set_init(hash_set<std::uint64_t> & hset)
{
#ifndef USE_STL_HASH
    hset.set_empty_key(0xFFFFFFFFFFFFFFFF);
#endif
}


}

#endif