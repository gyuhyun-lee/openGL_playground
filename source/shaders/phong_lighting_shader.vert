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

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

layout(location = 0) in vec3 p;  
layout(location = 1) in vec3 normal;  
layout(location = 2) in vec2 inTexCoord;  

layout(location = 0) out vec4 color;

void main()
{
    gl_Position = perFrameUbo.projection*perFrameUbo.view*perObjectUbo.model*vec4(p, 1.0);

	vec2 texCoord = inTexCoord;
	// Generate texture coord if the mapping location was GPU
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

		texCoord = generatedTexCoord;
	}

	vec3 vertexP = vec3(perObjectUbo.model*vec4(p, 1.0));

    vec3 N = normalize(vec3(perObjectUbo.model*vec4(normal, 0.0f)));
    vec3 V = normalize(perFrameUbo.cameraP - vertexP);
    float distanceToCamera = length(perFrameUbo.cameraP - vertexP);

    vec3 IEmissive =  perObjectUbo.IEmissive;
    vec3 IGlobalAmbient = perFrameUbo.globalAmbient;
    vec3 kAmbient = perObjectUbo.kAmbient*texture(diffuseTexture, texCoord).xyz;
    vec3 kDiffuse = perObjectUbo.kDiffuse*texture(diffuseTexture, texCoord).xyz;
    vec3 kSpecular = perObjectUbo.kSpecular*texture(specularTexture, texCoord).xyz;

    vec3 ILocal = IEmissive + kAmbient*IGlobalAmbient;

    for(uint lightIndex = 0; lightIndex < 16; ++lightIndex)
    {
        if(perFrameUbo.lights[lightIndex].isEnabled)
        {
			vec3 IAmbient = perFrameUbo.lights[lightIndex].IAmbient * kAmbient;

			float distance = length(perFrameUbo.lights[lightIndex].p - vertexP);
			float attenuationDenom = perFrameUbo.lights[lightIndex].c1 + 
									 perFrameUbo.lights[lightIndex].c2*distance + 
									 perFrameUbo.lights[lightIndex].c3*distance*distance;

			float attenuation = min(1.0f/attenuationDenom, 1.0f);

			uint type = perFrameUbo.lights[lightIndex].type;
			if(type == 0)
			{
				// point light
				vec3 L = normalize(perFrameUbo.lights[lightIndex].p - vertexP); // Assume that the light is looking at the center
				vec3 R = 2.0f*dot(N, L)*N - L;

				vec3 IDiffuse = perFrameUbo.lights[lightIndex].IDiffuse * kDiffuse * max(dot(N, L), 0.0f);
				vec3 ISpecular = perFrameUbo.lights[lightIndex].ISpecular * perObjectUbo.kSpecular * pow(max(dot(R, V), 0), perObjectUbo.ns);

				ILocal += attenuation*(IAmbient + IDiffuse + ISpecular);
			}
			else if(type == 1)
			{
				// directional light -> doesnt affected by distance, and light direction is always the same
				vec3 L = normalize(perFrameUbo.lights[lightIndex].p);
				vec3 R = 2.0f*dot(N, L)*N - L;
				
				vec3 IDiffuse = perFrameUbo.lights[lightIndex].IDiffuse * kDiffuse * max(dot(N, L), 0.0f);
				vec3 ISpecular = perFrameUbo.lights[lightIndex].ISpecular * perObjectUbo.kSpecular * pow(max(dot(R, V), 0), perObjectUbo.ns);
				ILocal += IAmbient + IDiffuse + ISpecular;
			}
			else if(type == 2)
			{
				// spotlight

				vec3 D = normalize(vertexP - perFrameUbo.lights[lightIndex].p);
				// unlike L which is dependent to the pixelP, this is the direction of the spotlight, which we assume that
				// it's always facing at (0, 0, 0)
				vec3 L = normalize(perFrameUbo.lights[lightIndex].p - vertexP);
				vec3 R = 2.0f*dot(N, L)*N - L;

				float cosAlpha = dot(D, -normalize(perFrameUbo.lights[lightIndex].p));
				float nom = cosAlpha - perFrameUbo.lights[lightIndex].outerConeAngleCos;
				float denom = perFrameUbo.lights[lightIndex].innerConeAngleCos - perFrameUbo.lights[lightIndex].outerConeAngleCos;

				float spotlightEffect = max(pow(nom/denom, perFrameUbo.lights[lightIndex].fallOff), 0.0f);

				vec3 IDiffuse = perFrameUbo.lights[lightIndex].IDiffuse * kDiffuse * max(dot(N, L), 0.0f);
				vec3 ISpecular = perFrameUbo.lights[lightIndex].ISpecular * perObjectUbo.kSpecular * pow(max(dot(R, V), 0), perObjectUbo.ns);

				ILocal += attenuation*IAmbient + attenuation*spotlightEffect*(IDiffuse + ISpecular);
			}
        }
    }

    float S = min(max((perFrameUbo.zFar - distanceToCamera)/(perFrameUbo.zFar - perFrameUbo.zNear), 0), 1.0f);

    vec3 IFinal = S*ILocal + (1.0f-S)*perFrameUbo.IFog;

	color = vec4(IFinal, 1.0f);
}
