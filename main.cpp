#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <fstream>
#include <sstream>
std::string filename = "cubes_positions.txt";
GLuint skyboxShaderProgram;
float animationTime = 0.0f;
bool isMoving = false;
float fov = 45.0f;
float zoomLevel = 5.0f;  // Initial distance from character
// Window dimensions
 unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;
glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 20.0f);  // Increased height and distance

glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Mouse control
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
unsigned int shaderProgram;
// Character
glm::vec3 characterPos = glm::vec3(0.0f, 1.0f, 0.0f);
float characterSize = 0.5f;
float characterRotation = 0.0f;
// Floor
float floorSize = 50.0f;

float gravity = -9.8f;
bool isJumping = false;
float jumpVelocity = 5.0f;
float verticalVelocity = 0.0f;

const char* skyboxVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    out vec3 TexCoords;

    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        TexCoords = aPos;
        vec4 pos = projection * view * vec4(aPos, 1.0);
        gl_Position = pos.xyww;
    }
)";


const char* skyboxFragmentShaderSource = R"(
    #version 330 core
    in vec3 TexCoords;

    out vec4 FragColor;

    uniform samplerCube skybox;

    void main() {
        FragColor = texture(skybox, TexCoords);
    }
)";


const char* vertexShaderSource = R"(
    #version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

)";

const char* fragmentShaderSource = R"(
    #version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform bool useTexture;
uniform vec3 color;
uniform sampler2D textureSampler;

void main()
{
    if (useTexture)
        FragColor = texture(textureSampler, TexCoord);
    else
        FragColor = vec4(color, 1.0);
}
)";

std::vector<float> characterVertices = {
    // Front face
    -0.25f, -0.75f,  0.25f,
     0.25f, -0.75f,  0.25f,
     0.25f,  0.75f,  0.25f,
    -0.25f,  0.75f,  0.25f,
    // Back face
    -0.25f, -0.75f, -0.25f,
     0.25f, -0.75f, -0.25f,
     0.25f,  0.75f, -0.25f,
    -0.25f,  0.75f, -0.25f,
    // Right face
     0.25f, -0.75f,  0.25f,
     0.25f, -0.75f, -0.25f,
     0.25f,  0.75f, -0.25f,
     0.25f,  0.75f,  0.25f,
     // Left face
     -0.25f, -0.75f,  0.25f,
     -0.25f, -0.75f, -0.25f,
     -0.25f,  0.75f, -0.25f,
     -0.25f,  0.75f,  0.25f,
     // Top face
     -0.25f,  0.75f,  0.25f,
      0.25f,  0.75f,  0.25f,
      0.25f,  0.75f, -0.25f,
     -0.25f,  0.75f, -0.25f,
     // Bottom face
     -0.25f, -0.75f,  0.25f,
      0.25f, -0.75f,  0.25f,
      0.25f, -0.75f, -0.25f,
     -0.25f, -0.75f, -0.25f,
};

std::vector<unsigned int> characterIndices = {
    0, 1, 2, 2, 3, 0,       // Front face
    4, 5, 6, 6, 7, 4,       // Back face
    8, 9, 10, 10, 11, 8,    // Right face
    12, 13, 14, 14, 15, 12, // Left face
    16, 17, 18, 18, 19, 16, // Top face
    20, 21, 22, 22, 23, 20  // Bottom face
};



float skyboxVertices[] = {
    // Positions
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f
};

unsigned int skyboxIndices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    0, 1, 5, 5, 4, 0,
    2, 3, 7, 7, 6, 2,
    0, 3, 7, 7, 4, 0,
    1, 2, 6, 6, 5, 1
};

GLuint skyboxVAO, skyboxVBO, skyboxEBO;
void setupSkybox()
{
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);

    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}
std::string parentDir = "./Standard-Cube-Map/";

