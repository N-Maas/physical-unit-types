#pragma once
// file containing the metaprogramming structures that are necessary for UnitCore.h

// prevents warnings for the .hpp file
// the include gets removed by pragma once
#include "UnitCore.h"

XPU_NAMESPACE_BEGIN(helpers)

// constexpr pow for calculating unit powers
constexpr double constexpr_pow(double val, int exp)
{
	if (exp == 0) {
		return 1;
	}
	else if (exp > 0) {
		return val * constexpr_pow(val, exp - 1);
	}
	return constexpr_pow(val, exp + 1) / val;
};

// generate string of a unit type
template< ConversionPolicy p >
std::string unit_name(PUnit<p>) {
	return "";
}

template< class Head, int power, class... PoUs, ConversionPolicy p >
std::string unit_name(PUnit<p, PowerOfUnit<Head, power>, PoUs...>) {
	std::string tail = unit_name(PUnit<p, PoUs...>(0));
	return Head::unitName() + (power == 1 ? std::string("") : (std::string("^")
		+ std::to_string(power))) + (tail.empty() ? std::string("") : std::string("*") + tail);
}

// HELPERS
template< class, ConversionPolicy >
struct to_punit;

template< class... PoUs, ConversionPolicy p >
struct to_punit< Unit<PoUs...>, p >
{
	typedef PUnit<p, PoUs...> type;
};

template< class >
struct to_unit;

template< class... PoUs, ConversionPolicy p >
struct to_unit< PUnit<p, PoUs...> >
{
	typedef Unit<PoUs...> type;
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
template< class... R_PoUs >
struct mult_units<Unit<R_PoUs...>, Unit<>, Unit<>>
{
	typedef Unit<R_PoUs...> type;
};

// base case 2
template< class... R_PoUs, class... PoUs >
struct mult_units<Unit<R_PoUs...>, Unit<>, Unit<PoUs...>>
{
	typedef Unit<R_PoUs..., PoUs...> type;
};

// base case 3
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
		typename mult_units<typename non_zero_append_to_unit<Unit<R_PoUs...>,
			PowerOfUnit<typename H1::unit_type, H1::power + H2::power>>::type, Unit<PoU1s...>, Unit<PoU2s...>>::type,
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

// helpers containing (intermediate) results of unit decomposition
template< class... PoUs >
struct neutral_result
{
	static constexpr double conversion_factor = 1;
	typedef Unit<PoUs...> type;
	typedef Unit<> powered_type;
};

template< class T, int power, typename... Ts >
struct powered_result;

template< class T, int power, class... Us, int... ps >
struct powered_result<T, power, Unit<PowerOfUnit<Us, ps>...>>
{
	static constexpr double conversion_factor = constexpr_pow(T::conversion_factor, power);
	typedef Unit<> type;
	typedef Unit<PowerOfUnit<Us, power * ps>...> powered_type;
};

// combine two results, multiplying factors and concatenating types
template< class R1, class R2 >
struct combine_results
{
	static constexpr double conversion_factor = R1::conversion_factor * R2::conversion_factor;
	typedef mult_units_t<typename R1::type, typename R2::type> type;
};

// algorithm for decomposition into unit type consisting of base units
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
	typedef std::conditional_t<Head_U::is_combined_unit,
			powered_result<Head_U, head_p, typename Head_U::decomposition_type>,
			neutral_result<>>
		p_result_t;

public:
	// double recursion: on the decomposed head (for higher composition depth) and on the tail, combining results
	typedef typename unit_decomposition<Unit<PoUs...>, combine_results<C_Result,
		std::conditional_t<Head_U::is_combined_unit,
			// case 1: is combined unit, append decomposition and call recursively
			typename unit_decomposition<typename p_result_t::powered_type, p_result_t>::type,
			// case 2: is not combined, append unchanged
			neutral_result<PowerOfUnit<Head_U, head_p>>>>
		>::type type;
};

template< class UnitT >
using apply_decomposition = typename unit_decomposition<UnitT, neutral_result<>>::type;

// conversion using unambiguous decomposition of both units
template< class Unit1, class Unit2 >
struct unit_conversion
{
private:
	typedef apply_decomposition<Unit1> left;
	typedef apply_decomposition<Unit2> right;

public:
	static constexpr bool is_convertible = std::is_same_v<typename left::type, typename right::type>;
	static constexpr double conversion_factor = left::conversion_factor / right::conversion_factor;
};

XPU_NAMESPACE_END(helpers)
