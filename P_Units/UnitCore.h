#pragma once

#include <type_traits>
#include <typeinfo>

enum class ConversionPolicy
{
	NoConversion,
	ExplicitConversion,
	ImplicitConversion
};

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

// fwd declarations
template< typename... Ts >
class Unit;

template< ConversionPolicy, typename... Ts >
class PUnit;


// HELPERS
template< class, ConversionPolicy >
struct to_punit;

template< class... PoUs, ConversionPolicy p >
struct to_punit< Unit<PoUs...>, p >
{
	typedef PUnit<p, PoUs...> type;
};

template< class, ConversionPolicy >
struct punit_set_policy;

template< class... PoUs, ConversionPolicy old_p, ConversionPolicy new_p >
struct punit_set_policy< PUnit<old_p, PoUs...>, new_p >
{
	typedef PUnit<new_p, PoUs...> type;
};

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
template< typename... Ts >
struct mult_units;

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

template< class Unit1, class Unit2, ConversionPolicy p >
using mult_punits_t = typename to_punit<mult_units_t<Unit1, Unit2>, p>::type;

// multiplicative inverse type of a unit
template< class >
struct inverse_unit;

template< class... Us, int... powers >
struct inverse_unit<Unit<PowerOfUnit<Us, powers>...>>
{
	typedef Unit<PowerOfUnit<Us, -powers>...> type;
};

template< class Unit1, class Unit2, ConversionPolicy p >
using div_punits_t = mult_punits_t<Unit1, typename inverse_unit<Unit2>::type, p>;

// unit_conversion_data defines the member is_convertible, that is true exactly if U1 can be converted to U2
// if a conversion is possible, the member conversion_factor holds the conversion factor
template< class U1, class U2 >
struct unit_conversion_data
{
	static constexpr bool is_convertible = std::is_same_v<typename U1::base_unit_type, typename U2::base_unit_type>;
	static constexpr double conversion_factor = is_convertible ? (U1::conversion_factor / U2::conversion_factor) : 0;
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
	typedef Unit<> type;
};

template< class R1, class R2 >
struct combine_results
{
	static constexpr bool is_convertible = R1::is_convertible && R2::is_convertible;
	static constexpr double conversion_factor = R1::conversion_factor * R2::conversion_factor;
	typedef mult_units_t<typename R1::type, typename R2::type> type;
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
	typedef std::conditional_t<unit_conversion_data<Searched_U, Head_U>::is_convertible,
		// case 1: conversion is found at head
		std::conditional_t<searched_p == head_p,
			// case 1 a: correct power
			convert_result_helper<unit_conversion_data<Searched_U, Head_U>, searched_p, R_PoUs..., PoUs...>,
			// case 1 b: wrong power
			convert_result_false>,
		// case 2: not found, recursion needed
		search_elementary_conversion<PowerOfUnit<Searched_U, searched_p>, Unit<R_PoUs..., PowerOfUnit<Head_U, head_p>>, Unit<PoUs...>>
	> C_Result;

public:
	static constexpr bool is_convertible = C_Result::is_convertible;
	static constexpr double conversion_factor = C_Result::conversion_factor;
	typedef typename C_Result::type type;
};

template< typename... Ts >
struct search_conversion;

// base case 1
template<class C_Result>
struct search_conversion<Unit<>, Unit<>, C_Result> {
	static constexpr bool is_convertible = C_Result::is_convertible;
	static constexpr double conversion_factor = C_Result::conversion_factor;
};

// base case 2
template<class... Right_PoUs, class C_Result>
struct search_conversion<Unit<>, Unit<Right_PoUs...>, C_Result> {
	static constexpr bool is_convertible = false;
	static constexpr double conversion_factor = 0;
};

template<class Head_PoU, class... Left_PoUs, class... Right_PoUs, class C_Result>
struct search_conversion<Unit<Head_PoU, Left_PoUs...>, Unit<Right_PoUs...>, C_Result> {
private:
	typedef search_elementary_conversion<Head_PoU, Unit<>, Unit<Right_PoUs...>> search_result;
	typedef std::conditional_t<(search_result::is_convertible && C_Result::is_convertible),
		search_conversion<Unit<Left_PoUs...>, typename search_result::type, combine_results<C_Result, search_result>>,
		convert_result_false
		> final_result;

public:
	static constexpr bool is_convertible = final_result::is_convertible;
	static constexpr double conversion_factor = final_result::conversion_factor;
};

