#include "DiceSolver.h"

using boost::math::factorial;
using std::vector;

DiceSolver::DiceSolver(unsigned min_ops, unsigned max_ops)
    : nums{1, 2, 3, 4}, min_ops(min_ops), max_ops(max_ops), scores()
{
    // Minimum number of operations
    auto minimum = this->nums.size() - 1;

    // Make sure min_ops is at least minimum
    if (this->min_ops < minimum) {
        this->min_ops = minimum;
    }

    // Make sure max_ops is at least minimum
    if (this->max_ops < minimum) {
        this->max_ops = minimum;
    }
}

void DiceSolver::solve()
{
    using namespace std::chrono;

    // Print out start time
    auto curr_time = std::time(nullptr);
    std::cout << "Started: " << ctime(&curr_time) << std::endl;

    // n is number of binary operations
    const auto n = nums.size() - 1;

    // partials includes combinations with numbers and binary operators
    vector<vector<int>> partials;
    for_each_combination(vector<int>{ADD, ADD, SUB, SUB, MUL, MUL, DIV, DIV, DIV, POW, POW, POW}, n, [&](const vector<int> &bops) {
        if (std::round(get_score(bops)) >= 2.0) {
            vector<int> partial(nums.begin(), nums.end());
            partial.insert(partial.end(), bops.begin(), bops.end());
            std::sort(partial.begin(), partial.end());
            partials.push_back(partial);
        }
    });

    // Get rid of duplicates
    std::sort(partials.begin(), partials.end());
    partials.erase(std::unique(partials.begin(), partials.end()), partials.end());

    // Iterate through number of unary operators
    for (unsigned length = min_ops; length <= max_ops; ++length) {
        std::cout << "Computing combinations with " << length << " operators..." << std::endl;
        auto start = steady_clock::now();

        // Generate combinations of unary operators
        auto r = length - n;
        vector<vector<int>> uop_combs;

        vector<int> comb(r, SQRT);
        for (std::size_t i = 0; i < r; ++i) {
            comb[i] = FACT;
            uop_combs.push_back(comb);
        }

        // Best scores for this iteration
        tbb::concurrent_vector<score_array> iter_scores;
        iter_scores.reserve(1 << (length - 3));

        tbb::parallel_for_each(uop_combs.begin(), uop_combs.end(), [&](auto &&uops) {
            // Current thread's best scores
            score_array thread_scores{};
            for (const auto &partial : partials) {
                // Generate the expression
                vector<int> expr(partial);
                expr.insert(expr.end(), uops.begin(), uops.end());
                // Sort so std::next_permutation can iterate through all perms
                std::sort(expr.begin(), expr.end());

                do {
                    // Parse the expression
                    double result = parse(expr);
                    //print_expr(std::cout, expr);
                    // Continue to next iteration if the expression is invalid
                    if (result == INVALID_EXPR) {
                        continue;
                    }

                    if (is_int(result)) {
                        int iresult = static_cast<int>(std::round(result));
                        // If the result is out of range
                        if (iresult < 0 || iresult > MAX) {
                            continue;
                        }
                        double score = get_score(expr);

                        // Record score (or not) in scores array
                        auto &entry = thread_scores[iresult];
                        if (score > entry.first) {
                            entry.first = score;
                            entry.second = expr;
                        }
                    }
                } while (std::next_permutation(expr.begin(), expr.end()));
            }
            iter_scores.push_back(std::move(thread_scores));
        });

        // Merge iter_scores into scores
        for (auto &arr : iter_scores) {
            for (size_t i = 0; i < MAX + 1; ++i) {
                if (arr[i].first > scores[i].first) {
                    scores[i] = std::move(arr[i]);
                }
            }
        }

        // Get elapsed time
        auto stop = steady_clock::now();
        auto ms = duration_cast<milliseconds>(stop - start);
        auto sec = ms / 1000;
        ms %= 1000;
        auto min = sec / 60;
        sec %= 60;

        double total = 0;

        // Print the best scores so far
        for (size_t i = 0; i < MAX + 1; ++i) {
            std::cout << i << '\t' << scores[i].first << '\t';
            total += scores[i].first;
            print_expr(std::cout, scores[i].second);
        }

        // Print the total score
        std::cout << "Total score:\t" << total << '\n';

        // Print duration for this calculation.
        std::cout << "Duration:\t"
                  << min.count() << " min "
                  << sec.count() << " sec "
                  << ms.count() << " ms\n"
                  << std::endl;
    }
}

