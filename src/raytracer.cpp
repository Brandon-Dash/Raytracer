// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"

#define M_PI 3.14159265358979323846264338327950288
#define MAX_T 10000.
#define MAX_REFLECTIONS 16

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

float hitsphere(point3 d, point3 e, point3 c, float r, point3 &hit, bool exit = false) {
	double disc = pow(glm::dot(d, e - c), 2) - glm::dot(d, d) * (glm::dot(e - c, e - c) - r * r);

	if (disc < 0)
		return 0;

	double rest = glm::dot(-d, e - c) / glm::dot(d, d);
	double sqrtdisc = sqrt(disc);
	double t;

	if (exit)
		t = rest + sqrtdisc / glm::dot(d, d);
	else
		t = rest - sqrtdisc / glm::dot(d, d);

	if (t < 0)
		return 0;

	hit = e + float(t) * d;
	return float(t);
}

bool pointInTriangle(const point3 &point, const point3 &p1, const point3 &p2, const point3 &p3, const point3 &n) {
	float test1 = glm::dot(glm::cross(point - p1, p2 - p1), n);
	float test2 = glm::dot(glm::cross(point - p2, p3 - p2), n);
	float test3 = glm::dot(glm::cross(point - p3, p1 - p3), n);

	return (test1 >= 0 && test2 >= 0 && test3 >= 0) || (test1 <= 0 && test2 <= 0 && test3 <= 0);
}

float hitplane(point3 d, point3 e, point3 a, point3 normal, point3 &hit, bool exit = false) {
	point3 n = normal;
	if (exit)
		n = -normal;

	double numerator = glm::dot(n, a - e);
	double denominator = glm::dot(n, d);
	double t = numerator / denominator;

	if (t <= 0 || numerator > 0) {
		return 0;
	}
	else {
		hit = e + float(t) * d;
		return float(t);
	}
}

float hitmesh(point3 d, point3 e, const json triangles, point3 &hit, point3 &hitNormal, bool exit = false) {
	float t_min = MAX_T;

	for (int i = 0; i < triangles.size(); i++) {
		std::vector<json> triangle = triangles[i];

		point3 p1 = vector_to_vec3(triangle[0]);
		point3 p2 = vector_to_vec3(triangle[1]);
		point3 p3 = vector_to_vec3(triangle[2]);

		point3 n = glm::cross(p2 - p1, p3 - p2);
		point3 hitpos;

		float t = hitplane(d, e, p1, n, hitpos, exit);

		if (t > 0.0 && t < t_min && pointInTriangle(hitpos, p1, p2, p3, n)) {
			hit = hitpos;
			hitNormal = glm::normalize(n);
			t_min = t;
		}
	}
	if (t_min == MAX_T)
		return 0.0;
	return t_min;
}

/****************************************************************************/

// lighting functions

void addDiffuse(const colour3 &Id, const colour3 &Kd, const point3 &N, const point3 &L, colour3 &colour) {
	colour3 diffuse = Id * Kd * glm::dot(N, L);
	for (int i = 0; i < 3; i++) {
		if (diffuse[i] < 0)
			diffuse[i] = 0;
	}

	colour = colour + diffuse;
}

void addSpecular(const colour3 &Is, const colour3 &Ks, const float a, const point3 &N, const point3 &L, const point3 &V, colour3 &colour) {
	point3 R = glm::normalize(2 * (glm::dot(N, L)) * N - L);
	float RdotV = glm::dot(R, V);

	if (RdotV > 0) {
		colour3 specular = Is * Ks * pow(RdotV, a);
		for (int i = 0; i < 3; i++) {
			if (specular[i] < 0)
				specular[i] = 0;
		}

		colour = colour + specular;
	}
}

bool shadowTest(const point3 &point, const point3 &lightPos, point3 &shadow) {
	point3 direction = lightPos - point;

	json &objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &object = *it;

		if (object["type"] == "sphere") {
			point3 c = vector_to_vec3(object["position"]);
			float r = float(object["radius"]);
			point3 hitpos;

			float t = hitsphere(direction, point, c, r, hitpos);
			if (t < 1.0 && t * glm::length(lightPos - point) > 1e-5) {
				json material = object["material"];
				if (material.find("transmissive") != material.end()) {
					shadow *= vector_to_vec3(material["transmissive"]);
				}
				else
					return false;
			}
		}
		else if (object["type"] == "plane") {
			point3 a = vector_to_vec3(object["position"]);
			point3 n = vector_to_vec3(object["normal"]);
			point3 hitpos;

			float t = hitplane(direction, point, a, n, hitpos);
			if (t < 1.0 && t * glm::length(lightPos - point) > 1e-5) {
				json material = object["material"];
				if (material.find("transmissive") != material.end()) {
					shadow *= vector_to_vec3(material["transmissive"]);
				}
				else
					return false;
			}
		}
		else if (object["type"] == "mesh") {
			std::vector<json> triangles = object["triangles"];

			for (int i = 0; i < triangles.size(); i++) {
				std::vector<json> triangle = triangles[i];

				point3 p1 = vector_to_vec3(triangle[0]);
				point3 p2 = vector_to_vec3(triangle[1]);
				point3 p3 = vector_to_vec3(triangle[2]);

				point3 n = glm::cross(p2 - p1, p3 - p2);
				point3 hitpos;

				float t = hitplane(direction, point, p1, n, hitpos);

				if (t < 1.0 && t * glm::length(lightPos - point) > 1e-5) {
					if (pointInTriangle(hitpos, p1, p2, p3, n)) {
						json material = object["material"];
						if (material.find("transmissive") != material.end()) {
							shadow *= vector_to_vec3(material["transmissive"]);
						}
						else
							return false;
					}
				}
			}
		}
	}
	return true;
}

