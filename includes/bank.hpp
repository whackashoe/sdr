#ifndef SDR_BANK_H_
#define SDR_BANK_H_

#include "constants.hpp"
#include "storage_concept.hpp"
#include "concept.hpp"


#include <vector>
#include <array>
#include <bitset>
#include <iostream>
#include <utility>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <future>
#include <thread>
#include <fstream>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <cassert>

namespace sdr
{


// this is the memory bank for the sdr memory unit
template <sdr::width_t Width>
class bank
{
private:
    // this holds our sets of vectors for easy comparison of different objects in storage
    std::vector<sdr::hash_set<sdr::position_t>> bitmap;

    // all inputs we have ever received, we store here compressed into storage
    std::vector<sdr::storage_concept> storage;



    // helpers are called from the public api
    // these just reduce code bloat

    template <typename Collection> std::size_t similarity_helper(
        const Collection & positions,
        const sdr::position_t d
    ) const {
#ifndef NDEBUG
        for(auto & i : positions) {
            assert(i < Width);
        }
        assert(d < Width);
#endif
        std::size_t result { 0 };

        for(sdr::position_t pos : positions) {
            result += bitmap[pos].count(d);
        }

        return result;
    }

    template <typename PCollection, typename WCollection>
    std::size_t weighted_similarity_helper(
        const PCollection & positions,
        const sdr::position_t d,
        const WCollection & weights
    ) const {
#ifndef NDEBUG
        for(auto & i : positions) {
            assert(i < Width);
        }
        assert(d < Width);
        assert(weights.size() == Width);
#endif
        double result { 0.0f };

        for(sdr::position_t pos : positions) {
            result += bitmap[pos].count(d) * weights[pos];
        }

        return result;
    }

    template <typename PCollection>
    std::size_t union_similarity_helper(
        const PCollection & collection,
        const std::vector<sdr::position_t> & positions
    ) const {
#ifndef NDEBUG
        for(auto & i : positions) {
            assert(i < Width);
        }
#endif
        std::size_t result { 0 };

        if(Width < sdr::UNION_SIMILARITY_SWITCH_WIDTH) {
            // this algorithm is faster for smaller Width, but much slower for large Width
            std::bitset<Width> punions;

            for(const sdr::position_t ppos : positions) {
                for(const sdr::position_t spos : storage[ppos].positions) {
                    punions.set(spos);
                }
            }

            for(const sdr::position_t cmp : collection) {
                result += punions[cmp];
            }

        } else {
            sdr::hash_set<sdr::position_t> punions;
            sdr::hash_set_init(punions);

            for(const sdr::position_t ppos : positions) {
                for(const sdr::position_t spos : storage[ppos].positions) {
                    punions.insert(spos);
                }
            }

            auto vit = punions.begin();
            for(auto cit = collection.begin(); vit != punions.end() && cit != collection.end(); ++cit) {
                sdr::position_t deref { (*vit) };
                const sdr::position_t cmp { (*cit) };

                while(deref < cmp && vit != punions.end()) {
                    ++vit;
                    deref = (*vit);
                }

                if(deref == cmp) {
                    ++result;
                    ++vit;
                }
            }
        }

        return result;
    }

    template <typename PCollection, typename WCollection>
    double weighted_union_similarity_helper(
        const PCollection & collection,
        const std::vector<sdr::position_t> & positions,
        const WCollection & weights
    ) const {
#ifndef NDEBUG
        for(auto & i : positions) {
            assert(i < Width);
        }
        assert(weights.size() == Width);
#endif
        double result { 0.0f };

        if(Width < sdr::UNION_SIMILARITY_SWITCH_WIDTH) {
            std::bitset<Width> punions;

            for(const sdr::position_t ppos : positions) {
                for(const sdr::position_t spos : storage[ppos].positions) {
                    punions.set(spos);
                }
            }

            for(const sdr::position_t cmp : collection) {
                result += punions[cmp] * weights[cmp];
            }
        } else {
            sdr::hash_set<sdr::position_t> punions;
            sdr::hash_set_init(punions);

            for(const sdr::position_t ppos : positions) {
                for(const sdr::position_t spos : storage[ppos].positions) {
                    punions.insert(spos);
                }
            }

            auto vit = punions.begin();
            for(auto cit = collection.begin(); vit != punions.end() && cit != collection.end(); ++cit) {
                sdr::position_t deref { (*vit) };
                const sdr::position_t cmp { (*cit) };

                while(deref < cmp && vit != punions.end()) {
                    ++vit;
                    deref = (*vit);
                }

                if(deref == cmp) {
                    result += weights[cmp];
                    ++vit;
                }
            }
        }

        return result;
    }

