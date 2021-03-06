# Math calculator
## Compile
```
$ ./prepare make
```
## Use
```
$ build/calc < test/expressions.txt
((((100))) - 2 * (((3 + 1) + (2 + 16 / 4)) * (11 - 6 - 1000 * 0)))
3 + 1 = 4
16 / 4 = 4
2 + 4 = 6
4 + 6 = 10
11 - 6 = 5
1000 * 0 = 0
5 - 0 = 5
10 * 5 = 50
2 * 50 = 100
100 - 100 = 0
Result: 0

1 + sqrt(10 - 2 * 3)
2 * 3 = 6
10 - 6 = 4
sqrt(4) = 2
1 + 2 = 3
Result: 3

1 + 5* (2 - 3) * 4
2 - 3 = -1
5 * -1 = -5
-5 * 4 = -20
1 + -20 = -19
Result: -19

1+ (2-3)*4
2 - 3 = -1
-1 * 4 = -4
1 + -4 = -3
Result: -3

1 + 2 * (3 + 4 + 5)
3 + 4 = 7
7 + 5 = 12
2 * 12 = 24
1 + 24 = 25
Result: 25

1+2*sqrt(1+8)/(4 + (2 - 3))
1 + 8 = 9
sqrt(9) = 3
2 * 3 = 6
2 - 3 = -1
4 + -1 = 3
6 / 3 = 2
1 + 2 = 3
Result: 3

1 - 2 * sqrt(9) + 7
sqrt(9) = 3
2 * 3 = 6
1 - 6 = -5
-5 + 7 = 2
Result: 2

sin(1) - cos(1) * tan(1)
sin(1) = 0.841471
cos(1) = 0.540302
tan(1) = 1.55741
0.540302 * 1.55741 = 0.841471
0.841471 - 0.841471 = 0
Result: 0

(sin(1) - cos(1)) * tan(1)
sin(1) = 0.841471
cos(1) = 0.540302
0.841471 - 0.540302 = 0.301169
tan(1) = 1.55741
0.301169 * 1.55741 = 0.469042
Result: 0.469042

1 + (sin(1) - 3) * 1
sin(1) = 0.841471
0.841471 - 3 = -2.15853
-2.15853 * 1 = -2.15853
1 + -2.15853 = -1.15853
Result: -1.15853

5 + 2 +(sin(1) - cos(1)) * tan(1) - 7
5 + 2 = 7
sin(1) = 0.841471
cos(1) = 0.540302
0.841471 - 0.540302 = 0.301169
tan(1) = 1.55741
0.301169 * 1.55741 = 0.469042
7 + 0.469042 = 7.46904
7.46904 - 7 = 0.469042
Result: 0.469042

2 * (5 - 4 / 2 - 1)
4 / 2 = 2
5 - 2 = 3
3 - 1 = 2
2 * 2 = 4
Result: 4

sin(cos(sin(cos(sin(cos(sin(cos(sin(cos(sin(0))-1))-1))-1))-1))-1)
sin(0) = 0
cos(0) = 1
1 - 1 = 0
sin(0) = 0
cos(0) = 1
1 - 1 = 0
sin(0) = 0
cos(0) = 1
1 - 1 = 0
sin(0) = 0
cos(0) = 1
1 - 1 = 0
sin(0) = 0
cos(0) = 1
1 - 1 = 0
sin(0) = 0
Result: 0
```
