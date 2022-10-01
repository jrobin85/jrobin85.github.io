#include <iostream>			// cout, cerr
#include <cstdlib>			// EXIT_FAILURE
#include <GL/glew.h>			// GLEW library
#include <GLFW/glfw3.h>			// GLFW library
#define STB_IMAGE_IMPLEMENTATION	// Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class
#include "../../1-Mod/OpenGLSample/OpenGLSample/stb_image.h"

using namespace std;			// Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Final Project";	// Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;   // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture id
    GLuint gTextureId;
    glm::vec2 gUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs
    GLuint gCubeProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 7.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(2.0f);

    // Cube and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(4.0f, 10.0f, 2.0f);
    glm::vec3 gLightScale(0.1f);

    // Lamp animation
    bool gIsLampOrbiting = true;

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar * cubeVertexShaderSource = GLSL(440,
    layout (location = 0) in vec3 position; 		// Vertex data from Vertex Attrib Pointer 0
    layout (location = 1) in vec3 normal; 		// VAP position 1 for normals
    layout (location = 2) in vec2 textureCoordinate;  	// Texture data

    out vec3 vertexNormal; 				// For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; 			// For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate; 			// variable to transfer texture data to the fragment shader

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); 	// Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; 	// get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate; 			// references incoming texture data
    }
);


/* Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; 		// For incoming normals
in vec3 vertexFragmentPos; 		// For incoming fragment position
in vec2 vertexTextureCoordinate; 	// Variable to hold incoming color data from vertex shader

out vec4 fragmentColor; 		// For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; 	// Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.3f; 			// Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; 	// Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); 				// Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); 	// Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);			// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; 				// Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; 					// Set specular light strength
    float highlightSize = 16.0f; 					// Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); 	// Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);			// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); 					// Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; 	// VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);	// Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor;		// For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); 	// Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader programs
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "../../resources/textures/texture.png";
    if (!UCreateTexture(texFilename, gTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(gTextureId);

    // Release shader programs
    UDestroyShaderProgram(gCubeProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // Forward
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // Backward
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // Left
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // Right
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Down
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // Up
        gCamera.ProcessKeyboard(UP, gDeltaTime);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos;	// reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transforms the camera: move the camera back (z axis)
    //glm::mat4 view = glm::translate(glm::vec3(0.0f, 0.0f, -5.0f));

    // Activate the cube VAO (used by cube and lamp)
    glBindVertexArray(gMesh.vao);

    // CUBE: draw cube
    //----------------
    // Set the shader to be used
    glUseProgram(gCubeProgramId);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    // 2. Rotates shape by in the x axis and y axes
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gCubeProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gCubeProgramId, "view");
    GLint projLoc = glGetUniformLocation(gCubeProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gCubeProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gCubeProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gCubeProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gCubeProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);	// Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh &mesh)
{
     // Position and Color data
    GLfloat verts[] = {
         // Vertex Positions   //Normals            //Texture Coordinates

         // Rubiks cube bottom
         // Triangle 1
         0.5f,  0.17f, 0.0f,   0.0f,  0.0f,  1.0f,  0.75f, 0.17f, // Front Top Right       0 Yellow
         0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,  0.75f, 0.0f,  // Front Bottom Right    1
        -0.5f,  0.17f, 0.0f,   0.0f,  0.0f,  1.0f,  0.5f,  0.17f, // Front Top Left        3

         // Triangle 2
         0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,  0.75f, 0.0f,  // Front Bottom Right    1 Yellow
        -0.5f, -0.5f,  0.0f,   0.0f,  0.0f,  1.0f,  0.5f,  0.0f,  // Front Bottom Left     2
        -0.5f,  0.17f, 0.0f,   0.0f,  0.0f,  1.0f,  0.5f,  0.17f, // Front Top Left        3

         // Triangle 3
         0.5f,  0.17f, 0.0f,   1.0f,  0.0f,  0.0f,  0.5f,  0.67f, // Front Top Right       0 Blue
         0.5f, -0.5f,  0.0f,   1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  // Front Bottom Right    1
         0.5f, -0.5f, -1.0f,   1.0f,  0.0f,  0.0f,  0.75f, 0.5f,  // Back Bottom Right     4

         // Triangle 4
         0.5f,  0.17f, 0.0f,   1.0f,  0.0f,  0.0f,  0.5f,  0.67f, // Front Top Right       0 Blue
         0.5f, -0.5f,  -1.0f,  1.0f,  0.0f,  0.0f,  0.75f, 0.5f,  // Back Bottom Right     4
         0.5f,  0.17f, -1.0f,  1.0f,  0.0f,  0.0f,  0.75f, 0.67f, // Back Top Right        5

         // Triangle 5
         0.5f,  0.17f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f,  0.75f, // Front Top Right       0 Black
         0.5f,  0.17f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  // Back Top Right        5
        -0.5f,  0.17f, -1.0f,  0.0f,  1.0f,  0.0f,  0.75f, 1.0f,  // Back Top Left         6

         // Triangle 6
         0.5f,  0.17f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f,  0.75f, // Front Top Right       0 Black
        -0.5f,  0.17f, 0.0f,   0.0f,  1.0f,  0.0f,  0.75f, 0.75f, // Front Top Left        3
        -0.5f,  0.17f, -1.0f,  0.0f,  1.0f,  0.0f,  0.75f, 1.0f,  // Back Top Left         6

         // Triangle 7
         0.5f, -0.5f,  -1.0f,  0.0f,  0.0f, -1.0f,  0.25f, 0.67f, // Back Bottom Right     4 Green
         0.5f,  0.17f, -1.0f,  0.0f,  0.0f, -1.0f,  0.25f, 0.5f,  // Back Top Right        5
        -0.5f,  0.17f, -1.0f,  0.0f,  0.0f, -1.0f,  0.5f,  0.5f,  // Back Top Left         6

         // Triangle 8
         0.5f, -0.5f,  -1.0f,  0.0f,  0.0f, -1.0f,  0.25f, 0.67f, // Back Bottom Right     4 Green
        -0.5f,  0.17f, -1.0f,  0.0f,  0.0f, -1.0f,  0.5f,  0.5f,  // Back Top Left         6
        -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f, -1.0f,  0.5f,  0.67f, // Back Bottom Left      7

         // Triangle 9
        -0.5f, -0.5f,  0.0f,  -1.0f,  0.0f,  0.0f,  0.75f, 0.25f, // Front Bottom Left     2 White
        -0.5f,  0.17f, 0.0f,  -1.0f,  0.0f,  0.0f,  0.75f, 0.42f, // Front Top Left        3
        -0.5f,  0.17f, -1.0f, -1.0f,  0.0f,  0.0f,  0.5f,  0.42f, // Back Top Left         6

         // Triangle 10
        -0.5f, -0.5f,  0.0f,  -1.0f,  0.0f,  0.0f,  0.75f, 0.25f, // Front Bottom Left     2 White
        -0.5f,  0.17f, -1.0f, -1.0f,  0.0f,  0.0f,  0.5f,  0.42f, // Back Top Left         6
        -0.5f, -0.5f,  -1.0f, -1.0f,  0.0f,  0.0f,  0.5f,  0.25f, // Back Bottom Left      7

         // Triangle 11
         0.5f, -0.5f,  0.0f,   0.0f, -1.0f,  0.0f,  0.25f, 0.5f,  // Front Bottom Right    1 Orange
         0.5f, -0.5f,  -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.5f,  // Back Bottom Right     4
        -0.5f, -0.5f,  -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.75f, // Back Bottom Left      7

         // Triangle 12
         0.5f, -0.5f,  0.0f,   0.0f, -1.0f,  0.0f,  0.25f, 0.5f,  // Front Bottom Right    1 Orange
        -0.5f, -0.5f,  0.0f,   0.0f, -1.0f,  0.0f,  0.25f, 0.75f, // Front Bottom Left     2
        -0.5f, -0.5f, -1.0f,   0.0f, -1.0f,  0.0f,  0.0f,  0.75f, // Back Bottom Left      7

         // Rubiks cube top
         // Triangle 13
         0.71f, 0.5f,  -0.5f,  0.71f,  0.0f,  0.71f,  0.5f,  0.75f, // Right Top             8 Green
         0.71f, 0.17f, -0.5f,  0.71f,  0.0f,  0.71f,  0.5f,  0.67f, // Right Bottom          9
         0.0f,  0.5f,   0.21f, 0.71f,  0.0f,  0.71f,  0.25f, 0.75f, // Front Top            11

         // Triangle 14
         0.71f, 0.17f, -0.5f,  0.71f,  0.0f,  0.71f,  0.5f,  0.67f, // Right Bottom          9 Green
         0.0f,  0.17f,  0.21f, 0.71f,  0.0f,  0.71f,  0.25f, 0.67f, // Front Bottom         10
         0.0f,  0.5f,   0.21f, 0.71f,  0.0f,  0.71f,  0.25f, 0.75f, // Front Top            11

         // Triangle 15
         0.71f, 0.5f,   -0.5f, 0.71f,  0.0f, -0.71f,  0.5f,  0.5f,  // Right Top             8 White
         0.71f, 0.17f,  -0.5f, 0.71f,  0.0f, -0.71f,  0.5f,  0.42f, // Right Bottom          9
         0.0f, 0.17f,  -1.21f, 0.71f,  0.0f, -0.71f,  0.75f, 0.42f, // Back Bottom          12

         // Triangle 16
         0.71f, 0.5f,   -0.5f, 0.71f,  0.0f, -0.71f,  0.5f,  0.5f,  // Right Top             8 White
         0.0f, 0.17f,  -1.21f, 0.71f,  0.0f, -0.71f,  0.75f, 0.42f, // Back Bottom          12
         0.0f,  0.5f,  -1.21f, 0.71f,  0.0f, -0.71f,  0.75f, 0.5f,  // Back Top             13

         // Triangle 17
         0.71f, 0.5f,   -0.5f, 0.0f,  1.0f,  0.71f,  0.5f,  0.0f,  // Right Top             8 Red
         0.0f,  0.5f,  -1.21f, 0.0f,  1.0f,  0.71f,  0.5f,  0.25f, // Back Top             13
         -0.71, 0.5f,  -0.5f,  0.0f,  1.0f,  0.71f,  0.25f, 0.25f, // Left Top             14

         // Triangle 18
         0.71f, 0.5f,   -0.5f, 0.0f,  1.0f,  0.71f,  0.5f,  0.0f,  // Right Top             8 Red
         0.0f,  0.5f,   0.21f, 0.0f,  1.0f,  0.71f,  0.25f, 0.0f,  // Front Top            11
         -0.71, 0.5f,  -0.5f,  0.0f,  1.0f,  0.71f,  0.25f, 0.25f, // Left Top             14

         // Triangle 19
         0.0f, 0.17f,  -1.21f,-0.71f,  0.0f, -0.71f,  0.5f,  0.25f, // Back Bottom          12 Yellow
         0.0f,  0.5f,  -1.21f,-0.71f,  0.0f, -0.71f,  0.5f,  0.17f, // Back Top             13
         -0.71, 0.5f,  -0.5f, -0.71f,  0.0f, -0.71f,  0.75f, 0.17f, // Left Top             14

         // Triangle 20
         0.0f, 0.17f,  -1.21f,-0.71f,  0.0f, -0.71f,  0.5f,  0.25f, // Back Bottom          12 Yellow
         -0.71, 0.5f,  -0.5f, -0.71f,  0.0f, -0.71f,  0.75f, 0.17f, // Left Top             14
         -0.71, 0.17f, -0.5f, -0.71f,  0.0f, -0.71f,  0.75f, 0.25f, // Left Bottom          15

         // Triangle 21
         0.0f,  0.17f,  0.21f,-0.71f,  0.0f,  0.71f,  0.75f, 0.67f, // Front Bottom         10 Blue
         0.0f,  0.5f,   0.21f,-0.71f,  0.0f,  0.71f,  0.75f, 0.75f, // Front Top            11
         -0.71, 0.5f,  -0.5f, -0.71f,  0.0f,  0.71f,  0.5f,  0.75f, // Left Top             14

         // Triangle 22
         0.0f,  0.17f,  0.21f,-0.71f,  0.0f,  0.71f,  0.75f, 0.67f, // Front Bottom         10 Blue
         -0.71, 0.5f,  -0.5f, -0.71f,  0.0f,  0.71f,  0.5f,  0.75f, // Left Top             14
         -0.71, 0.17f, -0.5f, -0.71f,  0.0f,  0.71f,  0.5f,  0.67f, // Left Bottom          15

         // Triangle 23
         0.71f, 0.17f,  -0.5f, 0.0f,  -1.0f,  0.0f,   1.0f,  0.75f, // Right Bottom           9 Black
         0.0f, 0.17f,  -1.21f, 0.0f,  -1.0f,  0.0f,   0.75f, 0.75f, // Back Bottom            12
         -0.71, 0.17f, -0.5f,  0.0f,  -1.0f,  0.0f,   0.75f, 1.0f,  // Left Bottom            15

         // Triangle 21
         0.71f, 0.17f,  -0.5f, 0.0f,  -1.0f,  0.0f,   1.0f,  0.75f, // Right Bottom           9 Black
         0.0f,  0.17f,  0.21f, 0.0f,  -1.0f,  0.0f,   1.0f,  1.0f,  // Front Bottom           10
         -0.71, 0.17f, -0.5f,  0.0f,  -1.0f,  0.0f,   0.75f, 1.0f,  // Left Bottom            15

         // Table Plane
         // Triangle 25
         -6.0f, -1.5f, -5.0f,  0.0f,   1.0f,  0.0f,   0.25f, 0.0f,   // Top Left               16
          4.0f, -1.5f, -5.0f,  0.0f,   1.0f,  0.0f,   0.0f,  0.0f,   // Top Right              17
          4.0f, -1.5f,  5.0f,  0.0f,   1.0f,  0.0f,   0.0f,  0.25f,  // Bottom Right           18
         // Triangle 26
         -6.0f, -1.5f, -5.0f,  0.0f,   1.0f,  0.0f,   0.25f, 0.0f,   // Top Left               16
          4.0f, -1.5f,  5.0f,  0.0f,   1.0f,  0.0f,   0.0f,  0.25f,  // Bottom Right           18
         -6.0f, -1.5f,  5.0f,  0.0f,   1.0f,  0.0f,   0.25f, 0.25f,  // Bottom Left            19

         //Book
         // Triangle 27
         2.12f, -0.5f, -1.21f, 0.71f,  0.0f,  0.71f,  0.25f, 0.5f,  // Right Top            
         2.12f, -1.5f, -1.21f, 0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Right Bottom         
         0.0f,  -0.5f,  0.91f, 0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Front Top            

         // Triangle 28
         2.12f, -1.5f, -1.21f, 0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Right Bottom          
         0.0f,  -1.5f,  0.91f, 0.71f,  0.0f,  0.71f,  0.0f,  0.25f, // Front Bottom         
         0.0f,  -0.5f,  0.91f, 0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Front Top            

         // Triangle 29
         2.12f, -0.5f, -1.21f, 0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Right Top             
         2.12f, -1.5f, -1.21f, 0.71f,  0.0f, -0.71f,  0.0f,  0.25f, // Right Bottom          
         0.71f, -1.5f, -2.62f, 0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Back Bottom          

         // Triangle 30
         2.12f, -0.5f, -1.21f, 0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Right Top             
         0.71f, -1.5f, -2.62f, 0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Back Bottom          
         0.71f, -0.5f, -2.62f, 0.71f,  0.0f, -0.71f,  0.25f, 0.5f,  // Back Top             

         // Triangle 31
         2.12f, -0.5f, -1.21f,  0.0f,  1.0f,  0.71f,  0.25f, 0.25f, // Right Top             
         0.71f, -0.5f, -2.62f,  0.0f,  1.0f,  0.71f,  0.25f, 0.5f,  // Back Top             
         -1.41f, -0.5f, -0.5f,  0.0f,  1.0f,  0.71f,  0.0f,  0.5f,  // Left Top             

         // Triangle 32
         2.12f, -0.5f, -1.21f,  0.0f,  1.0f,  0.71f,  0.25f, 0.25f, // Right Top             
         0.0f,  -0.5f,  0.91f,  0.0f,  1.0f,  0.71f,  0.0f,  0.25f, // Front Top            
         -1.41f, -0.5f, -0.5f,  0.0f,  1.0f,  0.71f,  0.0f,  0.5f,  // Left Top             

         // Triangle 33
         0.71f, -1.5f, -2.62f,-0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Back Bottom          
         0.71f, -0.5f, -2.62f,-0.71f,  0.0f, -0.71f,  0.0f,  0.25f, // Back Top             
         -1.41f, -0.5f, -0.5f,-0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Left Top             

         // Triangle 34
          0.71f,-1.5f, -2.62f,-0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Back Bottom          
         -1.41f, -0.5f, -0.5f,-0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Left Top             
         -1.41f, -1.5f, -0.5f,-0.71f,  0.0f, -0.71f,  0.25f, 0.5f,  // Left Bottom          

         // Triangle 35
         0.0f,  -1.5f,  0.91f,-0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Front Bottom         
         0.0f,  -0.5f,  0.91f,-0.71f,  0.0f,  0.71f,  0.25f, 0.5f,  // Front Top            
         -1.41f, -0.5f, -0.5f,-0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Left Top             

         // Triangle 36
         0.0f,  -1.5f,  0.91f,-0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Front Bottom         
         -1.41f, -0.5f, -0.5f,-0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Left Top             
         -1.41f, -1.5f, -0.5f,-0.71f,  0.0f,  0.71f,  0.0f,  0.25f, // Left Bottom          

         // Triangle 37
         2.12f, -1.5f, -1.21f,  0.0f, -1.0f,  0.0f,   0.25f, 0.25f, // Right Bottom           
         0.71f, -1.5f, -2.62f,  0.0f, -1.0f,  0.0f,   0.0f,  0.25f, // Back Bottom            
         -1.41f, -1.5f, -0.5f,  0.0f, -1.0f,  0.0f,   0.0f,  0.5f,  // Left Bottom            

         // Triangle 38
         2.12f, -1.5f, -1.21f,  0.0f, -1.0f,  0.0f,   0.25f, 0.25f, // Right Bottom          
         0.0f,  -1.5f,  0.91f,  0.0f, -1.0f,  0.0f,   0.25f, 0.5f,  // Front Bottom           
        -1.41f, -1.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f,  0.5f,  // Left Bottom     

         // Mailbox
         // Triangle 39
        -1.41f, -0.5f,  -0.15f, 0.71f,  0.0f,  0.71f,  0.25f, 0.5f,  // Right Top            
        -1.41f, -1.5f,  -0.15f, 0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Right Bottom         
        -2.47f,  -0.5f,  0.91f, 0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Front Top            

         // Triangle 40
        -1.41f, -1.5f,  -0.15f, 0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Right Bottom          
        -2.47f,  -1.5f, 0.91f,  0.71f,  0.0f,  0.71f,  0.0f,  0.25f, // Front Bottom         
        -2.47f,  -0.5f, 0.91f,  0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Front Top            

         // Triangle 41
        -1.41f, -0.5f, -0.15f,  0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Right Top             
        -1.41f, -1.5f, -0.15f,  0.71f,  0.0f, -0.71f,  0.0f,  0.25f, // Right Bottom          
        -2.83f, -1.5f, -1.56f,  0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Back Bottom          

         // Triangle 42
        -1.41f, -0.5f, -0.15f,  0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Right Top             
        -2.83f, -1.5f, -1.56f,  0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Back Bottom          
        -2.83f, -0.5f, -1.56f,  0.71f,  0.0f, -0.71f,  0.25f, 0.5f,  // Back Top          

         // Triangle 43
        -2.83f, -1.5f, -1.56f, -0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Back Bottom          
        -2.83f, -0.5f, -1.56f, -0.71f,  0.0f, -0.71f,  0.0f,  0.25f, // Back Top             
        -3.89f, -0.5f,  -0.5f, -0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Left Top             

         // Triangle 44
         -2.83f,-1.5f, -1.56f, -0.71f,  0.0f, -0.71f,  0.0f,  0.5f,  // Back Bottom          
         -3.89f, -0.5f, -0.5f, -0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Left Top             
         -3.89f, -1.5f, -0.5f, -0.71f,  0.0f, -0.71f,  0.25f, 0.5f,  // Left Bottom          

         // Triangle 45
        -2.47f,  -1.5f, 0.91f, -0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Front Bottom         
        -2.47f,  -0.5f, 0.91f, -0.71f,  0.0f,  0.71f,  0.25f, 0.5f,  // Front Top            
        -3.89f, -0.5f, -0.5f,  -0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Left Top             

         // Triangle 46
         -2.47f, -1.5f, 0.91f, -0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Front Bottom         
         -3.89f, -0.5f, -0.5f, -0.71f,  0.0f,  0.71f,  0.0f,  0.5f,  // Left Top             
         -3.89f, -1.5f, -0.5f, -0.71f,  0.0f,  0.71f,  0.0f,  0.25f, // Left Bottom          

         // Triangle 47
        -1.41f, -1.5f, -0.15f,  0.0f,  -1.0f,  0.0f,   0.25f, 0.25f, // Right Bottom           
        -2.83f, -1.5f, -1.56f,  0.0f,  -1.0f,  0.0f,   0.0f,  0.25f, // Back Bottom            
        -3.89f, -1.5f, -0.5f,   0.0f,  -1.0f,  0.0f,   0.0f,  0.5f,  // Left Bottom            

         // Triangle 48
        -1.41f, -1.5f, -0.15f,  0.0f,  -1.0f,  0.0f,   0.25f, 0.25f, // Right Bottom          
         -2.47f, -1.5f, 0.91f,  0.0f,  -1.0f,  0.0f,   0.25f, 0.5f,  // Front Bottom           
        -3.89f, -1.5f, -0.5f,   0.0f,  -1.0f,  0.0f,   0.0f,  0.5f,  // Left Bottom   

        // Triangle 49         
        -2.47f,  -0.5f, 0.91f,  0.71f,  0.0f,  0.71f,  0.0f,  0.25f, // Front Top 
        -1.41f, -0.5f, -0.15f,  0.71f,  0.0f,  0.71f,  0.25f, 0.25f, // Right Top  
        -1.94f,  0.25f, 0.38f,  0.71f,  0.0f,  0.71f,  0.25f, 0.5f,  // Front Crest

         // Triangle 50        
        -2.83f, -0.5f, -1.56f, -0.71f,  0.0f, -0.71f,  0.0f,  0.25f, // Back Top             
        -3.89f, -0.5f,  -0.5f, -0.71f,  0.0f, -0.71f,  0.25f, 0.25f, // Left Top 
        -3.36f, 0.25f, -1.03f, -0.71f,  0.0f, -0.71f,  0.25f, 0.5f,  // Back Crest

         // Triangle 51
        -1.94f,  0.25f,  0.38f, 0.71f,  0.5f, -0.71f,  0.25f, 0.25f, // Front Crest
        -1.41f, -0.5f,  -0.15f, 0.71f,  0.5f, -0.71f,  0.0f,  0.25f, // Right Top  
        -2.83f, -0.5f, -1.56f,  0.71f,  0.5f, -0.71f,  0.0f,  0.5f,  // Back Top   

         // Triangle 52
        -1.94f,  0.25f,  0.38f, 0.71f,  0.5f, -0.71f,  0.25f, 0.25f, // Front Crest
        -2.83f, -0.5f, -1.56f,  0.71f,  0.5f, -0.71f,  0.0f,  0.5f,  // Back Top   
        -3.36f, 0.25f, -1.03f,  0.71f,  0.5f, -0.71f,  0.25f, 0.5f,  // Back Crest

        // Triangle 53        
        -2.47f,  -0.5f, 0.91f,  -0.71f,  0.5f, 0.71f,  0.0f,  0.5f,  // Front Top 
        -1.94f,  0.25f,  0.38f, -0.71f,  0.5f, 0.71f,  0.25f, 0.25f, // Front Crest
        -3.36f, 0.25f, -1.03f,  -0.71f,  0.5f, 0.71f,  0.25f, 0.5f,  // Back Crest

        // Triangle 54        
        -2.47f,  -0.5f, 0.91f, -0.71f,  0.5f,  0.71f,  0.0f,  0.5f,  // Front Top 
        -3.36f, 0.25f, -1.03f, -0.71f,  0.5f,  0.71f,  0.25f, 0.5f,  // Back Crest           
        -3.89f, -0.5f,  -0.5f, -0.71f,  0.0f,  0.71f,  0.0f,  0.25f, // Left Top 

        // Rock
        // Triangle 55
         0.69f,  0.0f,  -1.0f,     0.0f, 0.4f, 1.0f,   0.5f,  0.25f, // Top ++z +y
         1.31f,  0.0f,  -1.0f,     0.0f, 0.4f, 1.0f,   0.25f, 0.25f,
         1.0f,   0.5f,  -1.19f,    0.0f, 0.4f, 1.0f,   0.25f, 0.5f,

        // Triangle 56
         0.69f,  0.0f,  -1.0f,    0.0f, -0.4f, 1.0f,   0.5f,  0.25f, // Bottom ++z -y
         1.31f,  0.0f,  -1.0f,    0.0f, -0.4f, 1.0f,   0.25f, 0.25f,
         1.0f,  -0.5f,  -1.19f,   0.0f, -0.4f, 1.0f,   0.25f, 0.5f,

        // Triangle 57
         0.69f,  0.0f,  -1.0f,  -0.71f, 0.6f, 0.71f,   0.5f,  0.25f, // Top +z -x ++y
         1.0f,   0.5f,  -1.19f, -0.71f, 0.6f, 0.71f,   0.25f, 0.25f,
         0.5f,   0.31f, -1.5f,  -0.71f, 0.6f, 0.71f,   0.25f, 0.5f,

        // Triangle 58
         0.69f,  0.0f,  -1.0f,   0.9f,  0.0f, 0.4f,    0.5f,  0.25f, // Side +z --x
         0.5f,   0.31f, -1.5f,  -0.9f,  0.0f, 0.4f,    0.25f, 0.25f,
         0.5f,  -0.31f, -1.5f,  -0.9f,  0.0f, 0.4f,    0.25f, 0.5f,

        // Triangle 58
         0.69f,  0.0f,  -1.0f,  -0.71f, -0.6f, 0.71f,  0.5f,  0.25f, // Bottom +z -x --y
         1.0f,  -0.5f,  -1.19f, -0.71f, -0.6f, 0.71f,  0.25f, 0.25f,
         0.5f,  -0.31f, -1.5f,  -0.71f, -0.6f, 0.71f,  0.25f, 0.5f,

        // Triangle 59
         1.31f,  0.0f,  -1.0f,   0.71f, 0.6f, 0.71f,   0.5f,  0.25f, // Top +z +x ++y
         1.0f,   0.5f,  -1.19f,  0.71f, 0.6f, 0.71f,   0.25f, 0.25f,
         1.5f,   0.31f, -1.5f,   0.71f, 0.6f, 0.71f,   0.25f, 0.5f,

        // Triangle 60
         1.31f,  0.0f,  -1.0f,   0.9f,  0.0f, 0.4f,    0.5f,  0.25f, // Side +z ++x
         1.5f,   0.31f, -1.5f,   0.9f,  0.0f, 0.4f,    0.25f, 0.25f,
         1.5f,  -0.31f, -1.5f,   0.9f,  0.0f, 0.4f,    0.25f, 0.5f,

        // Triangle 61
         1.31f,  0.0f,  -1.0f,  -0.71f, -0.6f, 0.71f,  0.5f,  0.25f, // Bottom +z +x --y
         1.0f,  -0.5f,  -1.19f, -0.71f, -0.6f, 0.71f,  0.25f, 0.25f,
         1.5f,  -0.31f, -1.5f,  -0.71f, -0.6f, 0.71f,  0.25f, 0.5f,

        // Triangle 62
         1.0f,  -0.5f,  -1.19f,  1.0f,  -1.0f, 0.0f,   0.5f,  0.25f, // Bottom ++x --y
         1.0f,  -0.5f,  -1.81f,  1.0f,  -1.0f, 0.0f,   0.25f, 0.25f,
         1.5f,  -0.31f, -1.5f,   1.0f,  -1.0f, 0.0f,   0.25f, 0.5f,

        // Triangle 63
         1.0f,  -0.5f,  -1.19f, -1.0f,  -1.0f, 1.0f,   0.5f,  0.25f, // Bottom  --x --y
         1.0f,  -0.5f,  -1.81f, -1.0f,  -1.0f, 1.0f,   0.25f, 0.25f,
         0.5f,  -0.31f, -1.5f,  -1.0f,  -1.0f, 1.0f,   0.25f, 0.5f,

        // Triangle 64
         0.69f,  0.0f,  -2.0f,  -0.4f, -0.6f, -0.9f,   0.5f,  0.25f, // Bottom --z -x -y
         1.0f,  -0.5f,  -1.81f, -0.4f, -0.6f, -0.9f,   0.25f, 0.25f,
         0.5f,  -0.31f, -1.5f,  -0.4f, -0.6f, -0.9f,   0.25f, 0.5f,

        // Triangle 65
         0.69f,  0.0f,  -2.0f,   0.0f, -0.4f, -1.0f,   0.5f,  0.25f, // Bottom --z -y
         1.31f,  0.0f,  -2.0f,   0.0f, -0.4f, -1.0f,   0.25f, 0.25f,
         1.0f,  -0.5f,  -1.81f,  0.0f, -0.4f, -1.0f,   0.25f, 0.5f,

        // Triangle 66
         1.31f,  0.0f,  -2.0f,   0.4f, -0.6f, -0.9f,   0.5f,  0.25f, // Bottom --z +x -y
         1.0f,  -0.5f,  -1.81f,  0.4f, -0.6f, -0.9f,   0.25f, 0.25f,
         1.5f,  -0.31f, -1.5f,   0.4f, -0.6f, -0.9f,   0.25f, 0.5f,

        // Triangle 67
         1.31f,  0.0f,  -2.0f,   0.9f,  0.0f, -0.4f,   0.5f,  0.25f, // Side -z ++x
         1.5f,   0.31f, -1.5f,   0.9f,  0.0f, -0.4f,   0.25f, 0.25f,
         1.5f,  -0.31f, -1.5f,   0.9f,  0.0f, -0.4f,   0.25f, 0.5f,

        // Triangle 68
         1.31f,  0.0f,  -2.0f,   0.71f, 0.6f, -0.71f,  0.5f,  0.25f, // Top -z +x ++y
         1.0f,   0.5f,  -1.81f,  0.71f, 0.6f, -0.71f,  0.25f, 0.25f,
         1.5f,   0.31f, -1.5f,   0.71f, 0.6f, -0.71f,  0.25f, 0.5f,

        // Triangle 69 *nice*
         0.69f,  0.0f,  -2.0f,   0.0f,  0.4f, -1.0f,   0.5f,  0.25f, // Top --z +y
         1.31f,  0.0f,  -2.0f,   0.0f,  0.4f, -1.0f,   0.25f, 0.25f,
         1.0f,   0.5f,  -1.81f,  0.0f,  0.4f, -1.0f,   0.25f, 0.5f,

        // Triangle 70
         0.69f,  0.0f,  -2.0f,  -0.71f, 0.4f, -0.71f,  0.5f,  0.25f, // Top -z -x +y
         1.0f,   0.5f,  -1.81f, -0.71f, 0.4f, -0.71f,  0.25f, 0.25f,
         0.5f,   0.31f, -1.5f,  -0.71f, 0.4f, -0.71f,  0.25f, 0.5f,

        // Triangle 71
         1.0f,   0.5f,  -1.19f, -1.0f,  1.0f, 0.0f,    0.5f,  0.25f, // Top --x ++y
         1.0f,   0.5f,  -1.81f, -1.0f,  1.0f, 0.0f,    0.25f, 0.25f,
         0.5f,   0.31f, -1.5f,  -1.0f,  1.0f, 0.0f,    0.25f, 0.5f,

        // Triangle 72
         1.0f,   0.5f,  -1.19f,  1.0f,  1.0f, 0.0f,    0.5f,  0.25f, // Top ++x ++y
         1.0f,   0.5f,  -1.81f,  1.0f,  1.0f, 0.0f,    0.25f, 0.25f,
         1.5f,   0.31f, -1.5f,   1.0f,  1.0f, 0.0f,    0.25f, 0.5f,

        // Triangle 73
         0.69f,  0.0f,  -2.0f,  -0.9f, -0.6f, -0.4f,   0.5f,  0.25f, // Bottom -z --x -y
         0.5f,   0.31f, -1.5f,  -0.9f, -0.6f, -0.4f,   0.25f, 0.25f,
         0.5f,  -0.31f, -1.5f,  -0.9f, -0.6f, -0.4f,   0.25f, 0.5f

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao);	// we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);						// Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);		// Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);	// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh &mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0);	// Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);	// links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);	// Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

