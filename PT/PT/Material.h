#pragma once

#include <iostream>
#include <glm/glm.hpp>
using namespace glm;

#include "Hittable.h"
#include "Tool.h"
struct HitRecord;

class Material
{
public:
	int mtl_type;	//0-Phong 1-Glass 2-Light
	virtual float pdf(vec3& wo, vec3& wi, HitRecord& rec) = 0;
	virtual vec3 bsdf(vec3& wo, vec3& wi, HitRecord& rec) = 0;
	virtual float scatter(vec3& wo, vec3& wi, HitRecord& rec) = 0;
};

class Texture {
public:
	Texture() { Texture(0, 0, nullptr); };
	Texture(int twidth, int theight, float* tdata, int tchannels=3);
	vec3 Get(vec2& uv);

	int width, height;
	float* data;
	int channels;
};

class PhongMaterial : public Material
{
public:
	PhongMaterial() {};
	PhongMaterial(vec3 t_Kd, vec3 t_Ks, float t_Ns);
	void SetEmissive(vec3 t_Ke);

	virtual float pdf(vec3& wo, vec3& wi, HitRecord& rec) override;
	virtual vec3 bsdf(vec3& wo, vec3& wi, HitRecord& rec) override;
	virtual float scatter(vec3& wo, vec3& wi, HitRecord& rec) override;

	float GetLambertianPDF(vec3& wo, vec3& wi, vec3& norm);
	float GetSpecularPDF(vec3& wo, vec3& wi, vec3& norm);
	float SampleLambertian(vec3& wo, vec3& wi, HitRecord& rec);
	float SampleSpecular(vec3& wo, vec3& wi, HitRecord& rec);

	vec3 local2world(vec3& p_local, vec3& axis_z);

	vec3 Kd;
	vec3 Ks;
	float Ns;

	vec3 Ke;
	bool is_emissive;

	Texture *texture;
	bool is_texture;

	int PhongOrBling = 1;//Phong-0, BlingPhong-1
};

class GlassMaterial : public Material
{
public:
	GlassMaterial() { GlassMaterial(0); };
	GlassMaterial(float tNi);

	virtual float pdf(vec3& wo, vec3& wi, HitRecord& rec) override { return 1.0; }
	virtual vec3 bsdf(vec3& wo, vec3& wi, HitRecord& rec) override {
		if (dot(wo, rec.n) * dot(wi, rec.n) > 0)
			return vec3(1.0, 1.0, 1.0);
		else return Tr;
	}
	virtual float scatter(vec3& wo, vec3& wi, HitRecord& rec) override;

	vec3 Tr;
	float Ni;
};
