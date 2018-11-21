#pragma once

#include "UnitCore.h"

DEFINE_BASE_UNIT(0, meters, m);

DEFINE_BASE_UNIT(1, seconds, s);

DEFINE_BASE_UNIT(2, gram, g);

DEFINE_DEPENDENT_UNIT(3, kilometers, km, m, 1000);
DEFINE_DEPENDENT_UNIT(4, centimeters, cm, m, 0.01);
DEFINE_DEPENDENT_UNIT(5, millimeters, mm, m, 0.001);

DEFINE_DEPENDENT_UNIT(6, minutes, min, s, 60);
DEFINE_DEPENDENT_UNIT(7, hours, h, min, 60);

DEFINE_DEPENDENT_UNIT(8, kilogram, kg, g, 1000);
DEFINE_DEPENDENT_UNIT(9, milligram, mg, g, 0.001);

DEFINE_DEPENDENT_UNIT(10, newton, N, kg * m / s / s, 1);
DEFINE_DEPENDENT_UNIT(11, joule, J, N * m, 1);
DEFINE_DEPENDENT_UNIT(12, watt, W, J / s, 1);

DEFINE_DEPENDENT_UNIT(13, miles_t, miles, km, 1.609344);
