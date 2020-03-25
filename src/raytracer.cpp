// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"
#define M_PI 3.14159265358979323846264338327950288

using json = nlohmann::json;

const char *PATH = "scenes/";

double fov = 60;
colour3 background_colour(0, 0, 0);

json scene;

/****************************************************************************/

// Helper functions

json find(json &j, const std::string key, const std::string value) {
	json::iterator it;
	for (it = j.begin(); it != j.end(); ++it) {
		if (it->find(key) != it->end()) {
			if ((*it)[key] == value) {
				return *it;
			}
		}
	}
	return json();
}

glm::vec3 vector_to_vec3(const std::vector<float> &v) {
	return glm::vec3(v[0], v[1], v[2]);
}

/****************************************************************************/

// Ray intersection functions

bool hitsphere(point3 d, point3 e, point3 c, float r, point3 &hit) {
	double disc = pow(glm::dot(d, e - c), 2) - glm::dot(d, d) * (glm::dot(e - c, e - c) - r * r);

	if (disc < 0)
		return false;

	double rest = glm::dot(-d, e - c) / glm::dot(d, d);
	double sqrtdisc = sqrt(disc);
	double t;

	if (disc > 0) {
		double t1 = rest + sqrtdisc / glm::dot(d, d);
		double t2 = rest - sqrtdisc / glm::dot(d, d);
		if (t1 < t2)
			t = t1;
		else
			t = t2;
	}
	else {
		t = rest;
	}

	hit = e + float(t) * d;
	return true;
}

bool hitplane(point3 d, point3 e, point3 a, point3 n, point3 &hit) {
	double numerator = glm::dot(n, a - e);
	double denominator = glm::dot(n, d);
	double t = numerator / denominator;
	//double t = glm::dot(n, a - e) / glm::dot(n, d);

	if (t <= 0 || numerator > 0) {
		return false;
	}
	else {
		hit = e + float(t) * d;
		return true;
	}
}

/****************************************************************************/

// lighting functions

void light(const point3 &p, const point3 &V, const point3 &N, json &material, colour3 &colour, bool pick) {
	json &lights = scene["lights"];
	colour = colour3(0.0, 0.0, 0.0);

	for (json::iterator it = lights.begin(); it != lights.end(); ++it) {
		json &light = *it;

		if (light["type"] == "ambient") {
			std::vector<float> amb = material["ambient"];
			colour3 Ka = vector_to_vec3(amb);

			std::vector<float> col = light["color"];
			colour3 Ia = vector_to_vec3(col);

			colour3 ambient = Ia * Ka;
			colour = colour + ambient;
		}

		else if (light["type"] == "directional") {
			std::vector<float> dif = material["diffuse"];
			colour3 Kd = vector_to_vec3(dif);

			//std::vector<float> spc = material["specular"];
			//colour3 Ks = vector_to_vec3(spc);

			std::vector<float> col = light["color"];
			colour3 Id = vector_to_vec3(col);

			std::vector<float> dir = light["direction"];
			point3 direction = vector_to_vec3(dir);

			point3 L = glm::normalize(-direction);

			colour3 diffuse = Id * Kd * glm::dot(N, L);
			for (int i = 0; i < 3; i++) {
				if (diffuse[i] < 0)
					diffuse[i] = 0;
			}

			colour = colour + diffuse;
		}

		else if (light["type"] == "point") {
			std::vector<float> dif = material["diffuse"];
			colour3 Kd = vector_to_vec3(dif);

			//std::vector<float> spc = material["specular"];
			//colour3 Ks = vector_to_vec3(spc);

			std::vector<float> col = light["color"];
			colour3 Id = vector_to_vec3(col);

			std::vector<float> pos = light["position"];
			point3 position = vector_to_vec3(pos);

			point3 L = glm::normalize(position - p);

			colour3 diffuse = Id * Kd * glm::dot(N, L);
			for (int i = 0; i < 3; i++) {
				if (diffuse[i] < 0)
					diffuse[i] = 0;
			}

			colour = colour + diffuse;
		}

		else if (light["type"] == "spot") {
			std::vector<float> dif = material["diffuse"];
			colour3 Kd = vector_to_vec3(dif);

			//std::vector<float> spc = material["specular"];
			//colour3 Ks = vector_to_vec3(spc);

			std::vector<float> col = light["color"];
			colour3 Id = vector_to_vec3(col);

			std::vector<float> pos = light["position"];
			point3 position = vector_to_vec3(pos);

			std::vector<float> dir = light["direction"];
			point3 direction = vector_to_vec3(dir);
			direction = glm::normalize(-direction);

			float cutoff_degrees = light["cutoff"];
			float cutoff = cutoff_degrees * M_PI / 180;

			point3 L = glm::normalize(position - p);

			if (glm::dot(L, direction) > cos(cutoff))
			{
				colour3 diffuse = Id * Kd * glm::dot(N, L);
				for (int i = 0; i < 3; i++) {
					if (diffuse[i] < 0)
						diffuse[i] = 0;
				}

				colour = colour + diffuse;
			}
		}
	}
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
}

bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick) {
	// NOTE 1: This is a demo, not ray tracing code! You will need to replace all of this with your own code...
  // NOTE 2: You can work with JSON objects directly (like this sample code), read the JSON objects into your own data structures once and render from those (probably in choose_scene), or hard-code the objects in your own data structures and choose them by name in choose_scene; e.g. choose_scene('e') would pick the same scene as the one in "e.json". Your choice.
  // If you want to use this JSON library, https://github.com/nlohmann/json for more information. The code below gives examples of everything you should need: getting named values, iterating over arrays, and converting types.

	// traverse the objects
	json &objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &object = *it;
		
		// every object in the scene will have a "type"
		if (object["type"] == "sphere") {
			// This is NOT ray-sphere intersection
			// Every sphere will have a position and a radius
			std::vector<float> pos = object["position"];
			point3 c = vector_to_vec3(pos);
			point3 d = s - e;
			float r = float(object["radius"]);

			point3 hitpos;
			bool didhit;

			didhit = hitsphere(d, e, c, r, hitpos);

			if (didhit) {
				if (pick)
					std::cout << "hitpos = {" << hitpos[0] << ", " << hitpos[1] << ", " << hitpos[2] << "}" << std::endl;

				// Every object will have a material
				json &material = object["material"];
				point3 N = glm::normalize(hitpos - c);
				point3 V = hitpos - e;

				light(hitpos, V, N, material, colour, pick);

				// This is NOT correct: it finds the first hit, not the closest
				return true;
			}
		}
		else if (object["type"] == "plane") {
			std::vector<float> pos = object["position"];
			point3 a = vector_to_vec3(pos);
			std::vector<float> norm = object["normal"];
			point3 n = vector_to_vec3(norm);
			point3 d = s - e;

			point3 hitpos;
			bool didhit;

			didhit = hitplane(d, e, a, n, hitpos);

			if (didhit) {
				if (pick)
					std::cout << "hitpos = {" << hitpos[0] << ", " << hitpos[1] << ", " << hitpos[2] << "}" << std::endl;

				json &material = object["material"];
				point3 N = glm::normalize(n);
				point3 V = hitpos - e;

				light(hitpos, V, N, material, colour, pick);

				return true;
			}
		}
	}

	return false;
}