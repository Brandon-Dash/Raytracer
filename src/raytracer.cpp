// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"
#include "objects.h"
#include "raymath.h"
#include "bvh.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"

using json = nlohmann::json;

const char *PATH = "scenes/";

double fov = 60;
colour3 background_colour(0, 0, 0);

json scene;

std::vector<Object*> Objects;
std::vector<Light*> Lights;

BVH* bvh;

/****************************************************************************/

// Helper functions

glm::vec3 vector_to_vec3(const std::vector<float> &v) {
	return glm::vec3(v[0], v[1], v[2]);
}

/****************************************************************************/

// additional ray functions

bool shadowRay(const point3 &point, const point3 &lightPos, point3 &shadow) {
	point3 direction = lightPos - point;

	for (int i = 0; i < Objects.size(); i++) {
		Object* object = Objects[i];

		float t = object->rayhit(point, direction);
		if (t < 1.0 && t * glm::length(lightPos - point) > 1e-5) {
			Material material = object->material;
			if (!isZero(material.transmissive)) {
				shadow *= material.transmissive;
			}
			else
				return false;
		}
	}
	return true;
}

/****************************************************************************/

void choose_scene(char const *fn) {
	if (fn == NULL) {
		std::cout << "Using default input file " << PATH << "c.json\n";
		fn = "c";
	}

	std::cout << "Loading scene " << fn << std::endl;
	
	std::string fname = PATH + std::string(fn) + ".json";
	std::fstream in(fname);
	if (!in.is_open()) {
		std::cout << "Unable to open scene file " << fname << std::endl;
		exit(EXIT_FAILURE);
	}
	
	in >> scene;
	
	json camera = scene["camera"];
	// these are optional parameters (otherwise they default to the values initialized earlier)
	if (camera.find("field") != camera.end()) {
		fov = camera["field"];
		std::cout << "Setting fov to " << fov << " degrees.\n";
	}
	if (camera.find("background") != camera.end()) {
		background_colour = vector_to_vec3(camera["background"]);
		std::cout << "Setting background colour to " << glm::to_string(background_colour) << std::endl;
	}

	json objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &object = *it;

		json materialjson = object["material"];
		Material material;

		if (materialjson.find("ambient") != materialjson.end())
			material.ambient = vector_to_vec3(materialjson["ambient"]);
		if (materialjson.find("diffuse") != materialjson.end())
			material.diffuse = vector_to_vec3(materialjson["diffuse"]);
		if (materialjson.find("specular") != materialjson.end())
			material.specular = vector_to_vec3(materialjson["specular"]);
		if (materialjson.find("reflective") != materialjson.end())
			material.reflective = vector_to_vec3(materialjson["reflective"]);
		if (materialjson.find("transmissive") != materialjson.end())
			material.transmissive = vector_to_vec3(materialjson["transmissive"]);
		if (materialjson.find("shininess") != materialjson.end())
			material.shininess = float(materialjson["shininess"]);
		if (materialjson.find("refraction") != materialjson.end())
			material.refraction = float(materialjson["refraction"]);

		if (object["type"] == "sphere") {
			point3 center = vector_to_vec3(object["position"]);
			float radius = float(object["radius"]);

			Objects.push_back(new Sphere(center, radius, material));
		}

		if (object["type"] == "plane") {
			point3 point = vector_to_vec3(object["position"]);
			point3 normal = vector_to_vec3(object["normal"]);

			Objects.push_back(new Plane(point, normal, material));
		}

		if (object["type"] == "mesh") {
			std::vector<json> triangles = object["triangles"];

			Mesh* mesh = new Mesh(material);

			for (int i = 0; i < triangles.size(); i++) {
				std::vector<json> trianglejson = triangles[i];

				point3 p0 = vector_to_vec3(trianglejson[0]);
				point3 p1 = vector_to_vec3(trianglejson[1]);
				point3 p2 = vector_to_vec3(trianglejson[2]);

				mesh->triangles.push_back(new Triangle(mesh, p0, p1, p2, material));
			}

			Objects.push_back(mesh);
		}
	}

	json lights = scene["lights"];
	for (json::iterator it = lights.begin(); it != lights.end(); ++it) {
		json &light = *it;

		colour3 colour = vector_to_vec3(light["color"]);

		if (light["type"] == "ambient") {
			Lights.push_back(new Ambient(colour));
		}

		if (light["type"] == "directional") {
			point3 direction = vector_to_vec3(light["direction"]);
			Lights.push_back(new Directional(colour, direction));
		}

		if (light["type"] == "point") {
			point3 position = vector_to_vec3(light["position"]);
			Lights.push_back(new Point(colour, position));
		}

		if (light["type"] == "spot") {
			point3 position = vector_to_vec3(light["position"]);
			point3 direction = vector_to_vec3(light["direction"]);
			float cutoff = float(light["cutoff"]);
			Lights.push_back(new Spot(colour, position, direction, cutoff));
		}
	}

	// Create the BVH

	bvh = new BVH(Objects);
}

bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick, int reflectionCount) {
	// NOTE 1: This is a demo, not ray tracing code! You will need to replace all of this with your own code...
  // NOTE 2: You can work with JSON objects directly (like this sample code), read the JSON objects into your own data structures once and render from those (probably in choose_scene), or hard-code the objects in your own data structures and choose them by name in choose_scene; e.g. choose_scene('e') would pick the same scene as the one in "e.json". Your choice.
  // If you want to use this JSON library, https://github.com/nlohmann/json for more information. The code below gives examples of everything you should need: getting named values, iterating over arrays, and converting types.

	if (reflectionCount > MAX_REFLECTIONS) {
		if (pick)
			std::cout << "Maximum number of reflections reached." << std::endl;
		colour = colour3(0, 0, 0);
		return false;
	}

	Object* hitObject = NULL;
	float t_min = MAX_T;
	point3 d = s - e;

	for (int i = 0; i < Objects.size(); i++) {
		Object* object = Objects[i];

		float t = object->rayhit(e, d);

		if (t > 0 && t < t_min) {
			hitObject = object;
			t_min = t;
		}
	}

	if (hitObject == NULL)
		return false;

	if (pick)
		std::cout << "object " << hitObject->type << " hit at {" << hitObject->cachedHitpoint[0] << ", " << hitObject->cachedHitpoint[1] << ", " << hitObject->cachedHitpoint[2] << "}" << std::endl;

	hitObject->lightPoint(e, d, Lights, colour, reflectionCount, pick);

	return true;
}

bool traceBVH(const point3& e, const point3& s, colour3& colour, bool pick, int reflectionCount) {
	if (reflectionCount > MAX_REFLECTIONS) {
		if (pick)
			std::cout << "Maximum number of reflections reached." << std::endl;
		colour = colour3(0, 0, 0);
		return false;
	}

	Object* hitObject = NULL;
	float t_min = MAX_T;
	point3 d = s - e;

	// Find hit using BVH

	if (hitObject == NULL)
		return false;

	if (pick)
		std::cout << "object " << hitObject->type << " hit at {" << hitObject->cachedHitpoint[0] << ", " << hitObject->cachedHitpoint[1] << ", " << hitObject->cachedHitpoint[2] << "}" << std::endl;

	hitObject->lightPoint(e, d, Lights, colour, reflectionCount, pick);

	return true;
}