// preparation: eliminating intern conversion possibilities of a unit
template< typename... Ts >
struct intern_conversion;

// base case
template< class U, class C_Result >
struct intern_conversion< U, Unit<>, Unit<>, C_Result >
{
	typedef C_Result type;
};

// case when end of right unit is reached: taking first unit, starting again
template< class U, class Head_U, int head_p, class... Left_PoUs , class C_Result >
struct intern_conversion< U, Unit<PowerOfUnit<Head_U, head_p>, Left_PoUs...>, Unit<>, C_Result >
{
	typedef typename intern_conversion<Head_U, Unit<>, Unit<Left_PoUs...>,
		combine_results<C_Result, convert_result_helper<convert_result_neutral, 0, PowerOfUnit<Head_U, head_p>>>>::type type;
};

template< class U, class... Left_PoUs, class Head_U, int head_p, class... Right_PoUs, class C_Result >
struct intern_conversion<U, Unit<Left_PoUs...>, Unit<PowerOfUnit<Head_U, head_p>, Right_PoUs...>, C_Result>
{
	typedef typename std::conditional_t<unit_conversion_data<Head_U, U>::is_convertible,
		// case 1: is convertible, combining the results
		intern_conversion<U, Unit<Left_PoUs...>, Unit<Right_PoUs...>,
			combine_results<C_Result, convert_result_helper<unit_conversion_data<Head_U, U>, head_p, PowerOfUnit<U, head_p>>>>,
		// case 2: not convertible, just appending to left unit
		intern_conversion<U, Unit<Left_PoUs..., PowerOfUnit<Head_U, head_p>>, Unit<Right_PoUs...>, C_Result>
	>::type type;
};

template< class UnitT >
using apply_intern_conversion = typename intern_conversion<void, UnitT, Unit<>, convert_result_neutral>::type;


// preparation: decomposition of combined units

// helper containing a conversion result
template< class T, int power, typename... Ts >
struct powered_unit_helper;

template< class T, int power, class... Us, int... ps >
struct powered_unit_helper<T, power, Unit<PowerOfUnit<Us, ps>...>>
{
	static constexpr bool is_convertible = true;
	static constexpr double conversion_factor = constexpr_pow(T::conversion_factor, power);
	typedef Unit<> type;
	typedef Unit<PowerOfUnit<Us, power * ps>...> powered_type;
};

// no result placeholder
struct empty_powered_unit
{
	typedef void type;
	typedef Unit<> powered_type;
};

// decomposition of all contained combined types
template< typename... Ts>
struct unit_decomposition;

// base case
template< class C_Result >
struct unit_decomposition<Unit<>, C_Result>
{
	typedef C_Result type;
};

template< class Head_U, int head_p, class... PoUs, class C_Result >
struct unit_decomposition<Unit<PowerOfUnit<Head_U, head_p>, PoUs...>, C_Result>
{
private:
	typedef std::conditional_t<Head_U::is_combined_unit, powered_unit_helper<Head_U, head_p,
		typename Head_U::decomposition_type>, empty_powered_unit> powered_result_type;

public:
	typedef typename unit_decomposition<Unit<PoUs...>, combine_results<C_Result,
		std::conditional_t<Head_U::is_combined_unit,
			// case 1: is combined unit, append decomposition and call recursively
			typename unit_decomposition<typename powered_result_type::powered_type, powered_result_type>::type,
			// case 2: is not combined, append unchanged
			convert_result_helper<convert_result_neutral, 0, PowerOfUnit<Head_U, head_p>>>>
		>::type type;
};

template< class UnitT >
using apply_decomposition = typename unit_decomposition<UnitT, convert_result_neutral>::type;


// sum up the steps
template< class Unit1, class Unit2 >
struct unit_conversion
{
private:
	typedef apply_decomposition<Unit1> dec1;
	typedef apply_decomposition<Unit2> dec2;
	typedef apply_intern_conversion<typename dec1::type> int1;
	typedef apply_intern_conversion<typename dec2::type> int2;
	typedef search_conversion<typename int1::type, typename int2::type, convert_result_neutral> search_result;

public:
	static constexpr bool is_convertible = search_result::is_convertible;
	static constexpr double conversion_factor = dec1::conversion_factor * int1::conversion_factor
		* search_result::conversion_factor / dec2::conversion_factor / int2::conversion_factor;
};


