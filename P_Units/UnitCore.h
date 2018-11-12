#pragma once

#include <type_traits>

// macros beginning with XPU_ are for internal usage (macros for the user begin with PUNITS_)
#define XPU_NAMESPACE_BEGIN(x) namespace x {
#define XPU_NAMESPACE_END(x) }

// enables implicit application of an operator requiring equal unit types (like +, <=, ...)
#define XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(x_op) \
	template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs, typename = std::enable_if_t< \
		!std::is_same_v<PUnit<p, Left_PoUs...>, PUnit<ConversionPolicy::ImplicitConversion, Right_PoUs...>> && \
		helpers::unit_conversion<Unit<Right_PoUs...>, Unit<Left_PoUs...>>::is_convertible>> \
	constexpr auto operator x_op (PUnit<p, Left_PoUs...> left, PUnit<ConversionPolicy::ImplicitConversion, Right_PoUs...> right) \
	{ \
		return left x_op PUnit<p, Left_PoUs...>(right); \
	} \
	\
	template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs, typename = std::enable_if_t< \
		p != ConversionPolicy::ImplicitConversion && helpers::unit_conversion<Unit<Right_PoUs...>, Unit<Left_PoUs...>>::is_convertible>> \
	constexpr auto operator x_op (PUnit<ConversionPolicy::ImplicitConversion, Left_PoUs...> left, PUnit<p, Right_PoUs...> right) \
	{ \
		return PUnit<p, Right_PoUs...>(left) x_op right; \
	}


// helper macro for unit definition
#define XPU_DEF_UNIT_HELPER(x_uid, x_uname, x_is_comb, x_cf, x_dct, x_ualias, x_upolicy) \
	XPU_NAMESPACE_BEGIN(punits) XPU_NAMESPACE_BEGIN(definitions) \
	struct x_uname \
	{ \
		static constexpr std::size_t unit_id = x_uid; \
		static constexpr bool is_combined_unit = x_is_comb; \
		static constexpr double conversion_factor = x_cf; \
		typedef x_dct decomposition_type; \
	}; \
	constexpr PUnit<punits::ConversionPolicy::x_upolicy, punits::PowerOfUnit<x_uname, 1>> x_ualias{ 1.0 }; \
	XPU_NAMESPACE_END(definitions) XPU_NAMESPACE_END(punits)

// macros for getting unit types
#define UNIT_T(x) std::remove_const_t<decltype(x)>
#define UNIT_T_P(x, policy) punits::helpers::punit_set_policy<UNIT_T(x), policy>::type

// using the namespace containing unit definitions and operators
#define PUNITS_USE_DEFINITIONS using namespace punits::definitions

// macros for unit definitions
// appending _P to the macro name additionally defines the default conversion policy for the unit
#define DEFINE_BASE_UNIT_P(x_uid, x_uname, x_ualias, x_upolicy) \
	XPU_DEF_UNIT_HELPER(x_uid, x_uname, false, 1.0, void, x_ualias, x_upolicy)

#define DEFINE_BASE_UNIT(x_uid, x_uname, x_ualias) \
	DEFINE_BASE_UNIT_P(x_uid, x_uname, x_ualias, ExplicitConversion)

#define DEFINE_DEPENDENT_UNIT_P(x_uid, x_uname, x_ualias, x_udecomposition_alias, x_uconversionfactor, x_upolicy) \
	XPU_DEF_UNIT_HELPER(x_uid, x_uname, true, x_uconversionfactor, typename punits::helpers::to_unit<UNIT_T(x_udecomposition_alias)>::type, x_ualias, x_upolicy)

#define DEFINE_DEPENDENT_UNIT(x_uid, x_uname, x_ualias, x_uda, x_ucf) \
	DEFINE_DEPENDENT_UNIT_P(x_uid, x_uname, x_ualias, x_uda, x_ucf, ExplicitConversion)

XPU_NAMESPACE_BEGIN(punits)

enum class ConversionPolicy
{
	NoConversion,
	ExplicitConversion,
	ImplicitConversion
};

// fwd declarations
template< typename... Ts >
class Unit;

template< ConversionPolicy, typename... Ts >
class PUnit;

template< class U, int pwr >
struct PowerOfUnit
{
	typedef U unit_type;
	static constexpr int power = pwr;
	static constexpr std::size_t unit_id = U::unit_id;
};

#include "UnitCore.hpp"

// core metaprogramming class representing a unit
template<>
class Unit<>
{
protected:
	double val;

	constexpr Unit<>(double val) : val(val) {}
};

template< class... Us, int... powers >
class Unit<PowerOfUnit<Us, powers>...>
{
protected:
	double val;

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
class PUnit : Unit<PoUs...>
{
public:
	constexpr explicit PUnit<policy, PoUs...>(double val) : Unit<PoUs...>(val) {}

