#ifndef MATHSET_HPP
#define MATHSET_HPP

#include <set>

template <typename T>
class mathset {
public:
    static bool is_subset(const std::set<T> & s1, const std::set<T> & s2);
    static bool is_superset(const std::set<T> & s1, const std::set<T> & s2);
    static std::set<T> intersection(const std::set<T> & s1, const std::set<T> & s2);
    static std::set<T> union_(const std::set<T> & s1, const std::set<T> & s2);
    static std::set<T> difference(const std::set<T> & s1, const std::set<T> & s2);
    static std::set<T> symmetric_difference(const std::set<T> & s1, const std::set<T> & s2);

    friend std::set<T> operator&(const std::set<T> & s1, const std::set<T> & s2);
    friend std::set<T> operator|(const std::set<T> & s1, const std::set<T> & s2);
    friend std::set<T> operator-(const std::set<T> & s1, const std::set<T> & s2);
    friend std::set<T> operator^(const std::set<T> & s1, const std::set<T> & s2);
};

template <typename T>
bool mathset<T>::is_subset(const std::set<T> & s1, const std::set<T> & s2) {
    for (T & it: s1)
        if (! s2.contains(it))
            return false;
    return true;
}

template <typename T>
bool mathset<T>::is_superset(const std::set<T> & s1, const std::set<T> & s2) {
    for (const T & it: s2)
        if (! s1.contains(it))
            return false;
    return true;
}


template <typename T>
std::set<T> mathset<T>::intersection(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        if (s2.contains(it))
            newSet.insert(it);
    return newSet;
}

template <typename T> std::set<T> operator&(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        if (s2.contains(it))
            newSet.insert(it);
    return newSet;
}


template <typename T>
std::set<T> mathset<T>::union_(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        newSet.insert(it);
    for (const T & it: s2)
        newSet.insert(it);
    return newSet;
}

template <typename T>
std::set<T> operator|(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        newSet.insert(it);
    for (const T & it: s2)
        newSet.insert(it);
    return newSet;
}

//A - B is the set of all elements in A that are not in B
template <typename T>
std::set<T> mathset<T>::difference(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        if (! s2.contains(it))
            newSet.insert(it);
    return newSet;
}

template <typename T>
std::set<T> operator-(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        if (! s2.contains(it))
            newSet.insert(it);
    return newSet;
}

//A ^ B = (A + B) - (A & B)
template <typename T>
std::set<T> mathset<T>::symmetric_difference(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        if (! s2.contains(it))
            newSet.insert(it);
    for (const T & it : s2)
        if (! s1.contains(it))
            newSet.insert(it);
    return newSet;
}

template <typename T>
std::set<T> operator^(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> newSet;
    for (const T & it: s1)
        if (! s2.contains(it))
            newSet.insert(it);
    for (const T & it : s2)
        if (! s1.contains(it))
            newSet.insert(it);
    return newSet;
}

#endif // MATHSET_HPP
