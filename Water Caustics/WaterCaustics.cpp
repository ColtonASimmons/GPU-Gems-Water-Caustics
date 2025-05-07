#include <GL/glew.h>
#include <GL/freeglut.h>

#include "cyCore.h"
#include "cyVector.h"
#include "cyMatrix.h"
#include "cyGL.h"

#include "lodepng.h"

//GLUT Functions
void Display();
void Idle();
void Keyboard(unsigned char key, int x, int y);
void MouseButton(int button, int state, int x, int y);
void MouseMotion(int x, int y);

//My Functions
void SetUpCaustics(char* groundTextureFile, char* lightMapTexture);
void SetUpWater();
void LoadMatrices();
void UpdateUniforms();

//Key Definitions
constexpr unsigned char EXIT_KEY = 27;
float windowHeight = 1080.0f;
float windowWidth = 1920.0f;

//Programs
cy::GLSLProgram causticsProgram;
cy::GLSLProgram waterProgram;

//Ground Variables
std::vector<cy::Vec3f> groundVertices;
std::vector<cy::Vec2f> groundTexCoords;
GLuint groundVAO;
std::vector<unsigned char> groundTextureData;
cy::GLTexture2D groundTexture;

//Light Map Variables
std::vector<unsigned char> lightMapTextureData;
cy::GLTexture2D lightMapTexture;
float lightDistance = 0.1f;

//Water Variables
std::vector<cy::Vec3f> waterVertices;
GLuint waterVAO;
cy::Vec4f waterColor = cy::Vec4f(0.0f, 0.0f, 1.0f, 0.7f);
GLuint waterVBO;

float waterHeight = 0.5f;
cy::Vec2f waveOrigin = cy::Vec2f(0.0f, 0.0f);

float VTXSIZE = 0.01f;
float WAVESIZE = 7.0f;
float FACTOR = 1.0f;
float SPEED = 2.0f;
float OCTAVES = 1.0f;

//Timing
float deltaTime = 0.0f;

//Camera Variables
float cameraDistance = 4.0f;
cy::Vec3f cameraPos = cy::Vec3f(cameraDistance, 2.0f, cameraDistance);
cy::Vec3f cameraTarget = cy::Vec3f(0.0f, 0.0f, 0.0f);
cy::Vec3f cameraUp = cy::Vec3f(0.0f, 1.0f, 0.0f);
float fov = 45.0f;
float aspect = windowWidth / windowHeight;
float zNear = 0.01f;
float zFar = 100.0f;

float cameraTheta = cy::Pi<float>() / 4.0f; 
float cameraPhi = cy::Pi<float>() / 4.0f;   
bool mouseLeftDown = false;
int lastMouseX, lastMouseY;


//Global Matrices
cy::Matrix4f viewMatrix;
cy::Matrix4f projectionMatrix;

//Macros
#define DEG_2_RAD(X) X * (cy::Pi<float>() / 180.0f) 

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    glutCreateWindow("Water Caustics");
    glutDisplayFunc(Display);
    glutIdleFunc(Idle);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);

    glewInit();
    glutInitContextFlags(GLUT_DEBUG);

    LoadMatrices();
    SetUpCaustics(argv[1], argv[2]);
    SetUpWater();

    //OpenGL initializations
    glClearColor(0, 0, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutMainLoop();
    return 0;
}

void Display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    causticsProgram.Bind();
    groundTexture.Bind(0);
    lightMapTexture.Bind(1);
    causticsProgram["ground"] = 0;
    causticsProgram["lightMap"] = 1;

    causticsProgram["time"] = deltaTime;
    causticsProgram["viewMatrix"] = viewMatrix;
    causticsProgram["projectionMatrix"] = projectionMatrix;

    glBindVertexArray(groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, groundVertices.size());

    waterProgram.Bind();
    waterProgram["time"] = deltaTime;
    waterProgram["viewMatrix"] = viewMatrix;
    waterProgram["projectionMatrix"] = projectionMatrix;

    glBindVertexArray(waterVAO);
    glDrawArrays(GL_PATCHES, 0, 4);

    glutSwapBuffers();
}

void Idle()
{
    deltaTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glutPostRedisplay();
}

void Keyboard(unsigned char key, int x, int y)
{
    if (key == EXIT_KEY)
    {
        glutLeaveMainLoop();
    }

    if (key == 'q')
    {
        waterHeight += 0.1f;
    }
    
    if (key == 'a')
    {
        waterHeight -= 0.1f;
    }

    if (key == 'w')
    {
        WAVESIZE += 1.0f;
    }

    if (key == 's')
    {
        WAVESIZE -= 1.0f;
    }

    if (key == 'e')
    {
        FACTOR += 0.1f;
    }

    if (key == 'd')
    {
        FACTOR -= 0.1f;
    }

    if (key == 'r')
    {
        SPEED += 0.1f;
    }

    if (key == 'f')
    {
        SPEED -= 0.1f;
    }

    if (key == 't')
    {
        OCTAVES += 1.0f;
    }

    if (key == 'g')
    {
        OCTAVES -= 1.0f;
    }

    if (key == 'y')
    {
        lightDistance += 0.1f;
    }

    if (key == 'h')
    {
        lightDistance -= 0.1f;
    }

    UpdateUniforms();
}

void MouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            mouseLeftDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else
        {
            mouseLeftDown = false;
        }
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        float ndcX = (2.0f * x) / windowWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * y) / windowHeight;

        cy::Vec4f rayStartNDC(ndcX, ndcY, -1.0f, 1.0f);
        cy::Vec4f rayEndNDC(ndcX, ndcY, 1.0f, 1.0f);

        cy::Matrix4f viewProj = projectionMatrix * viewMatrix;
        cy::Matrix4f invViewProj = viewProj.GetInverse();

        cy::Vec4f rayStartWorld = invViewProj * rayStartNDC;
        cy::Vec4f rayEndWorld = invViewProj * rayEndNDC;

        rayStartWorld /= rayStartWorld.w;
        rayEndWorld /= rayEndWorld.w;

        cy::Vec3f rayDir = (rayEndWorld - rayStartWorld).XYZ();
        rayDir.Normalize();

        cy::Vec3f origin = rayStartWorld.XYZ();
        float t = (waterHeight - origin.y) / rayDir.y;

        cy::Vec3f intersectPoint = origin + rayDir * t;

        waveOrigin = cy::Vec2f(intersectPoint.x, intersectPoint.z);

        waterProgram["waveOrigin"] = waveOrigin;
        causticsProgram["waveOrigin"] = waveOrigin;
    }

}

void MouseMotion(int x, int y)
{
    if (mouseLeftDown)
    {
        float sensitivity = 0.005f;
        float dx = float(x - lastMouseX);
        float dy = float(y - lastMouseY);

        cameraTheta += dx * sensitivity;
        cameraPhi -= dy * sensitivity;

        // Clamp phi to avoid gimbal lock
        const float epsilon = 0.01f;
        cameraPhi = std::max(epsilon, std::min(cy::Pi<float>() - epsilon, cameraPhi));

        lastMouseX = x;
        lastMouseY = y;

        // Recalculate camera position
        cameraPos = cy::Vec3f(
            cameraDistance * sinf(cameraPhi) * cosf(cameraTheta),
            cameraDistance * cosf(cameraPhi),
            cameraDistance * sinf(cameraPhi) * sinf(cameraTheta)
        );

        viewMatrix.SetView(cameraPos, cameraTarget, cameraUp);
    }
}

void SetUpCaustics(char* groundTextureFile, char* lightMapTextureFile)
{
    //Get ground texture from input
    unsigned gwidth, gheight;
    lodepng::decode(groundTextureData, gwidth, gheight, groundTextureFile);
    groundTexture.Initialize();
    groundTexture.SetImage(groundTextureData.data(), 4, gwidth, gheight);
    groundTexture.BuildMipmaps();
    groundTexture.Bind(0);

    //Get light map from input
    unsigned lmwidth, lmheight;
    lodepng::decode(lightMapTextureData, lmwidth, lmheight, lightMapTextureFile);
    lightMapTexture.Initialize();
    lightMapTexture.SetImage(lightMapTextureData.data(), 4, lmwidth, lmheight);
    lightMapTexture.BuildMipmaps();
    lightMapTexture.Bind(1);

    causticsProgram.BuildFiles("Caustics.vert", "Caustics.frag");

    //Vertex Shader Uniforms
    causticsProgram["viewMatrix"] = viewMatrix;
    causticsProgram["projectionMatrix"] = projectionMatrix;

    //Fragment Shader Uniforms
    causticsProgram["waveOrigin"] = waveOrigin;
    causticsProgram["waterHeight"] = waterHeight;
    causticsProgram["lightDistance"] = lightDistance;
    causticsProgram["VTXSIZE"] = VTXSIZE;
    causticsProgram["WAVESIZE"] = WAVESIZE;
    causticsProgram["FACTOR"] = FACTOR;
    causticsProgram["SPEED"] = SPEED;
    causticsProgram["OCTAVES"] = OCTAVES;

    //Vertices set up the naive way - 3 vectors for each plane
    groundVertices = {
        cy::Vec3f(-1.0f, 0.0f, -1.0f),
        cy::Vec3f(1.0f, 0.0f, -1.0f),
        cy::Vec3f(1.0f, 0.0f,  1.0f),

        cy::Vec3f(-1.0f, 0.0f, -1.0f),
        cy::Vec3f(1.0f, 0.0f,  1.0f),
        cy::Vec3f(-1.0f, 0.0f,  1.0f)
    };

    groundTexCoords = {
        cy::Vec2f(0.0f, 0.0f),
        cy::Vec2f(1.0f, 0.0f),
        cy::Vec2f(1.0f, 1.0f),

        cy::Vec2f(0.0f, 0.0f),
        cy::Vec2f(1.0f, 1.0f),
        cy::Vec2f(0.0f, 1.0f)
    };

    glGenVertexArrays(1, &groundVAO);
    glBindVertexArray(groundVAO);

    GLuint groundVBO;
    glGenBuffers(1, &groundVBO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cy::Vec3f) * groundVertices.size(),
        groundVertices.data(),
        GL_STATIC_DRAW
    );

    GLuint groundPos = glGetAttribLocation(causticsProgram.GetID(), "pos");
    glEnableVertexAttribArray(groundPos);
    glVertexAttribPointer(
        groundPos, 3, GL_FLOAT,
        GL_FALSE, 0, (GLvoid*)0
    );

    GLuint groundTBO;
    glGenBuffers(1, &groundTBO);
    glBindBuffer(GL_ARRAY_BUFFER, groundTBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cy::Vec2f) * groundTexCoords.size(),
        groundTexCoords.data(),
        GL_STATIC_DRAW
    );

    GLuint groundTxc = glGetAttribLocation(causticsProgram.GetID(), "txc");
    glEnableVertexAttribArray(groundTxc);
    glVertexAttribPointer(
        groundTxc, 2, GL_FLOAT,
        GL_FALSE, 0, (GLvoid*)0
    );
}

