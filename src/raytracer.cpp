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

#define TRIANGLE std::array<point3, 3>

using json = nlohmann::json;

const char *PATH = "scenes/";

double fov = 60;
colour3 background_colour(0, 0, 0);

json scene;

std::vector<Object*> Objects;
std::vector<Light*> Lights;

/****************************************************************************/

// Object classes

struct Material {
	colour3 ambient = colour3(0, 0, 0);
	colour3 diffuse = colour3(0, 0, 0);
	colour3 specular = colour3(0, 0, 0);
	float shininess = 0;
	colour3 reflective = colour3(0, 0, 0);
	colour3 transmissive = colour3(0, 0, 0);
	float refraction = 0;
};

class Object {
public:
	Material material;
	virtual float rayhit(point3 e, point3 d, point3 &hit) = 0;
};

class Sphere : public Object {
public:
	point3 center;
	float radius;
	Sphere(point3 center, float radius, Material material) {
		this->center = center;
		this->radius = radius;
		this->material = material;
	}

	float rayhit(point3 e, point3 d, point3 &hit) {
		double disc = pow(glm::dot(d, e - center), 2) - glm::dot(d, d) * (glm::dot(e - center, e - center) - radius * radius);

		if (disc < 0)
			return 0;

		double rest = glm::dot(-d, e - center) / glm::dot(d, d);
		double sqrtdisc = sqrt(disc);
		double t;
		
		t = rest - sqrtdisc / glm::dot(d, d);

		if (t < 0)
			return 0;

		hit = e + float(t) * d;
		return float(t);
	}
};

class Plane : public Object {
public:
	point3 point;
	point3 normal;
	Plane(point3 point, point3 normal, Material material) {
		this->point = point;
		this->normal = normal;
		this->material = material;
	}

	float rayhit(point3 e, point3 d, point3 &hit) {
		double numerator = glm::dot(normal, point - e);
		double denominator = glm::dot(normal, d);
		double t = numerator / denominator;

		if (t <= 0 || numerator > 0)
			return 0;

		hit = e + float(t) * d;
		return float(t);
	}
};

class Mesh : public Object {
public:
	std::vector<TRIANGLE> triangles;
	Mesh(Material material) {
		this->material = material;
	}

	float rayhit(point3 e, point3 d, point3 &hit) {
		float t_min = MAX_T;

		for (int i = 0; i < triangles.size(); i++) {
			TRIANGLE triangle = triangles[i];

			point3 p1 = triangle[0];
			point3 p2 = triangle[1];
			point3 p3 = triangle[2];

			point3 n = glm::cross(p2 - p1, p3 - p2);
			point3 hitpos;

			float t = hitplane(d, e, p1, n, hitpos, exit);

			if (t > 0.0 && t < t_min && pointInTriangle(hitpos, p1, p2, p3, n)) {
				hit = hitpos;
				t_min = t;
			}
		}
		if (t_min == MAX_T)
			return 0.0;
		return t_min;
	}
private:
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
};

/****************************************************************************/

// Light classes

class Light {
public:
	colour3 colour;
	virtual void calcLight() = 0;
};

class Ambient : public Light {
public:
	Ambient(colour3 colour) {
		this->colour = colour;
	}

	void calcLight() {
	}
};

class Directional : public Light {
public:
	point3 direction;
	Directional(colour3 colour, point3 direction) {
		this->colour = colour;
		this->direction = direction;
	}

	void calcLight() {
	}
};

class Point : public Light {
public:
	point3 position;
	Point(colour3 colour, point3 position) {
		this->colour = colour;
		this->position = position;
	}

	void calcLight() {
	}
};

class Spot : public Light {
public:
	point3 position;
	point3 direction;
	float cutoff;
	Spot(point3 colour, point3 position, point3 direction, float cutoff) {
		this->colour = colour;
		this->position = position;
		this->direction = direction;
		this->cutoff = cutoff;
	}

	void calcLight() {
	}
};

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

