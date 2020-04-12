#include "objects.h"
#include "raytracer.h"
#include "raymath.h"

#include <iostream>

/****************************************************************************/

// Bounding box

float BoundingBox::intersect(point3 e, point3 d) {
	// This code is taken from the lecture slide for Kay-Kajiya intersection.
	// return value -1 means miss, 0 means ray is originating inside the box
	float tnear = -MAX_T;
	float tfar = MAX_T;

	float min[3] = { minX, minY, minZ };
	float max[3] = { maxX, maxY, maxZ };

	for (int axis = 0; axis < 3; axis++) {
		if (d[axis] == 0 && (e[axis] < min[axis] || e[axis] > max[axis]))
			return -1;
		float t1 = (min[axis] - e[axis]) / d[axis];
		float t2 = (max[axis] - e[axis]) / d[axis];
		if (t1 > t2) std::swap(t1, t2);
		if (t1 > tnear) tnear = t1;
		if (t2 < tfar) tfar = t2;
		if (tnear > tfar) return -1;
		if (tfar < 0) return -1;
	}
	
	if (tnear < 0)
		return 0;
	else
		return tnear;
}

/****************************************************************************/

// Objects

/****************************************************************************/

// Object

void Object::lightPoint(point3 e, point3 d, std::vector<Light*> Lights, colour3& colour, int reflectionCount, bool pick) {
	point3 p = cachedHitpoint;
	point3 V = glm::normalize(-d);
	point3 N;
	getNormal(N);

	colour = colour3(0.0, 0.0, 0.0);

	if (!isZero(material.reflective)) {
		if (pick)
			std::cout << "reflection:" << std::endl;
		point3 R;
		reflectRay(V, N, R);
		bool hit = trace(p + float(1e-5) * R, p + R, colour, pick, reflectionCount + 1);
		if (!hit)
			colour = background_colour;

		if (pick && !hit)
			std::cout << "no additional objects hit" << std::endl;

		colour = colour * material.reflective;
	}

	for (int i = 0; i < Lights.size(); i++) {
		Lights[i]->lightPoint(p, N, V, material, colour);
	}

	if (!isZero(material.transmissive)) {
		if (pick)
			std::cout << "transmission:" << std::endl;

		colour3 transcolour = colour3(0.0, 0.0, 0.0);
		point3 transmissionOrigin, transmissionDirection;

		bool result = transmitRay(p, d, N, transmissionOrigin, transmissionDirection, pick);

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

		colour = (colour3(1.0, 1.0, 1.0) - material.transmissive) * colour + material.transmissive * transcolour;
	}
}

bool Object::transmitRay(point3 inPoint, point3 inVector, point3 inNormal, point3& outPoint, point3& outVector, bool pick) {
	if (material.refraction == 0) {
		outVector = inVector;
		outPoint = inPoint + 1e-5f * outVector;
		return true;
	}

	point3 currentPoint = inPoint;
	point3 innerVector;
	point3 outNormal;

	refractRay(inVector, inNormal, material.refraction, innerVector);

	bool result = false;
	int reflections = 0;
	while (!result && reflections < MAX_REFLECTIONS) {
		rayhit(currentPoint, innerVector, true);
		outPoint = cachedHitpoint;
		getNormal(outNormal);

		result = refractRay(innerVector, outNormal, material.refraction, outVector);

		if (!result) {
			if (pick)
				std::cout << "interior reflection at {" << outPoint[0] << ", " << outPoint[1] << ", " << outPoint[2] << "}" << std::endl;
			point3 R;
			reflectRay(-innerVector, outNormal, R);
			innerVector = R;
			currentPoint = outPoint;
			reflections++;
		}
	}
	return result;
}

/****************************************************************************/

// Sphere

Sphere::Sphere(point3 center, float radius, Material material) {
	this->center = center;
	this->radius = radius;
	this->material = material;
	type = "sphere";
	boundingBox.minX = center.x - radius;
	boundingBox.maxX = center.x + radius;
	boundingBox.minY = center.y - radius;
	boundingBox.maxY = center.y + radius;
	boundingBox.minZ = center.z - radius;
	boundingBox.maxZ = center.z + radius;
}

float Sphere::rayhit(point3 e, point3 d, bool exit) {
	double disc = pow(glm::dot(d, e - center), 2) - glm::dot(d, d) * (glm::dot(e - center, e - center) - radius * radius);

	if (disc < 0)
		return 0;

	double rest = glm::dot(-d, e - center) / glm::dot(d, d);
	double t;

	if (!exit)
		t = rest - sqrt(disc) / glm::dot(d, d);
	else
		t = rest + sqrt(disc) / glm::dot(d, d);

	if (t < 0)
		return 0;

	cachedHitpoint = e + float(t) * d;
	return float(t);
}

void Sphere::getNormal(point3& n) {
	n = glm::normalize(cachedHitpoint - center);
}

void Sphere::getCentroid(point3& c) {
	c = center;
}

/****************************************************************************/

// Plane

Plane::Plane(point3 point, point3 normal, Material material) {
	this->point = point;
	this->normal = normal;
	this->material = material;
	type = "plane";
}

float Plane::rayhit(point3 e, point3 d, bool exit) {
	point3 n = normal;
	if (exit)
		n = -normal;

	double numerator = glm::dot(n, point - e);
	double denominator = glm::dot(n, d);
	double t = numerator / denominator;

	if (t <= 0 || numerator > 0)
		return 0;

	cachedHitpoint = e + float(t) * d;
	return float(t);
}

