#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_specular1;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform Light light;
uniform PointLight pointLight;
uniform bool usePointLight;
uniform vec3 viewPos;
uniform bool useLighting; 

// Linterna
uniform bool flashlightOn;
uniform vec3 flashlightPos;
uniform vec3 flashlightDir;
uniform float cutoff;       // Ángulo interno de la linterna (coseno)
uniform float outerCutoff;  // Ángulo externo de la linterna (coseno)
uniform float constant;     // Atenuación constante
uniform float linear;       // Atenuación lineal
uniform float quadratic;    // Atenuación cuadrática

// Función para calcular point light
vec3 calcPointLight(PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseTexture, vec3 specularTexture)
{
    vec3 lightDir = normalize(pointLight.position - fragPos);
    
    // Difusa
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Especular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    
    // Atenuación
    float distance = length(pointLight.position - fragPos);
    float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));
    
    // Combinar resultados
    vec3 ambient = pointLight.ambient * diffuseTexture;
    vec3 diffuse = pointLight.diffuse * diff * diffuseTexture;
    vec3 specular = pointLight.specular * spec * specularTexture;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

void main()
{    
    // 1. Obtener texturas
    vec3 diffuseTexture = texture(texture_diffuse1, TexCoords).rgb;
    vec3 specularTexture = texture(texture_specular1, TexCoords).rgb;

    // 2. Normalizar vectores
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    // 3. Luz ambiente
    vec3 ambient = light.ambient * diffuseTexture;

    // 4. Luz difusa (Lambert)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseTexture;

    // 5. Luz especular (Phong)
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = light.specular * spec * specularTexture;

    // 6. Iluminación de la linterna (spotlight) - MEJORADA
    vec3 flashlightEffect = vec3(0.0);
    if (flashlightOn) {
        vec3 fragToFlashlight = normalize(flashlightPos - FragPos);
        float theta = dot(fragToFlashlight, normalize(-flashlightDir));

        if (theta > outerCutoff) {
            // Intensidad suavizada entre el ángulo interno y externo
            float intensity = smoothstep(outerCutoff, cutoff, theta);

            // Luz difusa de la linterna (más intensa)
            float diffFlashlight = max(dot(norm, fragToFlashlight), 0.0);
            vec3 flashlightDiffuse = vec3(2.5, 2.5, 2.0) * intensity * diffFlashlight * diffuseTexture; // Más intenso

            // Luz especular de la linterna (más intensa)
            vec3 reflectFlashlight = reflect(-fragToFlashlight, norm);
            float specFlashlight = pow(max(dot(viewDir, reflectFlashlight), 0.0), 32);
            vec3 flashlightSpecular = vec3(2.0) * intensity * specFlashlight * specularTexture; // Más intenso

            // Atenuación por distancia (menos atenuación = mayor alcance)
            float distance = length(flashlightPos - FragPos);
            float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

            flashlightEffect = (flashlightDiffuse + flashlightSpecular) * attenuation;
        }
    }

    // 7. Calcular point light si está habilitado
    vec3 pointLightEffect = vec3(0.0);
    if (usePointLight) {
        pointLightEffect = calcPointLight(pointLight, norm, FragPos, viewDir, diffuseTexture, specularTexture);
    }

    // 8. Combinar todas las luces
    vec3 result = ambient + diffuse + specular + flashlightEffect + pointLightEffect;
    
    // 9. Si no hay iluminación, usar solo el mapa emisivo
    if (!useLighting) {
        result = texture(texture_emissive1, TexCoords).rgb;
    }

    FragColor = vec4(result, 1.0);
}
