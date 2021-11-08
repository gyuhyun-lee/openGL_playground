#include "render.h"

static mesh *
GetMeshFromModel(std::vector<model>* models, model_type type)
{
	model* model = models->data() + type;
	return &model->mesh;
}

static void
RenderModel(model *model,
			glm::vec3 scale, r32 angleX, r32 angleY, r32 angleZ, glm::vec3 translate, 
			int windowWidth, int windowHeight, GLuint perObjectUbo, void *ubo, u32 uboSize, 
			GLuint diffuseTextureID, GLuint specularTextureID)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	if (diffuseTextureID)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseTextureID);
	}
	if (specularTextureID)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularTextureID);
	}

	// NOTE(joon) : Bind vertex buffer
	glBindVertexArray(model->vertexArrayID);
	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBufferID);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBufferID);

	// NOTE(joon) : Update uniform buffer
	glm::mat4 *modelMatrix = (glm::mat4 *)ubo;
	*modelMatrix = glm::translate(glm::mat4(1.0f), translate)*
					glm::rotate(angleZ, glm::vec3(0.0f, 0.0f, 1.0f)) * 
					glm::rotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f)) * 
					glm::rotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f)) * 
					glm::scale(scale);

	glBindBuffer(GL_UNIFORM_BUFFER, perObjectUbo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, perObjectUbo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, uboSize, ubo);

	glDrawElements(GL_TRIANGLES, (u32)model->mesh.indexBuffer.size(), GL_UNSIGNED_INT, 0);

	// NOTE(joon) : cleanup
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void
RenderFaceNormal(model *model, glm::vec3 scale, r32 angle, glm::vec3 translate, 
			int windowWidth, int windowHeight, GLuint perObjectUbo)
{

	glBindVertexArray(model->faceNormalArrayID);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, model->faceNormalBufferID);

	// NOTE(joon) : Update uniform buffer
	per_object_ubo ubo = {};
	ubo.model = glm::translate(glm::mat4(1.0f), translate)*glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(scale);

	glBindBuffer(GL_UNIFORM_BUFFER, perObjectUbo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, perObjectUbo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_object_ubo), &ubo);

	glDrawArrays(GL_LINES, 0, 2*(u32)model->mesh.faceNormalBuffer.size());

	// NOTE(joon) : cleanup
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void
RenderVertexNormal(model *model, glm::vec3 scale, r32 angle, glm::vec3 translate, 
			int windowWidth, int windowHeight, GLuint perObjectUbo)
{

	glBindVertexArray(model->vertexNormalArrayID);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexNormalBufferID);

	// NOTE(joon) : Update uniform buffer
	per_object_ubo ubo = {};
	ubo.model = glm::translate(glm::mat4(1.0f), translate)*glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(scale);

	glBindBuffer(GL_UNIFORM_BUFFER, perObjectUbo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, perObjectUbo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_object_ubo), &ubo);

	glDrawArrays(GL_LINES, 0, 2*(u32)model->mesh.vertexNormalLineBuffer.size());

	// NOTE(joon) : cleanup
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// NOTE(joon) : THIS ROUTINE IS EXTREMELY SLOW, as it updates the buffer every time you draw the line.
// Only use this for small amount of lines!
static void
RenderLine(glm::vec3 *line, GLuint lineVertexArrayID, GLuint lineVertexBufferID, 
			glm::vec3 scale, r32 angle, glm::vec3 translate, 
			int windowWidth, int windowHeight, GLuint perObjectUbo)
{
	glBindVertexArray(lineVertexArrayID);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, lineVertexBufferID);

#if 1
	glBufferData(GL_ARRAY_BUFFER,
				2 * sizeof(glm::vec3), line,
				GL_STATIC_DRAW);
#endif

	glm::vec3 start = line[0];
	glm::vec3 end = line[1];

	// NOTE(joon) : Update uniform buffer
	per_object_ubo ubo = {};
	ubo.model = glm::translate(glm::mat4(1.0f), translate)*glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(scale);

	glBindBuffer(GL_UNIFORM_BUFFER, perObjectUbo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, perObjectUbo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_object_ubo), &ubo);

	glDrawArrays(GL_LINES, 0, 2);
}

