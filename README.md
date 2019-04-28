# physical-unit-types

Usage of physical units (e.g. meters, seconds) as plain floating point numbers
is quite unsafe, because wrong conversions and comparable errors can easily
happen. This repository implements a small C++ library that allows for easy
definiton of types that represent physical units. This enforces correct usage of
the units by the compiler, while the resulting code can be optimized to the same
performance as of plain numbers. To achieve this, the implementation makes use
of template-metaprogramming.

This is primarily a proof-of-concept implementation. Some examples of usage can
be found in the
[examples.cpp](https://github.com/N-Maas/physical-unit-types/blob/master/P_Units/examples.cpp)
file.