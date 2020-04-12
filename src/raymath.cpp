#include "raymath.h"

bool refractRay(const point3& Vi, const point3& N, const float& refraction, point3& Vr) {
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

void reflectRay(const point3& V, const point3& N, point3& R) {
	R = glm::normalize(2 * glm::dot(N, V) * N - V);
}

bool pointInTriangle(const point3& point, const point3& p1, const point3& p2, const point3& p3, const point3& n) {
	float test1 = glm::dot(glm::cross(point - p1, p2 - p1), n);
	float test2 = glm::dot(glm::cross(point - p2, p3 - p2), n);
	float test3 = glm::dot(glm::cross(point - p3, p1 - p3), n);

	return (test1 >= 0 && test2 >= 0 && test3 >= 0) || (test1 <= 0 && test2 <= 0 && test3 <= 0);
}

void addDiffuse(const colour3& Id, const colour3& Kd, const point3& N, const point3& L, colour3& colour) {
	colour3 diffuse = Id * Kd * glm::dot(N, L);
	for (int i = 0; i < 3; i++) {
		if (diffuse[i] < 0)
			diffuse[i] = 0;
	}

	colour = colour + diffuse;
}

void addSpecular(const colour3& Is, const colour3& Ks, const float a, const point3& N, const point3& L, const point3& V, colour3& colour) {
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

bool isZero(const glm::vec3 vec) {
	return vec[0] == 0 && vec[1] == 0 && vec[2] == 0;
}