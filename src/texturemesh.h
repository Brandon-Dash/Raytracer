#ifndef TEXTUREMESH_H
#define TEXTUREMESH_H

#include "objects.h";
#include "EasyBMP/EasyBMP.h"
#include <array>

typedef glm::vec2 uvCoord;

class TextureMesh : public Mesh {
public:
	BMP texture;
	void getTexValue(float u, float v, colour3& colour);
};

class TextureTriangle : public Triangle {
	std::array<uvCoord, 3> uvCoords;
	void lightPoint(point3 e, point3 d, std::vector<Light*> Lights, colour3& colour, int reflectionCount, bool pick);
};

#endif