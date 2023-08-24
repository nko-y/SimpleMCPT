#pragma once

#include <iostream>
#include <glm/glm.hpp>
using namespace glm;

#include "Tool.h"
#include "Material.h"
#include "Hittable.h"
#include "Emittable.h"
#include "BoundingBox.h"

class Emittable;

class Triangle : public Hittable, public Emittable
{
public:
	Triangle() {};
	Triangle(vec3 in_p[3]);

	void SetUV(vec2 in_uv[3]);
	vec3 SampleOnePoint();

	virtual bool hit(Ray& r, float& t_min, float& t_max, HitRecord& rec) override;
	virtual BoundingBox GetBoundingBox() override;

	virtual float Area() override;
	virtual float SampleRay(HitRecord& rec, HitRecord& light_rec, Hittable* world, vec3& wi) override;
	virtual float pdf(HitRecord& rec, Hittable* world, Ray& r) override {
		return 1.0;
	}


	vec3 p[3];
	vec3 n;
	vec2 uv[3];
	bool has_uv;
	float area;
	Material* mtl;
	BoundingBox box;
};

class EmittableCluster : public Emittable
{
public:

	void Add(Emittable* emit_obj) {
		int idx = emit_list.size();
		emit_list.push_back(emit_obj);
		emit_idx.insert(std::make_pair(emit_obj, idx));
		idx = sample_prob.size();
		if (idx == 0) sample_prob.push_back(emit_obj->Area());
		else sample_prob.push_back(emit_obj->Area() + sample_prob[idx - 1]);
	}

	virtual float Area() override {
		float area = 0.0;
		for (auto i : emit_list) {
			area += i->Area();
		}
		return area;
	}

	virtual float SampleRay(HitRecord& rec, HitRecord& light_rec, Hittable* world, vec3& wi) override {
		int sample_num = sample_prob.size() - 1;
		float rand_area = get_uniform_random() * sample_prob[sample_num];
		int which = std::lower_bound(sample_prob.begin(), sample_prob.end(), rand_area) - sample_prob.begin();
		
		float pdf = emit_list[which]->SampleRay(rec, light_rec, world, wi);
		if (which == 0) {
			pdf = pdf * sample_prob[which] / sample_prob[sample_num];
		}
		else
		{
			pdf = pdf * (sample_prob[which] - sample_prob[which - 1]) / sample_prob[sample_num];
		}

		return pdf;
	}

	virtual float pdf(HitRecord& rec, Hittable* world, Ray& r) override {
		float pdf = 0.0;
		float t_min = 0.001;
		float t_max = INFINITY;
		if (world->hit(r, t_min, t_max, rec)) {
			if (rec.mtl->mtl_type == 2 && dot(rec.n, r.dir) < 0) {
				auto hit_tri = dynamic_cast<Triangle*>(rec.obj);
				vec3 distance = r.orig - rec.p;
				float area = hit_tri->Area();
				pdf = dot(distance, distance) / (area * dot(-r.dir, rec.n));
				int which = emit_idx.find(hit_tri)->second;
				if (which == 0) {
					pdf = pdf * sample_prob[which] / sample_prob[sample_prob.size() - 1];
				}
				else
				{
					pdf = pdf * (sample_prob[which] - sample_prob[which - 1]) / sample_prob[sample_prob.size() - 1];
				}
			}
		}
		return pdf;
	}

	std::vector<float> sample_prob;
	std::vector<Emittable*> emit_list;
	std::map<Emittable*, int> emit_idx;
};