    template <typename PCollection>
    std::vector<std::pair<sdr::position_t, std::size_t>> closest_helper(
        const PCollection & collection,
        const std::size_t amount
    ) const {
        std::vector<sdr::position_t> idx(storage.size());
        std::vector<unsigned>          v(storage.size());

        // if there are less than amount in storage, just return amount that exist
        const std::size_t partial_amount { (amount + 1 >= idx.size()) ? idx.size() : amount + 1 };

        std::iota(std::begin(idx), std::end(idx), 0);

        // count matching bits for each
        for(const sdr::position_t spos : collection) {
            for(const sdr::position_t bpos : bitmap[spos]) {
                ++v[bpos];
            }
        }

        std::partial_sort(std::begin(idx), std::begin(idx) + partial_amount, std::end(idx), [&](
            const sdr::position_t a,
            const sdr::position_t b
        ) {
            return v[a] > v[b];
        });

        // create std::pair for result
        std::vector<std::pair<sdr::position_t, std::size_t>> ret;
        ret.reserve(partial_amount);

        for(std::size_t i=1; i<partial_amount; ++i) {
            const sdr::position_t m { idx[i] };
            ret.emplace_back(std::make_pair(m, static_cast<std::size_t>(v[m])));
        }

        return ret;
    }

    template <typename PCollection, typename WCollection>
    std::vector<std::pair<sdr::position_t, double>> weighted_closest_helper(
        const PCollection collection,
        const std::size_t amount,
        const WCollection & weights
    ) const {
        std::vector<sdr::position_t> idx(storage.size());
        std::vector<double>            v(storage.size());

        // if there are less than amount in storage, just return amount that exist
        const std::size_t partial_amount { (amount + 1 >= idx.size()) ? idx.size() : amount + 1 };

        std::iota(std::begin(idx), std::end(idx), 0);

        // count matching bits for each
        for(const sdr::position_t spos : collection) {
            for(const sdr::position_t bpos : bitmap[spos]) {
                v[bpos] += weights[spos];
            }
        }

        std::partial_sort(std::begin(idx), std::begin(idx) + partial_amount, std::end(idx), [&](
            const sdr::position_t a,
            const sdr::position_t b
        ) {
            return v[a] > v[b];
        });

        // create std::pair for result
        std::vector<std::pair<sdr::position_t, double>> ret;
        ret.reserve(partial_amount);

        for(std::size_t i=1; i<partial_amount; ++i) {
            const sdr::position_t m { idx[i] };
            ret.emplace_back(std::make_pair(m, v[m]));
        }

        return ret;
    }

    std::vector<std::pair<sdr::position_t, std::size_t>> async_closest_helper_pos(
        const sdr::position_t pos,
        const std::size_t amount
    ) const {
        return closest_helper(storage[pos].positions, amount);
    }

    std::vector<std::pair<sdr::position_t, std::size_t>> async_closest_helper_concept(
        const sdr::concept & concept,
        const std::size_t amount
    ) const {
        return closest(concept.data, amount);
    }

    template <typename WCollection>
    std::vector<std::pair<sdr::position_t, double>> async_weighted_closest_helper_pos(
        const sdr::position_t pos,
        const std::size_t amount,
        const WCollection & weights
    ) const {
        return weighted_closest(pos, amount, weights);
    }

    template <typename WCollection>
    std::vector<std::pair<sdr::position_t, double>> async_weighted_closest_helper_concept(
        const sdr::concept & concept,
        const std::size_t amount,
        const WCollection & weights
    ) const {
        return weighted_closest(concept, amount, weights);
    }

public:
    bank() : bitmap(Width), storage()
    {
        for(auto & i : bitmap) {
            sdr::hash_set_init(i);
        }
    }

    // recomend you use .sdr extension
    bool save_to_file(const std::string & src) const
    {
        std::ofstream ofs(src, std::ios::out | std::ios::binary);

        if(! ofs) {
            std::cerr << "file unable to be opened for writing: " << src << std::endl;
            return false;
        }

        auto write = [&](std::uint32_t ref) {
            ofs.write(reinterpret_cast<char*>(&ref), sizeof(std::uint32_t));
        };

        // prefix
        write(sdr::F_PREFIX);

        // version
        write(sdr::F_VERSION);

        // width
        write(static_cast<std::uint32_t>(Width));

        //storage
        write(static_cast<std::uint32_t>(storage.size()));
        for(auto & item : storage) {
            write(static_cast<std::uint32_t>(item.positions.size()));

            for(auto & i : item.positions) {
                write(static_cast<std::uint32_t>(i));
            }
        }


        ofs.close();

        return true;
    }

