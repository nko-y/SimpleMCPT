#pragma once

#include <map>
#include <vector>

#include "Ray.h"
#include "Hittable.h"
#include "Triangle.h"

class Emittable
{
public:
	virtual float Area() = 0;
	virtual float SampleRay(HitRecord& rec, HitRecord& light_rec, Hittable* world, vec3& wi) = 0;
	virtual float pdf(HitRecord& rec, Hittable* world, Ray& r) = 0;
};
