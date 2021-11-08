#version 450

layout(binding = 0) uniform per_frame_ubo
{
    mat4 view;
	mat4 projection;

	vec3 cameraP;
	vec3 cameraDir;
	vec3 IFog;
	vec3 globalAmbient;

	float zNear;
	float zFar;

	//light lights[16];
}perFrameUbo;

layout(binding = 1) uniform per_object_ubo
{
    mat4 model;
	vec3 color;
}perObjectUbo;

layout(location = 0) in vec3 p;  
layout(location = 1) in vec3 normal;  
layout(location = 2) in vec2 texCoord;  

void main()
{
    gl_Position = perFrameUbo.projection*perFrameUbo.view*perObjectUbo.model*vec4(p, 1.0);
}