#pragma once

const float Pi = 3.14159265358979323846f;

#include <random>
#include <iostream>
#include <glm/glm.hpp>
inline float get_uniform_random(float low=0.0, float high=1.0) {
	std::random_device dev;
	std::mt19937 mt(dev());
	std::uniform_real_distribution<float> urd(low, high);
	return urd(mt);
}

inline void printvec3(glm::vec3 in) {
	std::cout << in[0] << " " << in[1] << " " << in[2] << std::endl;
}