#include "Material.h"

Texture::Texture(int twidth, int theight, float* tdata, int tchannels) {
	data = tdata;
	width = twidth;
	height = theight;
	channels = tchannels;
}

vec3 Texture::Get(vec2& uv) {
	float f_w = uv[0] * width;
	float f_h = (1.0 - uv[1]) * height;
	int w = (int)round(f_w);
	int h = (int)round(f_h);
	int idx = ((h % height + height) % height * width + (w % width + width) % width) * channels;
	return vec3(data[idx], data[idx + 1], data[idx + 2]);
}

PhongMaterial::PhongMaterial(vec3 t_Kd, vec3 t_Ks, float t_Ns) {
	texture = nullptr;

	Kd = t_Kd;
	Ks = t_Ks;
	Ns = t_Ns;

	is_emissive = false;
	is_texture = false;

	mtl_type = 0;
}

void PhongMaterial::SetEmissive(vec3 t_Ke) {
	is_emissive = true;
	Ke = t_Ke;
	mtl_type = 2;
}

vec3 PhongMaterial::local2world(vec3& p_local, vec3& axis_z) {
	vec3 axis_x, axis_y;
	do {
		axis_y = vec3(get_uniform_random(-1, 1), get_uniform_random(-1, 1), get_uniform_random(-1, 1));
	} while (glm::length(cross(axis_y, axis_z)) < 0.0000001);
	axis_y = normalize(cross(axis_y, axis_z));
	axis_x = normalize(cross(axis_y, axis_z));
	mat3 l2w = mat3(axis_x, axis_y, axis_z);
	return l2w * p_local;
}

float PhongMaterial::GetLambertianPDF(vec3& wo, vec3& wi, vec3& norm) {
	float pdf = dot(wi, norm) / Pi;
	if (pdf < 0) return 0;
	else return pdf;
}

float PhongMaterial::SampleLambertian(vec3& wo, vec3& wi, HitRecord& rec) {
	float cos_phi = cosf(get_uniform_random(0, 2 * Pi));
	float cos_theta = sqrtf(1.0f - get_uniform_random());
	float sin_phi = sqrtf(1.0f - cos_phi * cos_phi);
	float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
	vec3 p_local(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);

	wi = local2world(p_local, rec.n);

	if (dot(wi, rec.n) <= 0) return 0;
	return cos_theta / Pi;
}

float PhongMaterial::GetSpecularPDF(vec3& wo, vec3& wi, vec3& norm) {
	float pdf = 0;
	if (dot(norm, wi) <= 0) return 0.0;
	if (PhongOrBling==0) {
		//Phong Model
		vec3 true_wi = 2 * dot(wo, norm) * norm - wo;
		float val = dot(wi, true_wi);
		if (val <= 0) pdf = 0;
		else pdf = (Ns + 1) / (2 * Pi) * powf(val, Ns);
	}
	else {
		//Bling Phong Model
		vec3 half_vector = normalize(wo + wi);
		pdf = (Ns + 2) / (2 * Pi) * powf(dot(norm, half_vector), Ns+1);
		pdf = pdf / (4 * dot(half_vector, wo));
	}
	return pdf;
}

