#ifndef I3D_UTILITY_H
#define I3D_UTILITY_H

#define _USE_MATH_DEFINES
#include <cmath>

#include "Vector3D.h"
#include <random>

namespace utility {
	const float pi = std::acosf(-1.0);

	int randSign();
	float randFloat(float a, float b);
	int randInt(int a, int b);
	
	float toRadians(float angle);
	float toDegrees(float angle);

	float mapToRange(float value, float old_min, float old_max, float new_min, float new_max);

	static std::random_device engine;
}

#endif