bool refractRay(const point3 &Vi, const point3 &N, float refractionRatio, point3 &Vr) {
	float VidotN = glm::dot(Vi, N);

	if (pow(VidotN, 2) < 1 - pow(1 / refractionRatio, 2))
		return false;

	Vr = refractionRatio * (Vi - N * VidotN) - N * sqrt(1 - pow(refractionRatio, 2) * (1 - pow(VidotN, 2)));
	return true;
}

void reflectRay(const point3 &V, const point3 &N, point3 &R) {
	R = glm::normalize(2 * (glm::dot(N, V)) * N - V);
}

bool calcTransRay(const point3 &inPoint, const point3 &inVector, const point3 &inNormal, const json &object, point3 &outPoint, point3 &outVector) {
	json material = object["material"];
	float refraction = 1.0;
	point3 innerVector;

	if (material.find("refraction") != material.end()) {
		refraction = material["refraction"];
		refractRay(inVector, inNormal, 1.0 / refraction, innerVector);
	}
	else
		innerVector = inVector;

	if (object["type"] == "sphere") {
		if (refraction == 1.0) {
			outVector = innerVector;
			outPoint = inPoint + 1e-5f * outVector;
		}
		else {
			point3 c = vector_to_vec3(object["position"]);
			float r = float(object["radius"]);
			point3 hitpos;

			bool result = false;
			int recursion = 0;
			point3 currentPoint = inPoint;
			while (!result && recursion < 8) {
				hitsphere(innerVector, currentPoint, c, r, outPoint, true);
				point3 outNormal = glm::normalize(c - hitpos);

				result = refractRay(innerVector, outNormal, refraction, outVector);

				if (!result) {
					point3 R;
					reflectRay(innerVector, outNormal, R);
					innerVector = R;
					currentPoint = outPoint;
					recursion++;
				}
			}
			if (!result)
				return false;
		}
	}
	else if (object["type"] == "mesh") {
		std::vector<json> triangles = object["triangles"];
		point3 hitpos, outNormal;

		bool result = false;
		int recursion = 0;
		point3 currentPoint = inPoint;
		while (!result && recursion < 8) {
			hitmesh(innerVector, currentPoint, triangles, outPoint, outNormal, true);

			result = refractRay(innerVector, -outNormal, refraction, outVector);

			if (!result) {
				point3 R;
				reflectRay(-innerVector, -outNormal, R);
				innerVector = R;
				currentPoint = outPoint;
				recursion++;
			}
		}
		if(!result)
			return false;
	}
	else if (object["type"] == "plane") {
		outPoint = inPoint;
		outVector = innerVector;
	}
	return true;
}

