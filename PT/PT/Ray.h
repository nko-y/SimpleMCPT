#pragma once

#include <glm/glm.hpp>
using namespace glm;

class Ray
{
public:
	Ray() {};
	Ray(vec3& origin, vec3& direction) {
		orig = origin;
		dir = normalize(direction);
	}

	vec3 at(float t) {
		return orig + t * dir;
	}

	vec3 orig;
	vec3 dir;
};