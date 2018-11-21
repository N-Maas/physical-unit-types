#pragma once

#include "Example_Units.h"
#include <iostream>

// using declaration to enable using operators and aliases (m, s, kg, N, ...) for units
PUNITS_USE_DEFINITIONS;

void basic() {
	// use UNIT_T to get types of units
	// a distance with unit type meters
	UNIT_T(m) distance(10);

	meters val;
	// val = distance;
	// compiler error: UNIT_T(m) != meters
	// (actual type of UNIT_T(m) is:
	// punits::PUnit<punits::ConversionPolicy::ExplicitConversion,punits::PowerOfUnit<punits::definitions::meters,1>>)

	// name() creates a string containing value and unit
	std::cout << "distance = " << distance.name() << std::endl;

	// value() accesses the double value
	std::cout << "value of distance = " << distance.value() << std::endl;

	// implicit unit type definition
	distance = 42 * m;
	std::cout << "distance = " << distance.name() << std::endl;

	// explicit definition, (effectively) equivalent to time = 20 * s
	// declared in namespace punits
	UNIT_T(s) time = punits::makeUnit(20, s);
	std::cout << "time = " << time.name() << std::endl;

	// time = 0 * m;
	// compiler error: meters can not be converted to seconds
	std::cout << std::endl;
}

void operators_and_combined_units() {
	// units support basic operations
	std::cout << "3s + 12s = " << (3*s + 12*s).name() << std::endl;
	std::cout << "5m - 2m = " << (5*m - 2*m).name() << std::endl;
	std::cout << "-2h < 0h = " << (-2*h < 0*h) << std::endl;
	// std::cout << (3 * m + 1 * s).name() << std::endl;
	// std::cout << (3 * m + 1).name() << std::endl;
	// compiler errors

	// unit types can be combined using * and /
	UNIT_T(m/s) speed = 5 * m/s;
	std::cout << "speed = " << speed.name() << std::endl;
	speed = makeUnit(10, m/s);
	std::cout << "speed = " << speed.name() << std::endl;

	// multiplication produces a new type
	std::cout << "3m * 4kg / 2s = " << (3*m * 4*kg / (2*s)).name() << std::endl;

	std::cout << std::endl;
}

void unit_conversions() {
	// units can be converted by the compiler
	UNIT_T(m) distance = UNIT_T(m)(2.63 * km);
	std::cout << "distance = " << distance.name() << std::endl;

	// use conversions to calculate the cinetic energy of a car in joule
	std::cout << "1000kg * (100km/h)^2 = " << UNIT_T(J)(1000 * kg * (100 * km/h) * (100 * km/h)).name() << std::endl;
	// what is a kWh in joule?
	std::cout << "1kWh = " << UNIT_T(J)(1000 * W * h).name() << std::endl;
	// convert miles to km
	std::cout << "1 mile = " << UNIT_T(km)(1 * miles).name() << std::endl;

	// units can have different conversion policies:
	// NoConversion, ExplicitConversion, ImplicitConversion
	// use UNIT_T_P($unit, $policy) or makeUnit<$policy>($value, $unit) to explicitely specify the policy
	UNIT_T_P(m, punits::ConversionPolicy::ImplicitConversion) dist_implicit = punits::makeUnit<punits::ConversionPolicy::ImplicitConversion>(77, m);
	std::cout << "dist (m) = " << dist_implicit.name() << std::endl;

	// use implicit conversions
	UNIT_T(cm) dist_cm = dist_implicit;
	std::cout << "dist (cm) = " << dist_cm.name() << std::endl;
	std::cout << "0.5km + 77m = " << (0.5 * km + dist_implicit).name() << std::endl;

	// dist_cm = 77 * m;
	// compiler error: the unit definitions of Example_Units.h use ExplicitConversion as default conversion policy

	// conversion works for quite complex units, too
	UNIT_T(N * N * s * s / m) complex_value = punits::makeUnit<punits::ConversionPolicy::ImplicitConversion>(3, g / km * W * h);
	std::cout << "complex_value = " << complex_value.name() << std::endl;

	std::cout << std::endl;
}

int main() {
	basic();
	operators_and_combined_units();
	unit_conversions();
}