std::vector<std::string> facemap =
{
    parentDir+"px.png",
    parentDir + "nx.png",
    parentDir + "py.png",
    parentDir + "ny.png",
    parentDir + "pz.png",
    parentDir + "nz.png"
};

GLuint loadCubemap(std::vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int width, height, nrChannels;
    for (GLuint i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    return textureID;
}

struct Cube {
    glm::vec3 position;
    
};


bool isTargeting = false;
glm::vec3 targetPosition;
bool leftMouseButtonPressed = false;
// Cube properties
float cubeSize = 0.7f;
glm::vec3 cubeColor(0.8f, 0.4f, 0.1f);
std::vector<Cube> placedCubes;
unsigned int floorTexture;
unsigned int cubeTexture;
unsigned int loadTexture(const char* path)
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
bool rayIntersectsCube(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& cubeMin, const glm::vec3& cubeMax, float& distance)
{
    glm::vec3 invDir = 1.0f / rayDir;
    glm::vec3 t1 = (cubeMin - rayOrigin) * invDir;
    glm::vec3 t2 = (cubeMax - rayOrigin) * invDir;

    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);

    float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    if (tNear > tFar || tFar < 0) return false;

    distance = tNear;
    return true;
}
unsigned int cubeVAO, cubeVBO,cubeEBO;

void createCube()
{

    float vertices[] = {
        // positions          // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawCube(unsigned int shaderProgram, const glm::vec3& position, unsigned int texture)
{
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(cubeSize));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

}

void renderSkybox(GLuint skyboxShaderProgram, GLuint cubemapTexture, glm::mat4 view, glm::mat4 projection) {
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProgram);

    view = glm::mat4(glm::mat3(view)); // Remove translation part of the view matrix
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(skyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource);
void createFloor(unsigned int& VAO, unsigned int& VBO);
void createCharacter(unsigned int& characterVAO, unsigned int& characterVBO, unsigned int& characterEBO);
bool checkCollision();
glm::mat4 animateCharacter(float deltaTime, unsigned int& characterVAO);
void drawCharacterPart(const glm::mat4& model, const glm::vec3& position, const glm::vec3& scale, unsigned int& characterVAO);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    zoomLevel -= (float)yoffset * 0.5f;  // Adjust this multiplier to change zoom sensitivity
    if (zoomLevel < 1.0f)
        zoomLevel = 1.0f;
    if (zoomLevel > 20.0f)  // Adjust max zoom out distance as needed
        zoomLevel = 20.0f;
}


void createHouse() {

    std::ifstream inFile("starter_cubes.txt");
    if (inFile.is_open())
    {
        placedCubes.clear(); // Clear existing cubes before loading new ones

        std::string line;
        while (std::getline(inFile, line))
        {
            std::istringstream ss(line);
            std::string token;
            glm::vec3 position;

            std::getline(ss, token, ',');
            position.x = std::stof(token);
            std::getline(ss, token, ',');
            position.y = std::stof(token);
            std::getline(ss, token, ',');
            position.z = std::stof(token);

            placedCubes.push_back(Cube{ position }); // Use default color or set as needed
        }
        inFile.close();
        std::cout << "Cube positions loaded from " << filename << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file for reading" << std::endl;
    }

    
}

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Set the window size to the monitor's resolution
    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;
    // Create window
    createHouse();
    

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "OpenGL 3D Scene", primaryMonitor, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }


    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Create shader program
    shaderProgram = createShaderProgram(vertexShaderSource,fragmentShaderSource);
     skyboxShaderProgram = createShaderProgram(skyboxVertexShaderSource,skyboxFragmentShaderSource);
     floorTexture = loadTexture("./floor.jpg");
     cubeTexture = loadTexture("./cube.jpg");
    // Create floor and character VAOs
    unsigned int floorVAO, floorVBO;
    createFloor(floorVAO, floorVBO);

    unsigned int characterVAO, characterVBO,characterEBO;
    createCharacter(characterVAO, characterVBO,characterEBO);
    createCube();
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    setupSkybox();

    GLuint cubemapTexture = loadCubemap(facemap);
    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input processing
        processInput(window);

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader program
        glUseProgram(shaderProgram);
        glm::vec3 cameraOffset = -cameraFront * zoomLevel;
        glm::vec3 cameraPosition = characterPos + cameraOffset + glm::vec3(0.0f, 1.0f, 0.0f);  // Add some height offset

        glm::mat4 view = glm::lookAt(cameraPosition, characterPos + glm::vec3(0.0f, 1.0f, 0.0f), cameraUp);
        // Set up view and projection matrices
       
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        renderSkybox(skyboxShaderProgram, cubemapTexture, view, projection);

        // Draw floor
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        for (const auto& cube : placedCubes)
        {
            drawCube(shaderProgram, cube.position, cubeTexture);
        }

        // Draw targeting cube
        if (isTargeting)
        {
            glm::vec3 targetColor(1.0f, 1.0f, 1.0f);  // White for targeting
            drawCube(shaderProgram, targetPosition, cubeTexture);
        }

