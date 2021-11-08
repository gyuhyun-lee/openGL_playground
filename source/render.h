#ifndef RENDER_H
#define RENDER_H

struct camera
{
	glm::vec3 initP;
	r32 angle;
	glm::vec3 lookAtP;
	glm::vec3 up;

	r32 fovInDegree = 0.0f;
	r32 near = 0.0f;
	r32 far = 0.0f;
};
struct line
{
	glm::vec3 start;
	glm::vec3 end;
};

struct vertex
{
	glm::vec3 p; // -1 to 1
	glm::vec3 normal; // -1 to 1
	glm::vec2 texCoord;
};

struct mesh
{
	std::vector < vertex > vertexBuffer;
	std::vector < unsigned int > indexBuffer;

	std::vector < line > faceNormalBuffer;
	std::vector < line > vertexNormalLineBuffer;
};

enum light_type
{
	LightType_Point = 0,
	LightType_Directional = 1,
	LightType_SpotLight = 2,
};

struct light
{
	alignas(16) bool isEnabled;
	
	alignas(4) r32 angle;
	alignas(4) u32 type;

	alignas(16) glm::vec3 p;

	alignas(16) glm::vec3 IAmbient;
	alignas(16) glm::vec3 IDiffuse;
	alignas(16) glm::vec3 ISpecular;

	alignas(4) r32 c1;
	alignas(4) r32 c2;
	alignas(4) r32 c3;


	// spotlight only
	alignas(4) r32 innerConeAngleCos;
	alignas(4) r32 outerConeAngleCos;
	alignas(4) r32 fallOff;
};

struct per_frame_ubo
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;

	// TODO : align?
	alignas(16) glm::vec3 cameraP;
	alignas(16) glm::vec3 cameraDir;
	alignas(16) glm::vec3 IFog;
	alignas(16) glm::vec3 globalAmbient;

	alignas(4) r32 zNear;
	alignas(4) r32 zFar;

	// Only relevant if texture mapping location = GPU
	alignas(4) bool shouldGenerateTexCoordInGPU;
	alignas(4) int textureMappingMethod;
	alignas(4) bool shouldUseNormal;

	light lights[16];
};

struct per_object_ubo
{
	// for object ubo, model _must_ be at the top of the struct, like the header!
	alignas(16) glm::mat4 model;

	alignas(16) glm::vec3 IEmissive;

	alignas(4) r32 kAmbient;
	alignas(4) r32 kDiffuse;
	alignas(4) r32 kSpecular;

	alignas(4) r32 ns;
};

struct plain_per_object_ubo
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::vec3 color;
};

enum model_type
{ 
	ModelType_4Sphere,
	ModelType_bunny,
	ModelType_bunny_high_poly,
	ModelType_cube,
	ModelType_cube2,
	ModelType_cup,
	ModelType_lucy_princeton,
	ModelType_quad,
	ModelType_rhino,
	ModelType_sphere,
	ModelType_sphere_modified,
	ModelType_starwars1,
	ModelType_triangle,
	ModelType_count,
};

struct model
{
	struct mesh mesh;

	GLuint vertexArrayID = 0;
	GLuint vertexBufferID = 0;
	GLuint indexBufferID = 0;

	// NOTE(joon) : only for drawing face normal using GL_LINES!
	GLuint faceNormalArrayID = 0;
	GLuint faceNormalBufferID = 0;

	// NOTE(joon) : only for drawing vertex normal using GL_LINES!
	GLuint vertexNormalArrayID = 0;
	GLuint vertexNormalBufferID = 0;

	// NOTE : Light properties
	glm::vec3 IEmissive;
	r32 kAmbient;
	r32 kDiffuse;
	r32 kSpecular;
	r32 ns;
};

enum texture_mapping_method
{
	TextureMappingMethod_Planar = 0,
	TextureMappingMethod_Cylindrical = 1,
	TextureMappingMethod_Spherical = 2,
};

enum texture_mapping_location
{
	TextureMappingLocation_CPU = 0,
	TextureMappingLocation_GPU = 1,
};

#endif