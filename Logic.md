# Program Logic

## Technical details

Expressions are stored as `std::vector<int>`, with the numbers being exactly what they are, and operators being defined negative numbers.

Terms are stored as a POD struct with three data members:

    struct Term {
        double value;
        int sqrts;
        int facts;
    };

Scores are stored in score arrays of type `std::array<std::pair<double, std::vector<int>>, MAX + 1>`, with `MAX` being `20` in our case.

## Generating permutations

 1. Each combination of 3 binary operators is stored. Division and exponentiation are allowed to appear 3 times, but addition, subtraction, and multiplication are only allowed to appear 1 time each.
 2. Starting at the minimum number of operators and ending at the maximum number of operators, we iterate through and compute each possible number of operators sequentially. In other words, we start with, say, 4 operators, which means only 1 unary operator, and end at, say, 9, which means 6 unary operators.
 3. Combinations of the specified number of unary operators are created. Because there are only two, repetition is allowed, and order does not matter, there are `n + 1` combinations for `n` unary operators.
 4. The numbers (`1, 2, 3, 4`), a combination of binary operators, and a combination of unary operators are concatenated. All permutations of this combined sequence are iterated over and parsed.

## Parsing

The expressions are parsed as postfix expressions. We iterate through the expression and store a stack of terms. If the current element is a:

 - Number: Push to stack
 - Binary Operator: Pop two terms off of stack, apply operation, push result back
 - Unary Operator: Pop one term off of stack, apply operation, push result back

If at any point there are not enough terms on the stack, the expression is invalid. The last term on the stack is the result. If, after iterating through the entire expression, there are still multiple terms on the stack, the expression is invalid.

## Rules

### Unary operations

`X` = operand

The result will have its `sqrts` and `facts` copied from `X`.

#### Square root

 - `X.value` is 0 or 1: **INVALID**
 - Increment `X.sqrts` by 1
 - Reset `X.facts` to 0

#### Factorial

 - `X.value` is negative, greater than 10, or a non-integer: **INVALID**
 - `X.value` is 1 or 2: **INVALID**
 - Increment `X.facts` by 1
 - Reset `X.sqrts` to 0

### Binary operations

`L` = left operand, `R` = right operand

Unless otherwise stated, the result will have `sqrts = 0` and `facts = 0`. An exception is if the operation did not change the value, e.g. multiplying by 1 or adding 0. 

#### Addition

 - No rules

#### Subtraction

 - `L.value` equals `R.value` and either both `L.sqrts` and `R.sqrts` are non-zero or `L.facts` and `R.facts` are non-zero: **INVALID**

#### Multiplication

 - `L.value * R.value` is 1 and both `L.sqrts` and `R.sqrts` are non-zero: **INVALID**

#### Division

 - `L.sqrts` equals `R.sqrts` and is non-zero, and `L.facts` equals `R.facts` and is non-zero: **INVALID**
 - `L.value` equals `R.value` and either both `L.sqrts` and `R.sqrts` are non-zero or `L.facts` and `R.facts` are non-zero: **INVALID**
 - `L.value / R.value` is less than 0.1: **INVALID**
 - The result's `sqrts` is set to `L.sqrts`
 - `L.value` is a non-integer and `L.sqrts` is non-zero, or `R.value` is a non-integer and `R.sqrts` is non-zero: The result's `sqrts` is the greater of `L.sqrts` and `R.sqrts`

#### Exponentiation

 - `L.value` is 0 or 1, or `R.value` is greater than 6: **INVALID**
 - If `R.value` is an integer:
   - `R.value` is 2: Decrement `L.sqrts` by 1, result uses `L.sqrts` as its `sqrts`
   - `R.value` is 4: Decrement `L.sqrts` by 2, result uses `L.sqrts` as its `sqrts`
   - `R.value` is 6:
     - `R.facts` is 1 and `L.sqrts` is at least 1: **INVALID**
     - Decrement `L.sqrts` by 1, result uses `L.sqrts` as its `sqrts`
   - If `L.sqrts` is negative, set it to 0
 - `L.value ^ R.value` is less than 0.1: **INVALID**
