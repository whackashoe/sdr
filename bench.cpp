#include <chrono>
#include <iostream>
#include <random>

// this currently is waaay slower when Width=4096 and OnPercent = 0.02
//#define USE_SHERWOOD

#include "sdr.hpp"


std::size_t since_epoch()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

int main()
{
    constexpr std::size_t Width = 1024;
    constexpr float OnPercent = 0.01;

    sdr::bank<Width> memory;
    std::vector<std::bitset<Width>> concepts;

    {
        auto start = since_epoch();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> wdis(0, Width-1);

        const std::size_t amount_set = static_cast<std::size_t>(Width * OnPercent);

        for(std::size_t i=0; i<1000000; ++i) {
            std::bitset<Width> data;

            for(std::size_t j=0; j<amount_set; ++j) {
                data.set(wdis(gen));
            }

            concepts.push_back(data);
        }

        auto end = since_epoch();

        std::cout << "generation took: " << (end-start) << "ms" << std::endl;
    }

    {
        auto start = since_epoch();

        for(auto & concept : concepts) {
            memory.insert(sdr::field_to_list(concept));
        }

        auto end = since_epoch();

        std::cout << "insertion took: " << (end-start) << "ms" << std::endl;
    }

    {
        auto start = since_epoch();

        auto closest = memory.closest(0, 10);

        auto end = since_epoch();

        std::cout << "finding closest took: " << (end-start) << "ms" << std::endl;

        std::cout << "closest: " << std::endl;
        for(auto & it : closest) {
            std::cout << "\t" << it.first << "\t:: " << it.second << std::endl;
        }
    }

    {
        auto start = since_epoch();

        std::array<double, Width> weights;
        std::fill(weights.begin(), weights.end(), 1.1);
        auto closest = memory.weighted_closest(0, 10, weights);

        auto end = since_epoch();

        std::cout << "finding weighted closest took: " << (end-start) << "ms" << std::endl;

        std::cout << "closest: " << std::endl;
        for(auto & it : closest) {
            std::cout << "\t" << it.first << "\t:: " << it.second << std::endl;
        }
    }

    {
        std::vector<std::size_t> simlist(10);
        std::iota(simlist.begin(), simlist.end(), 1);

        auto start = since_epoch();

        auto similarity = memory.union_similarity(0, simlist);

        auto end = since_epoch();

        std::cout << "finding union similarity took: " << (end-start) << "ms" << std::endl;

        std::cout << "similarity: " << similarity << std::endl;
    }

    {
        auto start = since_epoch();

        auto matching = memory.matching({ 1000, 88 });

        auto end = since_epoch();

        std::cout << "finding matching took: " << (end-start) << "ms" << std::endl;
        std::cout << "matching.size() => " << matching.size() << std::endl;

        std::cout << "matching: " << std::endl;
        for(auto & it : matching) {
            std::cout << it << std::endl;
        }
    }

    return 0;
}