static void
PlanarTextureMapping(vertex *vertices, u32 vertexCount, b32 shouldUseP)
{
	for (u32 vertexIndex = 0;
		vertexIndex < vertexCount;
		++vertexIndex)
	{
		vertex *vertex = vertices + vertexIndex;
		glm::vec3 p = shouldUseP ? vertex->p : vertex->normal;

		r32 absX = abs(p.x);
		r32 absY = abs(p.y);
		r32 absZ = abs(p.z);

		r32 u = 0.0f;
		r32 v = 0.0f;

		if( absX >= absY && absX >= absZ )
		{
			// +-X
			if (p.x < 0.0f)
			{
				u = p.z;
			}
			else
			{
				u = -p.z;
			}

			v = p.y;
		}
		else if (absY >= absX && absY >= absZ)
		{
			// +-Y
			if (p.y < 0.0f)
			{
				v = p.z;
			}
			else
			{
				v = -p.z;
			}

			u = p.x;
		}
		else if (absZ >= absY && absZ >= absX)
		{
			// +-Z
			if (p.z < 0.0f)
			{
				u = -p.x;
			}
			else
			{
				u = p.x;
			}

			v = p.y;
		}

		vertex->texCoord.x = 0.5f*(u + 1.0f);
		vertex->texCoord.y = 0.5f*(v + 1.0f);
	}
}

static void
CylindricalTextureMapping(vertex *vertices, u32 vertexCount, b32 shouldUseP)
{
	for (u32 vertexIndex = 0;
		vertexIndex < vertexCount;
		++vertexIndex)
	{
		vertex *vertex = vertices + vertexIndex;
		glm::vec3 p = shouldUseP ? vertex->p : vertex->normal;

		r32 theta = atanf(p.y/p.x);

		vertex->texCoord.x = theta/Two_Pi32;
		vertex->texCoord.y = (p.z + 1.0f)/2.0f; // (z - zMin)/(zMax - zMin)
	}
}

static void
SphericalTextureMapping(vertex *vertices, u32 vertexCount, b32 shouldUseP)
{
	// bounding box is ranging from -1 to 1, spherical coordinate uses x,y,z
	r32 r = sqrtf(1+1+1);
	for (u32 vertexIndex = 0;
		vertexIndex < vertexCount;
		++vertexIndex)
	{
		vertex *vertex = vertices + vertexIndex;
		glm::vec3 p = shouldUseP ? vertex->p : vertex->normal;

		r32 theta = atanf(p.y/p.x);
		r32 pi = acosf(p.z/r);

		vertex->texCoord.x = theta/Two_Pi32; // theta/360
		vertex->texCoord.y = pi/Pi32; //pi/180
	}
}


static void
GenerateTexCoordForAllModels(std::vector<model> *models, i32 textureWidth, i32 textureHeight, int method, b32 shouldUseP)
{
	for (u32 modelIndex = 0;
		modelIndex < (u32)models->size();
		++modelIndex)
	{
		model *model = models->data() + modelIndex;
		switch (method)
		{
			case TextureMappingMethod_Planar:
			{
				PlanarTextureMapping(model->mesh.vertexBuffer.data(),
									(u32)model->mesh.vertexBuffer.size(), shouldUseP);
			}break;
			case TextureMappingMethod_Cylindrical:
			{
				CylindricalTextureMapping(model->mesh.vertexBuffer.data(),
									(u32)model->mesh.vertexBuffer.size(), shouldUseP);
			}break;
			case TextureMappingMethod_Spherical:
			{
				SphericalTextureMapping(model->mesh.vertexBuffer.data(),
									(u32)model->mesh.vertexBuffer.size(), shouldUseP);
			}break;
		}
		// update the buffer. otherwise, opengl will not know the change inside the vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, model->vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER,
					model->mesh.vertexBuffer.size() * sizeof(vertex), model->mesh.vertexBuffer.data(),
				GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}
