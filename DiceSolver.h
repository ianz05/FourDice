#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#define BOOST_MATH_DOMAIN_ERROR_POLICY errno_on_error
#define BOOST_MATH_POLE_ERROR_POLICY errno_on_error
#define BOOST_MATH_OVERFLOW_ERROR_POLICY errno_on_error
#include <boost/math/special_functions/factorials.hpp>

#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for_each.h>

#define ADD -1
#define SUB -2
#define MUL -3
#define DIV -4
#define POW -5
#define IS_BINARY(x) ((x >= POW) && (x <= ADD))

#define SQRT -6
#define FACT -7
#define IS_UNARY(x) ((x == SQRT) || (x == FACT))

#define INVALID_EXPR -1

#define MAX 20

class DiceSolver
{
    using score_array = std::array<std::pair<double, std::vector<int>>, MAX + 1>;

public:
    DiceSolver(unsigned min_ops, unsigned max_ops);
    void solve();
    static double parse_string(const std::string &expr_string);

private:
    struct Term {
        double value;
        int sqrts;
        int facts;
    };
    static double parse(const std::vector<int> &expr);
    static double get_score(const std::vector<int> &expr);
    static bool is_int(double d)
    {
        return is_close(d, std::round(d));
    }
    static bool is_close(double a, double b)
    {
        return std::abs(a - b) < 1e-8;
    }

    static void print_expr(std::ostream &os, const std::vector<int> &expr);

    template <typename V, typename Callable>
    static void for_each_combination(const V &v, std::size_t r, Callable f);

    const std::array<int, 4> nums;
    unsigned min_ops;
    unsigned max_ops;
    score_array scores;
};