	constexpr double value() const { return  Unit<PoUs...>::val; }

	template< ConversionPolicy new_p, class... NewPoUs, typename ConversionT = helpers::unit_conversion<Unit<PoUs...>, Unit<NewPoUs...>>,
		typename = std::enable_if_t<policy == ConversionPolicy::ExplicitConversion && (new_p <= policy) && ConversionT::is_convertible> >
	constexpr explicit operator PUnit<new_p, NewPoUs...>() const
	{
		return PUnit<new_p, NewPoUs...>(ConversionT::conversion_factor * value());
	}

	template< ConversionPolicy new_p, class... NewPoUs, typename ConversionT = helpers::unit_conversion<Unit<PoUs...>, Unit<NewPoUs...>>,
		typename = std::enable_if_t<policy == ConversionPolicy::ImplicitConversion && (new_p <= policy) && ConversionT::is_convertible>, typename = void >
	constexpr operator PUnit<new_p, NewPoUs...>() const
	{
		return PUnit<new_p, NewPoUs...>(ConversionT::conversion_factor * value());
	}
};

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

XPU_NAMESPACE_BEGIN(definitions)

// operators
template< ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator+ (PUnit<p, PoUs...> left, PUnit<p, PoUs...> right)
{
	return PUnit<p, PoUs...>(left.value() + right.value());
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(+)

template< ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator- (PUnit<p, PoUs...> left, PUnit<p, PoUs...> right)
{
	return PUnit<p, PoUs...>(left.value() - right.value());
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(-)

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr helpers::mult_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p> operator* (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return helpers::mult_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p>(left.value() * right.value());
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
constexpr helpers::div_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p> operator/ (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return helpers::div_punits_t<Unit<Left_PoUs...>, Unit<Right_PoUs...>, p>(left.value() / right.value());
}

template< ConversionPolicy p, class... PoUs >
constexpr helpers::div_punits_t<Unit<>, Unit<PoUs...>, p> operator/ (double left, PUnit<p, PoUs...> right)
{
	return helpers::div_punits_t<Unit<>, Unit<PoUs...>, p>(left / right.value());
}

template< ConversionPolicy p, class... PoUs >
constexpr PUnit<p, PoUs...> operator/ (PUnit<p, PoUs...> left, double right)
{
	return PUnit<p, PoUs...>(left.value() / right);
}

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr bool operator< (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return left.value() < right.value();
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(<)

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr bool operator> (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return left.value() > right.value();
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(>)

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr bool operator<= (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return left.value() <= right.value();
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(<=)

template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr bool operator>= (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return left.value() >= right.value();
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(>=)

// should equality comparison really be supported? (floating point...)
template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr bool operator== (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return left.value() == right.value();
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(==)

// should equality comparison really be supported? (floating point...)
template< ConversionPolicy p, class... Left_PoUs, class... Right_PoUs >
constexpr bool operator!= (PUnit<p, Left_PoUs...> left, PUnit<p, Right_PoUs...> right)
{
	return left.value() != right.value();
}

XPU_MAKE_OPERATOR_IMPLICITELY_APPLYABLE(!=)

// with SFINAE guard for implicit application
template< ConversionPolicy left_p, class... Left_PoUs, ConversionPolicy right_p, class... Right_PoUs,
	typename = decltype(std::declval<PUnit<left_p, Left_PoUs...>&>() = std::declval<PUnit<left_p, Left_PoUs...>>() + std::declval<PUnit<right_p, Right_PoUs...>>()) >
PUnit<left_p, Left_PoUs...>& operator+= (PUnit<left_p, Left_PoUs...>& left, PUnit<right_p, Right_PoUs...> right)
{
	return left = left + right;
}

// with SFINAE guard for implicit application
template< ConversionPolicy left_p, class... Left_PoUs, ConversionPolicy right_p, class... Right_PoUs,
	typename = decltype(std::declval<PUnit<left_p, Left_PoUs...>&>() = std::declval<PUnit<left_p, Left_PoUs...>>() - std::declval<PUnit<right_p, Right_PoUs...>>()) >
	PUnit<left_p, Left_PoUs...>& operator-= (PUnit<left_p, Left_PoUs...>& left, PUnit<right_p, Right_PoUs...> right)
{
	return left = left - right;
}

template< ConversionPolicy p, class... PoUs >
PUnit<p, PoUs...>& operator*= (PUnit<p, PoUs...>& left, double right)
{
	return left = left * right;
}

template< ConversionPolicy p, class... PoUs >
PUnit<p, PoUs...>& operator/= (PUnit<p, PoUs...>& left, double right)
{
	return left = left / right;
}

XPU_NAMESPACE_END(definitions)

XPU_NAMESPACE_END(punits)
