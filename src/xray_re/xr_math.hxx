#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xray_re
{
	template<typename T> static inline T deg2rad(T deg) { return T(deg*M_PI/180.); }

	template<typename T> static inline T clamp(T value, T min, T max)
	{
		return (value < min) ? min : ((value > max) ? max : value);
	}

	template<typename T> inline bool equivalent(T a, T b, T e = T(1e-6))
	{
		return (a < b) ? (b - a < e) : (a - b < e);
	}
}