    std::size_t load_from_file(const std::string & src)
    {
        std::ifstream ifs(src, std::ios::binary);
        if(! ifs) {
            std::cerr << "file not found: " << src << std::endl;
            return false;
        }

        auto read = [&]() -> std::uint32_t {
            std::uint32_t v;
            ifs.read(reinterpret_cast<char*>(&v), sizeof(std::uint32_t));
            return v;
        };

        // prefix
        const std::uint32_t prefix { read() };

        if(prefix != sdr::F_PREFIX) {
            std::cerr << "this is not a sdr file" << std::endl;
            return false;
        }


        // version
        const std::uint32_t version { read() };

        if(version != sdr::F_VERSION) {
            std::cerr << "wrong version. found:" << version << " expected: " << sdr::F_VERSION << std::endl;
            return false;
        }


        // width
        const std::uint32_t width { read() };

        if(width != Width) {
            std::cout << "wrong width. found: " << width  << " expected: " << Width << std::endl;
            return false;
        }

        // storage
        const std::size_t storage_size { read() };

        for(std::size_t i=0; i < storage_size; ++i) {
            const std::size_t size { static_cast<std::size_t>(read()) };


            std::vector<sdr::position_t> positions;

            for(std::size_t j=0; j < size; ++j) {
                positions.emplace_back(static_cast<sdr::position_t>(read()));
            }

            insert(sdr::concept(positions));
        }

        return storage_size;
    }

    sdr::position_t insert(const sdr::concept & concept)
    {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
#endif
        storage.emplace_back(sdr::storage_concept(concept));

        const sdr::position_t last_pos { storage.size() - 1 };

        for(sdr::position_t pos : concept.data) {
            bitmap[pos].insert(last_pos);
        }

        return last_pos;
    }

    void update(
        const sdr::position_t pos,
        const sdr::concept & concept
    ) {
#ifndef NDEBUG
        assert(pos < storage.size());
        for(auto & i : concept.data) {
            assert(i < Width);
        }
#endif
        sdr::storage_concept & storage_concept { storage[pos] };

        for(sdr::position_t i : storage_concept.positions) {
            bitmap[i].erase(pos);
        }

        storage_concept.clear();

        storage_concept.fill(concept);

        for(sdr::position_t p : storage_concept.positions) {
            bitmap[p].insert(pos);
        }
    }

    void clear()
    {
        storage.clear();

        for(auto & i : bitmap) {
            i.clear();
        }
    }

    std::size_t storage_size() const
    {
        return storage.size();
    }

    // find amount of matching bits between two vectors
    std::size_t similarity(
        const sdr::position_t a,
        const sdr::position_t b
    ) const {
        return similarity_helper(storage[a].positions, b);
    }

    std::size_t similarity(
        const sdr::concept & concept,
        const sdr::position_t b
    ) const {
        return similarity_helper(concept.data, b);
    }

    // find amount of matching bits between two vectors
    template <typename WCollection>
    double weighted_similarity(
        const sdr::position_t a,
        const sdr::position_t b,
        const WCollection & weights
    ) const {
        return weighted_similarity_helper(storage[a].positions, b, weights);
    }

    template <typename WCollection>
    double weighted_similarity(
        const sdr::concept & concept,
        const sdr::position_t b,
        const WCollection & weights
    ) const {
        return weighted_similarity_helper(concept.data, b, weights);
    }

    // find similarity of one object compared to the OR'd result of a list of objects
    std::size_t union_similarity(
        const sdr::position_t pos,
        const std::vector<sdr::position_t> & positions
    ) const {
        return union_similarity_helper(storage[pos].positions, positions);
    }

    std::size_t union_similarity(
        const sdr::concept & concept,
        const std::vector<sdr::position_t> & positions
    ) const {
        return union_similarity_helper(concept.data, positions);
    }

    template <typename WCollection>
    double weighted_union_similarity(
        const sdr::position_t pos,
        const std::vector<sdr::position_t> & positions,
        const WCollection & weights
    ) const {
        return weighted_union_similarity_helper(storage[pos].positions, positions, weights);
    }

    template <typename WCollection>
    double weighted_union_similarity(
        const sdr::concept & concept,
        const std::vector<sdr::position_t> & positions,
        const WCollection & weights
    ) const {
        return weighted_union_similarity_helper(concept.data, positions, weights);
    }

    // find most similar to object at pos
    // first refers to position
    // second refers to matching number of bits
    std::vector<std::pair<sdr::position_t, std::size_t>> closest(
        const sdr::position_t pos,
        const std::size_t amount
    ) const {
#ifndef NDEBUG
        assert(pos < storage.size());
#endif
        return closest_helper(storage[pos].positions, amount);
    }

