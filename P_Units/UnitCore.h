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

// constexpr pow

constexpr double constexpr_pow(const double val, int exp)
{
	if (exp == 0) {
		return 1;
	}
	else {
		if (exp > 0) {
			return val * constexpr_pow(val, exp - 1);
		}
		else {
			return constexpr_pow(val, exp + 1) / val;
		}
	}
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

// helper containing a conversion result
template< class T, int power, class... PoUs >
struct convert_result_helper
{
	static constexpr bool is_convertible = T::is_convertible;
	static constexpr double conversion_factor = constexpr_pow(T::conversion_factor, power);
	typedef Unit<PoUs...> type;
};

struct convert_result_false
{
	static constexpr bool is_convertible = false;
	static constexpr double conversion_factor = 0;
	typedef Unit<> type;
};

struct convert_result_neutral
{
	static constexpr bool is_convertible = true;
	static constexpr double conversion_factor = 1;
};

template< class R1, class R2 >
struct combine_results
{
	static constexpr bool is_convertible = R1::is_convertible && R2::is_convertible;
	static constexpr double conversion_factor = R1::conversion_factor * R2::conversion_factor;
};

// conversion of one PoU
template< typename... Ts >
struct search_elementary_conversion;

template< class Searched_U, int power, class... R_PoUs >
struct search_elementary_conversion<PowerOfUnit<Searched_U, power>, Unit<R_PoUs...>, Unit<>>
{
	static constexpr bool is_convertible = false;
	static constexpr double conversion_factor = 0;
	typedef Unit<> type;
};

template< class Searched_U, int searched_p, class... R_PoUs, class Head_U, int head_p, class... PoUs >
struct search_elementary_conversion<PowerOfUnit<Searched_U, searched_p>, Unit<R_PoUs...>, Unit<PowerOfUnit<Head_U, head_p>, PoUs...>>
{
private:
	typedef std::conditional_t<sym_conversion_data<Searched_U, Head_U>::is_convertible,
		// case 1: conversion is found at head
		std::conditional_t<searched_p == head_p,
			// case 1 a: correct power
			convert_result_helper<sym_conversion_data<Searched_U, Head_U>, searched_p, R_PoUs..., PoUs...>,
			// case 1 b: wrong power
			convert_result_false>,
		// case 2: not found, recursion needed
		search_elementary_conversion<PowerOfUnit<Searched_U, searched_p>, Unit<R_PoUs..., PowerOfUnit<Head_U, head_p>>, Unit<PoUs...>>
	> convert_result;

public:
	static constexpr bool is_convertible = convert_result::is_convertible;
	static constexpr double conversion_factor = convert_result::conversion_factor;
	typedef typename convert_result::type type;
};

template< typename... Ts >
struct search_conversion;

// base case 1
template<class convert_result>
struct search_conversion<Unit<>, Unit<>, convert_result> {
	static constexpr bool is_convertible = convert_result::is_convertible;
	static constexpr double conversion_factor = convert_result::conversion_factor;
};

// base case 2
template<class... Right_PoUs, class convert_result>
struct search_conversion<Unit<>, Unit<Right_PoUs...>, convert_result> {
	static constexpr bool is_convertible = false;
	static constexpr double conversion_factor = 0;
};

template<class Head_PoU, class... Left_PoUs, class... Right_PoUs, class convert_result>
struct search_conversion<Unit<Head_PoU, Left_PoUs...>, Unit<Right_PoUs...>, convert_result> {
private:
	typedef search_elementary_conversion<Head_PoU, Unit<>, Unit<Right_PoUs...>> search_result;
	typedef std::conditional_t<(search_result::is_convertible && convert_result::is_convertible),
		search_conversion<Unit<Left_PoUs...>, typename search_result::type, combine_results<convert_result, search_result>>,
		convert_result_false
		> final_result;

public:
	static constexpr bool is_convertible = final_result::is_convertible;
	static constexpr double conversion_factor = final_result::conversion_factor;
};

template< class U1, class U2 >
using get_factor_if_convertible = std::enable_if_t<search_conversion<U1, U2, convert_result_neutral>::is_convertible,
	search_conversion<U1, U2, convert_result_neutral>>;

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
