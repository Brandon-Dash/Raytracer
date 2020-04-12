#ifndef RAYMATH_H
#define RAYMATH_H

#include <glm/glm.hpp>

typedef glm::vec3 point3;
typedef glm::vec3 colour3;

void addDiffuse(const colour3& Id, const colour3& Kd, const point3& N, const point3& L, colour3& colour);
void addSpecular(const colour3& Is, const colour3& Ks, const float a, const point3& N, const point3& L, const point3& V, colour3& colour);
bool refractRay(const point3& Vi, const point3& N, const float& refraction, point3& Vr);
bool pointInTriangle(const point3& point, const point3& p1, const point3& p2, const point3& p3, const point3& n);
void reflectRay(const point3& V, const point3& N, point3& R);
bool isZero(const glm::vec3 vec);

#endif