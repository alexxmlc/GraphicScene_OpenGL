#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//components
vec3 ambient;
uniform float ambientStrength;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

// texture toggle
uniform int showTexture;

// sun
uniform int isLightSource;

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

#define NR_POINT_LIGHTS 6
uniform PointLight pointLights[NR_POINT_LIGHTS];

// for shadows
uniform sampler2D shadowMap;
uniform mat4 lightSpaceTrMatrix;
in vec4 fragPosLightSpace; // pos of pixel seen by the sun

//fog 
uniform float fogDensity;

// point light
vec3 computePointLight(PointLight light, vec3 normalEye, vec4 fragPosEye, vec3 viewDir) {
    vec3 lightDir = normalize(vec3(view * vec4(light.position, 1.0f)) - fragPosEye.xyz);

    // Diffuse
    float diff = max(dot(normalEye, lightDir), 0.0f);
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normalEye);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    
    // Attenuation 
    float distance = length(vec3(view * vec4(light.position, 1.0f)) - fragPosEye.xyz);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Combine
    vec3 ambient = ambientStrength * light.color;
    vec3 diffuse = diff * light.color;
    vec3 specular = spec * light.color; 
    
    return (ambient + diffuse + specular) * attenuation;
}

float computeShadow()
{
    // Transform the coords in [0.0, 1.0]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // out of the map then no shadow
    if (projCoords.z > 1.0) return 0.0;

    // read depth
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // actual pixel depth
    float currentDepth = projCoords.z;

    // check if is shadow, with bias for shadow acne
    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    float diff = max(dot(normalEye, lightDirN), 0.0f);
    diffuse = diff * lightColor;

    //compute specular light
    specular = vec3(0.0f);
    if (diff > 0.0f) {
        vec3 reflectDir = reflect(-lightDirN, normalEye);
        float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
        specular = specularStrength * specCoeff * lightColor;
    }
}

void main() {
    
    computeDirLight(); 
    
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(-fPosEye.xyz);

    // calc shadow
    float shadow = computeShadow();

    vec3 sunLight = ambient + (1.0 - shadow) * (diffuse + specular);

    vec3 finalColor = sunLight;

    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        finalColor += computePointLight(pointLights[i], normalEye, fPosEye, viewDir);
    }

    vec3 texColor = vec3(0.0f);
    if (showTexture == 1) {
         texColor = texture(diffuseTexture, fTexCoords).rgb;
    } else {
         texColor = vec3(0.7f, 0.7f, 0.7f);
    }

    // color without fog
    vec4 colorWithLight;
    if (isLightSource == 1) {
        colorWithLight = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
    } else {
        colorWithLight = vec4(min(finalColor * texColor, 1.0f), 1.0f);
    }

    // calc fog
    float dist = length(fPosEye.xyz);

    // result between 1 (clear) si 0 (total fog)
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0f, 1.0f);

    // gray for fog
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f); 
    
    // mix color with fog
    fColor = mix(fogColor, colorWithLight, fogFactor);
}