#include "SceneLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <tinyxml2.h>
#include <glm/glm.hpp>

using namespace glm;

void SceneLoader::InitVariable() {
	
}

SceneLoader::SceneLoader(std::string scene_path)
{
	InitVariable();
	this->scene_path = scene_path;
	ParseFileName();
	
	ReadObjectFile();
}


SceneLoader::~SceneLoader() {

}




void SceneLoader::ParseFileName() {
	int pos = 0;
	for (int i = scene_path.size() - 1; i >= 0; i--) {
		if (scene_path[i] == '/' || scene_path[i] == '\\') {
			pos = i;
			break;
		}
	}
	for (int i = pos + 1; i < scene_path.size(); i++) {
		scene_name += scene_path[i];
	}

	if (scene_name.size() == 0) {
		std::cout << "ERROR: Fail To Parse Scene Name" << std::endl;
		exit(1);
	}
}


//Ref: https://github.com/tinyobjloader/tinyobjloader
void SceneLoader::ReadObjectFile() {
	// Timer
	std::chrono::time_point<std::chrono::steady_clock> _start, _end;
	_start = std::chrono::steady_clock::now();

	std::string base_scene_path = scene_path + '/' + scene_name;

	// Load Object/XML File
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (base_scene_path+".obj").c_str(), scene_path.c_str());
	if (!err.empty()) {
		std::cerr << err << std::endl;
	}
	if (!ret) {
		exit(1);
	}

	//Load XML File
	tinyxml2::XMLDocument xml_loader;
	ret = xml_loader.LoadFile((base_scene_path + ".xml").c_str());
	if (ret != 0) {
		std::cerr << "Can't Load " << base_scene_path + ".xml" << std::endl;
		exit(1);
	}

	//Load Light Material
	std::map<std::string, vec3> light_node;
	for (auto node = xml_loader.FirstChildElement("light"); node!=nullptr; node = node->NextSiblingElement("light")) {
		std::string light_name = node->Attribute("mtlname");
		std::string light_radiance_string = node->Attribute("radiance");
		vec3 light_radiance;
		sscanf(light_radiance_string.c_str(), "%f,%f,%f", &light_radiance[0], &light_radiance[1], &light_radiance[2]);
		std::cout << light_name << " " << light_radiance[0] << " " << light_radiance[1] << " " << light_radiance[2] << std::endl;
		light_node.insert(std::make_pair(light_name, light_radiance));
	}

	//Load All Material
	for (auto mtl : materials) {
		// Glass Material
		if (mtl.ior > 1.0) {
			std::cout << mtl.name << " " << mtl.ior << " ";
			GlassMaterial* one_mtl = new GlassMaterial(mtl.ior);
			one_mtl->Tr = vec3(mtl.transmittance[0], mtl.transmittance[1], mtl.transmittance[2]);
			printvec3(one_mtl->Tr);
			mtl_list.push_back(static_cast<Material*>(one_mtl));
			continue;
		}

		// Common Material
		vec3 tp_diffuse, tp_specular;
		float tp_shininess;
		for (int i = 0; i < 3; i++) {
			tp_diffuse[i] = mtl.diffuse[i];
			tp_specular[i] = mtl.specular[i];
		}
		tp_shininess = mtl.shininess;
		PhongMaterial* one_mtl = new PhongMaterial(tp_diffuse, tp_specular, tp_shininess);
		std::cout << mtl.name << " ";
		std::cout << "diffuse:" << tp_diffuse[0] << " " << tp_diffuse[1] << " " << tp_diffuse[2] << " ";
		std::cout << "specular:" << tp_specular[0] << " " << tp_specular[1] << " " << tp_specular[2] << " ";
		std::cout << "shininess:" << tp_shininess << std::endl;

		if (light_node.count(mtl.name) > 0) {
			one_mtl->SetEmissive(light_node[mtl.name]);
		}

		if (mtl.diffuse_texname.length() > 0) {
			std::string texture_path = scene_path + "/" + mtl.diffuse_texname;
			int tx_width, tx_height, tx_channels;
			float* tx_data = stbi_loadf((texture_path).c_str(), &tx_width, &tx_height, &tx_channels, 3);
			if (tx_data == nullptr && tx_channels != 3) {
				std::cout << "Fail To Load Texture" << std::endl;
				exit(1);
			}
			one_mtl->is_texture = true;
			one_mtl->texture = new Texture(tx_width, tx_height, tx_data, tx_channels);
		}
		mtl_list.push_back(static_cast<Material*>(one_mtl));
	}
	std::cout << std::endl;


	// Loop Over Shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		std::cout << "shapes " << s << " Face Nums: " << shapes[s].mesh.num_face_vertices.size() << std::endl;
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
			vec3 p[3], n[3];
			vec2 uv[3];
			bool has_norm = false, has_uv = false;
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
				p[v] = vec3(vx, vy, vz);

				has_norm = (idx.normal_index >= 0);
				if (has_norm) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
					n[v] = vec3(nx, ny, nz);
				}

				has_uv = (idx.texcoord_index >= 0);
				if (has_uv) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					uv[v] = vec2(tx, ty);
				}
			}
			index_offset += fv;
			Triangle* onetri = new Triangle(p);
			if (has_norm && dot(onetri->n, n[0]) < 0) {
				onetri->n = -onetri->n;
			}
			if (has_uv) {
				onetri->SetUV(uv);
			}

			int mtl_id = shapes[s].mesh.material_ids[f];
			onetri->mtl = mtl_list[mtl_id];
			obj_list.push_back(static_cast<Hittable*>(onetri));

			if (onetri->mtl->mtl_type == 2) {
				light_list.Add(static_cast<Emittable*>(onetri));
			}
		}
	}
	std::cout << std::endl;

	//Load Camera
	auto camera_node = xml_loader.FirstChildElement("camera");
	if (!camera_node) {
		std::cout << "Can't Find Camera" << std::endl;
		exit(1);
	}
	vec3 dt;
	vec3 eye, up, lookat;
	float fovy;
	float aspect;
	int as_w, as_h;
	camera_node->FirstChildElement("eye")->QueryFloatAttribute("x", &eye[0]);
	camera_node->FirstChildElement("eye")->QueryFloatAttribute("y", &eye[1]);
	camera_node->FirstChildElement("eye")->QueryFloatAttribute("z", &eye[2]);
	camera_node->FirstChildElement("lookat")->QueryFloatAttribute("x", &lookat[0]);
	camera_node->FirstChildElement("lookat")->QueryFloatAttribute("y", &lookat[1]);
	camera_node->FirstChildElement("lookat")->QueryFloatAttribute("z", &lookat[2]);
	camera_node->FirstChildElement("up")->QueryFloatAttribute("x", &up[0]);
	camera_node->FirstChildElement("up")->QueryFloatAttribute("y", &up[1]);
	camera_node->FirstChildElement("up")->QueryFloatAttribute("z", &up[2]);
	camera_node->QueryFloatAttribute("fovy", &(fovy));
	camera_node->QueryIntAttribute("width", &(as_w));
	camera_node->QueryIntAttribute("height", &(as_h));
	cam = new Camera(eye, up, lookat, fovy, as_w, as_h);
	std::cout << "camera fovy: " << fovy << std::endl;
	std::cout << "camera width: " << as_w << std::endl;
	std::cout << "camera height: " << as_h << std::endl;
	std::cout << "camera eye: " << eye[0] << "," << eye[1] << "," << eye[2] << std::endl;
	std::cout << "camera lookat: " << lookat[0] << "," << lookat[1] << "," << lookat[2] << std::endl;
	std::cout << "camera up: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
	std::cout << std::endl;
	
	// Init Image Buffer
	buf = new ImageBuffer(as_w, as_h);

	_end = std::chrono::steady_clock::now();

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start);
	std::cout << "Finish Loading Scene. Time:" << " " << ms.count() << " ms" << std::endl << std::endl;

	// Building BVHTree
	_start = std::chrono::steady_clock::now();

	std::vector<Hittable*> copy_objs(this->obj_list);
	bvh_tree = BVH::BuildTree(copy_objs, 0, copy_objs.size());

	_end = std::chrono::steady_clock::now();
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start);
	std::cout << "Finish Building BVHTree. Time:" << " " << ms.count() << " ms" << std::endl << std::endl;
}

