#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <iostream>
using namespace glm;

class ImageBuffer
{
public:
	ImageBuffer() {};
	ImageBuffer(int w, int h);

	void ClearBuffer();
	void WriteToImage(std::string path_name, int sample_num, float gamma=2.2);

	int width, height;
	std::vector<std::vector<vec3>> image_array;

};

