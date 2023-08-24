#pragma once

#include <glm/glm.hpp>
using namespace glm;

#include "Material.h"
#include "Ray.h"
#include "BoundingBox.h"

struct HitRecord;
class Hittable;
class Material;

struct HitRecord
{
	vec3 p;
	vec3 n;
	float t;
	vec2 uv;
	Material* mtl;
	Hittable* obj;
};

class Hittable
{
public:
	virtual bool hit(Ray& r, float& t_min, float& t_max, HitRecord& rec) = 0;
	virtual BoundingBox GetBoundingBox() = 0;
};