float PhongMaterial::SampleSpecular(vec3& wo, vec3& wi, HitRecord& rec) {
	float cos_phi = cosf(get_uniform_random(0, 2 * Pi));
	float cos_theta = powf(1 - get_uniform_random(), 1.0f / (Ns + 2));
	float sin_phi = sqrtf(1.0f - cos_phi * cos_phi);
	float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
	vec3 p_local(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
	float pdf;

	if (PhongOrBling == 0) {
		vec3 true_wi = 2 * dot(wo, rec.n) * rec.n - wo;
		wi = local2world(p_local, true_wi);
		pdf = (Ns + 1) / (2 * Pi) * powf(cos_theta, Ns);
	}
	else
	{
		vec3 half_vector = local2world(p_local, rec.n);
		float cos_half_vector = dot(half_vector, wo);
		wi = 2.0f * cos_half_vector * half_vector - wo;
		pdf = (Ns + 2) / (2 * Pi) * powf(cos_theta, Ns+1);
		pdf = pdf / (4 * cos_half_vector);
	}

	if (dot(rec.n, wi) < 0) pdf = 0;
	return pdf;
}

float PhongMaterial::pdf(vec3& wo, vec3& wi, HitRecord& rec) {
	vec3 res_kd = Kd;
	if (is_texture) res_kd = texture->Get(rec.uv);
	if (dot(rec.n, wi) < 0) return 0.0;

	float Kd_max = -1, Ks_max = -1, prob_sum;
	Ks_max = std::log10f(Ns);
	for (int i = 0; i < 3; i++) {
		if (res_kd[i] > Kd_max) Kd_max = res_kd[i];
		//if (Ks[i] > Ks_max) Ks_max = Ks[i];
	}
	prob_sum = Kd_max + Ks_max;
	if (prob_sum < 0) return 0;
	float pdf = Kd_max / prob_sum * GetLambertianPDF(wo, wi, rec.n) + (1 - Kd_max / prob_sum) * GetSpecularPDF(wo, wi, rec.n);
	return pdf;
}

vec3 PhongMaterial::bsdf(vec3& wo, vec3& wi, HitRecord& rec) {
	if (dot(wi, rec.n) <= 0) return vec3(0, 0, 0);
	//printvec3(wi);
	vec3 res_kd = Kd;
	if (is_texture) res_kd = texture->Get(rec.uv);
	vec3 factor;

	if(PhongOrBling==0){
		vec3 true_wo = 2 * dot(wi, rec.n) * rec.n - wi;
		factor = res_kd + 0.5f * Ks * (Ns + 2) * powf(dot(true_wo, wo), Ns);
		factor = factor / Pi;
	}
	else
	{
		vec3 half_vector = normalize(wo + wi);
		//factor = res_kd + 0.125f * Ks * (Ns + 2) * powf(dot(half_vector, rec.n), Ns);
		//farbrausch
		factor = res_kd + 0.125f * Ks * (Ns + 2) * (Ns + 4)  * powf(dot(half_vector, rec.n), Ns) / (Ns + powf(2, -Ns / 2));
		factor = factor / Pi;
	}
	return factor;
}

float PhongMaterial::scatter(vec3& wo, vec3& wi, HitRecord& rec) {
	vec3 res_kd = Kd;
	if (is_texture) res_kd = texture->Get(rec.uv);

	float Kd_max = -1, Ks_max = -1, prob_sum;
	Ks_max = std::log10f(Ns);
	for (int i = 0; i < 3; i++) {
		if (res_kd[i] > Kd_max) Kd_max = res_kd[i];
		//if (Ks[i] > Ks_max) Ks_max = Ks[i];
	}

	prob_sum = Kd_max + Ks_max;
	if (prob_sum < 0) return 0;

	float pdf_lambersion, pdf_specular;
	float prob_sample_lambertian = Kd_max / prob_sum;
	if (get_uniform_random() < prob_sample_lambertian) {
		pdf_lambersion = SampleLambertian(wo, wi, rec);
		pdf_specular = GetSpecularPDF(wo, wi, rec.n);
	}
	else {
		pdf_specular = SampleSpecular(wo, wi, rec);
		pdf_lambersion = GetLambertianPDF(wo, wi, rec.n);
	}

	if (dot(wi, rec.n) <= 0) return 0;

	float pdff = prob_sample_lambertian * pdf_lambersion + (1 - prob_sample_lambertian) * pdf_specular;
	return pdff;
}

GlassMaterial::GlassMaterial(float tNi) {
	Ni = tNi;
	mtl_type = 1;
}


float GlassMaterial::scatter(vec3& wo, vec3& wi, HitRecord& rec) {
	float ior = Ni;
	if (dot(rec.n, wo) > 0) ior = 1.0 / ior;

	vec3 norm = rec.n;
	if (dot(norm, wo) < 0) norm = -norm;
	float cos_theta = dot(norm, wo);
	float sin_theta = sqrtf(1.0 - cos_theta * cos_theta);

	float r0 = (1 - ior) / (1 + ior);
	r0 = r0 * r0;
	r0 = r0 + (1 - r0) * powf(1 - cos_theta, 5);
	if (ior * sin_theta > 1.0 || r0 > get_uniform_random()) {
		wi = 2.0f * dot(norm, wo) * norm - wo;
	}
	else
	{
		float ct = dot(wo, norm);
		vec3 x_dir = ior * (-wo + ct * norm);
		vec3 y_dir = -sqrtf(1.0 - dot(x_dir, x_dir)) * norm;
		wi = x_dir + y_dir;
	}

	return 1.0; 
}