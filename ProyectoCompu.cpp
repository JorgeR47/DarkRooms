#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <cstdlib> // Para rand()
#include <ctime>   // Para seed aleatorio
#include <vector>  // Para el sistema de objetos dinámicos
#include <iomanip> // Para std::setprecision

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods); // NUEVO
void processInput(GLFWwindow* window);

const float groundLevel = 0.2f; // Nivel del suelo ajustado para backrooms


unsigned int loadTexture(const char* path);

// === SISTEMA DE RAYCASTING ===
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct RaycastHit {
    bool hit;
    glm::vec3 position;
    glm::vec3 normal;
    float distance;
};

struct DynamicObject {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    glm::vec3 color;
    bool visible;
    float timeCreated;
};

// Vector para almacenar objetos dinámicos creados por raycasting
std::vector<DynamicObject> dynamicObjects;

// Función para crear un rayo desde la cámara
Ray createCameraRay(const Camera& camera, float screenWidth, float screenHeight) {
    Ray ray;
    ray.origin = camera.Position;
    ray.direction = camera.Front;
    return ray;
}

// Función para intersección rayo-plano (suelo)
RaycastHit rayPlaneIntersection(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal) {
    RaycastHit hit;
    hit.hit = false;

    float denom = glm::dot(planeNormal, ray.direction);
    if (abs(denom) > 0.0001f) { // El rayo no es paralelo al plano
        glm::vec3 p0l0 = planePoint - ray.origin;
        float t = glm::dot(p0l0, planeNormal) / denom;
        if (t >= 0) { // Intersección hacia adelante
            hit.hit = true;
            hit.position = ray.origin + t * ray.direction;
            hit.normal = planeNormal;
            hit.distance = t;
        }
    }
    return hit;
}

// Función para realizar raycasting en el mundo mejorado
RaycastHit performRaycast(const Camera& camera, float screenWidth, float screenHeight) {
    Ray ray = createCameraRay(camera, screenWidth, screenHeight);

    // Intersección con el plano del suelo (Y = groundLevel)
    glm::vec3 groundPlanePoint(0.0f, groundLevel, 0.0f);
    glm::vec3 groundPlaneNormal(0.0f, 1.0f, 0.0f);

    RaycastHit groundHit = rayPlaneIntersection(ray, groundPlanePoint, groundPlaneNormal);

    // Si hay intersección directa con el plano, usarla
    if (groundHit.hit && groundHit.distance > 0.1f) { // Evitar intersecciones muy cercanas
        return groundHit;
    }

    // Si no hay intersección directa o es muy cercana, proyectar en la dirección de la cámara
    float projectionDistance = 15.0f; // Distancia de proyección aumentada
    glm::vec3 forwardPoint = ray.origin + glm::normalize(ray.direction) * projectionDistance;
    
    // Proyectar este punto al plano del suelo
    groundHit.hit = true;
    groundHit.position = glm::vec3(forwardPoint.x, groundLevel, forwardPoint.z);
    groundHit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    groundHit.distance = glm::distance(ray.origin, groundHit.position);

    return groundHit;
}