void light(const point3 &p, const point3 &V, const point3 &N, const json &object, colour3 &colour, bool pick, int reflectionCount) {
	json &lights = scene["lights"];
	json material = object["material"];

	colour3 Ka, Kd, Ks, Kr, Kt = colour3(0.0, 0.0, 0.0);
	float a = 0.0;

	if (material.find("ambient") != material.end())
		Ka = vector_to_vec3(material["ambient"]);
	if (material.find("diffuse") != material.end())
		Kd = vector_to_vec3(material["diffuse"]);
	if (material.find("specular") != material.end())
		Ks = vector_to_vec3(material["specular"]);
	if (material.find("shininess") != material.end())
		a = material["shininess"];

	colour = colour3(0.0, 0.0, 0.0);

	if (material.find("reflective") != material.end() && reflectionCount < MAX_REFLECTIONS) {
		Kr = vector_to_vec3(material["reflective"]);
		point3 R;
		reflectRay(V, N, R);
		bool hit = trace(p + float(1e-5) * R, p + R, colour, pick, reflectionCount + 1);
		if (!hit)
			colour = background_colour;
		colour = colour * Kr;
	}

	for (json::iterator it = lights.begin(); it != lights.end(); ++it) {
		json &light = *it;

		if (light["type"] == "ambient") {
			colour3 Ia = vector_to_vec3(light["color"]);

			colour3 ambient = Ia * Ka;
			colour = colour + ambient;
		}

		else if (light["type"] == "directional") {
			point3 direction = vector_to_vec3(light["direction"]);
			point3 L = glm::normalize(-direction);

			point3 lightPosition = p + float(MAX_T) * L; // virtual position of light for use in shadow test

			colour3 shadow = colour3(1.0, 1.0, 1.0);
			if (shadowTest(p, lightPosition, shadow)) {
				colour3 I = vector_to_vec3(light["color"]) * shadow;

				addDiffuse(I, Kd, N, L, colour);
				addSpecular(I, Ks, a, N, L, V, colour);
			}
		}

		else if (light["type"] == "point") {
			point3 position = vector_to_vec3(light["position"]);

			colour3 shadow = colour3(1.0, 1.0, 1.0);
			if (shadowTest(p, position, shadow)) {
				colour3 I = vector_to_vec3(light["color"]) * shadow;
				point3 L = glm::normalize(position - p);

				addDiffuse(I, Kd, N, L, colour);
				addSpecular(I, Ks, a, N, L, V, colour);
			}
		}

		else if (light["type"] == "spot") {
			point3 position = vector_to_vec3(light["position"]);

			colour3 shadow = colour3(1.0, 1.0, 1.0);
			if (shadowTest(p, position, shadow)) {
				point3 direction = vector_to_vec3(light["direction"]);
				direction = glm::normalize(-direction);
				float cutoff_degrees = light["cutoff"];
				float cutoff = cutoff_degrees * M_PI / 180;
				point3 L = glm::normalize(position - p);

				if (glm::dot(L, direction) > cos(cutoff)) {
					colour3 I = vector_to_vec3(light["color"]) * shadow;

					addDiffuse(I, Kd, N, L, colour);
					addSpecular(I, Ks, a, N, L, V, colour);
				}
			}
		}
	}

	if (material.find("transmissive") != material.end()) {
		Kt = vector_to_vec3(material["transmissive"]);
		colour3 transcolour = colour3(0.0, 0.0, 0.0);
		point3 transmissionOrigin, transmissionDirection;

		bool result = calcTransRay(p, -V, N, object, transmissionOrigin, transmissionDirection);

		if (result) {
			bool hit = trace(transmissionOrigin, transmissionOrigin + transmissionDirection, transcolour, pick, reflectionCount + 1);
			if (!hit)
				transcolour = background_colour;
		}

		colour = (colour3(1.0, 1.0, 1.0) - Kt) * colour + Kt * transcolour;
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

bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick, int reflectionCount) {
	// NOTE 1: This is a demo, not ray tracing code! You will need to replace all of this with your own code...
  // NOTE 2: You can work with JSON objects directly (like this sample code), read the JSON objects into your own data structures once and render from those (probably in choose_scene), or hard-code the objects in your own data structures and choose them by name in choose_scene; e.g. choose_scene('e') would pick the same scene as the one in "e.json". Your choice.
  // If you want to use this JSON library, https://github.com/nlohmann/json for more information. The code below gives examples of everything you should need: getting named values, iterating over arrays, and converting types.

	json hitObject = NULL;
	point3 p, N, V;
	float t_min = MAX_T;

	// traverse the objects
	json &objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &object = *it;
		
		// every object in the scene will have a "type"
		if (object["type"] == "sphere") {
			// Every sphere will have a position and a radius
			point3 c = vector_to_vec3(object["position"]);
			point3 d = s - e;
			float r = float(object["radius"]);
			point3 hitpos;

			float t = hitsphere(d, e, c, r, hitpos);

			if (t > 0 && t < t_min) {
				hitObject = object;
				p = hitpos;
				N = glm::normalize(hitpos - c);
				V = glm::normalize(e - hitpos);
				t_min = t;
			}
		}
		else if (object["type"] == "plane") {
			point3 a = vector_to_vec3(object["position"]);
			point3 n = vector_to_vec3(object["normal"]);
			point3 d = s - e;
			point3 hitpos;

			float t = hitplane(d, e, a, n, hitpos);

			if (t > 0 && t < t_min) {
				hitObject = object;
				p = hitpos;
				N = glm::normalize(n);
				V = glm::normalize(e - hitpos);
				t_min = t;
			}
		}
		else if (object["type"] == "mesh") {
			std::vector<json> triangles = object["triangles"];
			
			for (int i = 0; i < triangles.size(); i++) {
				point3 d = s - e;
				point3 hitpos;
				point3 n;

				float t = hitmesh(d, e, triangles, hitpos, n);

				if (t > 0 && t < t_min) {
					hitObject = object;
					p = hitpos;
					N = glm::normalize(n);
					V = glm::normalize(e - hitpos);
					t_min = t;
					
				}
			}
			
		}
	}

	if (hitObject == NULL)
		return false;

	if (pick)
		std::cout << "hitpos = {" << p[0] << ", " << p[1] << ", " << p[2] << "}" << std::endl;

	light(p, V, N, hitObject, colour, pick, reflectionCount);

	return true;
}