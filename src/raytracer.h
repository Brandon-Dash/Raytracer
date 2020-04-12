#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <glm/glm.hpp>

#define MAX_T 10000.
#define MAX_REFLECTIONS 16

typedef glm::vec3 point3;
typedef glm::vec3 colour3;

extern double fov;
extern colour3 background_colour;

void choose_scene(char const *fn);
bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick, int reflectionCount = 0);

bool shadowRay(const point3& point, const point3& lightPos, point3& shadow);


#endif