template< class U1, class U2 >
using get_factor_if_convertible = std::enable_if_t<search_conversion<U1, U2, convert_result_neutral>::is_convertible,
	search_conversion<U1, U2, convert_result_neutral>>;

// ----

// core metaprogramming class representing a unit
template<>
class Unit<>
{
protected:
	const double val;

	constexpr Unit<>(double val) : val(val) {}
};

template< class... Us, int... powers >
class Unit<PowerOfUnit<Us, powers>...>
{
protected:
	const double val;

	constexpr explicit Unit<PowerOfUnit<Us, powers>...>(double val) : val(val) {}
};


// wrapper for unit, adding the conversion policy
template<ConversionPolicy policy>
class PUnit<policy> : Unit<>
{
public:
	constexpr PUnit<policy>(double val) : Unit<>(val) {}

	constexpr operator double() const { return Unit<>::val; }

	constexpr double value() const { return Unit<>::val; }

	// necessary to restrict conversions to stricter policy?
	template< ConversionPolicy new_p, typename = std::enable_if_t<new_p <= policy> >
	constexpr operator PUnit<new_p>()
	{
		return PUnit<new_p>(value());
	}
};

template< ConversionPolicy policy, class... PoUs>
class PUnit<policy, PoUs...> : Unit<PoUs...>
{
public:
	constexpr explicit PUnit<policy, PoUs...>(double val) : Unit<PoUs...>(val) {}

	constexpr double value() const { return  Unit<PoUs...>::val; }

	template< ConversionPolicy new_p, class... NewPoUs, typename ConversionT = unit_conversion<Unit<PoUs...>, Unit<NewPoUs...>>,
		typename = std::enable_if_t<policy == ConversionPolicy::ExplicitConversion && (new_p <= policy) && ConversionT::is_convertible> >
	constexpr explicit operator PUnit<new_p, NewPoUs...>() const
	{
		return PUnit<new_p, NewPoUs...>(ConversionT::conversion_factor * value());
	}

	template< ConversionPolicy new_p, class... NewPoUs, typename ConversionT = unit_conversion<Unit<PoUs...>, Unit<NewPoUs...>>,
		typename = std::enable_if_t<policy == ConversionPolicy::ImplicitConversion && (new_p <= policy) && ConversionT::is_convertible>, typename = void >
	constexpr operator PUnit<new_p, NewPoUs...>() const
	{
		return PUnit<new_p, NewPoUs...>(ConversionT::conversion_factor * value());
	}
};


// operators
template<  ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator+ (PUnit<p, PoUs...> left, PUnit<p, PoUs...> right)
{
	return PUnit<p, PoUs...>(left.value() + right.value());
}

template<  ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator- (PUnit<p, PoUs...> left, PUnit<p, PoUs...> right)
{
	return PUnit<p, PoUs...>(left.value() - right.value());
}

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr mult_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p> operator* (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return mult_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p>(left.value() * right.value());
}

template< ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator* (double left, PUnit<p, PoUs...> right)
{
	return PUnit<p, PoUs...>(left * right.value());
}

template< ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator* (PUnit<p, PoUs...> left, double right)
{
	return PUnit<p, PoUs...>(left.value() * right);
}

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr div_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p> operator/ (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return div_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p>(left.value() / right.value());
}

template< ConversionPolicy p, class... PoUs >
constexpr div_punits_t<Unit<>, Unit<PoUs...>, p> operator/ (double left, PUnit<p, PoUs...> right)
{
	return div_punits_t<Unit<>, Unit<PoUs...>, p>(left / right.value());
}

template< ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator/ (PUnit<p, PoUs...> left, double right)
{
	return PUnit<p, PoUs...>(left.value() / right);
}

// conctruction functions
template< class... PoUs, ConversionPolicy p >
constexpr PUnit<p, PoUs...> makeUnit(double val, PUnit<p, PoUs...>)
{
	return PUnit<p, PoUs...>(val);
}

template< ConversionPolicy policy, class... PoUs, ConversionPolicy p >
constexpr PUnit<policy, PoUs...> makeUnit(double val, PUnit<p, PoUs...>)
{
	return PUnit<policy, PoUs...>(val);
}
