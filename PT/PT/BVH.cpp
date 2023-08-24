#include "BVH.h"

BVH::BVH() {
	child[0] = child[1] = nullptr;
};

BVH::BVH(Hittable* obj) {
	box = obj->GetBoundingBox();
	child[0] = child[1] = nullptr;
}

BVH::BVH(BoundingBox& in_box) {
	box = in_box;
	child[0] = child[1] = nullptr;
}

bool BVH::hit(Ray& r, float& t_min, float& t_max, HitRecord& rec) {
	BoundingBox child_box[2];
	child_box[0] = child[0]->GetBoundingBox();
	child_box[1] = child[1]->GetBoundingBox();

	bool is_hit[2];
	float t_hit[2];
	is_hit[0] = child_box[0].HitTest(r, t_min, t_max, t_hit[0]);
	is_hit[1] = child_box[1].HitTest(r, t_min, t_max, t_hit[1]);

	if (is_hit[0] && is_hit[1]) {
		int near, far;
		if (t_hit[0] < t_hit[1]) near = 0;
		else near = 1;
		far = 1 - near;
		if (child[near]->hit(r, t_min, t_max, rec)) {
			if (rec.t < t_hit[far])
				return true;
			HitRecord tp_far_rec;
			if (child[far]->hit(r, t_min, t_max, tp_far_rec) && tp_far_rec.t < rec.t) {
				rec = tp_far_rec;
			}
			return true;
		}
		else
		{
			return child[far]->hit(r, t_min, t_max, rec);
		}
	}

	if (is_hit[0]) return child[0]->hit(r, t_min, t_max, rec);
	if (is_hit[1]) return child[1]->hit(r, t_min, t_max, rec);

	return false;
}

BoundingBox BVH::GetBoundingBox() {
	return this->box;
}

template<int dim>
bool BVH::cmp(Hittable* first, Hittable* second) {
	return first->GetBoundingBox().min_v[dim] < second->GetBoundingBox().min_v[dim];
}

Hittable* BVH::BuildTree(std::vector<Hittable*>& objs, int from, int to) {
	if (to <= from) return nullptr;
	if (to - from == 1) return objs[from];

	BVH* node = new BVH();
	for (int i = from; i < to; i++) {
		BoundingBox tp = objs[i]->GetBoundingBox();
		node->box.UpdateBox(tp);
	}

	vec3 b_size = node->box.max_v - node->box.min_v;
	int max_idx = 0;
	for (int i = 0; i < 3; i++) {
		if (b_size[i] > b_size[max_idx]) {
			max_idx = i;
		}
	}

	auto cmp_func = cmp<0>;
	if (max_idx == 1) cmp_func = cmp<1>;
	else if (max_idx == 2) cmp_func = cmp<2>;
	std::sort(objs.begin() + from, objs.begin() + to, cmp_func);

	int middle_idx = (from + to) / 2;
	node->child[0] = BuildTree(objs, from, middle_idx);
	node->child[1] = BuildTree(objs, middle_idx, to);

	return static_cast<Hittable*>(node);
}