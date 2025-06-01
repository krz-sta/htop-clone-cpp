#ifndef HTOP_CLONE_TEMPLATES_HPP
#define HTOP_CLONE_TEMPLATES_HPP

#include <vector>
#include <algorithm>

namespace Templates {

template <typename T>
std::vector<T> uniquePreserveOrder(const std::vector<T>& input) {
    std::vector<T> result;
    for (const auto& item : input) {
        if (std::find(result.begin(), result.end(), item) == result.end()) {
            result.push_back(item);
        }
    }
    return result;
}

template <typename Iter, typename Compare>
Iter findMaxElement(Iter begin, Iter end, Compare comp) {
    if (begin == end) return end;
    Iter maxIt = begin;
    for (Iter it = std::next(begin); it != end; ++it) {
        if (comp(*maxIt, *it)) {
            maxIt = it;
        }
    }
    return maxIt;
}

}

#endif
