#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <omp.h>

#include "Material.h"
#include "Triangle.h"
#include "Hittable.h"
#include "Emittable.h"
#include "Camera.h"
#include "ImageBuffer.h"
#include "BVH.h"

class SceneLoader
{
public:
	SceneLoader(std::string scene_path);
	~SceneLoader();
	void Render(int sample_num = 1);
	vec3 SampleLight(vec3& wo, HitRecord& rec);

private:
	void InitVariable();
	void ParseFileName();
	void ReadObjectFile();

	vec3 Li(Ray& r);

	std::string scene_path;
	std::string scene_name;

	std::vector<Material*> mtl_list;
	std::vector<Hittable*> obj_list;
	EmittableCluster light_list;

	Camera* cam;
	ImageBuffer* buf;
	Hittable* bvh_tree;

	float epsilon = 0.00000001;
	std::vector<int> output_sample_num = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
};

