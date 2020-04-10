#include <glm/glm.hpp>

typedef glm::vec3 point3;
typedef glm::vec3 colour3;

extern double fov;
extern colour3 background_colour;

void choose_scene(char const *fn);
bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick, int reflectionCount = 0);

struct Material;
class Object;
struct Light;

bool isZero(const glm::vec3 vec);
void reflectRay(const point3& V, const point3& N, point3& R);
bool shadowTest(const point3& point, const point3& lightPos, point3& shadow);
void addDiffuse(const colour3& Id, const colour3& Kd, const point3& N, const point3& L, colour3& colour);
void addSpecular(const colour3& Is, const colour3& Ks, const float a, const point3& N, const point3& L, const point3& V, colour3& colour);
bool refractRay(const point3& Vi, const point3& N, const float& refraction, point3& Vr);
bool pointInTriangle(const point3& point, const point3& p1, const point3& p2, const point3& p3, const point3& n);