#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/gtx/transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <vector>

#include <stdint.h>
#include <float.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;

typedef float r32;
typedef double r64;

// NOTE : A fast way to assert certain codepath
#define Assert(expression) if(!(expression)) {int *a = 0; *a = 1;}

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define Pi32 3.1415926535897932384f
#define Two_Pi32 6.2831853071795864768f

#include "render.cpp"
#include "obj_reader.cpp"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

static r32
Clamp(r32 min, r32 value, r32 max)
{
	r32 result = value;
	if (result < min)
	{
		result = min;
	}
	else if (result > max)
	{
		result = max;
	}

	return result;
}

struct read_file_result
{
	std::vector<char> result;
};
#include <fstream>
static void
ReadFile(std::vector<char>* buffer, const char* fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (file.is_open())
	{
		size_t fileSize = (size_t)file.tellg();
		buffer->resize(fileSize);

		file.seekg(0);
		file.read(buffer->data(), fileSize);

		file.close();
	}
	else
	{
		printf("Cannot Read File!\n");
	}
}

struct load_shader_result
{
	GLuint vertexShader;
	GLuint fragmentShader;
};
load_shader_result
LoadShaderSpirV(const char* vertexShaderCodePath, const char* fragmentShaderCodePath)
{
	load_shader_result result = {};

	// NOTE(joon) : Load spirv shaders
	const char* vertexShaderFilePath = vertexShaderCodePath;
	const char* fragmentShaderFilePath = fragmentShaderCodePath;

	// NOTE(joon) : Get vertex and fragment binary spv files(compiled from GLSL)
	std::vector<char> vertexShaderCode;
	std::vector<char> fragmentShaderCode;
	ReadFile(&vertexShaderCode, vertexShaderFilePath);
	ReadFile(&fragmentShaderCode, fragmentShaderFilePath);

	result.vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderBinary(1, &result.vertexShader, GL_SHADER_BINARY_FORMAT_SPIR_V,
		vertexShaderCode.data(), (u32)vertexShaderCode.size());
	glSpecializeShader(result.vertexShader, "main", 0, 0, 0);

	GLint isCompiled = false;
	glGetShaderiv(result.vertexShader, GL_COMPILE_STATUS, &isCompiled);
	if (!isCompiled)
	{
		printf("SPRV files should be pre-compiled!\n");
	}

	result.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderBinary(1, &result.fragmentShader, GL_SHADER_BINARY_FORMAT_SPIR_V,
		fragmentShaderCode.data(), (u32)fragmentShaderCode.size());
	glSpecializeShader(result.fragmentShader, "main", 0, 0, 0);

	isCompiled = false;
	glGetShaderiv(result.fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	if (!isCompiled)
	{
		printf("SPRV files should be pre-compiled!\n");
	}

	return result;
}

#include <string>
GLuint
LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::string Line;
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else 
	{
		printf("Impossible to open %s.\n", vertex_file_path);
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::string Line;
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}
	else 
	{
		printf("Impossible to open %s.\n", fragment_file_path);
		return 0;
	}

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	GLint vertexShaderResult = GL_FALSE;
	GLint fragShaderResult = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, nullptr);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &vertexShaderResult);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, nullptr);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &fragShaderResult);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	GLuint programID = glCreateProgram();
	if (vertexShaderResult && fragShaderResult)
	{
		// Link the program
		printf("Linking program\n");
		glAttachShader(programID, VertexShaderID);
		glAttachShader(programID, FragmentShaderID);
		glLinkProgram(programID);

		// TODO: use glGetProgramInfoLog to see what kind of information it gives!
		GLint isProgramValid = false;

		// Check the program
		glGetProgramiv(programID, GL_LINK_STATUS, &isProgramValid);
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(programID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
			printf("%s\n", &ProgramErrorMessage[0]);
		}
		else
		{
			printf("Program succefully created and linked!\n\n");
		}
	}
	else
	{
		printf("vertex shader or fragment shader is not valid, cannot make a program\n\n");
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return programID;
}

