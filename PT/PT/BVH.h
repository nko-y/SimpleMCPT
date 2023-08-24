#pragma once

#include <vector>
#include <algorithm>

#include "BoundingBox.h"
#include "Triangle.h"
#include "Hittable.h"

class BVH : public Hittable
{
public:
	BVH();
	BVH(Hittable* obj);
	BVH(BoundingBox& in_box);
	static Hittable* BuildTree(std::vector<Hittable*>& objs, int from, int to);

	template<int dim>
	static bool cmp(Hittable* first, Hittable* second);

	virtual bool hit(Ray& r, float& t_min, float& t_max, HitRecord& rec) override;
	virtual BoundingBox GetBoundingBox() override;

	Hittable* child[2];
	BoundingBox box;
};