// Draw character
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (true)
        {
            glm::vec3 oldPos = characterPos;
            characterPos.y += verticalVelocity * deltaTime;
            verticalVelocity += gravity * deltaTime;

            if (checkCollision())
            {
                characterPos = oldPos;
                isJumping = false;
                verticalVelocity = 0.0f;
            }
            else if (characterPos.y <= 1.0f) // Ground level
            {
                characterPos.y = 1.0f;
                isJumping = false;
                verticalVelocity = 0.0f;
            }
        }
   
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.8f, 0.6f, 0.4f); // Skin color
        glUseProgram(shaderProgram);
 

        // Animate and draw character
        animateCharacter(deltaTime,characterVAO);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &characterVAO);
    glDeleteBuffers(1, &characterVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{


    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
bool print = true;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float characterSpeed = 0.003f;
    glm::vec3 newPos = characterPos;
    glm::vec3 movement(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        std::ofstream outFile(filename, std::ios::trunc);
        if (outFile.is_open())
        {
            for (const auto& cube : placedCubes)
            {
                outFile << cube.position.x << "," << cube.position.y << "," << cube.position.z << std::endl;
            }
            outFile.close();
            std::cout << "Cube positions saved to " << filename << std::endl;
        }
        else
        {
            std::cerr << "Failed to open file for writing" << std::endl;
        }
        
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        // Open file for reading
        std::ifstream inFile(filename);
        if (inFile.is_open())
        {
            placedCubes.clear(); // Clear existing cubes before loading new ones

            std::string line;
            while (std::getline(inFile, line))
            {
                std::istringstream ss(line);
                std::string token;
                glm::vec3 position;

                std::getline(ss, token, ',');
                position.x = std::stof(token);
                std::getline(ss, token, ',');
                position.y = std::stof(token);
                std::getline(ss, token, ',');
                position.z = std::stof(token);

                placedCubes.push_back(Cube{ position}); // Use default color or set as needed
            }
            inFile.close();
            std::cout << "Cube positions loaded from " << filename << std::endl;
        }
        else
        {
            std::cerr << "Failed to open file for reading" << std::endl;
        }
       
    }
    if (glm::length(movement) > 0.0f)
    {
        movement = glm::normalize(movement);
        newPos += movement * characterSpeed;

        // Calculate new rotation based on movement direction
        characterRotation = atan2(-movement.x, -movement.z);
    }

    // Check for collision before updating position
    glm::vec3 oldPos = characterPos;
    characterPos = newPos;
    if (checkCollision())
    {
        characterPos = oldPos; // Revert to old position if collision occurs
    }

    isMoving = (glm::length(movement) > 0.0f);


    // Check for collision before updating position
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping)
    {
        isJumping = true;
        verticalVelocity = jumpVelocity;
    }
    float gridSize = 0.75f;
    float offsetY = -0.4f;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        isTargeting = true;

        // Cast a ray from the camera
        glm::vec3 rayOrigin = cameraPos;
        glm::vec3 rayDir = glm::normalize(cameraFront);

        float minDistance = std::numeric_limits<float>::max();
        for (const auto& cube : placedCubes)
        {
            glm::vec3 cubeMin = cube.position - glm::vec3(cubeSize / 2.0f);
            glm::vec3 cubeMax = cube.position + glm::vec3(cubeSize / 2.0f);

            float distance;
            if (rayIntersectsCube(rayOrigin, rayDir, cubeMin, cubeMax, distance))
            {
                if (distance < minDistance)
                {
                    minDistance = distance;
                    targetPosition = cube.position + glm::normalize(rayDir) * cubeSize;
                    // Snap to the nearest grid point
                    targetPosition = glm::round(targetPosition / gridSize) * gridSize;
                    targetPosition.y += offsetY;
                }
            }
        }

        // If no cube was hit, target a position in front of the camera
        if (minDistance == std::numeric_limits<float>::max())
        {
            targetPosition = cameraPos + rayDir * 5.0f;
            // Snap to the nearest grid point
            targetPosition = glm::round(targetPosition / gridSize) * gridSize;
            targetPosition.y += offsetY;
        }

        // Ensure the target position is within a certain range from the character
        float maxPlacementRange = 2.0f;
        glm::vec3 directionToTarget = glm::normalize(targetPosition - characterPos);
        float distanceToTarget = glm::distance(characterPos, targetPosition);

        if (distanceToTarget > maxPlacementRange)
        {
            targetPosition = characterPos + directionToTarget * maxPlacementRange;
            targetPosition = glm::round(targetPosition / gridSize) * gridSize;
            targetPosition.y += offsetY;
        }

    }
    else
    {
        isTargeting = false;
    }

    // Left-click to place cube
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!leftMouseButtonPressed) {
            leftMouseButtonPressed = true;

            if (isTargeting) {
                // Round the target position to the nearest grid position
                glm::vec3 gridPosition = targetPosition;

                // Check if the position is already occupied
                bool canPlace = true;
          

                if (canPlace) {
                    placedCubes.push_back({ gridPosition});
                }
            }
        }
    }
    else {
        leftMouseButtonPressed = false;
    }

    
        //characterPos = glm::vec3(0.0f, 1.0f, 0.0f); // Reset position if collision occurs
    
    // Update camera position to follow the character
    cameraPos = characterPos + glm::vec3(0.0f, 1.0f, 3.0f);
}
GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex Shader Compilation Failed\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment Shader Compilation Failed\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