// preset 1 : Every light is the same type
static void 
ConfigureLightPreset1(light *lights, u32 lightCount)
{
	for (u32 lightIndex = 0;
		lightIndex < lightCount;
		++lightIndex)
	{
		light *light = lights + lightIndex;
		light->isEnabled = true;
		light->type = LightType_Point;

		light->IAmbient = glm::vec3(0.2f, 0.4f, 0.2f);
		light->IDiffuse = glm::vec3(0.8f, 0.4f, 0.4f);
		light->ISpecular = glm::vec3(0.4f, 0.2f, 0.6f);

		light->c1 = 0.2f;
		light->c2 = 0.08f;
		light->c3 = 0.015f;

		// spotlight only
		light->innerConeAngleCos = 0.9f;
		light->outerConeAngleCos = 0.7f;
		light->fallOff = 0.13f;
	}
}

// preset 2 : Every light is the same type with different colors
static void 
ConfigureLightPreset2(light *lights, u32 lightCount)
{
	for (u32 lightIndex = 0;
		lightIndex < lightCount;
		++lightIndex)
	{
		light *light = lights + lightIndex;
		light->isEnabled = true;
		light->type = LightType_SpotLight;

		light->IAmbient = glm::vec3((r32)(rand()%255)/255.0f, (r32)(rand()%255)/255.0f, (r32)(rand()%255)/255.0f);
		light->IDiffuse = glm::vec3((r32)(rand()%255)/255.0f, (r32)(rand()%255)/255.0f, (r32)(rand()%255)/255.0f);
		light->ISpecular = 0.6f*glm::vec3((r32)(rand()%255)/255.0f, (r32)(rand()%255)/255.0f, (r32)(rand()%255)/255.0f);

		light->c1 = 0.08f;
		light->c2 = 0.02f;
		light->c3 = 0.011f;

		// spotlight only
		light->innerConeAngleCos = 0.9f;
		light->outerConeAngleCos = 0.7f;
		light->fallOff = 0.26f;
	}
}

static void 
ConfigureLightPreset3(light *lights, u32 lightCount)
{
	for (u32 lightIndex = 0;
		lightIndex < lightCount;
		++lightIndex)
	{
		u32 nom = lightCount - lightIndex;

		light *light = lights + lightIndex;
		light->isEnabled = true;

		switch (lightIndex % 3)
		{
			case 0:
			{
				light->type = LightType_Point;
			}break;
			case 1:
			{
				light->type = LightType_Directional;
			}break;
			case 2:
			{
				light->type = LightType_SpotLight;
			}break;
		}

		light->IAmbient = glm::vec3(nom/(float)lightCount, nom/(float)lightCount, nom/(float)lightCount);
		light->IDiffuse = glm::vec3(nom/(float)lightCount, nom/(float)lightCount, nom/(float)lightCount);
		light->ISpecular = glm::vec3(nom/(float)lightCount, nom/(float)lightCount, nom/(float)lightCount);

		light->c1 = 0.08f;
		light->c2 = 0.02f;
		light->c3 = 0.011f;

		// spotlight only
		light->innerConeAngleCos = 0.9f;
		light->outerConeAngleCos = 0.7f;
		light->fallOff = 0.13f;
	}
}

