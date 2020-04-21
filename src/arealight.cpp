#include "arealight.h"
#include "raytracer.h"
#include "raymath.h"

#define RAND ((float)rand() / RAND_MAX) // random number between 0 and 1

void AreaLight::lightPoint(point3 p, point3 N, point3 V, Material material, colour3& pointColour) {
	colour3 totalColour = colour3(0.0, 0.0, 0.0);

	for (int i = 0; i < numSamples; i++) {
		point3 lightPos;
		randomPoint(lightPos);
		colour3 shadow = colour3(1.0, 1.0, 1.0);
		if (shadowRay(p, lightPos, shadow)) {
			colour3 I = colour * shadow;
			point3 L = glm::normalize(lightPos - p);

			addDiffuse(I, material.diffuse, N, L, totalColour);
			addSpecular(I, material.specular, material.shininess, N, L, V, totalColour);
		}
	}
	pointColour = totalColour / float(numSamples);
}

RectangularAreaLight::RectangularAreaLight(colour3 colour, point3 position, point3 normal, float width, float height, point3 orientation, int numSamples) {
	type = "rectangularAreaLight";
	this->colour = colour;
	this->position = position;
	this->normal = normal;
	this->width = width;
	this->height = height;
	this->numSamples = numSamples;

	planeX = glm::normalize(glm::cross(orientation, normal));
	planeY = glm::normalize(glm::cross(normal, planeX));
}

void RectangularAreaLight::randomPoint(point3& p) {
	float xDisplacement = (2 * RAND - 1) * width;
	float yDisplacement = (2 * RAND - 1) * height;

	p = position + planeX * xDisplacement + planeY * yDisplacement;
}

CircularAreaLight::CircularAreaLight(colour3 colour, point3 position, point3 normal, float radius, int numSamples) {
	type = "circularAreaLight";
	this->colour = colour;
	this->position = position;
	this->normal = normal;
	this->radius = radius;
	this->numSamples = numSamples;

	planeX = glm::normalize(glm::cross(point3(0, 1, 0), normal));
	planeY = glm::normalize(glm::cross(normal, planeX));
}

void CircularAreaLight::randomPoint(point3& p) {
	float angle = RAND * 2 * M_PI;
	float displacement = radius * sqrt(RAND);

	float xDisplacement = displacement * cos(angle);
	float yDisplacement = displacement * sin(angle);

	p = position + planeX * xDisplacement + planeY * yDisplacement;
}