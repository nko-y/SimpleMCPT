#include <iostream>
#include <string>

#include "SceneLoader.h"

int main(int argc, char** argv) {
	//std::string scene_path = "./scene/stairscase";
	std::string scene_path = "./scene/cornell-box";
	//std::string scene_path = "./scene/veach-mis";
	int sample_number = 64;

	if (argc > 1) {
		scene_path = std::string(argv[1]);
	}
	if (argc > 2) {
		sample_number = std::stoi(argv[2]);
	}

	SceneLoader my_scene(scene_path);
	my_scene.Render(sample_number);
}