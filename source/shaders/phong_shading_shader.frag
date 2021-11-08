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

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

layout (location = 0) in vec3 fragWorldNormal;
layout (location = 1) in vec3 fragWorldP;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec4 fragColor;

void main()
{
    vec3 N = normalize(fragWorldNormal);
    vec3 V = normalize(perFrameUbo.cameraP - fragWorldP);
    float distanceToCamera = length(perFrameUbo.cameraP - fragWorldP);

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

			float distance = length(perFrameUbo.lights[lightIndex].p - fragWorldP);
			float attenuationDenom = perFrameUbo.lights[lightIndex].c1 + 
									 perFrameUbo.lights[lightIndex].c2*distance + 
									 perFrameUbo.lights[lightIndex].c3*distance*distance;

			float attenuation = min(1.0f/attenuationDenom, 1.0f);

			uint type = perFrameUbo.lights[lightIndex].type;
			if(type == 0)
			{
				// point light
				vec3 L = normalize(perFrameUbo.lights[lightIndex].p - fragWorldP); // Assume that the light is looking at the center
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

				vec3 D = normalize(fragWorldP - perFrameUbo.lights[lightIndex].p);
				// unlike L which is dependent to the fragP, this is the direction of the spotlight, which we assume that
				// it's always facing at (0, 0, 0)
				vec3 L = normalize(perFrameUbo.lights[lightIndex].p -fragWorldP);
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

	fragColor = vec4(IFinal, 1.0f);
} 