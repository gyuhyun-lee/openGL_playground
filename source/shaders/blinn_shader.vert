#version 450

struct light
{
    bool isEnabled;

	float angle;
	uint type;

	vec3 p;

	vec3 IAmbient;
	vec3 IDiffuse;
	vec3 ISpecular;

	float c1;
	float c2;
	float c3;

	float innerConeAngleCos;
	float outerConeAngleCos;
	float fallOff;
};

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

	bool shouldGenerateTexCoordInGPU;
	int textureMappingMethod;
	bool shouldUseNormal;

	light lights[16];
}perFrameUbo;

layout(binding = 1) uniform per_object_ubo
{
    mat4 model;

	vec3 IEmissive;

	float kAmbient;
	float kDiffuse;
	float kSpecular;

	float ns;
}perObjectUbo;

vec2
PlanarTextureMapping(vec3 p)
{
	vec2 result = vec2(0);

	float absX = abs(p.x);
	float absY = abs(p.y);
	float absZ = abs(p.z);

	float u = 0.0f;
	float v = 0.0f;

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

	result.x = 0.5f*(u + 1.0f);
	result.y = 0.5f*(v + 1.0f);

	return result;
}


vec2
CylindricalTextureMapping(vec3 p)
{
	vec2 result = vec2(0);
	float Two_Pi32 = 6.2831853071795864768f;

	float theta = atan(p.y/p.x);

	result.x = theta/Two_Pi32;
	result.y = (p.z + 1.0f)/2.0f; //(z - zMin)/(zMax - zMin)

	return result;
}

vec2
SphericalTextureMapping(vec3 p)
{
	vec2 result = vec2(0);

	float Pi32 = 3.1415926535897932384f;
	float Two_Pi32 = 6.2831853071795864768f;

	float r = sqrt(1+1+1);
	float theta = atan(p.y/p.x);
	float pi = acos(p.z/r);

	result.x = theta/Two_Pi32; // theta/360
	result.y = pi/Pi32; //pi/180

	return result;
}

layout(location = 0) in vec3 p;  
layout(location = 1) in vec3 normal;  
layout(location = 2) in vec2 texCoord;  

layout (location = 0) out vec3 fragNormal;
layout (location = 1) out vec3 fragWorldP;
layout (location = 2) out vec2 fragTexCoord;

void main()
{
    gl_Position = perFrameUbo.projection*perFrameUbo.view*perObjectUbo.model*vec4(p, 1.0);

    fragNormal = vec3((perObjectUbo.model*vec4(normal, 0.0f)));
    fragWorldP = vec3((perObjectUbo.model*vec4(p, 1.0f)));

	if(perFrameUbo.shouldGenerateTexCoordInGPU)
	{
		vec2 generatedTexCoord = vec2(0);

		vec3 pToUse = p;
		if(perFrameUbo.shouldUseNormal)
		{
			pToUse = normal;
		}

		if(perFrameUbo.textureMappingMethod == 0)
		{
			// planar
			generatedTexCoord = PlanarTextureMapping(pToUse);
		}
		else if(perFrameUbo.textureMappingMethod == 1)
		{
			// cylindrical
			generatedTexCoord = CylindricalTextureMapping(pToUse);
		}
		else if(perFrameUbo.textureMappingMethod == 2)
		{
			// spherical
			generatedTexCoord = SphericalTextureMapping(pToUse);
		}

		fragTexCoord = generatedTexCoord;
	}
	else
	{
		fragTexCoord = texCoord;
	}
}