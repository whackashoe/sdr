#ifndef SDR_CONCEPT_H_
#define SDR_CONCEPT_H_

#include "constants.hpp"

#include <vector>
#include <array>
#include <bitset>
#include <algorithm>
#include <cstddef>
#include <iterator>


namespace sdr
{

// this holds all of the traits for a single concept
// ie - if width is 1024, this could hold up to 1024 values between 0 and 1024
// designed to be fast to create, pass around, and as util to compress sparse data
struct concept
{
    std::vector<sdr::position_t> data;

    // for vector of positions we assume the data has already been dealt with
    concept(const std::vector<sdr::position_t> & input)
    : data(input) {}

    // otherwise we assume that is hasn't, thus we only add non zero fields
    template <sdr::width_t W>
    concept(const std::bitset<W> & input)
     : data()
    {
        for(std::size_t i=0; i < input.size(); ++i) {
            if(input[i]) {
                data.emplace_back(i);
            }
        }
    }

    // store [amount] largest ones from input
    template <typename T, sdr::width_t W>
    concept(
        const std::array<T, W> & input,
        const std::size_t amount
    )
     : data()
    {
        std::vector<sdr::position_t> idx(input.size());
        std::iota(std::begin(idx), std::end(idx), 0);

        std::partial_sort(std::begin(idx), std::begin(idx) + amount, std::end(idx), [&](
            const T a,
            const T b
        ) {
            return input[a] > input[b];
        });

        for(std::size_t i=0; i < amount; ++i) {
            // end if value is 0
            if(! input[idx[i]]) {
                break;
            }

            data.emplace_back(idx[i]);
        }
    }
};

}; //namespace sdr

#endif