double DiceSolver::parse_string(const std::string &expr_string)
{
    vector<int> expr;
    for (auto c : expr_string) {
        if ('0' <= c && c <= '9') {
            expr.push_back(c - '0');
        } else {
            switch (c) {
            case '+':
                expr.push_back(ADD);
                break;
            case '-':
                expr.push_back(SUB);
                break;
            case '*':
                expr.push_back(MUL);
                break;
            case '/':
                expr.push_back(DIV);
                break;
            case '^':
                expr.push_back(POW);
                break;
            case 'V':
                expr.push_back(SQRT);
                break;
            case '!':
                expr.push_back(FACT);
                break;
            }
        }
    }
    return parse(expr);
}

double DiceSolver::parse(const vector<int> &expr)
{
    // Reset errno (for factorial)
    errno = 0;

    // Array of terms (shouldn't hardcode the length, but boosts performance)
    std::array<Term, 4> terms;
    // Current length of array
    unsigned length = 0;

    /*
        Logic to parse (postfix):
        Iterate through expression, case:
            Number: Push to stack (terms)
            Binary Operator: Pop two terms off of stack, apply operation, push result back
            Unary Operator: Pop one term off of stack, apply operation, push result back
        If at any point not enough terms on stack, expression is invalid
        Result is last term on stack
    */
    for (auto val : expr) {
        // Unary operators
        if (IS_UNARY(val)) {
            // Not enough terms
            if (length == 0) {
                return INVALID_EXPR;
            }
            // Term to apply operation to
            auto &x = terms[length - 1];
            switch (val) {
            case SQRT:
                // Square root of 0 or 1
                if (is_close(x.value, 0) || is_close(x.value, 1) || x.powed) {
                    return INVALID_EXPR;
                }

                x.value = std::sqrt(x.value);
                ++x.sqrts;
                // Reset factorial counter
                x.facts = 0;
                break;
            case FACT:
                // Factorial of negative, large numbers, or non-integers
                if (x.value < 0 || x.value > 10 || !is_int(x.value)) {
                    return INVALID_EXPR;
                }

                auto rounded = static_cast<unsigned>(std::round(x.value));
                // Factorial of 1 or 2
                if (rounded == 1 || rounded == 2) {
                    return INVALID_EXPR;
                }
                x.value = factorial<double>(rounded);
                // If factorial errors somehow
                if (errno) {
                    return INVALID_EXPR;
                }
                ++x.facts;
                // Reset sqrt counter
                x.sqrts = 0;
                x.powed = false;
                break;
            }
            // Binary operators
        } else if (IS_BINARY(val)) {
            // Not enough terms
            if (length < 2) {
                return INVALID_EXPR;
            }
            // RHS term
            auto &r = terms[--length];
            // LHS term
            auto &l = terms[length - 1];
            // Copy old value for comparison later
            auto old = l.value;
            // Flag to not reset sqrts
            bool keep_sqrts = false;
            switch (val) {
            case ADD:
                if (is_close(r.value, 0)) {
                    keep_sqrts = true;
                    break;
                }
                l.powed = false;

                l.value += r.value;
                break;
            case SUB:
                // Condition to check excessive sqrts/factorials
                if (is_close(l.value, r.value) && ((l.sqrts > 0 && r.sqrts > 0) || (l.facts > 0 && r.facts > 0))) {
                    return INVALID_EXPR;
                }
                if (is_close(r.value, 0)) {
                    keep_sqrts = true;
                    break;
                }
                l.powed = false;

                l.value -= r.value;
                break;
            case MUL:
                if (is_close(r.value, 1)) {
                    keep_sqrts = true;
                    break;
                }
                l.powed = false;

                l.value *= r.value;
                // Excessive square roots when the product is 1
                if (is_close(l.value, 1) && l.sqrts >= 1 && r.sqrts >= 1) {
                    return INVALID_EXPR;
                }
                break;
            case DIV:
                // Division by 1 allowed, early exit because the number is unchanged
                if (is_close(r.value, 1)) {
                    keep_sqrts = true;
                    break;
                }
                l.powed = false;

                // Condition to check excessive sqrts/factorials
                if ((l.sqrts == r.sqrts && l.sqrts > 0 && l.facts == r.facts && l.facts > 0) || (is_close(l.value, r.value) && ((l.sqrts > 0 && r.sqrts > 0) || (l.facts > 0 && r.facts > 0)))) {
                    return INVALID_EXPR;
                }

                l.value /= r.value;

                // If the division was used to get very close to zero
                if (std::abs(l.value) < 0.1) {
                    return INVALID_EXPR;
                }

                // To keep sqrts in exprs like 1 / v2, v2 / 3, vv2 / v2
                if ((!is_int(r.value) && r.sqrts > 0) || (!is_int(old) && l.sqrts > 0)) {
                    l.sqrts = std::max(l.sqrts, r.sqrts);
                }
                keep_sqrts = true;

                break;
            case POW:
                if (is_close(l.value, 0) || is_close(l.value, 1) || std::round(r.value) >= 32) {
                    // Avoid 0 ** (complex number) and 1 ** (complex number)
                    // Also disallow large exponents
                    return INVALID_EXPR;
                } else if (is_int(r.value)) {
                    int rounded = static_cast<int>(std::round(r.value));
                    if (rounded == 1) {
                        // Exponent of 1 allowed, early exit because the number is unchanged
                        keep_sqrts = true;
                        break;
                    } else if (rounded == 2) {
                        // Square of number
                        keep_sqrts = true;
                        --l.sqrts;
                    } else if (rounded == 4) {
                        // Square of square of number
                        keep_sqrts = true;
                        l.sqrts -= 2;
                    } else if (rounded == 6) {
                        // v2 ** 3! == 2 ** 3
                        if (r.facts == 1 && l.sqrts > 0) {
                            return INVALID_EXPR;
                        }
                        // 6 is even, so 1 sqrt is "cancelled out"
                        keep_sqrts = true;
                        --l.sqrts;
                    } else if (rounded == 24) {
                        // 24 is divisible by 4, so 2 sqrts are "cancelled out"
                        keep_sqrts = true;
                        l.sqrts -= 2;
                    }

                    // Ensure sqrts is non-negative
                    if (l.sqrts < 0) {
                        l.sqrts = 0;
                    }
                }

                l.value = std::pow(l.value, r.value);

                // If the power was used to get very close to zero
                if (std::abs(l.value) < 0.1) {
                    return INVALID_EXPR;
                }

                l.powed = true;

                break;
            }
            // If there was a change in the value
            if (!is_close(old, l.value)) {
                if (!keep_sqrts) {
                    l.sqrts = 0;
                }
                l.facts = 0;
            }
        } else {
            // Number, push to stack
            auto &n = terms[length++];
            n.value = val;
            n.sqrts = 0;
            n.facts = 0;
            n.powed = false;
        }
    }

    // If somehow the number of binary operators was incorrect
    if (length != 1) {
        return INVALID_EXPR;
    }

    // The result
    return terms[0].value;
}