void SceneLoader::Render(int sample_num) {
	//Timer
	std::chrono::time_point<std::chrono::steady_clock> _start, _end;

	std::string save_img_path = scene_path + '/' + scene_name +"_final_spp" + std::to_string(sample_num) + ".jpg";
	buf->ClearBuffer();

	_start = std::chrono::steady_clock::now();
	//Render[1. each point sample numbers/ sample all points then loop]
#ifdef _OPENMP
		std::cout << "Omp Max Thread: " << omp_get_max_threads() << std::endl;
#endif
	for (int cnt = 0; cnt < sample_num; cnt++) {
		for (int h = 0; h < buf->height; h++) {
			printf("Sample %d/%d: Line %5d/%5d\r", cnt+1, sample_num, h, buf->height);
#pragma omp parallel for 
			for (int w = 0; w < buf->width; w++) {
				Ray r = cam->CastRay(w, h);
				vec3 res_light = Li(r);
				for (int dim = 0; dim < 3; dim++) {
					if (!std::isfinite(res_light[0])) {
						std::cout << "Not Finite Light" << std::endl;
						exit(1);
					}
				}
				buf->image_array[h][w] += res_light;
			}
		}
		for (int i = 0; i < output_sample_num.size(); i++) {
			if (output_sample_num[i] == cnt + 1) {
				std::string tp_save_img_path = scene_path + '/' + scene_name + "_spp" + std::to_string(cnt + 1) + ".jpg";
				buf->WriteToImage(tp_save_img_path, cnt+1, 2.2);
			}
		}
	}

	

	_end = std::chrono::steady_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start);
	std::cout << "Finish Rendering. Time:" << " " << ms.count() << " ms." << " Sample: " << sample_num << std::endl << std::endl;

	buf->WriteToImage(save_img_path, sample_num, 2.2);
}


