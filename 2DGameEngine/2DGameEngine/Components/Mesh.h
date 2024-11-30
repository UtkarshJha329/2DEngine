#pragma once

#include <vector>

class Mesh{

public:

	std::vector<float> vertices;
	std::vector<int> indices;

	unsigned int VBO, EBO, VAO;

};