void SetUpWater()
{
    waterProgram.BuildFiles("Water.vert", "Water.frag", nullptr, "Water.tesc", "Water.tese");

    //Tessellation Control Shader Uniforms
    waterProgram["tessellationLevel"] = 64.0f;

    //Tessellation Evaluation Shader Uniforms
    waterProgram["viewMatrix"] = viewMatrix;
    waterProgram["projectionMatrix"] = projectionMatrix;

    waterProgram["waterHeight"] = waterHeight;
    waterProgram["waveOrigin"] = waveOrigin;

    waterProgram["VTXSIZE"] = VTXSIZE;
    waterProgram["WAVESIZE"] = WAVESIZE;
    waterProgram["FACTOR"] = FACTOR;
    waterProgram["SPEED"] = SPEED;
    waterProgram["OCTAVES"] = OCTAVES;

    //Fragment Shader Uniforms
    waterProgram["waterColor"] = waterColor;

    //Vertices set up for tessellation
    waterVertices = {
        cy::Vec3f(-1.0f, 0.0f, -1.0f),
        cy::Vec3f(1.0f, 0.0f, -1.0f),
        cy::Vec3f(1.0f, 0.0f,  1.0f),
        cy::Vec3f(-1.0f, 0.0f,  1.0f)
    };

    glGenVertexArrays(1, &waterVAO);
    glBindVertexArray(waterVAO);

    GLuint waterVBO;
    glGenBuffers(1, &waterVBO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cy::Vec3f) * waterVertices.size(),
        waterVertices.data(),
        GL_STATIC_DRAW
    );

    GLuint waterPos = glGetAttribLocation(waterProgram.GetID(), "pos");
    glEnableVertexAttribArray(waterPos);
    glVertexAttribPointer(
        waterPos, 3, GL_FLOAT,
        GL_FALSE, 0, (GLvoid*)0
    );

    glPatchParameteri(GL_PATCH_VERTICES, 4);
}

void LoadMatrices()
{
    //View matrix from camera variables
    viewMatrix.SetView(cameraPos, cameraTarget, cameraUp);

    //Projection Matrix
    projectionMatrix.SetPerspective(DEG_2_RAD(fov), aspect, zNear, zFar);
}

void UpdateUniforms()
{
    waterProgram["waterHeight"] = waterHeight;
    waterProgram["WAVESIZE"] = WAVESIZE;
    waterProgram["FACTOR"] = FACTOR;
    waterProgram["SPEED"] = SPEED;
    waterProgram["OCTAVES"] = OCTAVES;
    waterProgram["waveOrigin"] = waveOrigin;

    causticsProgram["waterHeight"] = waterHeight;
    causticsProgram["WAVESIZE"] = WAVESIZE;
    causticsProgram["FACTOR"] = FACTOR;
    causticsProgram["SPEED"] = SPEED;
    causticsProgram["OCTAVES"] = OCTAVES;
    causticsProgram["waveOrigin"] = waveOrigin;
    causticsProgram["lightDistance"] = lightDistance;
}