vec3 SceneLoader::Li(Ray& r) {
	vec3 color(0, 0, 0);
	Ray nowray = r;
	HitRecord rec;
	vec3 wo, wi;

	int max_bounce = 2;
	float min_hit_value = 0.001;
	float max_hit_value = INFINITY;
	bool is_light = true;
	vec3 factor = vec3(1, 1, 1);

	for (int bn = 0; ; bn++) {
		// Not Intersect
		if (!bvh_tree->hit(r, min_hit_value, max_hit_value, rec)) break;

		// Intersect with light
		if (rec.mtl->mtl_type == 2) {
			if (is_light && dot(rec.n, r.dir) < 0) {
				color = color + factor * static_cast<PhongMaterial*>(rec.mtl)->Ke;
			}
			break;
		}

		// Intersect with Phong Material
		wo = -r.dir;
		// Intersect with glass
		if (rec.mtl->mtl_type == 1) {

			rec.mtl->scatter(wo, wi, rec);
			factor = factor * rec.mtl->bsdf(wo, wi, rec);
			r = Ray(rec.p, wi);
			continue;
		}

		// If first not intersect with light, set common ray
		is_light = false;

		if (dot(rec.n, wo) < 0) rec.n = -rec.n;
		//std::cout << bn << " " << factor[0]<<" "<<factor[1]<<" "<<factor[2] << std::endl;
		color = color + factor * SampleLight(wo, rec) ;
		float pdf = rec.mtl->scatter(wo, wi, rec);
		if (dot(wi, rec.n) > 0 && pdf > epsilon) {
			r = Ray(rec.p, wi);
			factor = factor * rec.mtl->bsdf(wo, wi, rec) * dot(wi, rec.n) / pdf;
		}
		else break;

		if (bn >= 3) {
			int which = 0;
			for (int i = 1; i < 3; i++) {
				if (factor[i] > factor[which]) which = i;
			}
			if (get_uniform_random() < factor[which]) factor = factor / factor[which];
			else break;
		}
	}

	return color;
}


vec3 SceneLoader::SampleLight(vec3& wo, HitRecord& rec) {
	HitRecord light_rec;
	vec3 wi;
	float bsdf_pdf;
	vec3 bsdf_pdf_factor;
	vec3 final_color(0, 0, 0);
	float weight;

	float light_pdf = light_list.SampleRay(rec, light_rec, bvh_tree, wi);
	if (light_pdf > epsilon) {
		bsdf_pdf = rec.mtl->pdf(wo, wi, rec);
		if (bsdf_pdf > epsilon) {
			bsdf_pdf_factor = rec.mtl->bsdf(wo, wi, rec);
			if (light_rec.mtl->mtl_type == 2) {
				weight = light_pdf * light_pdf / (light_pdf * light_pdf + bsdf_pdf * bsdf_pdf);
				final_color += weight * static_cast<PhongMaterial*>(light_rec.mtl)->Ke * bsdf_pdf_factor * dot(wi, rec.n) / light_pdf;
			}
			else
			{
				std::cout << "Not From Light 1" << std::endl;
				exit(1);
			}
		}
	}
	
	bsdf_pdf = rec.mtl->scatter(wo, wi, rec);
	if (bsdf_pdf > epsilon) {
		Ray r(rec.p, wi);
		light_pdf = light_list.pdf(light_rec, bvh_tree, r);
		if (light_pdf > epsilon) {
			bsdf_pdf_factor = rec.mtl->bsdf(wo, wi, rec);
			if (light_rec.mtl->mtl_type == 2) {
				weight = bsdf_pdf * bsdf_pdf / (light_pdf * light_pdf + bsdf_pdf * bsdf_pdf);
				final_color += weight * static_cast<PhongMaterial*>(light_rec.mtl)->Ke * bsdf_pdf_factor * dot(wi, rec.n) / bsdf_pdf;
			}
			else
			{
				std::cout << "Not From Light 2" << std::endl;
				exit(1);
			}
		}
	}

	return final_color;
}