#include "ImageBuffer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

ImageBuffer::ImageBuffer(int w, int h) {
	width = w;
	height = h;

	image_array.resize(h);
	for (int i = 0; i < h; i++) {
		image_array[i].resize(w);
	}
}

void ImageBuffer::ClearBuffer() {
	for (int h = 0; h < height; h++) {
		for (int w = 0; w < width; w++) {
			image_array[h][w] = vec3(0, 0, 0);
		}
	}
}

void ImageBuffer::WriteToImage(std::string path_name, int sample_num, float gamma) {
	unsigned char* img = new unsigned char[width * height * 3];
	int idx = 0;
	for (int h = 0; h < height; h++) {
		for (int w = 0; w < width; w++) {
			vec3 color = image_array[h][w] * (float)(1.0 / sample_num);
			for (int i = 0; i < 3; i++) {
				if (color[i] >= 1.0) color[i] = 1.0;
				if (color[i] < 0) {
					std::cout << "Color Negative Error!" << h << " " << w << std::endl;
					color[i] = 1.0;
				}
				color[i] = powf(color[i], 1.0 / gamma);
				img[idx] = static_cast<unsigned char>(color[i]*255);
				idx++;
			}
		}
	}
	stbi_write_jpg(path_name.c_str(), width, height, 3, img, 100);
}