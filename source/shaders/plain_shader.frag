#version 450

layout(binding = 1) uniform per_object_ubo
{
    mat4 model;
	vec3 color;
}perObjectUbo;

layout (location = 0) out vec4 fragColor;

void main()
{
	fragColor = vec4(perObjectUbo.color, 1.0f);
} 