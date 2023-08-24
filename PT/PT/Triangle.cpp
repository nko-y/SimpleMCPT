#include "Triangle.h"


Triangle::Triangle(vec3 in_p[3]) {
	for (int i = 0; i < 3; i++) {
		p[i] = in_p[i];
	}
	n = normalize(cross(p[1] - p[0], p[2] - p[0]));
	area = 0.5 * fabsf(length(cross(p[1] - p[0], p[2] - p[0])));
	has_uv = false;
	mtl = nullptr;
	for (int i = 0; i < 3; i++) {
		box.UpdateBox(p[i]);
	}
}

void Triangle::SetUV(vec2 in_uv[3]) {
	for (int i = 0; i < 3; i++) {
		uv[i] = in_uv[i];
	}
	has_uv = true;
}

BoundingBox Triangle::GetBoundingBox() {
	return this->box;
}

bool Triangle::hit(Ray& r, float& t_min, float& t_max, HitRecord& rec) {
	const float EPSILON = 0.0000001;
	vec3 vertex0 = p[0];
	vec3 vertex1 = p[1];
	vec3 vertex2 = p[2];
	vec3 edge1, edge2, h, s, q;
	float a, f, u, v, w;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	h = cross(r.dir, edge2);
	a = dot(edge1, h);

	if (a > -EPSILON && a < EPSILON)
		return false;
	f = 1.0 / a;
	s = r.orig - vertex0;
	u = f * dot(s, h);
	if (u < 0.0f || u > 1.0f)
		return false;
	q = cross(s, edge1);
	v = f * dot(r.dir, q);
	if (v < 0.0f || u + v > 1.0f)
		return false;
	w = 1.0 - u - v;
	float t = f * dot(edge2, q);
	if (t > t_min && t < t_max) {
		rec.p = r.at(t);
		rec.t = t;
		rec.n = n;
		rec.uv = uv[0] * w + uv[1] * u + uv[2] * v;
		rec.mtl = mtl;
		rec.obj = static_cast<Hittable*>(this);

		return true;
	}
	else
	{
		return false;
	}
}


float Triangle::Area() {
	return this->area;
}

// https://math.stackexchange.com/questions/538458/how-to-sample-points-on-a-triangle-surface-in-3d
vec3 Triangle::SampleOnePoint() {
	float sqrt_a = sqrtf(get_uniform_random());
	float b = get_uniform_random();
	return p[0] * (1.0f - sqrt_a) + p[1] * (sqrt_a * (1 - b)) + p[2] * (sqrt_a * b);
}

float Triangle::SampleRay(HitRecord& rec, HitRecord& light_rec, Hittable* world, vec3& wi) {
	float pdf = 0;
	
	vec3 sample_point = SampleOnePoint();
	wi = sample_point - rec.p;
	Ray light_ray(rec.p, wi);
	wi = light_ray.dir;

	float t_min = 0.001;
	float t_max = INFINITY;
	float t_near = 0.0000001;
	if (world->hit(light_ray, t_min, t_max, light_rec)) {
		if (light_rec.mtl->mtl_type==2 && dot(light_rec.p - sample_point, light_rec.p - sample_point) < t_near) {
			if (dot(wi, rec.n) > 0 && dot(wi, light_rec.n) < 0) {
				vec3 dist = rec.p - light_rec.p;
				float area = Area();
				pdf = dot(dist, dist) / (area * dot(-wi, light_rec.n));
			}
		}
	}
	
	return pdf;
}