int main()
{
	srand ((u32)time(NULL));
	if (!glfwInit())
	{
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 1);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	int windowWidth = 1920;
	int windowHeight = 1080;

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight,
		"CS300 Assignment 2", 0, 0);

	if (!window)
	{
		printf("Failed to open GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, true);

	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to initialize glew!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// NOTE(joon) : Load imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	ImGui::StyleColorsDark();

	ImVec4 imguiClearColor = ImVec4(0.45f, 0.45f, 0.55f, 1.0f);

	printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// TODO(joon) : DO NOT HARD CODE THE MODEL COUNT!
	std::vector<std::string>textureNames = { "4Sphere.obj",
											"bunny.obj",
											"bunny_high_poly.obj",
											"cube.obj",
											"cube2.obj",
											"cup.obj",
											"lucy_princeton.obj",
											"quad.obj",
											"rhino.obj",
											"sphere.obj",
											"sphere_modified.obj",
											"starwars1.obj",
											"triangle.obj" };

	std::vector<model> models(textureNames.size());
	for (u32 textureNameIndex = 0;
		textureNameIndex < textureNames.size();
		++textureNameIndex)
	{
		model* model = models.data() + textureNameIndex;

		std::string directoryPath = "textures/";
		std::string filePath = directoryPath + textureNames[textureNameIndex].c_str();

		ReadOBJFileLineByLine(&model->mesh, filePath.c_str());
	}
	
	std::vector<const char *>vertexShaderPaths = 
	{
		"source/shaders/phong_shading_shader.vert",
		"source/shaders/phong_lighting_shader.vert",
		"source/shaders/blinn_shader.vert"
	};
	
	std::vector<const char *>fragmentShaderPaths = 
	{
		"source/shaders/phong_shading_shader.frag",
		"source/shaders/phong_lighting_shader.frag",
		"source/shaders/blinn_shader.frag"
	};

	GLuint plainProgram = LoadShaders("source/shaders/plain_shader.vert", "source/shaders/plain_shader.frag");

	GLuint lightingPrograms[3] = {};
	lightingPrograms[0] = LoadShaders(vertexShaderPaths[0], fragmentShaderPaths[0]);
	lightingPrograms[1] = LoadShaders(vertexShaderPaths[1], fragmentShaderPaths[1]);
	lightingPrograms[2] = LoadShaders(vertexShaderPaths[2], fragmentShaderPaths[2]);

	for (u32 modelIndex = 0;
		modelIndex < models.size();
		++modelIndex)
	{
		model* model = models.data() + modelIndex;

		glGenVertexArrays(1, &model->vertexArrayID);
		glBindVertexArray(model->vertexArrayID);

		glGenBuffers(1, &model->vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, model->vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER,
			model->mesh.vertexBuffer.size() * sizeof(vertex), model->mesh.vertexBuffer.data(),
			GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, p));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, normal));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texCoord));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glGenBuffers(1, &model->indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->mesh.indexBuffer.size() * sizeof(unsigned int), model->mesh.indexBuffer.data(),
			GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	// NOTE(joon) : Create uniform buffer to pass information into shaders
	GLuint perFrameUboID;
	glGenBuffers(1, &perFrameUboID);
	glBindBuffer(GL_UNIFORM_BUFFER, perFrameUboID);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(per_frame_ubo), 0, GL_STATIC_DRAW);

	GLuint perObjectUboID;
	glGenBuffers(1, &perObjectUboID);
	glBindBuffer(GL_UNIFORM_BUFFER, perObjectUboID);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(per_object_ubo), 0, GL_STATIC_DRAW);

	GLuint plainPerObjectUboID;
	glGenBuffers(1, &plainPerObjectUboID);
	glBindBuffer(GL_UNIFORM_BUFFER, plainPerObjectUboID);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(per_object_ubo), 0, GL_STATIC_DRAW);

	// NOTE(joon) : GL_LINES routine for small amount of lines
	GLuint lineVertexArrayID;
	GLuint lineVertexBufferID;
	glGenVertexArrays(1, &lineVertexArrayID);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindVertexArray(lineVertexArrayID);
	glm::vec3 line[2] = { {0, 0, 0}, {1, 0, 0} };
	glGenBuffers(1, &lineVertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, lineVertexBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		2 * sizeof(glm::vec3), line,
		GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	// NOTE(joon) : Generate custom sphere model
	model sphereModel;
	GenerateSphereModel(&sphereModel, 0.5f, 72, 24);
	glGenVertexArrays(1, &sphereModel.vertexArrayID);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(sphereModel.vertexArrayID);

	glGenBuffers(1, &sphereModel.vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, sphereModel.vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER,
		sphereModel.mesh.vertexBuffer.size() * sizeof(vertex), sphereModel.mesh.vertexBuffer.data(),
		GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)sizeof(glm::vec3));
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &sphereModel.indexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereModel.indexBufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereModel.mesh.indexBuffer.size() * sizeof(unsigned int), sphereModel.mesh.indexBuffer.data(),
		GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	// NOTE(joon) : Create face normal buffer for GL_LINES
	for (u32 modelIndex = 0;
		modelIndex < models.size();
		++modelIndex)
	{
		model* model = models.data() + modelIndex;

		glGenVertexArrays(1, &model->faceNormalArrayID);
		glEnableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(model->faceNormalArrayID);

		glGenBuffers(1, &model->faceNormalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, model->faceNormalBufferID);
		glBufferData(GL_ARRAY_BUFFER, model->mesh.faceNormalBuffer.size()*sizeof(line), model->mesh.faceNormalBuffer.data(),
					GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	}

	// NOTE(joon) : Create vertex normal buffer for GL_LINES
	for (u32 modelIndex = 0;
		modelIndex < models.size();
		++modelIndex)
	{
		model* model = models.data() + modelIndex;

		glGenVertexArrays(1, &model->vertexNormalArrayID);
		glEnableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(model->vertexNormalArrayID);

		glGenBuffers(1, &model->vertexNormalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, model->vertexNormalBufferID);
		glBufferData(GL_ARRAY_BUFFER, model->mesh.vertexNormalLineBuffer.size()*sizeof(line), model->mesh.vertexNormalLineBuffer.data(),
					GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	}

	// Generate diffuse texture
	int textureWidth = 0;
	int textureHeight = 0;
	int textureChannels = 0;
	stbi_uc *diffuseTexture = stbi_load("textures/metal_roof_diff_512x512.png", &textureWidth, &textureHeight, &textureChannels, STBI_rgb);
	if (!diffuseTexture)
	{
		printf("Failed to load the texture \n");
	}
	GLuint diffuseTextureID = 0;
	glGenTextures(1, &diffuseTextureID);
	glBindTexture(GL_TEXTURE_2D, diffuseTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  // GL_RGBA_32UI?
				textureWidth, textureHeight, 
				0,
				GL_RGB,
				GL_UNSIGNED_BYTE,
				(void *)diffuseTexture);
	GLenum error = glGetError();
	if (error)
	{
		printf("Failed to generate the texture\n");
	}
	stbi_image_free(diffuseTexture);

	// Generate specular texture
	stbi_uc *specularTexture = stbi_load("textures/metal_roof_spec_512x512.png", &textureWidth, &textureHeight, &textureChannels, STBI_rgb);
	if (!specularTexture)
	{
		printf("Failed to load the texture\n");
	}
	GLuint specularTextureID = 0;
	glGenTextures(1, &specularTextureID);
	glBindTexture(GL_TEXTURE_2D, specularTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  // GL_RGBA_32UI?
				textureWidth, textureHeight, 
				0,
				GL_RGB,
				GL_UNSIGNED_BYTE,
				(void *)specularTexture);
	error = glGetError();
	if (error)
	{
		printf("Failed to generate the texture\n");
	}
	stbi_image_free(specularTexture);

	GenerateTexCoordForAllModels(&models, textureWidth, textureHeight, TextureMappingMethod_Planar, true);

	camera camera;
	camera.initP = { 13, 6, 0 };
	camera.angle = 0.0f;
	camera.lookAtP = { 0, 0, 0 };
	camera.up = { 0, 1, 0 };
	camera.fovInDegree = 45.0f;
	camera.near = 0.1f;
	camera.far = 100.0f;

	// Initialize the lights
	r32 lightRadius = 4.0f;
	light lights[16] = {};
	for (u32 lightIndex = 0;
		lightIndex < ArrayCount(lights);
		++lightIndex)
	{
		light *light = lights + lightIndex;
		light->angle = (Two_Pi32 / ArrayCount(lights)) * lightIndex;

		light->IAmbient = glm::vec3(0.8f, 0.8f, 0.8f);
		light->IDiffuse = glm::vec3(0.8f, 0.8f, 0.8f);
		light->ISpecular = glm::vec3(1.0f, 1.0f, 1.0f);

		light->c1 = 0.2f;
		light->c2 = 0.04f;
		light->c3 = 0.015f;

		// spotlight only
		light->innerConeAngleCos = 0.9f;
		light->outerConeAngleCos = 0.7f;
		light->fallOff = 0.13f;
	}
	ConfigureLightPreset1(lights, ArrayCount(lights));

	float angle = 0.0f;
	bool isGameRunning = true;
	bool shouldDrawVertexNormal = false;
	bool shouldDrawFaceNormal = false;
	bool shouldLightRotate = true;
	i32 selectedProgramIndex = 0;

	// imgui texture options
	bool shouldRemapTexture = true;
	int selectedMappingLocationIndex = TextureMappingLocation_CPU;
	int selectedTextureEntity = 0;
	bool didTextureEntityChanged = false;

	int selectedPresetIndex = 0;
	bool isPresetSelected = false;

	int selectedModelIndex = 0;

	per_frame_ubo perFrameUbo = {};
	perFrameUbo.IFog = glm::vec3(0.2f, 0.5f, 0.7f);
	perFrameUbo.zNear = 0.1f;
	perFrameUbo.zFar = 20.0f;
	perFrameUbo.shouldGenerateTexCoordInGPU = false;
	perFrameUbo.textureMappingMethod = TextureMappingMethod_Planar;

	per_object_ubo perObjectUbo = {};
	perObjectUbo.kAmbient = 0.2f;
	perObjectUbo.kDiffuse = 0.6f;
	perObjectUbo.kSpecular = 0.9f;
	perObjectUbo.ns = 10;

	for (u32 programIndex = 0;
		programIndex < ArrayCount(lightingPrograms);
		++programIndex)
	{
		glUseProgram(lightingPrograms[programIndex]);
		glUniform1i(glGetUniformLocation(lightingPrograms[programIndex], "diffuseTexture"), 0);
		glUniform1i(glGetUniformLocation(lightingPrograms[programIndex], "specularTexture"), 1);
	}

	while (!glfwWindowShouldClose(window) && isGameRunning)
	{
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			isGameRunning = false;
		}

		for (u32 lightIndex = 0;
			lightIndex < ArrayCount(lights);
			++lightIndex)
		{
			light* light = lights + lightIndex;

			if (shouldLightRotate)
			{
				light->angle += 0.015f;
			}
			light->p = lightRadius * glm::vec3(cos(light->angle), 0, sin(light->angle));
		}

		glm::vec3 cameraP = glm::vec3(glm::rotate(camera.angle, glm::vec3(0, 1, 0)) * glm::vec4(camera.initP, 1.0f));
		perFrameUbo.view = glm::lookAt(cameraP, camera.lookAtP, camera.up);
		perFrameUbo.projection = glm::perspective(glm::radians(camera.fovInDegree), windowWidth / (float)windowHeight, camera.near, camera.far);
		perFrameUbo.cameraP = cameraP;
		perFrameUbo.cameraDir = glm::normalize(cameraP - glm::vec3(0, 0, 0));

		// per object will be filled by imgui

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//symotion-prefix) NOTE(joon) : config imgui
		ImGui::Begin("Options");
		ImGui::Text("Preset");
		const char* presets[3] = {"1", "2", "3"};
        isPresetSelected = ImGui::Combo("Preset", (int *)&selectedPresetIndex, presets, ArrayCount(presets), 0);
		ImGui::Separator();
		ImGui::Text("Shaders");
		const char* shaderTypes[3] = {"Phong Shading", "Phong Lighting", "Blinn"};
        ImGui::Combo("Shader Types", (int *)&selectedProgramIndex, shaderTypes, ArrayCount(shaderTypes), 0);
		bool shouldReloadShader = false;
		shouldReloadShader = ImGui::Button("Reload", ImVec2(100, 0));
		ImGui::Separator();
		ImGui::Text("Texture Mapping");
		const char* textureMappingTypes[] = {"Planar", "Cylindrical", "Spherical"};
		shouldRemapTexture = 
			ImGui::Combo("Texture Mapping Types", (int *)&perFrameUbo.textureMappingMethod, textureMappingTypes, ArrayCount(textureMappingTypes), 0);
		const char* textureGenerateLocations[2] = {"CPU", "GPU"};
		ImGui::Combo("TexCoord Generate Location", (int *)&selectedMappingLocationIndex, textureGenerateLocations, ArrayCount(textureGenerateLocations), 0);
		const char* textureEntities[] = {"Position", "Normal"};
		didTextureEntityChanged = ImGui::Combo("Texture Entity", (int *)&perFrameUbo.shouldUseNormal, textureEntities, ArrayCount(textureEntities), 0);
		ImGui::Separator();
		ImGui::Text("Global Constants");
		ImGui::SliderFloat3("Global Ambient", (float *)&perFrameUbo.globalAmbient, 0.0f, 1.0f, "%.5f", 0);
		ImGui::SliderFloat3("IFog", (float *)&perFrameUbo.IFog, 0.0f, 1.0f, "%.5f", 0);
		ImGui::SliderFloat("Fog Near", (float *)&perFrameUbo.zNear, 0.0f, 100.0f, "%.5f", 0);
		ImGui::SliderFloat("Fog Far", (float *)&perFrameUbo.zFar, 0, 100.0f, "%.5f", 0);
		ImGui::Separator();
		ImGui::Text("Model");
		const char* modelNames[] = {"4Sphere", "Bunny", "BunnyHighPoly", "Cube", "Cube2", "Cup", "Lucy", "Quad", "Rhino", "Sphere", "SphereModified", "StarWars", "Triangle"};
		ImGui::Combo("Models", (int *)&selectedModelIndex, modelNames, ArrayCount(modelNames), 0);
		ImGui::SliderFloat3("IEmissive", (float *)&perObjectUbo.IEmissive, 0.0f, 1.0f, "%.5f", 0);
		ImGui::SliderFloat("kAmbient", (float *)&perObjectUbo.kAmbient, 0.0f, 1.0f, "%.5f", 0);
		ImGui::SliderFloat("kDiffuse", (float *)&perObjectUbo.kDiffuse, 0.0f, 1.0f, "%.5f", 0);
		ImGui::SliderFloat("kSpecular", (float *)&perObjectUbo.kSpecular, 0.0f, 1.0f, "%.5f", 0);
		//ImGui::SliderFloat("ns", (float *)&perObjectUbo.ns, 0.0f, 1.0f, "%.5f", 0);

		static bool shouldDrawVertexNormal = false;
		ImGui::Checkbox("Vertex Normal", &shouldDrawVertexNormal);
		static bool shouldDrawFaceNormal = false;
		ImGui::Checkbox("Face Normal", &shouldDrawFaceNormal);
		ImGui::Separator();

		ImGui::Text("Camera");
		ImGui::SliderFloat("Orbit", (float *)&camera.angle, 0.0f, Two_Pi32, "%.5f", 0);
		ImGui::Separator();

		ImGui::Text("Lights");
		ImGui::SliderFloat("Radius", (float *)&lightRadius, 2.0f, 8.0f, "%.5f", 0);
		ImGui::Checkbox("Rotate", &shouldLightRotate);
		ImGui::Separator();
		for (u32 lightIndex = 0;
			lightIndex < ArrayCount(lights);
			++lightIndex)
		{
			ImGui::PushID(lightIndex);
			light *light = lights + lightIndex;
			ImGui::Text("Light %u", lightIndex);
			ImGui::Checkbox("Enable", &light->isEnabled);

			const char* lightTypes[3] = {"Point", "Directional", "SpotLight"};
            ImGui::Combo("Light Types", (int *)&light->type, lightTypes, (int)ArrayCount(lightTypes), 0);

			if (light->isEnabled)
			{
				ImGui::SliderFloat3("Ambient", (float *)&light->IAmbient, 0.0f, 1.0f, "%.5f", 0);
				ImGui::SliderFloat3("Diffuse", (float *)&light->IDiffuse, 0.0f, 1.0f, "%.5f", 0);
				ImGui::SliderFloat3("Specular", (float *)&light->ISpecular, 0.0f, 1.0f, "%.5f", 0);
				ImGui::SliderFloat("C1", (float *)&light->c1, 0.0f, 1.0f, "%.5f", 0);
				ImGui::SliderFloat("C2", (float *)&light->c2, 0.0f, 1.0f, "%.5f", 0);
				ImGui::SliderFloat("C3", (float *)&light->c3, 0.0f, 1.0f, "%.5f", 0);

				if (light->type == LightType_SpotLight)
				{
					ImGui::SliderFloat("Cos(Inner Cone Angle)", (float *)&light->innerConeAngleCos, 0.0f, 1.0f, "%.5f", 0);
					ImGui::SliderFloat("Cos(Outer Cone Angle)", (float *)&light->outerConeAngleCos, 0.0f, 1.0f, "%.5f", 0);
					ImGui::SliderFloat("FallOff", (float *)&light->fallOff, 0.0f, 10.0f, "%.5f", 0);

					light->innerConeAngleCos = Clamp(light->outerConeAngleCos, light->innerConeAngleCos, 1.0f);
					light->outerConeAngleCos = Clamp(0.0f, light->outerConeAngleCos, light->innerConeAngleCos);
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();

		if (isPresetSelected)
		{
			switch (selectedPresetIndex)
			{
				case 0:
				{
					ConfigureLightPreset1(lights, ArrayCount(lights));
				}break;
				case 1:
				{
					ConfigureLightPreset2(lights, ArrayCount(lights));
				}break;
				case 2:
				{
					ConfigureLightPreset3(lights, ArrayCount(lights));
				}break;
			}
		}

		if (selectedMappingLocationIndex == TextureMappingLocation_GPU)
		{
			perFrameUbo.shouldGenerateTexCoordInGPU = true;
		}
		else
		{
			perFrameUbo.shouldGenerateTexCoordInGPU = false;
			// As this modify shouldRemapTexture value, this should come before the if(shouldRemapTexture)
			shouldRemapTexture = true;
		}

		if (shouldReloadShader)
		{
			GLuint newProgram = LoadShaders(vertexShaderPaths[selectedProgramIndex], fragmentShaderPaths[selectedProgramIndex]);

			if (newProgram)
			{
				glDeleteProgram(lightingPrograms[selectedProgramIndex]);
				lightingPrograms[selectedProgramIndex] = newProgram;
			}
		}

		if (shouldRemapTexture || didTextureEntityChanged)
		{
			b32 shouldUseP = true;
			if (perFrameUbo.shouldUseNormal == 1)
			{
				shouldUseP = false;
			}

			GenerateTexCoordForAllModels(&models, textureWidth, textureHeight, perFrameUbo.textureMappingMethod, shouldUseP);
			shouldRemapTexture = false;
		}
		
		glClearColor(perFrameUbo.IFog.x, perFrameUbo.IFog.y, perFrameUbo.IFog.z, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		int displayWidth;
		int displayHeight;
		glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
		glViewport(0, 0, displayWidth, displayHeight);

		glUseProgram(lightingPrograms[selectedProgramIndex]);

		// update per frame uniform buffer
		u32 lightSize = ArrayCount(lights)*sizeof(light);
		u32 offsetOfLightInsideUniformBuffer = offsetof(per_frame_ubo, lights);
		glBindBuffer(GL_UNIFORM_BUFFER, perFrameUboID);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, perFrameUboID);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_frame_ubo) - lightSize, &perFrameUbo);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetOfLightInsideUniformBuffer, lightSize, lights);

		model* model = models.data() + selectedModelIndex;

		// floor
		RenderModel(&models[7], 
					glm::vec3(7, 7, 7), -Pi32/2.0f, 0, 0, glm::vec3(0, -2, 0),
					windowWidth, windowHeight, perObjectUboID, &perObjectUbo, sizeof(perObjectUbo), 0, 0);

		RenderModel(model, 
					glm::vec3(2, 2, 2), 0, 0, 0, glm::vec3(0, 0, 0),
					windowWidth, windowHeight, perObjectUboID, &perObjectUbo, sizeof(perObjectUbo), diffuseTextureID, specularTextureID);
#if 1
		if (shouldDrawFaceNormal)
		{
			RenderFaceNormal(model, glm::vec3(1, 1, 1), 0, glm::vec3(0, 0, 0),
							windowWidth, windowHeight, perObjectUboID);
		}
		if (shouldDrawVertexNormal)
		{
			RenderVertexNormal(model, glm::vec3(1, 1, 1), 0, glm::vec3(0, 0, 0),
							windowWidth, windowHeight, perObjectUboID);
		}

		// NOTE(joon) : draw orbital line!
		u32 lineDensity = 1000;
		float angleForEachLine = Two_Pi32 / (r32)lineDensity;
		for (u32 lineIndex = 0;
			lineIndex < 1000;
			++lineIndex)
		{
			glm::vec3 start = {};
			start.x = lightRadius * cos(lineIndex * angleForEachLine);
			start.y = 0;
			start.z = lightRadius * sin(lineIndex * angleForEachLine);

			glm::vec3 end = {};
			end.x = lightRadius * cos((lineIndex+1) * angleForEachLine);
			end.y = 0.0f;
			end.z = lightRadius * sin((lineIndex+1) * angleForEachLine);

			glm::vec3 line[2] = {start, end};
			RenderLine(line, lineVertexArrayID, lineVertexBufferID,
				glm::vec3(1, 1, 1), 0, glm::vec3(0, 0, 0),
				windowWidth, windowHeight, perObjectUboID);

		}
#endif

		glUseProgram(plainProgram);
		for (u32 lightIndex = 0;
			lightIndex < ArrayCount(lights);
			++lightIndex)
		{
			light *light = lights + lightIndex;

			if (light->isEnabled)
			{
				plain_per_object_ubo ubo = {};
				ubo.color = light->IDiffuse;
				RenderModel(&sphereModel,
					glm::vec3(0.5f, 0.5f, 0.5f), 0, 0, 0, light->p,
					windowWidth, windowHeight, perObjectUboID, &ubo, sizeof(ubo), 0, 0);
			}
		}

		// NOTE(joon) : render imgui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		angle += 0.00f;

		// NOTE(joon) : cleanup
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