void Plane::getNormal(point3& n) {
	n = glm::normalize(normal);
}

void Plane::getCentroid(point3& c) {
	// shouldn't be called for planes
	throw std::exception("Not implemented.");
}

bool Plane::transmitRay(point3 inPoint, point3 inVector, point3 inNormal, point3& outPoint, point3& outVector, bool pick) {
	// planes don't refract
	outVector = inVector;
	outPoint = inPoint + 1e-5f * outVector;
	return true;
}

/****************************************************************************/

// Triangle

Triangle::Triangle(Mesh* mesh, point3 p0, point3 p1, point3 p2, Material material) {
	this->type = "triangle";
	this->material = material;
	this->mesh = mesh;
	this->points[0] = p0;
	this->points[1] = p1;
	this->points[2] = p2;
	this->normal = glm::normalize(glm::cross(p1 - p0, p2 - p1));
	boundingBox.minX = std::min({ points[0].x, points[1].x, points[2].x });
	boundingBox.maxX = std::max({ points[0].x, points[1].x, points[2].x });
	boundingBox.minY = std::min({ points[0].y, points[1].y, points[2].y });
	boundingBox.maxY = std::max({ points[0].y, points[1].y, points[2].y });
	boundingBox.minZ = std::min({ points[0].z, points[1].z, points[2].z });
	boundingBox.maxZ = std::max({ points[0].z, points[1].z, points[2].z });
}

float Triangle::rayhit(point3 e, point3 d, bool exit) {
	float t = Plane(points[0], normal, material).rayhit(e, d, exit);
	point3 hitpos = e + t * d;

	if (t <= 0 || !pointInTriangle(hitpos, points[0], points[1], points[2], normal))
		return 0;
	else {
		cachedHitpoint = e + t * d;
		return t;
	}
}

void Triangle::getNormal(point3& n) {
	n = normal;
}

void Triangle::getCentroid(point3& c) {
	c.x = (points[0].x + points[1].x + points[2].x) / 3;
	c.y = (points[0].y + points[1].y + points[2].y) / 3;
	c.z = (points[0].z + points[1].z + points[2].z) / 3;
}

bool Triangle::transmitRay(point3 inPoint, point3 inVector, point3 inNormal, point3& outPoint, point3& outVector, bool pick) {
	return mesh->transmitRay(inPoint, inVector, inNormal, outPoint, outVector, pick);
}

/****************************************************************************/

// Mesh

Mesh::Mesh(Material material) {
	this->material = material;
	type = "mesh";
}

float Mesh::rayhit(point3 e, point3 d, bool exit) {
	float t_min = MAX_T;

	for (int i = 0; i < triangles.size(); i++) {
		Triangle* triangle = triangles[i];

		float t = triangle->rayhit(e, d, exit);
		point3 hitpos = e + t * d;

		if (t > 0.0 && t < t_min) {
			cachedHitpoint = hitpos;
			triangle->getNormal(cachedHitNormal);
			t_min = t;
		}
	}
	if (t_min == MAX_T)
		return 0.0;
	return t_min;
}

void Mesh::getNormal(point3& n) {
	n = cachedHitNormal;
}

void Mesh::getCentroid(point3& c) {
	// shouldn't be called for meshes
	throw std::exception("Not implemented.");
}

/****************************************************************************/

// Lights

/****************************************************************************/

// Ambient

Ambient::Ambient(colour3 colour) {
	this->colour = colour;
	type = "ambient";
}

void Ambient::lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour) {
	colour3 I = colour;

	colour3 ambient = I * material.ambient;
	pointColour += ambient;
}

/****************************************************************************/

// Directional

Directional::Directional(colour3 colour, point3 direction) {
	this->colour = colour;
	this->direction = glm::normalize(direction);
	type = "directional";
}

void Directional::lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour) {
	point3 L = -direction;
	point3 lightPosition = p + float(MAX_T) * L; // virtual position of light for use in shadow test

	colour3 shadow = colour3(1.0, 1.0, 1.0);
	if (shadowRay(p, lightPosition, shadow)) {
		colour3 I = colour * shadow;

		addDiffuse(I, material.diffuse, N, L, pointColour);
		addSpecular(I, material.specular, material.shininess, N, L, V, pointColour);
	}
}

/****************************************************************************/

// Point

Point::Point(colour3 colour, point3 position) {
	this->colour = colour;
	this->position = position;
	type = "point";
}

void Point::lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour) {
	colour3 shadow = colour3(1.0, 1.0, 1.0);
	if (shadowRay(p, position, shadow)) {
		colour3 I = colour * shadow;
		point3 L = glm::normalize(position - p);

		addDiffuse(I, material.diffuse, N, L, pointColour);
		addSpecular(I, material.specular, material.shininess, N, L, V, pointColour);
	}
}

/****************************************************************************/

// Spot

Spot::Spot(point3 colour, point3 position, point3 direction, float cutoff) {
	this->colour = colour;
	this->position = position;
	this->direction = glm::normalize(direction);
	this->cutoff = cutoff;
	type = "spot";
}

void Spot::lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour) {
	colour3 shadow = colour3(1.0, 1.0, 1.0);
	if (shadowRay(p, position, shadow)) {
		point3 dir = -direction;
		point3 L = glm::normalize(position - p);

		if (glm::dot(L, dir) > cos(cutoff * M_PI / 180)) {
			colour3 I = colour * shadow;

			addDiffuse(I, material.diffuse, N, L, pointColour);
			addSpecular(I, material.specular, material.shininess, N, L, V, pointColour);
		}
	}
}