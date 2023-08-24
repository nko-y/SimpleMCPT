#pragma once

#include <glm/glm.hpp>
using namespace glm;

#include "Ray.h"

class BoundingBox {
public:
	BoundingBox() {
		min_v = vec3(INFINITY, INFINITY, INFINITY);
		max_v = vec3(-INFINITY, -INFINITY, -INFINITY);
	};

	void UpdateBox(vec3& v) {
		for (int i = 0; i < 3; i++) {
			min_v[i] = v[i] < min_v[i] ? v[i] : min_v[i];
			max_v[i] = v[i] > max_v[i] ? v[i] : max_v[i];
		}
	}

	void UpdateBox(BoundingBox& b) {
		this->UpdateBox(b.min_v);
		this->UpdateBox(b.max_v);
	}

	bool HitTest(Ray& r, float t_min, float t_max, float& t_hit) {
		for (int i = 0; i < 3; i++) {
			float tcmin = (min_v[i] - r.orig[i]) / r.dir[i];
			float tcmax = (max_v[i] - r.orig[i]) / r.dir[i];
			if (r.dir[i] < 0) {
				float tp = tcmin;
				tcmin = tcmax;
				tcmax = tp;
			}
			t_min = tcmin > t_min ? tcmin : t_min;
			t_max = tcmax < t_max ? tcmax : t_max;
			if (t_max < t_min) return false;
		}
		t_hit = t_min;
		return true;
	}

	vec3 min_v;
	vec3 max_v;
};