double DiceSolver::get_score(const vector<int> &expr)
{
    // Base score of 1
    double score = 1;
    for (auto op : {DIV, POW, SQRT, FACT}) {
        // Geometrically decaying sequence for each present operator
        score += 2 - std::pow(0.5, std::count(expr.cbegin(), expr.cend(), op) - 1);
    }
    return score;
}

void DiceSolver::print_expr(std::ostream &os, const vector<int> &expr)
{
    for (auto i : expr) {
        switch (i) {
        case ADD:
            os << '+';
            break;
        case SUB:
            os << '-';
            break;
        case MUL:
            os << '*';
            break;
        case DIV:
            os << '/';
            break;
        case POW:
            os << '^';
            break;
        case SQRT:
            os << 'V';
            break;
        case FACT:
            os << '!';
            break;
        default:
            os << i;
        }
        os << ' ';
    }
    std::cout << '\n';
}

template <typename V, typename Callable>
void DiceSolver::for_each_combination(const V &v, std::size_t r, Callable f)
{
    auto n = v.size();
    vector<bool> mask(n);
    std::fill(mask.end() - r, mask.end(), true);
    V comb;
    comb.reserve(r);
    do {
        for (typename V::size_type i = 0; i < n; ++i)
            if (mask[i]) {
                comb.push_back(v[i]);
            }
        f(comb);
        comb.clear();
    } while (std::next_permutation(mask.begin(), mask.end()));
}
