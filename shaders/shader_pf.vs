#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPos;  // Posición del fragmento en espacio mundial
out vec3 Normal;   // Normal en espacio mundial

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;    
    FragPos = vec3(model * vec4(aPos, 1.0));  // Convertir a espacio mundial
    Normal = mat3(transpose(inverse(model))) * aNormal; // Transformar normales
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