// Función para crear un objeto dinámico en la posición del raycast
void createDynamicObject(const glm::vec3& position) {
    DynamicObject obj;
    obj.position = position;
    obj.position.y += 0.1f; // Elevarlo ligeramente sobre el suelo
    obj.scale = glm::vec3(0.1f, 0.1f, 0.1f);
    obj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    // Color aleatorio
    obj.color = glm::vec3(
        static_cast<float>(rand()) / RAND_MAX,
        static_cast<float>(rand()) / RAND_MAX,
        static_cast<float>(rand()) / RAND_MAX
    );

    obj.visible = true;
    obj.timeCreated = glfwGetTime();

    dynamicObjects.push_back(obj);

    std::cout << "Objeto creado en: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

// Función para renderizar objetos dinámicos (cubos simples)
void renderDynamicObjects(Shader& shader) {
    // Vertices para un cubo simple
    static float cubeVertices[] = {
        // Posiciones          // Normales
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    static unsigned int cubeVAO = 0;
    static unsigned int cubeVBO = 0;

    // Crear VAO y VBO para el cubo si no existe
    if (cubeVAO == 0) {
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    // Renderizar cada objeto dinámico
    for (const auto& obj : dynamicObjects) {
        if (obj.visible) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, obj.scale);

            shader.setMat4("model", model);
            shader.setVec3("objectColor", obj.color);

            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindVertexArray(0);
}

// settings
const unsigned int SCR_WIDTH = 1280; // Ancho de la ventana
const unsigned int SCR_HEIGHT = 1080; // Alto de la ventana

//gravedad y mecánicas de movimiento mejoradas
float gravity = -20.0f; // Gravedad más intensa para mejor sensación
float verticalVelocity = 0.0f; // Velocidad vertical de la cámara
bool isOnGround = true; // Estado para controlar si está en el suelo
const float jumpForce = 6.0f; // Fuerza de salto mejorada
const float terminalVelocity = -25.0f; // Velocidad terminal máxima de caída

// camera
Camera camera(glm::vec3(1.21534f, 0.2f, 0.541507f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

glm::vec3 initialCameraPosition = glm::vec3(1.21534f, 0.2f, 0.541507f); // Posición inicial dentro del backroom

// Configuración del movimiento para cada enemigo
struct Enemy {
    glm::vec3 initialPosition;      //Posición inicial de aparición
    glm::vec3 position;
    glm::vec2 direction;
    float speed;
    float changeDirectionTime;
    float rotationAngle; // Nuevo: ángulo de rotación
};

glm::vec3 getRandomPositionAround(const glm::vec3& center, float minRadius, float maxRadius) {
    // Generar un ángulo aleatorio
    float angle = static_cast<float>(rand()) / RAND_MAX * 2 * glm::pi<float>();

    // Generar un radio aleatorio entre minRadius y maxRadius
    float radius = minRadius + static_cast<float>(rand()) / RAND_MAX * (maxRadius - minRadius);

    // Calcular posición X y Z
    float x = center.x + radius * cos(angle);
    float z = center.z + radius * sin(angle);

    return glm::vec3(x, 0.2f, z); // Mantener la Y en el nivel del backroom
}

// Inicialización de los enemigos fantasmas con sus posiciones iniciales ajustadas para backrooms
Enemy fantasma1 = { glm::vec3(-20.0f, 0.2f, -20.0f),  glm::vec3(-20.0f, 0.2f, -20.0f), glm::vec2(0.0f, 1.0f), 0.4f, 0.0f };
Enemy fantasma2 = { glm::vec3(25.0f, 0.2f, -25.0f), glm::vec3(25.0f, 0.2f, -25.0f), glm::vec2(0.0f, 1.0f), 0.4f, 0.0f };
Enemy fantasma3 = { getRandomPositionAround(initialCameraPosition, 18.0f, 25.0f), glm::vec3(20.0f, 0.2f, 20.0f), glm::vec2(0.0f, 1.0f), 0.5f, 0.0f };
Enemy fantasma4 = { getRandomPositionAround(initialCameraPosition, 18.0f, 25.0f), glm::vec3(-20.0f, 0.2f, 20.0f), glm::vec2(0.0f, 1.0f), 0.5f, 0.0f };

void respawnEnemies(std::vector<Enemy*>& enemies, const glm::vec3& playerPosition) {
    for (Enemy* enemy : enemies) {
        if (!(enemy == &fantasma1 || enemy == &fantasma2)) {
            // Generar nueva posición aleatoria alrededor del jugador (más lejos)
            enemy->position = getRandomPositionAround(playerPosition, 15.0f, 20.0f);
            enemy->position.y = 0.2f; // Asegurar que estén en el suelo del backroom
        }

    }
}

void resetPlayer(Camera& camera, std::vector<Enemy*>& enemies) {
    camera.Position = initialCameraPosition;
    camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.Yaw = -90.0f;
    camera.Pitch = 0.0f;
    camera.updateCameraVectors();

    // Reposicionar enemigos lejos del jugador
    respawnEnemies(enemies, camera.Position);
}

std::vector<Enemy*> enemies = { &fantasma1, &fantasma2, &fantasma3, &fantasma4 };

// Semilla de números aleatorios
void initRandom() {
    srand(static_cast<unsigned>(time(0)));
}

// Método para actualizar la posición de un enemigo
void updateEnemy(Enemy& enemy, float deltaTime) {
    enemy.changeDirectionTime += deltaTime;

    // Cambio de dirección cada 1.5 segundos
    if (enemy.changeDirectionTime >= 1.5f) {
        int randomDirection = rand() % 4; // 0 = adelante, 1 = atrás, 2 = izquierda, 3 = derecha
        switch (randomDirection) {
        case 0: enemy.direction = glm::vec2(0.0f, 1.0f); break; // Adelante
        case 1: enemy.direction = glm::vec2(0.0f, -1.0f); break; // Atrás
        case 2: enemy.direction = glm::vec2(-1.0f, 0.0f); break; // Izquierda
        case 3: enemy.direction = glm::vec2(1.0f, 0.0f); break; // Derecha
        }
        enemy.changeDirectionTime = 0.0f;
    }

    // Movimiento al enemigo en la dirección actual
    enemy.position.x += enemy.direction.x * enemy.speed * deltaTime;
    enemy.position.z += enemy.direction.y * enemy.speed * deltaTime;
    enemy.position.y = 0.2f; // Mantener en el suelo del backroom

    // Limitación de la posición dentro del área de juego expandida para backrooms
    if (enemy.position.x > 8.0f) enemy.position.x = 8.0f;
    if (enemy.position.x < -8.0f) enemy.position.x = -8.0f;
    if (enemy.position.z > 8.0f) enemy.position.z = 8.0f;
    if (enemy.position.z < -8.0f) enemy.position.z = -8.0f;
}

// Método para actualizar la posición de los fantasmas especiales.
void updateSpecialEnemy(Enemy& enemy, float deltaTime, const glm::vec3& playerPosition) {
    glm::vec3 directionToPlayer = playerPosition - enemy.position;

    // Normalización de la dirección para que sea un vector unitario
    glm::vec2 normalizedDirection = glm::normalize(glm::vec2(directionToPlayer.x, directionToPlayer.z));

    if (&enemy == &fantasma3) {
        // El fantasma3 sigue directamente al jugador
        enemy.direction = normalizedDirection;
    }
    else if (&enemy == &fantasma4) {
        // Aumenta la velocidad del fantasma4 para que sea más rápido
        enemy.speed = 0.7f;

        // El fantasma4 persigue al jugador, pero con una ligera desviación para diferenciarse del fantasma3
        glm::vec2 perpendicular(-normalizedDirection.y, normalizedDirection.x);  // Vector perpendicular
        enemy.direction = normalizedDirection + (perpendicular * 0.3f); // Desviación leve
        enemy.direction = glm::normalize(enemy.direction); // Normalizar para mantener la magnitud
    }

    // Mover al enemigo en la dirección calculada
    enemy.position.x += enemy.direction.x * enemy.speed * deltaTime;
    enemy.position.z += enemy.direction.y * enemy.speed * deltaTime;
    enemy.position.y = 0.2f; // Mantener en el suelo del backroom

    // Permite que los enemigos se acerquen más al jugador y tengan un campo de movimiento más amplio en backrooms
    if (enemy.position.x > 20.0f) enemy.position.x = 20.0f;  // Aumentar rango de movimiento en X
    if (enemy.position.x < -20.0f) enemy.position.x = -20.0f; // Aumentar rango de movimiento en X
    if (enemy.position.z > 20.0f) enemy.position.z = 20.0f;  // Aumentar rango de movimiento en Z
    if (enemy.position.z < -20.0f) enemy.position.z = -20.0f;  // Permitir que se acerque más al jugador (en Z)

    // Calculo del ángulo de rotación
    enemy.rotationAngle = glm::degrees(atan2(normalizedDirection.x, normalizedDirection.y));
}

//Iluminación linterna
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// Point light position - EXACTAMENTE en el centro del modelo de luz
glm::vec3 pointLightPos(7.08137f, 1.0f, 1.86732f);

// Sistema de luces intermitentes automático
float lightCycleTime = 0.0f;
bool lightsActive = true;
const float LIGHTS_ON_TIME = 10.0f;  // 10 segundos con luz
const float LIGHTS_OFF_TIME = 5.0f;  // 5 segundos sin luz

// Portal position (nether2) - Nueva coordenada al suelo
glm::vec3 portalPos(-12.9196f, -0.1f, 7.04449f);
bool gameWon = false;

// Posiciones de las TVs con light maps - Pegadas al suelo
glm::vec3 tvCartoonPos(4.20507f, 0.0f, 3.24999f);
glm::vec3 tvLightPos1(-6.25234f, 0.0f, 4.66678f);
glm::vec3 tvLightPos2(-10.1882f, 0.0f, 7.00581f);
glm::vec3 tvLightPos3(-13.8627f, 0.0f, 8.14567f);

// Variables para rotación de TVs
float tvRotationAngle = 0.0f;

//Tiempo de inclinación
float cameraTiltTime = 0.0f;

// luna movimiento
float angle = 0.0f;

//Velocidad camara
float cameraSpeed = 1.0f; // Velocidad de movimiento de la cámara

//Apagar/prender linterna
bool flashlightOn = true;  // La linterna comienza encendida

// === NUEVOS MODELOS Y POSICIONES ===

// Posiciones de las arañas (saltan) - Ajustadas al suelo
glm::vec3 spiderPositions[] = {
    glm::vec3(6.17464f, -0.1f, -2.96906f),
    glm::vec3(2.95486f, -0.1f, -2.98859f),
    glm::vec3(6.45351f, -0.1f, -1.71961f),
    glm::vec3(11.9634f, -0.1f, 3.76906f),
    glm::vec3(9.50997f, -0.1f, 9.35402f),
    glm::vec3(-1.15953f, -0.1f, 7.61438f)
};
const int numSpiders = 6;
float spiderJumpTimes[numSpiders] = {0.0f}; // Tiempo de salto para cada araña
float spiderHeights[numSpiders] = {0.0f}; // Altura actual de cada araña

// Posición de Snoop Dogg (mira al jugador cuando lo miras) - Ajustado al suelo
glm::vec3 snoopDoggPos(-11.1981f, 0.0f, 10.1127f);
float snoopDoggRotation = 0.0f;

// Posiciones de los gatos (giran continuamente) - Ajustadas al suelo
glm::vec3 gatoPositions[] = {
    glm::vec3(3.28471f, 0.0f, -3.44806f),
    glm::vec3(13.1246f, 0.0f, 1.48878f),
    glm::vec3(13.9297f, 0.0f, 9.10163f),
    glm::vec3(2.88328f, 0.0f, 5.63403f),
    glm::vec3(-12.2928f, 0.0f, 8.89291f)
};
const int numGatos = 5;
float gatoRotations[numGatos] = {0.0f}; // Rotación actual de cada gato

bool checkEnemyCollision(const glm::vec3& playerPos, const std::vector<Enemy*>& enemies, float collisionThreshold = 0.5f) {
    for (const Enemy* enemy : enemies) {
        float distance = glm::distance(playerPos, enemy->position);
        if (distance < collisionThreshold) {
            return true; // Colisión detectada
        }
    }
    return false;
}

bool checkPortalCollision(const glm::vec3& playerPos, const glm::vec3& portalPosition, float collisionThreshold = 1.0f) {
    float distance = glm::distance(playerPos, portalPosition);
    return distance < collisionThreshold;
}

void resetPlayer(Camera& camera) {
    camera.Position = initialCameraPosition;
    camera.Front = glm::vec3(0.0f, 0.0f, -1.0f); // Orientación inicial
    camera.Yaw = -90.0f; // Asegura que la dirección sea consistente
    camera.Pitch = 0.0f;
    camera.updateCameraVectors();
}

glm::mat4 projection;

// === FUNCIONES DE COMPORTAMIENTO PARA NUEVOS MODELOS ===

// Función para actualizar el salto de las arañas
void updateSpiderJumps(float deltaTime) {
    for (int i = 0; i < numSpiders; i++) {
        spiderJumpTimes[i] += deltaTime;
        
        // Cada araña salta cada 2-4 segundos (aleatorio)
        float jumpInterval = 2.0f + (i * 0.5f); // Diferentes intervalos para cada araña
        
        if (spiderJumpTimes[i] >= jumpInterval) {
            spiderJumpTimes[i] = 0.0f;
        }
        
        // Calcular altura del salto (parábola)
        float jumpProgress = spiderJumpTimes[i] / jumpInterval;
        if (jumpProgress <= 0.3f) { // Solo salta en el primer 30% del intervalo
            float normalizedTime = jumpProgress / 0.3f; // 0-1
            spiderHeights[i] = 0.8f * sin(normalizedTime * glm::pi<float>()); // Salto parabólico
        } else {
            spiderHeights[i] = 0.0f;
        }
    }
}

// Función para actualizar la rotación de Snoop Dogg mirando al jugador
void updateSnoopDoggLook(const glm::vec3& playerPos, const glm::vec3& playerFront) {
    // Calcular si el jugador está mirando hacia Snoop Dogg
    glm::vec3 directionToSnoop = glm::normalize(snoopDoggPos - playerPos);
    float dotProduct = glm::dot(glm::normalize(playerFront), directionToSnoop);
    
    // Si el jugador está mirando hacia Snoop Dogg (dot product > 0.5)
    if (dotProduct > 0.5f) {
        // Snoop Dogg mira hacia el jugador
        glm::vec3 directionToPlayer = playerPos - snoopDoggPos;
        snoopDoggRotation = glm::degrees(atan2(directionToPlayer.x, directionToPlayer.z));
    }
}

// Función para actualizar la rotación de los gatos
void updateGatoRotations(float deltaTime) {
    for (int i = 0; i < numGatos; i++) {
        gatoRotations[i] += deltaTime * 180.0f; // Faster rotation for cats
    }
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "The Mentor Backflows", NULL, NULL);
    /*
    // Obtener el monitor principal y su resolución
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Crear la ventana en modo pantalla completa con la resolución del monitor
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "The Legacy of the Curse", primaryMonitor, NULL);
    */

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback); // NUEVO: Callback para clicks del mouse

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------

    Shader ourShader("shaders/shader_pf.vs", "shaders/shader_pf.fs");

    // load models
    // -----------

    Model escenarioModel("modelos/backroom/backrooms.obj");
    Model nether2Model("modelos/nether2/untitled.obj");
    Model fantasmaModel("modelos/fantasma/light.obj");
    Model tvCartoonModel("modelos/tvcartoon/light.obj");
    Model tvLightModel("modelos/tvlight/light.obj");
    Model spiderModel("modelos/spider/spider.obj");
    Model snoopDoggModel("modelos/snoop_dogg/snopdog.obj");
    Model gatoModel("modelos/gato/gato.obj");


    //TEXTURA SUELO
    float floorVertices[] = {
        // Posiciones        // Normales       // Coordenadas de textura (reducidas para menos repetición)
        -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f,  10.0f,
         50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
         50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,

        -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f,  10.0f,
         50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  0.0f,   0.0f
    };

    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    glBindVertexArray(floorVAO);
    // Posiciones
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normales
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Coordenadas UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    unsigned int floorTexture = loadTexture("textures/suelo.jpeg");
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Inicializar la semilla de números aleatorios
    srand(time(0));

    // Mostrar instrucciones del sistema de raycasting
    std::cout << "\n========================================" << std::endl;
    std::cout << "   🎮 THE MENTOR BACKFLOWS 🎮" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "🎯 OBJETIVO: ¡Encuentra el Portal Nether para ganar!" << std::endl;
    std::cout << "📍 Portal ubicado en: (" << portalPos.x << ", " << portalPos.y << ", " << portalPos.z << ")" << std::endl;
    std::cout << "👻 FANTASMAS: Presentes pero no te dañan - Solo ambientación" << std::endl;
    std::cout << "�️ ARAÑAS: 6 arañas que saltan en diferentes intervalos" << std::endl;
    std::cout << "🐱 GATOS: 5 gatos que giran continuamente a diferentes velocidades" << std::endl;
    std::cout << "🎤 SNOOP DOGG: Te mira cuando tú lo miras" << std::endl;
    std::cout << "📺 TVs: COMENTADAS (no aparecen en el juego)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "CONTROLES SIMPLIFICADOS:" << std::endl;
    std::cout << "- Click Derecho: Obtener coordenadas PRECISAS del punto donde miras" << std::endl;
    std::cout << "- WASD: Movimiento" << std::endl;
    std::cout << "- SHIFT: Correr" << std::endl;
    std::cout << "- ESPACIO: Salto MEJORADO (con coyote time y salto variable)" << std::endl;
    std::cout << "- F: Encender/Apagar linterna (MEJORADA - Mayor alcance e intensidad)" << std::endl;
    std::cout << "- ESC: Salir del juego" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "🚀 MEJORAS DEL SISTEMA:" << std::endl;
    std::cout << "   - Gravedad más realista (-20.0f)" << std::endl;
    std::cout << "   - Salto mejorado (6.0f) con coyote time" << std::endl;
    std::cout << "   - Raycasting DIRECTO (5 decimales) - Solo click derecho" << std::endl;
    std::cout << "   - Velocidad terminal para caídas suaves" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "⚡ SISTEMA DE LUCES INTERMITENTES AUTOMÁTICO:" << std::endl;
    std::cout << "   - Point Light: 10 segundos ENCENDIDO, 5 segundos APAGADO" << std::endl;
    std::cout << "   - Linterna (F): Funciona independientemente del ciclo automático" << std::endl;
    std::cout << "Point Light inicializado en posición: (" << pointLightPos.x << ", " << pointLightPos.y << ", " << pointLightPos.z << ")" << std::endl;
    std::cout << "Modelo de luz renderizado en: (" << pointLightPos.x << ", " << pointLightPos.y << ", " << pointLightPos.z << ")" << std::endl;
    std::cout << "Cámara inicial en: (" << initialCameraPosition.x << ", " << initialCameraPosition.y << ", " << initialCameraPosition.z << ")" << std::endl;
    
    float distanceToLight = glm::distance(initialCameraPosition, pointLightPos);
    std::cout << "Distancia de cámara a point light: " << distanceToLight << " unidades" << std::endl;
    
    std::cout << "Estado inicial: LUCES ENCENDIDAS (10 segundos)" << std::endl;
    std::cout << "NOTA: Sistema de luces completamente automático" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Sistema de luces intermitentes automático
        lightCycleTime += deltaTime;
        if (lightsActive) {
            if (lightCycleTime >= LIGHTS_ON_TIME) {
                lightsActive = false;
                lightCycleTime = 0.0f;
                std::cout << "🌑 POINT LIGHT APAGADO - 5 segundos (linterna sigue funcionando)" << std::endl;
            }
        } else {
            if (lightCycleTime >= LIGHTS_OFF_TIME) {
                lightsActive = true;
                lightCycleTime = 0.0f;
                std::cout << "💡 POINT LIGHT ENCENDIDO - 10 segundos" << std::endl;
            }
        }

        // Actualizar rotación de las TVs
        tvRotationAngle += 30.0f * deltaTime; // 30 grados por segundo
        if (tvRotationAngle >= 360.0f) {
            tvRotationAngle -= 360.0f;
        }

        // === ACTUALIZAR COMPORTAMIENTOS DE NUEVOS MODELOS ===
        updateSpiderJumps(deltaTime);
        updateSnoopDoggLook(camera.Position, camera.Front);
        updateGatoRotations(deltaTime);

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // La velocidad de la cámara ahora se maneja dentro de processInput()
        // camera.MovementSpeed = cameraSpeed; // Comentado para evitar conflictos

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setBool("useLighting", true);

        // Efecto de inclinación de cámara mejorado
        float tiltIntensity = isOnGround ? 0.5f : 0.1f; // Menos balanceo en el aire
        float tiltAngle = glm::radians(tiltIntensity) * sin(cameraTiltTime);  // Oscilación suave
        // El cameraTiltTime ahora se maneja en processInput() según el movimiento
        // Crear matriz de rotación para la inclinación
        glm::mat4 tilt = glm::rotate(glm::mat4(1.0f), tiltAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        // Modificar la matriz de vista con inclinación
        glm::mat4 view = camera.GetViewMatrix() * tilt;

        // view/projection transformations
        projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);
        //glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //world transformation
        glm::mat4 model = glm::mat4(1.0f);

        // === RENDERIZAR EL SUELO CON SU TEXTURA === (Comentado para permitir entrada al backroom)
        /*
        ourShader.use();
        model = glm::mat4(1.0f);
        ourShader.setMat4("model", model);

        // Activar textura del suelo SOLO para el suelo
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        ourShader.setInt("texture_emissive1", 0); // Enlazar con el shader

        // Dibujar el suelo
        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Desactivar la textura del suelo después de usarla
        glBindTexture(GL_TEXTURE_2D, 0);
        */

        //calculo gravedad y mecánicas de movimiento mejoradas
        // Aplicar gravedad mejorada cuando no está en el suelo
        if (camera.Position.y > groundLevel || !isOnGround) {
            verticalVelocity += gravity * deltaTime; // Aplicar gravedad
            
            // Limitar velocidad terminal para evitar caídas extremas
            if (verticalVelocity < terminalVelocity) {
                verticalVelocity = terminalVelocity;
            }
            
            camera.Position.y += verticalVelocity * deltaTime; // Actualizar posición Y
            isOnGround = false; // No está en el suelo
        }
        
        // Cuando toca o está por debajo del suelo
        if (camera.Position.y <= groundLevel) {
            camera.Position.y = groundLevel; // Fijar en el nivel del suelo
            if (verticalVelocity < 0) { // Solo resetear si estaba cayendo
                verticalVelocity = 0.0f; // Restablecer velocidad vertical
            }
            isOnGround = true; // Está en el suelo
        }

        //Uso de ourShader
        ourShader.use();
        ourShader.setBool("useLighting", true); // Activar iluminación
        ourShader.setBool("flashlightOn", flashlightOn); // Linterna funciona independientemente del sistema automático
        ourShader.setVec3("flashlightPos", camera.Position); // Posición de la linterna
        ourShader.setVec3("flashlightDir", camera.Front); // Dirección de la linterna
        ourShader.setFloat("cutoff", glm::cos(glm::radians(20.0f))); // Ángulo interno más amplio
        ourShader.setFloat("outerCutoff", glm::cos(glm::radians(25.0f))); // Ángulo externo más amplio
        ourShader.setFloat("constant", 1.0f); // Atenuación constante
        ourShader.setFloat("linear", 0.045f); // Atenuación lineal reducida para mayor alcance
        ourShader.setFloat("quadratic", 0.0075f); // Atenuación cuadrática reducida para mayor alcance

        // Configurar point light ANTES de renderizar objetos - Solo activo cuando lightsActive es true
        ourShader.setBool("usePointLight", lightsActive);
        ourShader.setVec3("pointLight.position", pointLightPos);
        if (lightsActive) {
            ourShader.setVec3("pointLight.ambient", glm::vec3(0.5f, 0.4f, 0.25f));  // Ambiente súper intenso
            ourShader.setVec3("pointLight.diffuse", glm::vec3(3.0f, 2.5f, 1.5f));   // Difusa extremadamente intensa
            ourShader.setVec3("pointLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));  // Especular blanco
        } else {
            ourShader.setVec3("pointLight.ambient", glm::vec3(0.0f, 0.0f, 0.0f));   // Sin iluminación
            ourShader.setVec3("pointLight.diffuse", glm::vec3(0.0f, 0.0f, 0.0f));   // Sin iluminación
            ourShader.setVec3("pointLight.specular", glm::vec3(0.0f, 0.0f, 0.0f));  // Sin iluminación
        }
        ourShader.setFloat("pointLight.constant", 1.0f);
        ourShader.setFloat("pointLight.linear", 0.014f);      // Atenuación mínima
        ourShader.setFloat("pointLight.quadratic", 0.0007f);  // Atenuación súper mínima
        
        // Pasar la posición de la cámara (para el cálculo especular)
        ourShader.setVec3("viewPos", camera.Position);

        // render the loaded model
        // Renderizar el escenario (backrooms - escalado para permitir entrada)
        ourShader.use();
        ourShader.setBool("useLighting", true);
        // No activar ninguna textura manual aquí, dejar que el modelo use sus propias texturas
        glm::mat4 cityModel = glm::mat4(1.0f);
        cityModel = glm::translate(cityModel, glm::vec3(0.0f, -0.1f, 0.0f)); // Ligeramente hundido para mejor acceso
        cityModel = glm::scale(cityModel, glm::vec3(0.5f, 0.5f, 0.5f));	// Escala aumentada para permitir entrada
        ourShader.setMat4("model", cityModel);
        escenarioModel.Draw(ourShader);

        // Actualizar la posición de cada fantasma - TODOS te siguen
        updateSpecialEnemy(fantasma1, deltaTime, camera.Position);
        updateSpecialEnemy(fantasma2, deltaTime, camera.Position);
        updateSpecialEnemy(fantasma3, deltaTime, camera.Position);
        updateSpecialEnemy(fantasma4, deltaTime, camera.Position);

        // Verificar colisión con el portal (objetivo del juego)
        if (!gameWon && checkPortalCollision(camera.Position, portalPos)) {
            gameWon = true;
            std::cout << "\n🎉🎉🎉 ¡¡¡FELICIDADES!!! 🎉🎉🎉" << std::endl;
            std::cout << "🏆 ¡HAS ENCONTRADO EL PORTAL NETHER! 🏆" << std::endl;
            std::cout << "✨ ¡¡¡GANASTE EL JUEGO!!! ✨" << std::endl;
            std::cout << "🎮 El programa se cerrará automáticamente..." << std::endl;

            // Close the program
            glfwSetWindowShouldClose(window, true);
        }

        // Verificar colisión con enemigos - Los fantasmas reinician al jugador
        if (checkEnemyCollision(camera.Position, enemies)) {
            resetPlayer(camera, enemies); // Reiniciar al jugador
            std::cout << "¡Has sido atrapado por un fantasma! Reiniciando posición..." << std::endl;
        }

        // Dibujo de cada fantasma con su nueva posición
        glm::mat4 transform;

        transform = glm::translate(glm::mat4(1.0f), fantasma1.position);
        transform = glm::rotate(transform, glm::radians(fantasma1.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación
        transform = glm::scale(transform, glm::vec3(0.15f, 0.15f, 0.15f)); // Más grande
        ourShader.setMat4("model", transform);
        fantasmaModel.Draw(ourShader);

        transform = glm::translate(glm::mat4(1.0f), fantasma2.position);
        transform = glm::rotate(transform, glm::radians(fantasma2.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación
        transform = glm::scale(transform, glm::vec3(0.15f, 0.15f, 0.15f)); // Más grande
        ourShader.setMat4("model", transform);
        fantasmaModel.Draw(ourShader);

        transform = glm::translate(glm::mat4(1.0f), fantasma3.position);
        transform = glm::rotate(transform, glm::radians(fantasma3.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación
        transform = glm::scale(transform, glm::vec3(0.18f, 0.18f, 0.18f)); // Más grande
        ourShader.setMat4("model", transform);
        fantasmaModel.Draw(ourShader);

        transform = glm::translate(glm::mat4(1.0f), fantasma4.position);
        transform = glm::rotate(transform, glm::radians(fantasma4.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación
        transform = glm::scale(transform, glm::vec3(0.2f, 0.2f, 0.2f)); // Más grande
        ourShader.setMat4("model", transform);
        fantasmaModel.Draw(ourShader);

        // === RENDERIZAR NUEVOS MODELOS ===
        
        // Renderizar las arañas que saltan
        ourShader.use();
        ourShader.setBool("useLighting", true);
        for (int i = 0; i < numSpiders; i++) {
            glm::mat4 spiderTransform = glm::mat4(1.0f);
            glm::vec3 spiderPos = spiderPositions[i];
            spiderPos.y += spiderHeights[i]; // Añadir altura del salto
            spiderTransform = glm::translate(spiderTransform, spiderPos);
            spiderTransform = glm::scale(spiderTransform, glm::vec3(0.08f, 0.08f, 0.08f)); // Arañas pequeñas
            ourShader.setMat4("model", spiderTransform);
            spiderModel.Draw(ourShader);
        }

        // Renderizar Snoop Dogg (mira al jugador)
        glm::mat4 snoopTransform = glm::mat4(1.0f);
        snoopTransform = glm::translate(snoopTransform, snoopDoggPos);
        snoopTransform = glm::rotate(snoopTransform, glm::radians(snoopDoggRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        snoopTransform = glm::scale(snoopTransform, glm::vec3(0.2f, 0.2f, 0.2f)); // Snoop Dogg tamaño medio
        ourShader.setMat4("model", snoopTransform);
        snoopDoggModel.Draw(ourShader);

        // Renderizar los gatos que giran
        for (int i = 0; i < numGatos; i++) {
            glm::mat4 gatoTransform = glm::mat4(1.0f);
            gatoTransform = glm::translate(gatoTransform, gatoPositions[i]);
            gatoTransform = glm::rotate(gatoTransform, glm::radians(gatoRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
            gatoTransform = glm::scale(gatoTransform, glm::vec3(0.50f, 0.50f, 0.50f)); // Gatos pequeños
            ourShader.setMat4("model", gatoTransform);
            gatoModel.Draw(ourShader);
        }

        /*
        // Renderizar las TVs con light maps y rotación
        ourShader.use();
        ourShader.setBool("useLighting", false); // Sin iluminación para que se vean emisivas como TVs
        
        // TV Cartoon en primera posición - MÁS GRANDE
        glm::mat4 modelTVCartoon = glm::mat4(1.0f);
        modelTVCartoon = glm::translate(modelTVCartoon, tvCartoonPos);
        modelTVCartoon = glm::rotate(modelTVCartoon, glm::radians(tvRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación Y
        modelTVCartoon = glm::scale(modelTVCartoon, glm::vec3(0.75f, 0.75f, 0.75f)); // Escala más grande
        ourShader.setMat4("model", modelTVCartoon);
        tvCartoonModel.Draw(ourShader);

        // TV Light en segunda posición - MÁS PEQUEÑA
        glm::mat4 modelTVLight1 = glm::mat4(1.0f);
        modelTVLight1 = glm::translate(modelTVLight1, tvLightPos1);
        modelTVLight1 = glm::rotate(modelTVLight1, glm::radians(tvRotationAngle + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación diferente
        modelTVLight1 = glm::scale(modelTVLight1, glm::vec3(0.01f, 0.01f, 0.01f)); // Escala más pequeña
        ourShader.setMat4("model", modelTVLight1);
        tvLightModel.Draw(ourShader);

        // TV Light en tercera posición - MÁS PEQUEÑA
        glm::mat4 modelTVLight2 = glm::mat4(1.0f);
        modelTVLight2 = glm::translate(modelTVLight2, tvLightPos2);
        modelTVLight2 = glm::rotate(modelTVLight2, glm::radians(tvRotationAngle + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación aún más diferente
        modelTVLight2 = glm::scale(modelTVLight2, glm::vec3(0.16f, 0.16f, 0.16f)); // Escala más pequeña
        ourShader.setMat4("model", modelTVLight2);
        tvLightModel.Draw(ourShader);

        // TV Light en cuarta posición - MÁS PEQUEÑA
        glm::mat4 modelTVLight3 = glm::mat4(1.0f);
        modelTVLight3 = glm::translate(modelTVLight3, tvLightPos3);
        modelTVLight3 = glm::rotate(modelTVLight3, glm::radians(tvRotationAngle + 270.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación opuesta
        modelTVLight3 = glm::scale(modelTVLight3, glm::vec3(0.14f, 0.14f, 0.14f)); // Escala más pequeña
        ourShader.setMat4("model", modelTVLight3);
        tvLightModel.Draw(ourShader);

        // Reactivar iluminación para otros objetos
        ourShader.setBool("useLighting", true);
        */

        // Renderizar el Portal Nether2 (objetivo del juego)
        ourShader.use();
        ourShader.setBool("useLighting", true);
        glm::mat4 modelPortal = glm::mat4(1.0f);
        modelPortal = glm::translate(modelPortal, portalPos);
        modelPortal = glm::scale(modelPortal, glm::vec3(0.15f, 0.15f, 0.15f));
        // Portal estático sin rotación
        modelPortal = glm::rotate(modelPortal, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", modelPortal);
        nether2Model.Draw(ourShader);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Movimiento básico - aplicar velocidad modificada por correr
    float currentSpeed = cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        currentSpeed = 2.5f; // Velocidad de correr
    }

    // Aplicar la velocidad actual a la cámara
    float oldSpeed = camera.MovementSpeed;
    camera.MovementSpeed = currentSpeed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Restaurar velocidad original
    camera.MovementSpeed = oldSpeed;

    // Salto mejorado - solo si está en el suelo y con mejor control
    static bool spacePressed = false;
    static float coyoteTime = 0.0f; // Tiempo de gracia para saltar después de dejar el suelo
    const float maxCoyoteTime = 0.15f; // 150ms de tiempo de gracia
    
    // Actualizar coyote time
    if (isOnGround) {
        coyoteTime = maxCoyoteTime;
    } else {
        coyoteTime -= deltaTime;
        if (coyoteTime < 0) coyoteTime = 0;
    }
    
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed && (isOnGround || coyoteTime > 0)) {
        verticalVelocity = jumpForce; // Aplicar fuerza de salto mejorada
        isOnGround = false; // Ya no está en el suelo
        coyoteTime = 0; // Resetear coyote time
        spacePressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spacePressed = false;
        // Salto variable - si sueltas espacio antes, reduces la altura del salto
        if (verticalVelocity > 0) {
            verticalVelocity *= 0.5f;
        }
    }

    //balanceo de la camara (solo cuando se mueve)
    bool isMoving = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) && isOnGround;

    if (isMoving) {
        float tiltSpeed = (currentSpeed > 2.0f) ? 0.8f : 0.5f; // Balanceo más rápido al correr
        cameraTiltTime += deltaTime * tiltSpeed;
    }
    else {
        cameraTiltTime = 0.0f;  // Reinicia la inclinación si el jugador está quieto
    }

    //Apagar/Prender linterna
    static bool flashlightKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !flashlightKeyPressed) {
        flashlightOn = !flashlightOn; // Alternar el estado de la linterna
        flashlightKeyPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        flashlightKeyPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Callback para clicks del mouse - SIMPLIFICADO para raycasting directo
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Click derecho para obtener coordenadas directamente
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // Obtener dimensiones de la ventana
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Realizar raycasting mejorado
        RaycastHit hit = performRaycast(camera, (float)width, (float)height);

        // Verificar que la distancia sea razonable
        if (hit.distance > 50.0f) {
            std::cout << "\n⚠️  ADVERTENCIA: Punto muy lejano (>" << hit.distance << " unidades)" << std::endl;
        }

        // Mostrar coordenadas mejoradas
        std::cout << "\n=== 🎯 COORDENADAS PRECISAS ===" << std::endl;
        std::cout << "📍 X: " << std::fixed << std::setprecision(5) << hit.position.x << std::endl;
        std::cout << "📍 Y: " << std::fixed << std::setprecision(5) << hit.position.y << std::endl;
        std::cout << "📍 Z: " << std::fixed << std::setprecision(5) << hit.position.z << std::endl;
        std::cout << "📏 Distancia: " << std::fixed << std::setprecision(2) << hit.distance << " unidades" << std::endl;
        
        // Mostrar coordenadas en formato para copiar fácilmente
        std::cout << "\n--- 📋 PARA COPIAR ---" << std::endl;
        std::cout << "glm::vec3(" << std::fixed << std::setprecision(5) << hit.position.x << "f, " 
                  << hit.position.y << "f, " << hit.position.z << "f)" << std::endl;
        
        // También mostrar la posición actual de la cámara para referencia
        std::cout << "\n=== 📷 CÁMARA ACTUAL ===" << std::endl;
        std::cout << "X: " << std::fixed << std::setprecision(5) << camera.Position.x << std::endl;
        std::cout << "Y: " << std::fixed << std::setprecision(5) << camera.Position.y << std::endl;
        std::cout << "Z: " << std::fixed << std::setprecision(5) << camera.Position.z << std::endl;
        
        // Mostrar distancia entre cámara y punto clickeado
        float distanceFromCamera = glm::distance(camera.Position, hit.position);
        std::cout << "🔗 Distancia desde cámara: " << std::fixed << std::setprecision(2) << distanceFromCamera << " unidades" << std::endl;
        std::cout << "========================\n" << std::endl;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
