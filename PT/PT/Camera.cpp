#include "Camera.h"

#include <glm/ext.hpp>
#include "Tool.h"

Camera::Camera(vec3 t_eye, vec3 t_up, vec3 t_lookat, float t_fovy, int t_w, int t_h) {
	eye = t_eye;
	up = t_up;
	lookat = t_lookat;
	fovy = t_fovy;
	aspect = 1.0 * t_w / t_h;
	width = t_w;
	height = t_h;

	float zl = 1;
	float yl = 2 * tan(radians(fovy * 0.5)) * zl;
	float xl = yl * aspect;

	vec4 left_top_corner_cam = vec4(-xl / 2, yl / 2, -zl, 1);
	vec4 right_top_corner_cam = vec4(xl / 2, yl / 2, -zl, 1);
	vec4 left_bottom_corner_cam = vec4(-xl / 2, -yl / 2, -zl, 1);
	
	
	auto cam_to_world = inverse(glm::lookAt(eye, lookat, up));

	vec3 left_top_corner_world = vec3(cam_to_world * left_top_corner_cam);
	vec3 right_top_corener_world = vec3(cam_to_world * right_top_corner_cam);
	vec3 left_bottom_corner_world = vec3(cam_to_world * left_bottom_corner_cam);
	left_top_corner = left_top_corner_world;

	dw = (right_top_corener_world - left_top_corner_world);
	dh = (left_bottom_corner_world - left_top_corner_world);
	for (int i = 0; i < 3; i++) {
		dw[i] /= width;
		dh[i] /= height;
	}
}

Ray Camera::CastRay(int x, int y) {
	// border
	float x_pos = x + get_uniform_random() - 0.5;
	float y_pos = y + get_uniform_random() - 0.5;
	vec3 ray_dir = left_top_corner + x_pos * dw + y_pos * dh - eye;
	return Ray(eye, ray_dir);
}