#pragma once

#include <glm/glm.hpp>
using namespace glm;

#include "Ray.h"

class Camera
{
public:
	Camera() {};
	Camera(vec3 t_eye, vec3 t_up, vec3 t_lookat, float t_fovy, int t_w, int t_h);
	Ray CastRay(int x, int y);

	vec3 eye;
	vec3 up;
	vec3 lookat;
	float fovy;
	float aspect;

	int width, height;
	vec3 dw, dh;
	vec3 left_top_corner;
};

