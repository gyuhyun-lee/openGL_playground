#include <fstream>

struct vertex_hit
{
	glm::vec3 normalSum;
	u32 hitCount = 0;
};

#define Minimum(a, b) ((a < b) ? a : b)
#define Maximum(a, b) ((a > b)? a : b)

static void
UpdateBoundingBox(glm::vec3 *min, glm::vec3 *max, glm::vec3 value)
{
	min->x = Minimum(min->x, value.x);
	min->y = Minimum(min->y, value.y);
	min->z = Minimum(min->z, value.z);

	max->x = Maximum(max->x, value.x);
	max->y = Maximum(max->y, value.y);
	max->z = Maximum(max->z, value.z);
}

static void
GenerateVertexAndFaceNormals(mesh *mesh)
{
	std::vector<vertex_hit>vertexHitCount(mesh->vertexBuffer.size());
	u32 faceCount = (u32)mesh->indexBuffer.size()/3;
	mesh->faceNormalBuffer.resize(faceCount);
	for (u32 faceIndex = 0;
		faceIndex < faceCount;
		++faceIndex)
	{
		line* line = mesh->faceNormalBuffer.data() + faceIndex;

		GLuint firstIndex = mesh->indexBuffer[3*(u64)faceIndex + 0];
		GLuint secondIndex = mesh->indexBuffer[3*(u64)faceIndex + 1];
		GLuint thirdIndex = mesh->indexBuffer[3*(u64)faceIndex + 2];

		glm::vec3 firstVertex = mesh->vertexBuffer[firstIndex].p;
		glm::vec3 secondVertex = mesh->vertexBuffer[secondIndex].p;
		glm::vec3 thirdVertex = mesh->vertexBuffer[thirdIndex].p;

		glm::vec3 v01 = secondVertex - firstVertex;
		glm::vec3 v02 = thirdVertex - firstVertex;

		glm::vec3 faceNormal = glm::normalize(glm::cross(v01, v02));
		line->start = (firstVertex + secondVertex + thirdVertex)/3.0f;
		line->end = line->start + faceNormal;

		++vertexHitCount[firstIndex].hitCount;
		vertexHitCount[firstIndex].normalSum += faceNormal;

		++vertexHitCount[secondIndex].hitCount;
		vertexHitCount[secondIndex].normalSum += faceNormal;

		++vertexHitCount[thirdIndex].hitCount;
		vertexHitCount[thirdIndex].normalSum += faceNormal;
	}

	mesh->vertexNormalLineBuffer.resize(mesh->vertexBuffer.size());
	for (u32 vertexIndex = 0;
		vertexIndex < mesh->vertexBuffer.size();
		++vertexIndex)
	{
		mesh->vertexBuffer[vertexIndex].normal = glm::normalize((vertexHitCount[vertexIndex].normalSum / (r32)vertexHitCount[vertexIndex].hitCount));

		mesh->vertexNormalLineBuffer[vertexIndex].start = mesh->vertexBuffer[vertexIndex].p;
		mesh->vertexNormalLineBuffer[vertexIndex].end = mesh->vertexBuffer[vertexIndex].p + mesh->vertexBuffer[vertexIndex].normal;
	}
}