    std::vector<std::pair<sdr::position_t, std::size_t>> closest(
        const sdr::concept & concept,
        const std::size_t amount
    ) const {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
#endif
        return closest_helper(concept.data, amount);
    }

    std::future<std::vector<std::pair<sdr::position_t, std::size_t>>> async_closest(
        const sdr::position_t pos,
        const std::size_t amount
    ) const {
#ifndef NDEBUG
        assert(pos < storage.size());
#endif
        return std::async(std::launch::async, &bank<Width>::async_closest_helper_pos, this, pos, amount);
    }

    std::future<std::vector<std::pair<sdr::position_t, std::size_t>>> async_closest(
        const sdr::concept & concept,
        const std::size_t amount
    ) const {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
#endif
        return std::async(std::launch::async, &bank<Width>::async_closest_helper_concept, this, concept, amount);
    }

    // find most similar to object at pos
    // first refers to position
    // second refers to matching number of bits
    template <typename WCollection>
    std::vector<std::pair<sdr::position_t, double>> weighted_closest(
        const sdr::position_t pos,
        const std::size_t amount,
        const WCollection & weights
    ) const {
#ifndef NDEBUG
        assert(pos < storage.size());
        assert(weights.size() == Width);
#endif
        return weighted_closest_helper(storage[pos].positions, amount, weights);
    }

    template <typename WCollection>
    std::vector<std::pair<sdr::position_t, double>> weighted_closest(
        const sdr::concept & concept,
        const std::size_t amount,
        const WCollection & weights
    ) const {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
        assert(weights.size() == Width);
#endif
        return weighted_closest_helper(concept.data, amount, weights);
    }

    template <typename WCollection>
    std::future<std::vector<std::pair<sdr::position_t, double>>> async_weighted_closest(
        const sdr::position_t pos,
        const std::size_t amount,
        const WCollection & weights
    ) const {
        return std::async(std::launch::async, &bank<Width>::async_weighted_closest_helper_pos, this, pos, amount, weights);
    }

    template <typename WCollection>
    std::future<std::vector<std::pair<sdr::position_t, double>>> async_weighted_closest(
        const sdr::concept & concept,
        const std::size_t amount,
        const WCollection & weights
    ) const {
        return std::async(std::launch::async, &bank<Width>::async_weighted_closest_helper_concept, this, concept, amount, weights);
    }

    // return all items matching all in data
    std::vector<sdr::position_t> matching(const sdr::concept & concept) const
    {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
#endif
        sdr::hash_set<sdr::position_t> matching;
        sdr::hash_set_init(matching);

        for(const std::size_t item : concept.data) {
            for(const std::size_t pos : bitmap[item]) {
                for(const std::size_t m : concept.data) {
                    if(bitmap[m].count(pos) == 0) {
                        goto skip;
                    }
                }

                matching.insert(pos);
                skip:;
            }
        }

        return { std::begin(matching), std::end(matching) };
    }

    // has to match amount in data
    std::vector<sdr::position_t> matching(
        const sdr::concept & concept,
        const std::size_t amount
    ) const {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
#endif
        sdr::hash_set<sdr::position_t> matching;
        sdr::hash_set_init(matching);

        for(const std::size_t item : concept.data) {
            for(const std::size_t pos : bitmap[item]) {
                std::size_t amount_matching { 0 };

                for(const std::size_t m : concept.data) {
                    amount_matching += bitmap[m].count(pos);
                }

                if(amount_matching >= amount) {
                    matching.insert(pos);
                }
            }
        }

        return { std::begin(matching), std::begin(matching) };
    }

    // has to match amount in data
    template <typename WCollection>
    std::vector<sdr::position_t> weighted_matching(
        const sdr::concept & concept,
        const double amount,
        const WCollection & weights
    ) const {
#ifndef NDEBUG
        for(auto & i : concept.data) {
            assert(i < Width);
        }
        assert(weights.size() == Width);
#endif
        sdr::hash_set<sdr::position_t> matching;
        sdr::hash_set_init(matching);

        for(const std::size_t item : concept.data) {
            for(const std::size_t pos : bitmap[item]) {
                double amount_matching { 0 };

                for(const std::size_t m : concept.data) {
                    amount_matching += bitmap[m].count(pos) * weights[m];
                }

                if(amount_matching >= amount) {
                    matching.insert(pos);
                }
            }
        }

        return { std::begin(matching), std::end(matching) };
    }
};

} //namespace sdr

#endif