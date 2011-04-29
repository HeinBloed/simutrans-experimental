/*
 * fraction32_t.h
 *
 *  Created on: 27.04.2011
 *      Author: Bernd Gabriel
 */

#ifndef FRACTION32_T_H_
#define FRACTION32_T_H_

#include <math.h>

typedef long int32;

const int32 common_factor(int32 a, int32 b);
const int ild(int32 x);

inline int32 min(int32 a, int32 b)
{
	if (a <= b)
		return a;
	return b;
}

inline int32 max(int32 a, int32 b)
{
	if (a >= b)
		return a;
	return b;
}

class fraction32_t {
public:
	int32 n;
	int32 d;

	// constructors

	inline fraction32_t() {}

	inline fraction32_t(int32 nominator, int32 denominator)
	{
		n = nominator;
		d = denominator;
	}

	inline fraction32_t(int value)
	{
		n = value;
		d = 1;
	}

	inline fraction32_t(int32 value)
	{
		n = value;
		d = 1;
	}

	fraction32_t(double value);

	// operators: additon and subtraction

	const fraction32_t & operator += (const fraction32_t &f);

	inline const fraction32_t & operator -= (const fraction32_t &f)
	{
		return *this += fraction32_t(-f.n, f.d);
	}

	inline const fraction32_t operator + (const fraction32_t &f) const
	{
		fraction32_t r(n, d);
		return r += f;
	}

	inline const fraction32_t operator - (const fraction32_t &f) const
	{
		fraction32_t r(n, d);
		return r += fraction32_t(-f.n, f.d);
	}

	inline const fraction32_t & operator += (int32 value)
	{
		return *this += fraction32_t(value, 1);
	}

	inline const fraction32_t & operator -= (int32 value)
	{
		return *this += fraction32_t(value, -1);
	}

	inline const fraction32_t operator + (int32 value) const
	{
		fraction32_t r(n, d);
		return r += fraction32_t(value, 1);
	}

	inline const fraction32_t operator - (int32 value) const
	{
		fraction32_t r(n, d);
		return r += fraction32_t(value, -1);
	}

	// operators: multiplication and division

	const fraction32_t & operator *= (const fraction32_t &f);

	inline const fraction32_t & operator /= (const fraction32_t &f)
	{
		return *this *= fraction32_t(f.d, f.n);
	}

	inline const fraction32_t operator * (const fraction32_t &f) const
	{
		fraction32_t r(n, d);
		return r *= f;
	}

	inline const fraction32_t operator / (const fraction32_t &f) const
	{
		fraction32_t r(n, d);
		return r *= fraction32_t(f.d, f.n);
	}

	inline const fraction32_t & operator *= (int32 value)
	{
		return *this *= fraction32_t(value, 1);
	}

	inline const fraction32_t & operator /= (int32 value)
	{
		return *this *= fraction32_t(1, value);
	}

	inline const fraction32_t operator * (int32 value) const
	{
		fraction32_t r(n, d);
		return r *= fraction32_t(value, 1);
	}

	inline const fraction32_t operator / (int32 value) const
	{
		fraction32_t r(n, d);
		return r *= fraction32_t(1, value);
	}

	// operators: comparators

	inline bool operator < (const fraction32_t &f) const
	{
		if (n < f.n && d >= f.d)
			return true;
		if (n == f.n && d > f.d)
			return true;
		fraction32_t r = *this / f;
		r.shrink();
		return r.n < r.d;
	}

	inline bool operator <= (const fraction32_t &f) const
	{
		if (n <= f.n && d >= f.d)
			return true;
		fraction32_t r = *this / f;
		r.shrink();
		return r.n <= r.d;
	}

	inline bool operator == (const fraction32_t &f) const
	{
		if (n == f.n && d == f.d)
			return true;
		fraction32_t r = *this / f;
		r.shrink();
		return r.n == r.d;
	}

	inline bool operator != (const fraction32_t &f) const
	{
		if (n == f.n && d == f.d)
			return false;
		fraction32_t r = *this / f;
		r.shrink();
		return r.n != r.d;
	}

	inline bool operator >= (const fraction32_t &f) const
	{
		if (n >= f.n && d <= f.d)
			return true;
		fraction32_t r = *this / f;
		r.shrink();
		return r.n >= r.d;
	}

	inline bool operator > (const fraction32_t &f) const
	{
		if (n > f.n && d <= f.d)
			return true;
		if (n == f.n && d < f.d)
			return true;
		fraction32_t r = *this / f;
		r.shrink();
		return r.n > r.d;
	}

	// shorten and shrink
	//
	// shorten() without loss of the exact value, but requires a common factor in nominator and denominator.
	// shrink() with loss of the exact value. average deviation: 1 / 2^16

	inline const fraction32_t shorten() const
	{
		fraction32_t r;
		int32 factor = common_factor(n, d);
		if (factor > 1)
		{
			r.n = n / factor;
			r.d = d / factor;
		}
		else
		{
			r.n = n;
			r.d = d;
		}
		return r;
	}

	inline const fraction32_t & shorten()
	{
		int32 factor = common_factor(n, d);
		if (factor > 1)
		{
			n /= factor;
			d /= factor;
		}
		return *this;
	}

	const fraction32_t shrink() const;

	const fraction32_t & shrink();
};

const fraction32_t log(const fraction32_t &x);
const fraction32_t exp(const fraction32_t &x);

inline const fraction32_t pow(const fraction32_t &base, const fraction32_t &expo)
{
	return exp(expo * log(base));
}


#endif /* FRACTION_T_H_ */