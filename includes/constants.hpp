#ifndef SDR_CONSTANTS_H_
#define SDR_CONSTANTS_H_

#include <sparsehash/dense_hash_set>
#include <utility>
#include <limits>

namespace sdr
{

typedef std::size_t position_t;
typedef std::size_t width_t;

constexpr std::uint32_t F_PREFIX  { 0x5D };
constexpr std::uint32_t F_VERSION { 0x01 };

constexpr width_t UNION_SIMILARITY_SWITCH_WIDTH { 65536 };

template <typename T>
using hash_set = google::dense_hash_set<T, std::hash<T>>;

template <typename T>
void hash_set_init(hash_set<T> & hset)
{
    hset.set_empty_key(std::numeric_limits<T>::max());
}


}

#endif