bool refract(const point3 &Vi, const point3 &N, const float &refraction, point3 &Vr) {
	float VidotN = glm::dot(Vi, N);
	float refratio = 1.0 / refraction;
	point3 n = N;
	if (VidotN < 0)
		VidotN = -VidotN;
	else {
		refratio = 1.0 / refratio;
		n = -N;
	}

	float k = 1 - pow(refratio, 2) * (1 - pow(VidotN, 2));

	if (k < 0)
		return false;
	Vr = glm::normalize(refratio * Vi + (refratio * VidotN - sqrt(k)) * n);
	return true;
}

void reflectRay(const point3 &V, const point3 &N, point3 &R) {
	R = glm::normalize(2 * glm::dot(N, V) * N - V);
}

bool calcTransRay(const point3 &inPoint, const point3 &inVector, const point3 &inNormal, const json &object, point3 &outPoint, point3 &outVector, bool pick) {
	json material = object["material"];
	float refraction = 1.0;
	point3 innerVector;

	if (material.find("refraction") != material.end()) {
		refraction = material["refraction"];
		refract(inVector, inNormal, refraction, innerVector);
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

			bool result = false;
			int reflections = 0;
			point3 currentPoint = inPoint;
			while (!result && reflections < 8) {
				hitsphere(innerVector, currentPoint, c, r, outPoint, true);
				point3 outNormal = glm::normalize(outPoint - c);

				result = refract(innerVector, outNormal, refraction, outVector);

				if (!result) {
					point3 R;
					reflectRay(-innerVector, outNormal, R);
					innerVector = R;
					currentPoint = outPoint;
					reflections++;
					if (pick)
						std::cout << "interior reflection at {" << outPoint[0] << ", " << outPoint[1] << ", " << outPoint[2] << "}" << std::endl;
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
		int reflections = 0;
		point3 currentPoint = inPoint;
		while (!result && reflections < 8) {
			hitmesh(innerVector, currentPoint, triangles, outPoint, outNormal, true);

			result = refract(innerVector, outNormal, refraction, outVector);

			if (!result) {
				point3 R;
				reflectRay(-innerVector, outNormal, R);
				innerVector = R;
				currentPoint = outPoint;
				reflections++;
				if (pick)
					std::cout << "interior reflection at {" << outPoint[0] << ", " << outPoint[1] << ", " << outPoint[2] << "}" << std::endl;
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
		if (pick)
			std::cout << "reflection:" << std::endl;

		Kr = vector_to_vec3(material["reflective"]);
		point3 R;
		reflectRay(V, N, R);
		bool hit = trace(p + float(1e-5) * R, p + R, colour, pick, reflectionCount + 1);
		if (!hit)
			colour = background_colour;

		if (pick && !hit)
			std::cout << "no additional objects hit" << std::endl;

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
		if (pick)
			std::cout << "refraction:" << std::endl;

		Kt = vector_to_vec3(material["transmissive"]);
		colour3 transcolour = colour3(0.0, 0.0, 0.0);
		point3 transmissionOrigin, transmissionDirection;

		bool result = calcTransRay(p, -V, N, object, transmissionOrigin, transmissionDirection, pick);

		if (result) {
			if (pick)
				std::cout << "exit object at {" << transmissionOrigin[0] << ", " << transmissionOrigin[1] << ", " << transmissionOrigin[2] << "}" << std::endl;
			bool hit = trace(transmissionOrigin, transmissionOrigin + transmissionDirection, transcolour, pick, reflectionCount + 1);
			if (!hit)
				transcolour = background_colour;

			if (pick && !hit)
				std::cout << "no additional objects hit" << std::endl;
		}
		else if (pick)
			std::cout << "ray lost due to too many internal reflections" << std::endl;

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

				TRIANGLE triangle;

				triangle[0] = vector_to_vec3(trianglejson[0]);
				triangle[1] = vector_to_vec3(trianglejson[1]);
				triangle[2] = vector_to_vec3(trianglejson[2]);

				mesh->triangles.push_back(triangle);
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

	if (hitObject == NULL)
		return false;

	if (pick)
		std::cout << "object " << hitObject["type"] << " hit at {" << p[0] << ", " << p[1] << ", " << p[2] << "}" << std::endl;

	light(p, V, N, hitObject, colour, pick, reflectionCount);

	return true;
}