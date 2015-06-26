#ifndef SDR_H_
#define SDR_H_

#include <vector>
#include <array>
#include <bitset>
#include <iostream>
#include <utility>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <unordered_set>


#ifdef USE_SHERWOOD
#include "sherwood_map.hpp"
#endif

namespace sdr
{

typedef std::size_t position_t;
typedef std::size_t width_t;

#ifdef USE_SHERWOOD
template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>,typename A = std::allocator<std::pair<K, V> > >
using thin_sherwood_map = sherwood_map<K, V, H, E, A>;

template<typename K, typename V>
using hashtable_t = thin_sherwood_map<K, V>;
#else
template<typename K, typename V>
using hashtable_t = std::unordered_map<K, V>;
#endif



template <width_t Width>
std::vector<position_t> field_to_list(const std::bitset<Width> & data)
{
    std::vector<position_t> positions;

    for(std::size_t i=0; i<Width; ++i) {
        if(data[i]) {
            positions.push_back(i);
        }
    }

    return positions;
}


template <width_t Width>
struct node
{
    // store the positions of all set bits from 0 -> width
    std::unordered_set<position_t> positions;

    node(const std::vector<position_t> & data) : positions(data.begin(), data.end())
    {}

    void fill(const std::vector<position_t> & data)
    {
        std::copy(data.begin(), data.end(), std::inserter(positions, positions.begin()));
    }

    void clear()
    {
        positions.clear();
    }

    void print() const
    {
        const std::size_t sroot = std::sqrt(Width);

        for(std::size_t y=0; y < sroot; ++y) {
            for(std::size_t x=0; x < sroot; ++x) {
                std::cout << (positions.count((y * sroot) + x) ? '1' : '0');
            }

            std::cout << std::endl;
        }
    }
};

template <width_t Width>
struct bank
{
    // all inputs we have ever received, we store here compressed into storage
    std::vector<node<Width>> storage;

    // this holds our sets of vectors for easy comparison of different objects in storage
    std::array<std::unordered_set<position_t>, Width> bitmap;

    bank() : storage(), bitmap()
    {}

    void print(const position_t pos) const
    {
        storage[pos].print();
    }

    position_t insert(const std::vector<position_t> & data)
    {
        storage.push_back(node<Width>(data));

        const position_t last_pos = storage.size() - 1;

        for(position_t pos : data) {
            bitmap[pos].insert(last_pos);
        }

        return last_pos;
    }

    void update(const position_t pos, const std::vector<position_t> & data)
    {
        node<Width> & node = storage[pos];

        for(position_t i : node.positions) {
            bitmap[i].erase(pos);
        }

        node.clear();

        node.fill(data);

        for(position_t p : node.positions) {
            bitmap[p].insert(pos);
        }
    }

    // find amount of matching bits between two vectors
    std::size_t similarity(const position_t a, const position_t b) const
    {
        std::size_t result = 0;

        for(position_t pos : storage[a].positions) {
            result += bitmap[pos].count(b);
        }

        return result;
    }

    // find similarity of one object compared to the OR'd result of a list of objects
    std::size_t union_similarity(const position_t pos, const std::vector<position_t> & positions) const
    {
        std::bitset<Width> punions;

        for(const position_t ppos : positions) {
            for(const position_t spos : storage[ppos].positions) {
                punions.set(spos);
            }
        }

        std::size_t result = 0;

        for(const position_t cmp : storage[pos].positions) {
            result += punions[cmp];
        }

        return result;
    }

    // find most similar to object at pos
    // first refers to position
    // second refers to matching number of bits
    std::vector<std::pair<position_t, std::size_t>> closest(const position_t pos, const std::size_t amount) const
    {
        hashtable_t<position_t, std::size_t> matching_m;

        for(const position_t spos : storage[pos].positions) {
            for(const position_t bpos : bitmap[spos]) {
                ++matching_m[bpos];
            }
        }

        // we dont care about ourselves
        matching_m.erase(pos);

        std::vector<std::pair<position_t, std::size_t>> matching_v(matching_m.begin(), matching_m.end());
        std::cout << "matching_v.size() => " << matching_v.size() << std::endl;

        const auto partial_end = matching_v.begin() + ((amount >= matching_v.size()) ? matching_v.size() : amount);

        std::partial_sort(matching_v.begin(), partial_end, matching_v.end(), [](
            const std::pair<position_t, std::size_t> a,
            const std::pair<position_t, std::size_t> b
        ) {
            return (a.second) > (b.second);
        });

        return std::vector<std::pair<position_t, std::size_t>>(matching_v.begin(), partial_end);
    }

    // return all items matching all in data
    std::vector<position_t> matching(const std::vector<position_t> & data) const
    {
        std::unordered_set<position_t> matching;

        for(const std::size_t item : data) {
            for(const std::size_t pos : bitmap[item]) {
                for(const std::size_t m : data) {
                    if(bitmap[m].count(pos) == 0) {
                        goto skip;
                    }
                }

                matching.insert(pos);
                skip:;
            }
        }

        return std::vector<position_t>(matching.begin(), matching.end());
    }

    // has to match amount in data
    std::vector<position_t> matching(const std::vector<position_t> & data, const std::size_t amount) const
    {
        std::unordered_set<position_t> matching;

        for(const std::size_t item : data) {
            for(const std::size_t pos : bitmap[item]) {
                std::size_t amount_matching = 0;

                for(const std::size_t m : data) {
                    amount_matching += bitmap[m].count(pos);
                }

                if(amount_matching >= amount) {
                    matching.insert(pos);
                }
            }
        }

        return std::vector<position_t>(matching.begin(), matching.end());
    }
};

} //namespace sdr

#endif