void createFloor(unsigned int& VAO, unsigned int& VBO)
{
    float vertices[] = {
        // positions        // texture coords
        -floorSize, 0.0f, -floorSize,  0.0f, 0.0f,
         floorSize, 0.0f, -floorSize,  floorSize, 0.0f,
         floorSize, 0.0f,  floorSize,  floorSize, floorSize,
        -floorSize, 0.0f, -floorSize,  0.0f, 0.0f,
         floorSize, 0.0f,  floorSize,  floorSize, floorSize,
        -floorSize, 0.0f,  floorSize,  0.0f, floorSize
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void createCharacter(unsigned int& characterVAO, unsigned int& characterVBO,unsigned int& characterEBO)
{
    glGenVertexArrays(1, &characterVAO);
    glGenBuffers(1, &characterVBO);
    glGenBuffers(1, &characterEBO);

    glBindVertexArray(characterVAO);

    glBindBuffer(GL_ARRAY_BUFFER, characterVBO);
    glBufferData(GL_ARRAY_BUFFER, characterVertices.size() * sizeof(float), characterVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, characterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, characterIndices.size() * sizeof(unsigned int), characterIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

glm::mat4 animateCharacter(float deltaTime, unsigned int& characterVAO)
{
    glm::mat4 model = glm::mat4(1.0f);

    if (isMoving)
    {
        animationTime += deltaTime * 10.0f; // Adjust speed as needed
        float legSwing = sin(animationTime) * 0.2f;

        // Torso
        model = glm::translate(model, characterPos);
        model = glm::rotate(model, characterRotation, glm::vec3(0.0f, 1.0f, 0.0f));

        // Left leg
        

        glm::mat4 leftLeg = glm::translate(model, glm::vec3(-0.125f, -0.5f, 0.0f));
        leftLeg = glm::rotate(leftLeg, legSwing, glm::vec3(1.0f, 0.0f, 0.0f));
        leftLeg = glm::translate(leftLeg, glm::vec3(0.125f, 0.5f, 0.0f));

        // Right leg
        glm::mat4 rightLeg = glm::translate(model, glm::vec3(0.125f, -0.5f, 0.0f));
        rightLeg = glm::rotate(rightLeg, -legSwing, glm::vec3(1.0f, 0.0f, 0.0f));
        rightLeg = glm::translate(rightLeg, glm::vec3(-0.125f, 0.5f, 0.0f));

        // Draw character parts
        drawCharacterPart(model, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.75f, 0.25f), characterVAO); // Torso
        drawCharacterPart(leftLeg, glm::vec3(-0.125f, -0.40f, 0.0f), glm::vec3(0.25f, 0.75f, 0.25f), characterVAO); // Left leg
        drawCharacterPart(rightLeg, glm::vec3(0.125f, -0.40f, 0.0f), glm::vec3(0.25f, 0.75f, 0.25f), characterVAO); // Right leg
    }
    else
    {
        model = glm::translate(model, characterPos);
        model = glm::rotate(model, characterRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 leftLeg = glm::translate(model, glm::vec3(-0.125f, -0.75f, 0.0f));
        glm::mat4 rightLeg = glm::translate(model, glm::vec3(0.125f, -0.75f, 0.0f));
        drawCharacterPart(model, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.75f, 0.25f), characterVAO); // Torso
        drawCharacterPart(leftLeg, glm::vec3(0.0f, 0.37f, 0.0f), glm::vec3(0.25f, 0.75f, 0.25f), characterVAO); // Left leg
        drawCharacterPart(rightLeg, glm::vec3(0.0f, 0.37f, 0.0f), glm::vec3(0.25f, 0.75f, 0.25f), characterVAO); // Right leg
    }

    return model;
}

void drawCharacterPart(const glm::mat4& model, const glm::vec3& position, const glm::vec3& scale, unsigned int& characterVAO)
{
    glm::mat4 partModel = glm::translate(model, position);
    partModel = glm::scale(partModel, scale);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(partModel));
    glBindVertexArray(characterVAO);
    glDrawElements(GL_TRIANGLES, characterIndices.size(), GL_UNSIGNED_INT, 0);
}

bool checkCollision()
{
    // Check floor boundaries
    if (characterPos.x < -floorSize || characterPos.x > floorSize ||
        characterPos.z < -floorSize || characterPos.z > floorSize)
    {
        return true;
    }

    // Check collision with placed cubes
    for (const auto& cube : placedCubes)
    {
        glm::vec3 cubeMin = cube.position - glm::vec3(cubeSize / 2.0f);
        glm::vec3 cubeMax = cube.position + glm::vec3(cubeSize / 2.0f);

        // Expand the cube bounds slightly to account for character size
        float characterRadius = 0.5f; // Adjust based on your character's size
        cubeMin -= glm::vec3(characterRadius);
        cubeMax += glm::vec3(characterRadius);

        if (characterPos.x-0.45f >= cubeMin.x && characterPos.x+0.45f <= cubeMax.x &&
            characterPos.y >= cubeMin.y && characterPos.y-0.45f <= cubeMax.y &&
            characterPos.z - 0.45f >= cubeMin.z && characterPos.z + 0.45f <= cubeMax.z)
        {
            return true;
        }
    }

    return false;
}