// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"
#include "objects.h"
#include "raymath.h"
#include "bvh.h"
#include "texturemesh.h"
#include "bump.h"
#include "csg.h"
#include "arealight.h"

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

glm::vec2 vector_to_vec2(const std::vector<float>& v) {
	return glm::vec2(v[0], v[1]);
}

csg_node* create_csgNode(json& nodejson) {
	csg_node* node;

	if (nodejson.find("operation") != nodejson.end()) {
		if (nodejson["operation"] == "union")
			node = new csg_node(Union);
		if (nodejson["operation"] == "intersection")
			node = new csg_node(Intersection);
		if (nodejson["operation"] == "difference")
			node = new csg_node(Difference);

		json& first = nodejson["first"];
		json& second = nodejson["second"];

		node->first = create_csgNode(first);
		node->second = create_csgNode(second);
	}

	else {
		if (nodejson["type"] == "sphere") {
			point3 center = vector_to_vec3(nodejson["position"]);
			float radius = float(nodejson["radius"]);
			node = new csg_node(new Sphere(center, radius, Material()));
		}
		if (nodejson["type"] == "box") {
			BoundingBox box;
			point3 p1 = vector_to_vec3(nodejson["point1"]);
			point3 p2 = vector_to_vec3(nodejson["point2"]);
			box.minX = std::min(p1.x, p2.x);
			box.maxX = std::max(p1.x, p2.x);
			box.minY = std::min(p1.y, p2.y);
			box.maxY = std::max(p1.y, p2.y);
			box.minZ = std::min(p1.z, p2.z);
			box.maxZ = std::max(p1.z, p2.z);

			node = new csg_node(new Box(box, Material()));

		}
		if (nodejson["type"] == "mesh") {
			std::vector<json> triangles = nodejson["triangles"];

			Mesh* mesh = new Mesh(Material());

			for (int i = 0; i < triangles.size(); i++) {
				std::vector<json> trianglejson = triangles[i];

				point3 p0 = vector_to_vec3(trianglejson[0]);
				point3 p1 = vector_to_vec3(trianglejson[1]);
				point3 p2 = vector_to_vec3(trianglejson[2]);

				mesh->triangles.push_back(new Triangle(mesh, p0, p1, p2, Material()));
			}

			mesh->setBox();

			node = new csg_node(mesh);
		}
	}
	return node;
}

/****************************************************************************/

// additional ray functions

bool shadowRay(const point3& point, const point3& lightPos, point3& shadow) {
	return bvh->calcShadow(point, lightPos, shadow);
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

			mesh->setBox();

			Objects.push_back(mesh);
		}

		if (object["type"] == "texturemesh") {
			std::vector<json> triangles = object["triangles"];
			std::vector<json> uvCoords = object["uvCoords"];
			std::string texturefile = object["texture"];

			TextureMesh* mesh = new TextureMesh(material, PATH + texturefile);

			for (int i = 0; i < triangles.size(); i++) {
				std::vector<json> trianglejson = triangles[i];
				std::vector<json> uvjson = uvCoords[i];

				point3 p0 = vector_to_vec3(trianglejson[0]);
				point3 p1 = vector_to_vec3(trianglejson[1]);
				point3 p2 = vector_to_vec3(trianglejson[2]);

				uvCoord uv0 = vector_to_vec2(uvjson[0]);
				uvCoord uv1 = vector_to_vec2(uvjson[1]);
				uvCoord uv2 = vector_to_vec2(uvjson[2]);

				mesh->triangles.push_back(new TextureTriangle(mesh, p0, p1, p2, uv0, uv1, uv2, material));
			}

			Objects.push_back(mesh);
		}

		if (object["type"] == "bumpsphere") {
			point3 center = vector_to_vec3(object["position"]);
			float radius = float(object["radius"]);
			std::string bumpmapfile = object["bumpmap"];
			float bumpDepth = object["bumpdepth"];

			Objects.push_back(new BumpSphere(center, radius, material, PATH + bumpmapfile, bumpDepth));
		}

		if (object["type"] == "csgobject") {
			csgObject* newobject = new csgObject(material);
			newobject->root = create_csgNode(object);
			newobject->setBox();

			Objects.push_back(newobject);
		}

		if (object["type"] == "box") {
			BoundingBox box;
			point3 p1 = vector_to_vec3(object["point1"]);
			point3 p2 = vector_to_vec3(object["point2"]);
			box.minX = std::min(p1.x, p2.x);
			box.maxX = std::max(p1.x, p2.x);
			box.minY = std::min(p1.y, p2.y);
			box.maxY = std::max(p1.y, p2.y);
			box.minZ = std::min(p1.z, p2.z);
			box.maxZ = std::max(p1.z, p2.z);

			Objects.push_back(new Box(box, material));
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
		if (light["type"] == "rectangular") {
			point3 position = vector_to_vec3(light["position"]);
			point3 normal = vector_to_vec3(light["normal"]);
			float width = light["width"];
			float height = light["height"];
			point3 orientation = vector_to_vec3(light["orientation"]);
			int numSamples = light["samples"];
			Lights.push_back(new RectangularAreaLight(colour, position, normal, width, height, orientation, numSamples));
		}
		if (light["type"] == "circular") {
			point3 position = vector_to_vec3(light["position"]);
			point3 normal = vector_to_vec3(light["normal"]);
			float radius = light["radius"];
			int numSamples = light["samples"];
			Lights.push_back(new CircularAreaLight(colour, position, normal, radius, numSamples));
		}
	}

	// Create the BVH

	bvh = new BVH(Objects);
}

bool trace(const point3& e, const point3& s, colour3& colour, bool pick, int reflectionCount) {
	if (reflectionCount > MAX_REFLECTIONS) {
		if (pick)
			std::cout << "Maximum number of reflections reached." << std::endl;
		colour = colour3(0, 0, 0);
		return false;
	}

	Object* hitObject = NULL;
	point3 d = s - e;

	hitObject = bvh->findNearest(e, d);

	if (hitObject == NULL)
		return false;

	if (pick)
		std::cout << "object " << hitObject->type << " hit at {" << hitObject->cachedHitpoint[0] << ", " << hitObject->cachedHitpoint[1] << ", " << hitObject->cachedHitpoint[2] << "}" << std::endl;

	hitObject->lightPoint(e, d, Lights, colour, reflectionCount, pick);

	return true;
}