void
ReadOBJFileLineByLine(mesh *mesh, const char* fileName)
{
	glm::vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 max(FLT_MIN, FLT_MIN, FLT_MIN);

	std::vector< glm::vec3> vertexNormalBuffer;

	glm::vec3 verticesAverage = {};

	std::ifstream file;
	file.open(fileName);

	if (file.bad() || file.eof() || file.fail())
	{
		printf("Invalid OBJ file!\n");
	}

	while (!file.eof())
	{
		char buffer[256] = "\0";
		file.getline(buffer, ArrayCount(buffer), 
						'\n'); // NOTE(joon) : stop when this character appears

		const char* delimit = " \r\n\t";

		char* token = strtok(buffer, delimit);

		if (token)
		{
			switch (token[0])
			{
				// NOTE(joon) : vertex
				case 'v':
				{
					if (token[1] == '\0')
					{
						vertex vertex;
						token = strtok(nullptr, delimit);
						vertex.p.x = (GLfloat)atof(token);

						token = strtok(nullptr, delimit);
						vertex.p.y = (GLfloat)atof(token);

						token = strtok(0, delimit);
						vertex.p.z = (GLfloat)atof(token);

						UpdateBoundingBox(&min, &max, vertex.p);
						verticesAverage += vertex.p;
						mesh->vertexBuffer.push_back(vertex);
					}

					// We don't load vertex normal from the file,
					// we generate it ourselves.
#if 0
					else if (token[1] == 'n')
					{
						// NOTE(joon) : vertex normal
						glm::vec3 normal;

						token = strtok(0, delimit);
						if (token == nullptr)
						{
							break;
						}
						normal.x = (GLfloat)atof(token);

						token = strtok(0, delimit);
						if (token == nullptr)
						{
							break;
						}
						normal.y = (GLfloat)atof(token);

						token = strtok(0, delimit);
						if (token == nullptr)
						{
							break;
						}
						normal.z = (GLfloat)atof(token);

						vertexNormalBuffer.push_back(glm::normalize(normal));
					}
#endif
				}break;

				// NOTE(joon) : face
				case 'f':
				{
					GLuint firstIndex;

					GLuint secondIndex;

					GLuint thirdIndex;

					token = strtok(0, delimit);
					if (token == nullptr)
					{
						break;
					}
					firstIndex = (unsigned int)(atoi(token) - 1); // NOTE(joon) : obj file index starts from 1, but ours start from 0

					token = strtok(0, delimit);
					if (token == nullptr)
					{
						break;
					}
					secondIndex = (unsigned int)(atoi(token) - 1);

					token = strtok(0, delimit);
					if (token == nullptr)
					{
						break;
					}
					thirdIndex = (unsigned int)(atoi(token) - 1);

					mesh->indexBuffer.push_back(firstIndex);
					mesh->indexBuffer.push_back(secondIndex);
					mesh->indexBuffer.push_back(thirdIndex);

					// NOTE(joon) : Get all the indexes inside the line 'face'
					token = strtok(nullptr, delimit);
					while (token != nullptr)
					{
						secondIndex = thirdIndex;
						thirdIndex = static_cast<unsigned int&&>(atoi(token) - 1);

						mesh->indexBuffer.push_back(firstIndex);
						mesh->indexBuffer.push_back(secondIndex);
						mesh->indexBuffer.push_back(thirdIndex);

						token = strtok(nullptr, delimit);
					}
				}break;

				case '#':
				default: 
				break;
			}
		}
	}

	// NOTE(joon) : Center the model to (0, 0)
	verticesAverage /= mesh->vertexBuffer.size();
	for (u32 vertexIndex = 0;
		vertexIndex < mesh->vertexBuffer.size();
		++vertexIndex)
	{
		mesh->vertexBuffer[vertexIndex].p -= verticesAverage;
	}

	r32 xDiff = (max.x - min.x)/2.0f;
	r32 yDiff = (max.y - min.y)/2.0f;
	r32 zDiff = (max.z - min.z)/2.0f;

	r32 maxDiff = Maximum(Maximum(xDiff, yDiff), zDiff);
	
	// NOTE(joon) : Resize the model so that it can fit inside the [-1, 1] bounding box
	for (u32 vertexIndex = 0;
		vertexIndex < mesh->vertexBuffer.size();
		++vertexIndex)
	{
		mesh->vertexBuffer[vertexIndex].p.x /= maxDiff;
		mesh->vertexBuffer[vertexIndex].p.y /= maxDiff;
		mesh->vertexBuffer[vertexIndex].p.z /= maxDiff;
	}
	
	GenerateVertexAndFaceNormals(mesh);
}

void
GenerateSphereModel(model *model, r32 radius, u32 horizontalCount, u32 verticalCount)
{
	r32 horizontalStep = 2 * Pi32 / (r32)horizontalCount;
	r32 verticalStep = Pi32 / (r32)verticalCount;
	r32 horizontalAngle;
	r32	verticalAngle;

	for (u32 verticalIndex = 0; 
		verticalIndex <= verticalCount; 
		++verticalIndex)
	{
		glm::vec3 p;
		glm::vec3 normal;
		r32 oneOverRadius = 1.0f / radius;

		verticalAngle = Pi32 / 2 - verticalIndex * verticalStep;
		r32 xy = radius * cosf(verticalAngle);
		p.z = radius * sinf(verticalAngle);

		for (u32 horizontalIndex = 0; 
			horizontalIndex <= horizontalCount; 
			++horizontalIndex)
		{
			horizontalAngle = horizontalIndex * horizontalStep;

			vertex vertex;

			// vertex position
			p.x = xy * cosf(horizontalAngle); 
			p.y = xy * sinf(horizontalAngle);
			vertex.p = p;

			// normalized vertex normal
			normal = { p.x * oneOverRadius,
						p.y * oneOverRadius,
						 p.z * oneOverRadius };
			vertex.normal = normal;

			model->mesh.vertexBuffer.push_back(vertex);
		}
	}

	for (u32 verticalIndex = 0; 
		verticalIndex < verticalCount; 
		++verticalIndex)
	{
		u32 firstIndex = verticalIndex * (horizontalCount + 1);
		u32 secondIndex = firstIndex + horizontalCount + 1; 

		for (u32 horizontalIndex = 0; 
			horizontalIndex < horizontalCount; 
			++horizontalIndex, ++firstIndex, ++secondIndex)
		{
			if (verticalIndex != 0)
			{
				model->mesh.indexBuffer.push_back(firstIndex);
				model->mesh.indexBuffer.push_back(secondIndex);
				model->mesh.indexBuffer.push_back(firstIndex + 1);
			}

			if (verticalIndex != (verticalCount - 1))
			{
				model->mesh.indexBuffer.push_back(firstIndex + 1);
				model->mesh.indexBuffer.push_back(secondIndex);
				model->mesh.indexBuffer.push_back(secondIndex + 1);
			}
		}
	}
}
