#pragma once

#include <type_traits>
#include <typeinfo>

template< class U, int pwr >
struct PowerOfUnit
{
	typedef U unit_type;
	static constexpr int power = pwr;
	static constexpr size_t unit_id = U::unit_id;
};


// ----

template< typename... Ts >
class Unit;


// HELPERS

template< typename... Ts >
struct mult_units;

// helper to test for power of 0
template< typename... Ts >
struct non_zero_append_to_unit;

template< class... PoUs, class U, int power>
struct non_zero_append_to_unit<Unit<PoUs...>, PowerOfUnit<U, power>>
{
	typedef Unit<PoUs..., PowerOfUnit<U, power>> type;
};

template< class... PoUs, class U>
struct non_zero_append_to_unit<Unit<PoUs...>, PowerOfUnit<U, 0>>
{
	typedef Unit<PoUs...> type;
};


// multiplication of two Unit Types
// base case 1
template< class... R_PoUs, class... PoUs >
struct mult_units<Unit<R_PoUs...>, Unit<>, Unit<PoUs...>>
{
	typedef Unit<R_PoUs..., PoUs...> type;
};

// base case 2
template< class... R_PoUs, class... PoUs >
struct mult_units<Unit<R_PoUs...>, Unit<PoUs...>, Unit<>>
{
	typedef Unit<R_PoUs..., PoUs...> type;
};

template< class... R_PoUs, class H1, class... PoU1s, class H2, class... PoU2s >
struct mult_units<Unit<R_PoUs...>, Unit<H1, PoU1s...>, Unit<H2, PoU2s...>>
{
	typedef std::conditional_t < std::is_same_v<typename H1::unit_type, typename H2::unit_type>,
		// case 1: first elements are equal, elements are combined
		typename mult_units<typename non_zero_append_to_unit<Unit<R_PoUs...>, PowerOfUnit<typename H1::unit_type, H1::power + H2::power>>::type, Unit<PoU1s...>, Unit<PoU2s...>>::type,
		std::conditional_t<(H1::unit_id < H2::unit_id),
		// case 2: H1 < H2, H1 is appended
		typename mult_units<Unit<R_PoUs..., H1>, Unit<PoU1s...>, Unit<H2, PoU2s...>>::type,
		// case 3: H2 < H1, H2 is appended
		typename mult_units<Unit<R_PoUs..., H2>, Unit<H1, PoU1s...>, Unit<PoU2s...>>::type>
		> type;
};

template< class Unit1, class Unit2 >
using mult_units_t = typename mult_units<Unit<>, Unit1, Unit2>::type;

// conversion between two unit types
template< class U1, class U2 >
struct unit_conversion_data
{
	static constexpr bool is_convertible = std::is_same_v<U1, U2>;
	static constexpr double conversion_factor = is_convertible ? 1 : 0;
};

// if a conversion is possible, elementary_convert_to defines the member value holding the conversion factor
// if no conversion is possible, elementary_convert_to can not be resolved
template< class U1, class U2 >
struct sym_conversion_data
{
	static constexpr bool is_convertible = unit_conversion_data<U1, U2>::is_convertible || unit_conversion_data<U2, U1>::is_convertible;
	static constexpr double conversion_factor = is_convertible ? (unit_conversion_data<U1, U2>::is_convertible ?
		unit_conversion_data<U1, U2>::conversion_factor : 1.0 / unit_conversion_data<U2, U1>::conversion_factor) : 0;
};

// ----

template<>
class Unit<>
{
	const double val;

public:
	constexpr explicit Unit<>(const double& val) : val(val) {}

	constexpr double value() const { return val; }
};

template< class... Us, int... powers >
class Unit<PowerOfUnit<Us, powers>...>
{
	const double val;

public:
	constexpr explicit Unit<PowerOfUnit<Us, powers>...>(const double& val) : val(val) {}

	constexpr double value() const { return val; }
};

template< class... PoUs >
constexpr Unit<PoUs...> operator+ (Unit<PoUs...> left, Unit<PoUs...> right)
{
	return Unit<PoUs...>(left.value() + right.value());
}

template< class... Left_PoUs, class... Right_PoUs >
constexpr mult_units_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>> operator* (Unit<Left_PoUs...> left, Unit<Right_PoUs...> right)
{
	return mult_units_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>>(left.value() * right.value());
}
