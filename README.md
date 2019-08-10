# FourDice
This program brute forces permutations of numbers and operations to create other numbers.

The original problem was to use the numbers 1, 2, 3, and 4 to create integers between 0 and 20 inclusive. For reasons of performance, these values are hard-coded into the program.

It utilizes postfix notation instead of the conventional infix notation in order to generate all possible expressions. Postfix notation avoids the use of brackets, and the concept of order of operations is nonexistent, making expression generation much simpler.

## Example

Using the numbers 1, 2, 3, and 4 and some operations, we can create the number 11 with `2 * 3 + 4 + 1`. We can create 14 with `(sqrt(4) + 2 * 3!) ^ 1`.

## Allowed operations

The binary operations of addition, subtraction, multiplication, division, and exponentiation are allowed.

Two unary operations are also allowed, being square root and factorial.

## Scoring

Each valid expression has a base score of 1. The operations of division, exponentiation, square root, and factorial are special in the fact that they can award extra points.

Each use of the previous four operators awards additional points according to a geometrically decaying pattern: the first use awards 1 point, the second 0.5, the third 0.25, and so on.

## Illegal expressions

Expressions which utilize tricks to use an infinite amount of score-earning operators are disallowed. These involve using the unary operators many times because the number of binary operators is always limited to 3 (for 4 numbers).

Firstly, the square roots of 0 and 1 are prohibited. The factorials of 1 and 2 are prohibited. In general, any unary operation that does not change the value of the argument is not allowed.

There are some strategies that are forbidden. Three such strategies are:

 - A excessively complex exponent of 0 or 1: for example, `1 ^ (sqrt(2) / 3!! * sqrt(4!))` to option 1. (In this context, "complex" means using many operators, not a number with an imaginary part.)
 - An extremely large exponent with infinite square roots: for example, `sqrt(sqrt(sqrt(... sqrt(2 / 1)))) ^ (3! * 4!)!` to obtain 2. The exponent is an absurd value, but it is "cancelled out" by a corresponding amount of square roots. 
 - Two equal numbers with subtraction/division: for example, `1 + 3!!!!! - (4 + 2)!!!!` to obtain 1. `3!` and `(4 + 2)` are equivalent, and adding the factorials after does not change the result.

Other strategies do exist, but they all follow one overall pattern: if the same result can be obtained by removing any number of unary operators, the expression is illegal.

### How can we prevent these strategies?

It's nearly impossible to detect "if the same result can be obtained by removing any number of unary operators, the expression is illegal" in an algorithmic fashion. Instead, the program uses a series of rules that prevent excessive unary operators. These rules are highlighted in the **Rules** section of `Logic.md`.

## Usage

First, `#include "DiceSolver.h"`. Then, instantiate a `DiceSolver` with two arguments: the minimum number of operations and the maximum number of operations for the computation. Then, call its `solve` method.

An example `main.cpp` is included. It contains these simple contents:

    #include "DiceSolver.h"

    int main()
    {
        DiceSolver solver(4, 12);
        solver.solve();
    }
