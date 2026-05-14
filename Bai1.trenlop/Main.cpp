#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
#include <algorithm>
#include <string>
#include <vector>

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_LIGHT_MODEL_COLOR_CONTROL
#define GL_LIGHT_MODEL_COLOR_CONTROL 0x81F8
#endif
#ifndef GL_SEPARATE_SPECULAR_COLOR
#define GL_SEPARATE_SPECULAR_COLOR 0x81FA
#endif
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif
#ifndef GL_RGBA16F_ARB
#define GL_RGBA16F_ARB 0x881A
#endif

float camX = 0.0f, camY = 1.62f, camZ = 10.90f;
float yaw = -90.0f, pitch = 9.0f;
float lookX, lookY, lookZ;
float speed = 3.0f;

int lastMouseX, lastMouseY;
bool isMousePressed = false;
bool keyStates[256] = { false };
bool specialKeyStates[512] = { false };
double lastFrameTime = 0.0;
GLFWwindow* window = 0;
unsigned int floorTex, woodTex;

typedef void (APIENTRY* StoreGenFramebuffersProc)(GLsizei, GLuint*);
typedef void (APIENTRY* StoreBindFramebufferProc)(GLenum, GLuint);
typedef void (APIENTRY* StoreDeleteFramebuffersProc)(GLsizei, const GLuint*);
typedef GLenum(APIENTRY* StoreCheckFramebufferStatusProc)(GLenum);
typedef void (APIENTRY* StoreFramebufferTexture2DProc)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY* StoreGenRenderbuffersProc)(GLsizei, GLuint*);
typedef void (APIENTRY* StoreBindRenderbufferProc)(GLenum, GLuint);
typedef void (APIENTRY* StoreDeleteRenderbuffersProc)(GLsizei, const GLuint*);
typedef void (APIENTRY* StoreRenderbufferStorageProc)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY* StoreFramebufferRenderbufferProc)(GLenum, GLenum, GLenum, GLuint);
typedef void (APIENTRY* StoreGenerateMipmapProc)(GLenum);

StoreGenFramebuffersProc storeGenFramebuffers = 0;
StoreBindFramebufferProc storeBindFramebuffer = 0;
StoreDeleteFramebuffersProc storeDeleteFramebuffers = 0;
StoreCheckFramebufferStatusProc storeCheckFramebufferStatus = 0;
StoreFramebufferTexture2DProc storeFramebufferTexture2D = 0;
StoreGenRenderbuffersProc storeGenRenderbuffers = 0;
StoreBindRenderbufferProc storeBindRenderbuffer = 0;
StoreDeleteRenderbuffersProc storeDeleteRenderbuffers = 0;
StoreRenderbufferStorageProc storeRenderbufferStorage = 0;
StoreFramebufferRenderbufferProc storeFramebufferRenderbuffer = 0;
StoreGenerateMipmapProc storeGenerateMipmap = 0;

bool canUseFramebufferPresent = false;
bool canUseAnisotropy = false;
bool canUseFloatPresentColor = false;
float maxAnisotropy = 1.0f;

GLuint presentFBO = 0;
GLuint presentColorTex = 0;
GLuint presentDepthBuffer = 0;
GLuint presentProgram = 0;
GLint presentSceneTexLoc = -1;
GLint presentExposureLoc = -1;
GLint presentInvGammaLoc = -1;
bool presentPathReady = false;
int presentWidth = 1;
int presentHeight = 1;

const float PRESENT_EXPOSURE = 1.08f;
const float PRESENT_INV_GAMMA = 1.0f / 2.22f;

struct MeshVertex {
    float px, py, pz;
    float nx, ny, nz;
};

struct MeshChunk {
    std::string name;
    std::vector<MeshVertex> vertices;
};

std::vector<MeshChunk> mannequinMesh;
bool mannequinMeshLoaded = false;
std::vector<MeshChunk> shoeMesh;
bool shoeMeshLoaded = false;

const float STORE_HALF_W = 5.95f;
const float STORE_FRONT_Z = 5.92f;
const float STORE_BACK_Z = -5.70f;
const float CEILING_Y = 4.28f;

void setMat(float r, float g, float b, float specStrength = 0.14f, float shine = 12.0f);

template <typename T>
bool loadGLProc(T& proc, const char* primaryName, const char* fallbackName = 0) {
    proc = reinterpret_cast<T>(glfwGetProcAddress(primaryName));
    if (!proc && fallbackName) {
        proc = reinterpret_cast<T>(glfwGetProcAddress(fallbackName));
    }
    return proc != 0;
}

void destroyPresentResources();
bool resizePresentTargets(int width, int height);

void initOptionalGLFeatures() {
    loadGLProc(storeGenerateMipmap, "glGenerateMipmap", "glGenerateMipmapEXT");

    canUseFramebufferPresent =
        loadGLProc(storeGenFramebuffers, "glGenFramebuffers", "glGenFramebuffersEXT") &&
        loadGLProc(storeBindFramebuffer, "glBindFramebuffer", "glBindFramebufferEXT") &&
        loadGLProc(storeDeleteFramebuffers, "glDeleteFramebuffers", "glDeleteFramebuffersEXT") &&
        loadGLProc(storeCheckFramebufferStatus, "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT") &&
        loadGLProc(storeFramebufferTexture2D, "glFramebufferTexture2D", "glFramebufferTexture2DEXT") &&
        loadGLProc(storeGenRenderbuffers, "glGenRenderbuffers", "glGenRenderbuffersEXT") &&
        loadGLProc(storeBindRenderbuffer, "glBindRenderbuffer", "glBindRenderbufferEXT") &&
        loadGLProc(storeDeleteRenderbuffers, "glDeleteRenderbuffers", "glDeleteRenderbuffersEXT") &&
        loadGLProc(storeRenderbufferStorage, "glRenderbufferStorage", "glRenderbufferStorageEXT") &&
        loadGLProc(storeFramebufferRenderbuffer, "glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT");

    if (glfwExtensionSupported("GL_EXT_texture_filter_anisotropic") == GLFW_TRUE) {
        canUseAnisotropy = true;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        if (maxAnisotropy < 1.0f) {
            maxAnisotropy = 1.0f;
        }
    }

    canUseFloatPresentColor =
        glfwExtensionSupported("GL_ARB_texture_float") == GLFW_TRUE ||
        glfwExtensionSupported("GL_ATI_texture_float") == GLFW_TRUE ||
        glfwExtensionSupported("GL_EXT_color_buffer_half_float") == GLFW_TRUE;
}

GLuint compilePresentShader(GLenum shaderType, const char* source) {
    GLuint shader = glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }

    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool buildPresentProgram() {
    const char* vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.st;\n"
        "}\n";

    const char* fragmentSource =
        "#version 120\n"
        "uniform sampler2D uSceneTex;\n"
        "uniform float uExposure;\n"
        "uniform float uInvGamma;\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    vec3 color = texture2D(uSceneTex, vTexCoord).rgb * uExposure;\n"
        "    // Use a filmic shoulder so warm fixtures keep detail instead of clipping into flat poster whites.\n"
        "    color = max(color - vec3(0.004), vec3(0.0));\n"
        "    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);\n"
        "    color = pow(max(color, vec3(0.0)), vec3(uInvGamma));\n"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}\n";

    GLuint vertexShader = compilePresentShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compilePresentShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }

    presentProgram = glCreateProgram();
    if (!presentProgram) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glAttachShader(presentProgram, vertexShader);
    glAttachShader(presentProgram, fragmentShader);
    glLinkProgram(presentProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(presentProgram, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(presentProgram);
        presentProgram = 0;
        return false;
    }

    presentSceneTexLoc = glGetUniformLocation(presentProgram, "uSceneTex");
    presentExposureLoc = glGetUniformLocation(presentProgram, "uExposure");
    presentInvGammaLoc = glGetUniformLocation(presentProgram, "uInvGamma");

    glUseProgram(presentProgram);
    if (presentSceneTexLoc >= 0) {
        glUniform1i(presentSceneTexLoc, 0);
    }
    glUseProgram(0);
    return true;
}

GLuint sceneProgram = 0;

bool buildSceneProgram() {
    const char* vs =
        "#version 120\n"
        "varying vec3 vNormal;\n"
        "varying vec3 vFragPos;\n"
        "varying vec4 vColor;\n"
        "varying vec3 vViewPos;\n"
        "void main() {\n"
        "    vNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
        "    vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;\n"
        "    vFragPos = vec3(viewPos);\n"
        "    vViewPos = -vec3(viewPos);\n"
        "    vColor = gl_Color;\n"
        "    gl_Position = ftransform();\n"
        "}\n";

    const char* fs =
        "#version 120\n"
        "varying vec3 vNormal;\n"
        "varying vec3 vFragPos;\n"
        "varying vec4 vColor;\n"
        "varying vec3 vViewPos;\n"
        "void main() {\n"
        "    vec3 norm = normalize(vNormal);\n"
        "    vec3 viewDir = normalize(vViewPos);\n"
        "    vec3 result = gl_FrontMaterial.ambient.rgb * vColor.rgb;\n"
        "    for(int i = 0; i < 8; i++) {\n"
        "        vec3 lightDir = normalize(gl_LightSource[i].position.xyz - vFragPos);\n"
        "        float theta = dot(lightDir, normalize(-gl_LightSource[i].spotDirection));\n"
        "        if (theta > gl_LightSource[i].spotCosCutoff) {\n"
        "            float diff = max(dot(norm, lightDir), 0.0);\n"
        "            vec3 diffuse = diff * gl_LightSource[i].diffuse.rgb * vColor.rgb;\n"
        "            vec3 halfwayDir = normalize(lightDir + viewDir);\n"
        "            float spec = pow(max(dot(norm, halfwayDir), 0.0), gl_FrontMaterial.shininess);\n"
        "            // Fake a tiny bit of roughness based on shininess for PBR feel\n"
        "            float roughness = 1.0 - clamp(gl_FrontMaterial.shininess / 128.0, 0.0, 1.0);\n"
        "            vec3 specular = spec * gl_LightSource[i].specular.rgb * gl_FrontMaterial.specular.rgb;\n"
        "            float distance = length(gl_LightSource[i].position.xyz - vFragPos);\n"
        "            float att = 1.0 / (gl_LightSource[i].constantAttenuation + gl_LightSource[i].linearAttenuation * distance + gl_LightSource[i].quadraticAttenuation * distance * distance);\n"
        "            float epsilon = 0.05;\n"
        "            float intensity = smoothstep(gl_LightSource[i].spotCosCutoff, gl_LightSource[i].spotCosCutoff + epsilon, theta);\n"
        "            result += (diffuse + specular) * att * intensity * pow(max(theta, 0.0), gl_LightSource[i].spotExponent);\n"
        "        }\n"
        "    }\n"
        "    gl_FragColor = vec4(result, vColor.a);\n"
        "}\n";

    GLuint vertexShader = compilePresentShader(GL_VERTEX_SHADER, vs);
    GLuint fragmentShader = compilePresentShader(GL_FRAGMENT_SHADER, fs);
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }

    sceneProgram = glCreateProgram();
    if (!sceneProgram) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glAttachShader(sceneProgram, vertexShader);
    glAttachShader(sceneProgram, fragmentShader);
    glLinkProgram(sceneProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(sceneProgram, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(sceneProgram);
        sceneProgram = 0;
        return false;
    }
    return true;
}

void destroyPresentResources() {
    if (sceneProgram) {
        glDeleteProgram(sceneProgram);
        sceneProgram = 0;
    }
    if (presentDepthBuffer && storeDeleteRenderbuffers) {
        storeDeleteRenderbuffers(1, &presentDepthBuffer);
    }
    if (presentColorTex) {
        glDeleteTextures(1, &presentColorTex);
    }
    if (presentFBO && storeDeleteFramebuffers) {
        storeDeleteFramebuffers(1, &presentFBO);
    }
    if (presentProgram) {
        glDeleteProgram(presentProgram);
    }

    presentDepthBuffer = 0;
    presentColorTex = 0;
    presentFBO = 0;
    presentProgram = 0;
    presentSceneTexLoc = -1;
    presentExposureLoc = -1;
    presentInvGammaLoc = -1;
    presentPathReady = false;
}

bool resizePresentTargets(int width, int height) {
    if (!canUseFramebufferPresent) {
        return false;
    }

    width = std::max(width, 1);
    height = std::max(height, 1);
    if (presentFBO && presentColorTex && presentDepthBuffer && width == presentWidth && height == presentHeight) {
        return true;
    }

    presentWidth = width;
    presentHeight = height;

    if (!presentFBO) {
        storeGenFramebuffers(1, &presentFBO);
    }
    if (!presentColorTex) {
        glGenTextures(1, &presentColorTex);
    }
    if (!presentDepthBuffer) {
        storeGenRenderbuffers(1, &presentDepthBuffer);
    }

    glBindTexture(GL_TEXTURE_2D, presentColorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    storeBindRenderbuffer(GL_RENDERBUFFER, presentDepthBuffer);
    storeRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, presentWidth, presentHeight);

    storeBindFramebuffer(GL_FRAMEBUFFER, presentFBO);
    GLenum internalFormat = canUseFloatPresentColor ? GL_RGBA16F_ARB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, presentWidth, presentHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    storeFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, presentColorTex, 0);
    storeFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, presentDepthBuffer);

    GLenum status = storeCheckFramebufferStatus(GL_FRAMEBUFFER);
    // Fall back to 8-bit color if the driver exposes texture float but cannot render to it in this legacy path.
    if (status != GL_FRAMEBUFFER_COMPLETE && internalFormat != GL_RGBA) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, presentWidth, presentHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        storeFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, presentColorTex, 0);
        status = storeCheckFramebufferStatus(GL_FRAMEBUFFER);
    }

    storeBindFramebuffer(GL_FRAMEBUFFER, 0);
    storeBindRenderbuffer(GL_RENDERBUFFER, 0);

    return status == GL_FRAMEBUFFER_COMPLETE;
}

bool initPresentPath(int width, int height) {
    if (!canUseFramebufferPresent) {
        return false;
    }

    destroyPresentResources();
    if (!buildPresentProgram()) {
        return false;
    }
    if (!resizePresentTargets(width, height)) {
        destroyPresentResources();
        return false;
    }

    presentPathReady = true;
    return true;
}

void drawLayeredSignText(const char* text,
                         float centerX, float y, float z, float scale,
                         float r, float g, float b,
                         float glowAlpha,
                         float shadowOffsetX, float shadowOffsetY);

void drawPresentPass() {
    // Restore the legacy fixed-function state after the fullscreen pass so the next scene frame starts unchanged.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_VIEWPORT_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(presentProgram);
    if (presentExposureLoc >= 0) {
        glUniform1f(presentExposureLoc, PRESENT_EXPOSURE);
    }
    if (presentInvGammaLoc >= 0) {
        glUniform1f(presentInvGammaLoc, PRESENT_INV_GAMMA);
    }

    glBindTexture(GL_TEXTURE_2D, presentColorTex);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glUseProgram(0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void drawStorefrontHeaderOverlay() {
    glUseProgram(0);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_VIEWPORT_BIT | GL_LINE_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    glColor4f(0.010f, 0.010f, 0.010f, 0.98f);
    glVertex2f(-1.0f, 0.665f);
    glVertex2f( 1.0f, 0.665f);
    glColor4f(0.040f, 0.037f, 0.032f, 0.98f);
    glVertex2f( 1.0f, 0.985f);
    glVertex2f(-1.0f, 0.985f);

    glColor4f(0.0f, 0.0f, 0.0f, 0.96f);
    glVertex2f(-0.975f, 0.705f);
    glVertex2f( 0.975f, 0.705f);
    glVertex2f( 0.975f, 0.945f);
    glVertex2f(-0.975f, 0.945f);

    glColor4f(1.0f, 0.78f, 0.48f, 0.45f);
    glVertex2f(-0.970f, 0.938f);
    glVertex2f( 0.970f, 0.938f);
    glColor4f(1.0f, 0.86f, 0.62f, 0.08f);
    glVertex2f( 0.970f, 0.888f);
    glVertex2f(-0.970f, 0.888f);

    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    glVertex2f(-1.0f, 0.642f);
    glVertex2f( 1.0f, 0.642f);
    glVertex2f( 1.0f, 0.682f);
    glVertex2f(-1.0f, 0.682f);
    glEnd();

    glLineWidth(1.4f);
    glBegin(GL_LINES);
    glColor4f(0.55f, 0.48f, 0.38f, 0.55f);
    glVertex2f(-0.98f, 0.942f); glVertex2f(0.98f, 0.942f);
    glColor4f(0.10f, 0.085f, 0.065f, 0.9f);
    glVertex2f(-0.98f, 0.704f); glVertex2f(0.98f, 0.704f);
    glEnd();

    drawLayeredSignText("ICON MODE", 0.0f, 0.815f, 0.0f, 0.00235f,
                        1.0f, 0.98f, 0.92f, 0.46f, 0.006f, 0.008f);
    drawLayeredSignText("MEN'S WEAR", 0.0f, 0.735f, 0.0f, 0.00105f,
                        0.92f, 0.90f, 0.86f, 0.30f, 0.004f, 0.006f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void renderScene();

float toRad(float deg) {
    return deg * 0.017453292519943295f;
}

void setMatByName(const std::string& name) {
    if (name == "Boots_Cube.004") {
        setMat(0.91f, 0.90f, 0.86f, 0.14f, 22.0f);
    } else if (name == "WhiteShirt_Cube.003") {
        setMat(0.94f, 0.92f, 0.88f, 0.016f, 6.0f);
    } else if (name == "BluePants_Cube.001") {
        setMat(0.10f, 0.12f, 0.17f, 0.022f, 8.0f);
    } else if (name == "Body_Cube") {
        setMat(0.84f, 0.82f, 0.78f, 0.016f, 7.0f);
    } else {
        setMat(0.73f, 0.72f, 0.70f, 0.028f, 8.0f);
    }
}

bool loadMesh(const char* path, std::vector<MeshChunk>& meshChunks) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config) || !reader.Valid()) {
        meshChunks.clear();
        return false;
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();

    std::vector<MeshChunk> loadedChunks;
    loadedChunks.reserve(shapes.size());

    for (size_t s = 0; s < shapes.size(); ++s) {
        const tinyobj::shape_t& shape = shapes[s];
        MeshChunk chunk;
        chunk.name = shape.name;
        chunk.vertices.reserve(shape.mesh.indices.size());

        bool hasMissingNormal = false;
        for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
            const tinyobj::index_t& idx = shape.mesh.indices[i];
            if (idx.vertex_index < 0 || idx.normal_index < 0 ||
                (3 * idx.vertex_index + 2) >= static_cast<int>(attrib.vertices.size()) ||
                (3 * idx.normal_index + 2) >= static_cast<int>(attrib.normals.size())) {
                hasMissingNormal = true;
                break;
            }

            MeshVertex vertex;
            vertex.px = attrib.vertices[3 * idx.vertex_index + 0];
            vertex.py = attrib.vertices[3 * idx.vertex_index + 1];
            vertex.pz = attrib.vertices[3 * idx.vertex_index + 2];
            vertex.nx = attrib.normals[3 * idx.normal_index + 0];
            vertex.ny = attrib.normals[3 * idx.normal_index + 1];
            vertex.nz = attrib.normals[3 * idx.normal_index + 2];
            chunk.vertices.push_back(vertex);
        }

        if (hasMissingNormal) {
            meshChunks.clear();
            return false;
        }

        if (!chunk.vertices.empty()) {
            loadedChunks.push_back(chunk);
        }
    }

    meshChunks.swap(loadedChunks);
    return !meshChunks.empty();
}

bool loadMannequinMesh(const char* path) {
    mannequinMeshLoaded = loadMesh(path, mannequinMesh);
    return mannequinMeshLoaded;
}

bool loadShoeMesh(const char* path) {
    shoeMeshLoaded = loadMesh(path, shoeMesh);
    return shoeMeshLoaded;
}

void drawLoadedMannequinMesh() {
    for (size_t c = 0; c < mannequinMesh.size(); ++c) {
        const MeshChunk& chunk = mannequinMesh[c];
        setMatByName(chunk.name);
        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < chunk.vertices.size(); ++i) {
            const MeshVertex& vertex = chunk.vertices[i];
            glNormal3f(vertex.nx, vertex.ny, vertex.nz);
            glVertex3f(vertex.px, vertex.py, vertex.pz);
        }
        glEnd();
    }
}

void drawLoadedShoeMesh(float r, float g, float b) {
    for (size_t c = 0; c < shoeMesh.size(); ++c) {
        const MeshChunk& chunk = shoeMesh[c];
        if (chunk.name == "Boots_Cube.004") {
            setMat(std::min(r * 1.04f, 1.0f), std::min(g * 1.04f, 1.0f), std::min(b * 1.02f, 1.0f), 0.17f, 24.0f);
        } else {
            setMat(0.92f, 0.90f, 0.86f, 0.06f, 8.0f);
        }
        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < chunk.vertices.size(); ++i) {
            const MeshVertex& vertex = chunk.vertices[i];
            glNormal3f(vertex.nx, vertex.ny, vertex.nz);
            glVertex3f(vertex.px, vertex.py, vertex.pz);
        }
        glEnd();
    }
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    bool hasImage = (data != 0);

    glBindTexture(GL_TEXTURE_2D, textureID);
    if (!storeGenerateMipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }
    if (hasImage) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        unsigned char fallback[3] = { 196, 192, 186 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
    }

    if (storeGenerateMipmap) {
        storeGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (canUseAnisotropy) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(6.0f, maxAnisotropy));
    }
    return textureID;
}

void updateCamera() {
    lookX = camX + cos(toRad(yaw)) * cos(toRad(pitch));
    lookY = camY + sin(toRad(pitch));
    lookZ = camZ + sin(toRad(yaw)) * cos(toRad(pitch));
}

void normalize3(float& x, float& y, float& z) {
    float len = sqrt(x * x + y * y + z * z);
    if (len > 0.00001f) {
        x /= len;
        y /= len;
        z /= len;
    }
}

void cross3(float ax, float ay, float az,
            float bx, float by, float bz,
            float& rx, float& ry, float& rz) {
    rx = ay * bz - az * by;
    ry = az * bx - ax * bz;
    rz = ax * by - ay * bx;
}

void applyLookAt(float eyeX, float eyeY, float eyeZ,
                 float centerX, float centerY, float centerZ,
                 float upX, float upY, float upZ) {
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;
    normalize3(fx, fy, fz);

    normalize3(upX, upY, upZ);

    float sx, sy, sz;
    cross3(fx, fy, fz, upX, upY, upZ, sx, sy, sz);
    normalize3(sx, sy, sz);

    float ux, uy, uz;
    cross3(sx, sy, sz, fx, fy, fz, ux, uy, uz);

    GLfloat matrix[16] = {
         sx,  ux, -fx, 0.0f,
         sy,  uy, -fy, 0.0f,
         sz,  uz, -fz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    glMultMatrixf(matrix);
    glTranslatef(-eyeX, -eyeY, -eyeZ);
}

void applyPerspective(float fovyDeg, float aspect, float zNear, float zFar) {
    float top = tan(toRad(fovyDeg) * 0.5f) * zNear;
    float bottom = -top;
    float right = top * aspect;
    float left = -right;
    glFrustum(left, right, bottom, top, zNear, zFar);
}

void setMat(float r, float g, float b, float specStrength, float shine) {
    glColor3f(r, g, b);
    GLfloat ambient[] = { r * 0.18f, g * 0.17f, b * 0.16f, 1.0f };
    GLfloat diffuse[] = { r * 0.98f, g * 0.98f, b * 0.97f, 1.0f };
    GLfloat spec[] = { specStrength * 0.74f, specStrength * 0.76f, specStrength * 0.80f, 1.0f };
    float tunedShine = std::min(shine * 1.08f + 2.0f, 72.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, tunedShine);
}

void drawBox(float sx, float sy, float sz) {
    glPushMatrix();
    glScalef(sx, sy, sz);
    float h = 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-h, -h,  h); glVertex3f( h, -h,  h); glVertex3f( h,  h,  h); glVertex3f(-h,  h,  h);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f( h, -h, -h); glVertex3f(-h, -h, -h); glVertex3f(-h,  h, -h); glVertex3f( h,  h, -h);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f( h, -h,  h); glVertex3f( h, -h, -h); glVertex3f( h,  h, -h); glVertex3f( h,  h,  h);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-h, -h, -h); glVertex3f(-h, -h,  h); glVertex3f(-h,  h,  h); glVertex3f(-h,  h, -h);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-h,  h,  h); glVertex3f( h,  h,  h); glVertex3f( h,  h, -h); glVertex3f(-h,  h, -h);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-h, -h, -h); glVertex3f( h, -h, -h); glVertex3f( h, -h,  h); glVertex3f(-h, -h,  h);
    glEnd();
    glPopMatrix();
}

void drawRoundedBox(float sx, float sy, float sz, float radius = 0.0f) {
    if (radius <= 0.001f) { drawBox(sx, sy, sz); return; }
    float hx = sx * 0.5f - radius;
    float hy = sy * 0.5f - radius;
    float hz = sz * 0.5f - radius;
    if (hx < 0.0f) hx = 0.0f;
    if (hy < 0.0f) hy = 0.0f;
    if (hz < 0.0f) hz = 0.0f;
    int seg = 8;
    glPushMatrix();
    // 6 faces (rounded quad strips)
    for (int face = 0; face < 6; ++face) {
        glPushMatrix();
        float nx = 0, ny = 0, nz = 0;
        if (face == 0) { nz = 1; glTranslatef(0, 0, hz); }
        else if (face == 1) { nz = -1; glTranslatef(0, 0, -hz); }
        else if (face == 2) { nx = 1; glTranslatef(hx, 0, 0); glRotatef(90, 0, 1, 0); }
        else if (face == 3) { nx = -1; glTranslatef(-hx, 0, 0); glRotatef(-90, 0, 1, 0); }
        else if (face == 4) { ny = 1; glTranslatef(0, hy, 0); glRotatef(-90, 1, 0, 0); }
        else { ny = -1; glTranslatef(0, -hy, 0); glRotatef(90, 1, 0, 0); }
        // flat center face
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        float fx = (face >= 2 && face <= 3) ? hz : hx;
        float fy = (face >= 4) ? hz : hy;
        glVertex3f(-fx, -fy, radius); glVertex3f(fx, -fy, radius);
        glVertex3f(fx, fy, radius); glVertex3f(-fx, fy, radius);
        glEnd();
        // top/bottom edge strips
        for (int edge = 0; edge < 4; ++edge) {
            float edgeX = (edge == 0 || edge == 3) ? -fx : fx;
            float dirX = (edge == 0 || edge == 3) ? -1.0f : 1.0f;
            float edgeY = (edge < 2) ? -fy : fy;
            float dirY = (edge < 2) ? -1.0f : 1.0f;
            bool horiz = (edge == 0 || edge == 2);
            glBegin(GL_QUAD_STRIP);
            float len = horiz ? 2.0f * fx : 2.0f * fy;
            int steps = (int)(len / radius * 2.0f);
            if (steps < 2) steps = 2;
            if (horiz) {
                for (int i = 0; i <= seg; ++i) {
                    float a = 1.5708f * i / seg;
                    float cz = cos(a) * radius;
                    float cy = sin(a) * radius * dirY;
                    glNormal3f(0, sin(a) * dirY, cos(a));
                    glVertex3f(-fx, edgeY + cy, cz);
                    glVertex3f(fx, edgeY + cy, cz);
                }
            } else {
                for (int i = 0; i <= seg; ++i) {
                    float a = 1.5708f * i / seg;
                    float cz = cos(a) * radius;
                    float cx = sin(a) * radius * dirX;
                    glNormal3f(sin(a) * dirX, 0, cos(a));
                    glVertex3f(edgeX + cx, -fy, cz);
                    glVertex3f(edgeX + cx, fy, cz);
                }
            }
            glEnd();
        }
        glPopMatrix();
    }
    // 8 corner spheres
    float cx[] = {-hx, hx, -hx, hx, -hx, hx, -hx, hx};
    float cy[] = {-hy, -hy, hy, hy, -hy, -hy, hy, hy};
    float cz[] = {hz, hz, hz, hz, -hz, -hz, -hz, -hz};
    for (int c = 0; c < 8; ++c) {
        glPushMatrix();
        glTranslatef(cx[c], cy[c], cz[c]);
        int ss = seg;
        for (int st = 0; st < ss; ++st) {
            float p0 = -1.5708f + 3.14159f * st / ss;
            float p1 = -1.5708f + 3.14159f * (st + 1) / ss;
            glBegin(GL_QUAD_STRIP);
            for (int sl = 0; sl <= ss; ++sl) {
                float th = 6.28318f * sl / ss;
                float ct = cos(th), st2 = sin(th);
                float x0 = cos(p0) * ct, y0 = sin(p0), z0 = cos(p0) * st2;
                float x1 = cos(p1) * ct, y1 = sin(p1), z1 = cos(p1) * st2;
                glNormal3f(x0, y0, z0); glVertex3f(x0 * radius, y0 * radius, z0 * radius);
                glNormal3f(x1, y1, z1); glVertex3f(x1 * radius, y1 * radius, z1 * radius);
            }
            glEnd();
        }
        glPopMatrix();
    }
    glPopMatrix();
}

void drawScaledSphere(float sx, float sy, float sz, int slices = 48, int stacks = 36) {
    glPushMatrix();
    glScalef(sx, sy, sz);
    for (int stack = 0; stack < stacks; ++stack) {
        float phi0 = -1.57079632679f + 3.14159265359f * stack / stacks;
        float phi1 = -1.57079632679f + 3.14159265359f * (stack + 1) / stacks;
        glBegin(GL_QUAD_STRIP);
        for (int slice = 0; slice <= slices; ++slice) {
            float theta = 6.28318530718f * slice / slices;
            float c = cos(theta);
            float s = sin(theta);
            float x0 = cos(phi0) * c;
            float y0 = sin(phi0);
            float z0 = cos(phi0) * s;
            float x1 = cos(phi1) * c;
            float y1 = sin(phi1);
            float z1 = cos(phi1) * s;
            glNormal3f(x0, y0, z0); glVertex3f(x0, y0, z0);
            glNormal3f(x1, y1, z1); glVertex3f(x1, y1, z1);
        }
        glEnd();
    }
    glPopMatrix();
}

void drawCylinderY(float radiusTop, float radiusBottom, float height, int slices = 40) {
    float slope = (radiusTop - radiusBottom) / height;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float angle = 6.28318530718f * i / slices;
        float x = cos(angle);
        float z = sin(angle);
        glNormal3f(x, slope, z);
        glVertex3f(x * radiusTop, 0.0f, z * radiusTop);
        glVertex3f(x * radiusBottom, height, z * radiusBottom);
    }
    glEnd();
}

void drawDiskY(float radius, float ySign) {
    int slices = 48;
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, ySign >= 0.0f ? 1.0f : -1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= slices; ++i) {
        int index = (ySign >= 0.0f) ? i : (slices - i);
        float angle = 6.28318530718f * index / slices;
        glVertex3f(cos(angle) * radius, 0.0f, sin(angle) * radius);
    }
    glEnd();
}

float textGlyphWidth(char c) {
    if (c == ' ') return 42.0f;
    if (c == 'I' || c == '\'') return 28.0f;
    return 72.0f;
}

void glyphLine(float x0, float y0, float x1, float y1) {
    glVertex3f(x0, y0, 0.0f);
    glVertex3f(x1, y1, 0.0f);
}

void drawGlyph(char c, float x) {
    switch (c) {
    case 'A':
        glyphLine(x + 4, 0, x + 36, 100); glyphLine(x + 68, 0, x + 36, 100); glyphLine(x + 18, 42, x + 54, 42); break;
    case 'C':
        glyphLine(x + 66, 92, x + 14, 92); glyphLine(x + 14, 92, x + 8, 50); glyphLine(x + 8, 50, x + 14, 8); glyphLine(x + 14, 8, x + 66, 8); break;
    case 'D':
        glyphLine(x + 10, 0, x + 10, 100); glyphLine(x + 10, 96, x + 54, 86); glyphLine(x + 54, 86, x + 66, 50); glyphLine(x + 66, 50, x + 54, 14); glyphLine(x + 54, 14, x + 10, 4); break;
    case 'E':
        glyphLine(x + 10, 0, x + 10, 100); glyphLine(x + 10, 96, x + 66, 96); glyphLine(x + 10, 50, x + 58, 50); glyphLine(x + 10, 4, x + 66, 4); break;
    case 'I':
        glyphLine(x + 4, 96, x + 28, 96); glyphLine(x + 16, 96, x + 16, 4); glyphLine(x + 4, 4, x + 28, 4); break;
    case 'M':
        glyphLine(x + 8, 0, x + 8, 100); glyphLine(x + 8, 100, x + 36, 46); glyphLine(x + 36, 46, x + 64, 100); glyphLine(x + 64, 100, x + 64, 0); break;
    case 'N':
        glyphLine(x + 8, 0, x + 8, 100); glyphLine(x + 8, 100, x + 64, 0); glyphLine(x + 64, 0, x + 64, 100); break;
    case 'O':
        glyphLine(x + 14, 6, x + 58, 6); glyphLine(x + 58, 6, x + 66, 50); glyphLine(x + 66, 50, x + 58, 94); glyphLine(x + 58, 94, x + 14, 94); glyphLine(x + 14, 94, x + 6, 50); glyphLine(x + 6, 50, x + 14, 6); break;
    case 'R':
        glyphLine(x + 10, 0, x + 10, 100); glyphLine(x + 10, 96, x + 58, 92); glyphLine(x + 58, 92, x + 64, 64); glyphLine(x + 64, 64, x + 10, 54); glyphLine(x + 28, 54, x + 68, 0); break;
    case 'S':
        glyphLine(x + 66, 92, x + 14, 92); glyphLine(x + 14, 92, x + 10, 54); glyphLine(x + 10, 54, x + 60, 48); glyphLine(x + 60, 48, x + 66, 8); glyphLine(x + 66, 8, x + 10, 8); break;
    case 'W':
        glyphLine(x + 4, 100, x + 18, 0); glyphLine(x + 18, 0, x + 36, 46); glyphLine(x + 36, 46, x + 54, 0); glyphLine(x + 54, 0, x + 68, 100); break;
    case '\'':
        glyphLine(x + 12, 100, x + 8, 70); break;
    default:
        glyphLine(x + 8, 0, x + 8, 100); glyphLine(x + 8, 100, x + 64, 100); glyphLine(x + 64, 100, x + 64, 0); glyphLine(x + 64, 0, x + 8, 0); break;
    }
}

void drawText(const char* text, float x, float y, float z, float scale) {
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    glLineWidth(2.2f);
    glBegin(GL_LINES);
    float cursor = 0.0f;
    for (const char* c = text; *c != '\0'; ++c) {
        if (*c != ' ') {
            drawGlyph(*c, cursor);
        }
        cursor += textGlyphWidth(*c) + 16.0f;
    }
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawCenteredText(const char* text, float centerX, float y, float z, float scale) {
    float width = 0.0f;
    for (const char* c = text; *c != '\0'; ++c) {
        width += textGlyphWidth(*c) + 16.0f;
    }
    drawText(text, centerX - width * scale * 0.5f, y, z, scale);
}

void drawWarmStrip(float sx, float sy, float sz) {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.86f, 0.62f);
    drawBox(sx, sy, sz);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glColor4f(1.0f, 0.82f, 0.58f, 0.085f);
    drawBox(sx * 1.06f, sy * 2.20f, sz * 1.06f);
    glColor4f(1.0f, 0.92f, 0.80f, 0.035f);
    drawBox(sx * 0.94f, sy * 3.10f, sz * 0.94f);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    glEnable(GL_LIGHTING);
}

void drawTexturedQuadXZ(float width, float depth, float tileX, float tileZ, unsigned int textureID) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-width * 0.5f, 0.0f, -depth * 0.5f);
    glTexCoord2f(tileX, 0.0f); glVertex3f( width * 0.5f, 0.0f, -depth * 0.5f);
    glTexCoord2f(tileX, tileZ); glVertex3f( width * 0.5f, 0.0f,  depth * 0.5f);
    glTexCoord2f(0.0f, tileZ); glVertex3f(-width * 0.5f, 0.0f,  depth * 0.5f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawTexturedQuadXY(float width, float height, float tileX, float tileY, unsigned int textureID) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-width * 0.5f, -height * 0.5f, 0.0f);
    glTexCoord2f(tileX, 0.0f); glVertex3f( width * 0.5f, -height * 0.5f, 0.0f);
    glTexCoord2f(tileX, tileY); glVertex3f( width * 0.5f,  height * 0.5f, 0.0f);
    glTexCoord2f(0.0f, tileY); glVertex3f(-width * 0.5f,  height * 0.5f, 0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawGlassPanel(float width, float height, float r, float g, float b, float alpha = 0.18f) {
    // Draw the glass after opaque framing so the fixed pipeline keeps transparency artifacts manageable.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(r, g, b, alpha);

    GLfloat spec[] = { 0.52f, 0.52f, 0.52f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 92.0f);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-width * 0.5f, -height * 0.5f, 0.0f);
    glVertex3f( width * 0.5f, -height * 0.5f, 0.0f);
    glVertex3f( width * 0.5f,  height * 0.5f, 0.0f);
    glVertex3f(-width * 0.5f,  height * 0.5f, 0.0f);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-width * 0.5f,  height * 0.5f, -0.002f);
    glVertex3f( width * 0.5f,  height * 0.5f, -0.002f);
    glVertex3f( width * 0.5f, -height * 0.5f, -0.002f);
    glVertex3f(-width * 0.5f, -height * 0.5f, -0.002f);
    glEnd();

    // Stack a cool streak and a warmer body tint so the pane reads like layered glazing instead of a flat alpha card.
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, alpha * 0.36f);
    glVertex3f(-width * 0.12f, -height * 0.44f, 0.001f);
    glVertex3f( width * 0.04f, -height * 0.44f, 0.001f);
    glColor4f(1.0f, 1.0f, 1.0f, alpha * 0.06f);
    glVertex3f( width * 0.26f,  height * 0.44f, 0.001f);
    glVertex3f( width * 0.08f,  height * 0.44f, 0.001f);

    glColor4f(r * 1.08f, g * 1.04f, b * 1.02f, alpha * 0.08f);
    glVertex3f(-width * 0.34f, -height * 0.30f, 0.0005f);
    glVertex3f( width * 0.34f, -height * 0.30f, 0.0005f);
    glColor4f(r * 0.88f, g * 0.92f, b * 1.02f, alpha * 0.22f);
    glVertex3f( width * 0.24f,  height * 0.08f, 0.0005f);
    glVertex3f(-width * 0.24f,  height * 0.08f, 0.0005f);
    glEnd();
    glEnable(GL_LIGHTING);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawSoftEllipseDecal(float x, float y, float z,
                         float radiusX, float radiusZ,
                         float r, float g, float b,
                         float centerAlpha, float edgeAlpha = 0.0f) {
    const int slices = 40;
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(radiusX, 1.0f, radiusZ);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, centerAlpha);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= slices; ++i) {
        float angle = 6.28318530718f * i / slices;
        glColor4f(r, g, b, edgeAlpha);
        glVertex3f(cos(angle), 0.0f, sin(angle));
    }
    glEnd();
    glPopMatrix();
}

void drawReflectionPool(float x, float z, float sx, float sz, float intensity) {
    drawSoftEllipseDecal(x, 0.038f, z, sx * 1.30f, sz * 1.18f, 0.16f, 0.17f, 0.18f, intensity * 0.18f, 0.0f);
    drawSoftEllipseDecal(x, 0.039f, z + sz * 0.08f, sx * 0.74f, sz * 0.54f, 0.90f, 0.80f, 0.64f, intensity * 0.10f, 0.0f);
    drawSoftEllipseDecal(x, 0.040f, z - sz * 0.02f, sx * 0.42f, sz * 0.28f, 1.0f, 0.98f, 0.92f, intensity * 0.05f, 0.0f);
}

void drawLayeredSignText(const char* text,
                         float centerX, float y, float z, float scale,
                         float r, float g, float b,
                         float glowAlpha,
                         float shadowOffsetX, float shadowOffsetY) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(r * 0.94f, g * 0.89f, b * 0.82f, glowAlpha * 0.34f);
    drawCenteredText(text, centerX, y - 0.008f, z - 0.010f, scale * 1.05f);
    glColor4f(1.0f, 0.98f, 0.92f, glowAlpha * 0.62f);
    drawCenteredText(text, centerX, y, z - 0.004f, scale * 1.015f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.18f, 0.17f, 0.16f, 0.68f);
    drawCenteredText(text, centerX + shadowOffsetX, y - shadowOffsetY, z - 0.014f, scale);
    glColor4f(r, g, b, 1.0f);
    drawCenteredText(text, centerX, y, z, scale);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawFloorGlow(float radius, float r, float g, float b) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawSoftEllipseDecal(0.0f, 0.0f, 0.0f, radius * 1.12f, radius * 0.88f, r * 0.62f, g * 0.58f, b * 0.54f, 0.028f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.0f, 0.0f, radius * 0.80f, radius * 0.66f, r * 0.84f, g * 0.80f, b * 0.72f, 0.035f, 0.0f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawSoftEllipseDecal(0.0f, 0.0f, 0.0f, radius * 0.62f, radius * 0.50f, r, g, b, 0.032f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.001f, 0.0f, radius * 0.28f, radius * 0.19f, 1.0f, 0.95f, 0.82f, 0.022f, 0.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawSoftShadow(float x, float z, float sx, float sz, float alpha) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    drawSoftEllipseDecal(x, 0.040f, z, sx * 1.18f, sz * 1.02f, 0.08f, 0.07f, 0.06f, alpha * 0.32f, 0.0f);
    drawSoftEllipseDecal(x, 0.043f, z, sx * 0.76f, sz * 0.68f, 0.0f, 0.0f, 0.0f, alpha * 1.30f, 0.0f);
    drawSoftEllipseDecal(x + sx * 0.04f, 0.045f, z + sz * 0.08f, sx * 0.42f, sz * 0.34f, 0.0f, 0.0f, 0.0f, alpha * 0.46f, 0.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawContactShadows() {
    drawSoftShadow(0.00f,  3.05f, 1.64f, 0.68f, 0.21f);
    drawSoftShadow(0.00f,  3.05f, 0.88f, 0.28f, 0.10f);
    drawSoftShadow(1.88f,  1.05f, 1.38f, 0.62f, 0.17f);
    drawSoftShadow(1.88f,  1.05f, 0.78f, 0.26f, 0.09f);
    drawSoftShadow(0.00f, -0.62f, 1.48f, 0.64f, 0.18f);
    drawSoftShadow(0.00f, -0.62f, 0.84f, 0.26f, 0.09f);
    drawSoftShadow(-0.68f, -1.95f, 0.52f, 0.40f, 0.16f);
    drawSoftShadow( 0.68f, -1.95f, 0.52f, 0.40f, 0.16f);
    drawSoftShadow(0.00f, -4.18f, 1.76f, 0.54f, 0.23f);
    drawSoftShadow(0.00f, -4.08f, 1.04f, 0.19f, 0.10f);
    drawSoftShadow(-4.35f, 5.48f, 0.42f, 0.30f, 0.24f);
    drawSoftShadow( 4.35f, 5.48f, 0.42f, 0.30f, 0.24f);
    drawSoftShadow(-4.35f, 5.58f, 0.16f, 0.09f, 0.11f);
    drawSoftShadow( 4.35f, 5.58f, 0.16f, 0.09f, 0.11f);
}

void drawHeroFloorPass() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // Use bounded reflection pools instead of a full mirror so the polished floor stays believable in this fixed-function path.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawSoftEllipseDecal(0.0f, 0.036f, 0.05f, 1.18f, 5.10f, 0.15f, 0.16f, 0.17f, 0.048f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.036f, 0.05f, 0.66f, 4.40f, 0.30f, 0.26f, 0.22f, 0.032f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.037f, 5.22f, 2.52f, 0.44f, 0.92f, 0.84f, 0.70f, 0.048f, 0.0f);
    drawSoftEllipseDecal(-3.56f, 0.038f, 5.24f, 0.20f, 0.86f, 1.0f, 0.88f, 0.72f, 0.036f, 0.0f);
    drawSoftEllipseDecal( 3.56f, 0.038f, 5.24f, 0.20f, 0.86f, 1.0f, 0.88f, 0.72f, 0.036f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.037f, -4.70f, 1.34f, 0.32f, 0.88f, 0.82f, 0.70f, 0.032f, 0.0f);

    drawReflectionPool(0.00f,  3.05f, 0.96f, 0.44f, 0.18f);
    drawReflectionPool(1.88f,  1.05f, 0.88f, 0.40f, 0.17f);
    drawReflectionPool(0.00f, -0.62f, 0.94f, 0.40f, 0.16f);
    drawReflectionPool(0.00f, -4.18f, 1.26f, 0.28f, 0.20f);
    drawReflectionPool(-2.24f, -4.35f, 0.32f, 0.16f, 0.12f);
    drawReflectionPool( 2.24f, -4.35f, 0.32f, 0.16f, 0.12f);
    drawReflectionPool(-4.35f,  5.48f, 0.22f, 0.14f, 0.15f);
    drawReflectionPool( 4.35f,  5.48f, 0.22f, 0.14f, 0.15f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawSoftEllipseDecal(0.0f, 0.039f, 5.28f, 1.42f, 0.16f, 1.0f, 0.95f, 0.84f, 0.022f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.039f, -4.74f, 0.80f, 0.12f, 1.0f, 0.95f, 0.84f, 0.014f, 0.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawVolumetricLightCone(float x, float z, float radius, float alpha) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glColor4f(1.0f, 0.88f, 0.62f, alpha * 1.4f);
    glPushMatrix();
    glTranslatef(x, 0.08f, z);
    drawCylinderY(radius, 0.035f, CEILING_Y - 0.35f, 36);
    glPopMatrix();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawStoreAtmosphere() {
    drawVolumetricLightCone(-3.25f,  3.35f, 0.92f, 0.018f);
    drawVolumetricLightCone( 3.25f,  3.35f, 0.92f, 0.018f);
    drawVolumetricLightCone(-3.25f,  0.65f, 0.86f, 0.015f);
    drawVolumetricLightCone( 3.25f,  0.65f, 0.86f, 0.015f);
    drawVolumetricLightCone(-3.25f, -2.15f, 0.88f, 0.015f);
    drawVolumetricLightCone( 3.25f, -2.15f, 0.88f, 0.015f);
    drawVolumetricLightCone( 0.00f,  2.05f, 0.98f, 0.017f);
    drawVolumetricLightCone( 0.00f, -3.45f, 1.02f, 0.017f);
}

void drawFoldedPants(float r, float g, float b) {
    glPushMatrix();

    setMat(r * 0.98f, g * 0.98f, b, 0.045f, 6.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.017f, 0.0f); drawRoundedBox(0.228f, 0.030f, 0.178f, 0.008f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.043f, -0.015f); drawRoundedBox(0.194f, 0.014f, 0.132f, 0.006f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.020f, 0.020f); drawScaledSphere(0.120f, 0.014f, 0.088f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.018f, 0.082f); drawScaledSphere(0.210f, 0.014f, 0.028f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.017f, -0.086f); drawScaledSphere(0.195f, 0.011f, 0.026f); glPopMatrix();
    // Soft edge wrapping at front and back
    glPushMatrix(); glTranslatef(0.0f, 0.026f, 0.090f); drawScaledSphere(0.180f, 0.008f, 0.016f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.026f, -0.088f); drawScaledSphere(0.170f, 0.007f, 0.014f); glPopMatrix();

    setMat(r * 0.80f, g * 0.80f, b * 0.82f, 0.035f, 5.0f);
    glPushMatrix(); glTranslatef(-0.054f, 0.038f, 0.024f); glRotatef(8.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.054f, 0.016f, 0.114f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.054f, 0.038f, 0.024f); glRotatef(-8.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.054f, 0.016f, 0.114f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.053f, -0.036f); drawScaledSphere(0.050f, 0.012f, 0.044f); glPopMatrix();

    // Layer a darker inset under the upper fold so the waist break reads as cloth depth instead of paint.
    setMat(r * 0.62f, g * 0.62f, b * 0.66f, 0.028f, 4.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.039f, -0.012f); drawScaledSphere(0.156f, 0.008f, 0.076f); glPopMatrix();

    setMat(r * 0.70f, g * 0.70f, b * 0.74f, 0.036f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.050f, 0.010f); drawScaledSphere(0.008f, 0.008f, 0.068f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.055f, -0.040f); drawScaledSphere(0.090f, 0.005f, 0.006f); glPopMatrix();

    setMat(std::min(r * 1.05f, 1.0f), std::min(g * 1.05f, 1.0f), std::min(b * 1.04f, 1.0f), 0.025f, 4.0f);
    glPushMatrix(); glTranslatef(-0.056f, 0.044f, 0.046f); glRotatef(18.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.024f, 0.009f, 0.054f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.056f, 0.044f, 0.046f); glRotatef(-18.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.024f, 0.009f, 0.054f); glPopMatrix();

    glPopMatrix();
}

void drawFoldedPantsStack(float r, float g, float b, int layers) {
    for (int i = 0; i < layers; ++i) {
        float tint = 1.0f - i * 0.040f;
        glPushMatrix();
        glTranslatef(0.0f, i * 0.026f, 0.0f);
        drawFoldedPants(r * tint, g * tint, b * tint);
        glPopMatrix();
    }
}

void drawShoppingBag(float r, float g, float b) {
    glPushMatrix();

    setMat(r, g, b, 0.09f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.11f, 0.0f); drawRoundedBox(0.170f, 0.220f, 0.085f, 0.006f); glPopMatrix();
    setMat(r * 0.86f, g * 0.86f, b * 0.86f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.205f, 0.0f); drawRoundedBox(0.155f, 0.020f, 0.070f, 0.004f); glPopMatrix();

    setMat(std::min(r * 1.05f, 1.0f), std::min(g * 1.05f, 1.0f), std::min(b * 1.03f, 1.0f), 0.05f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.11f, 0.0435f); drawBox(0.112f, 0.124f, 0.004f); glPopMatrix();
    setMat(r * 0.72f, g * 0.72f, b * 0.72f, 0.05f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.020f, 0.0f); drawBox(0.118f, 0.010f, 0.050f); glPopMatrix();
    setMat(0.80f, 0.78f, 0.74f, 0.16f, 18.0f);
    glPushMatrix(); glTranslatef(-0.058f, 0.215f, 0.0f); glRotatef(90.0f, 0.0f, 0.0f, 1.0f); drawCylinderY(0.004f, 0.004f, 0.056f, 10); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.058f, 0.215f, 0.0f); glRotatef(90.0f, 0.0f, 0.0f, 1.0f); drawCylinderY(0.004f, 0.004f, 0.056f, 10); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(0.75f, 0.74f, 0.71f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.050f, 0.218f, 0.044f); glVertex3f(-0.032f, 0.268f, 0.044f); glVertex3f(0.032f, 0.268f, 0.044f); glVertex3f(0.050f, 0.218f, 0.044f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.050f, 0.218f, -0.044f); glVertex3f(-0.032f, 0.268f, -0.044f); glVertex3f(0.032f, 0.268f, -0.044f); glVertex3f(0.050f, 0.218f, -0.044f);
    glEnd();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawAccessoryTray(float r, float g, float b) {
    glPushMatrix();

    setMat(0.11f, 0.10f, 0.09f, 0.06f, 6.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.012f, 0.0f); drawRoundedBox(0.320f, 0.024f, 0.220f, 0.006f); glPopMatrix();
    setMat(r * 0.92f, g * 0.92f, b * 0.92f, 0.10f, 11.0f);
    glPushMatrix(); glTranslatef(-0.074f, 0.034f, -0.010f); drawBox(0.115f, 0.020f, 0.168f); glPopMatrix();
    setMat(std::min(r * 1.08f, 1.0f), std::min(g * 1.07f, 1.0f), std::min(b * 1.05f, 1.0f), 0.12f, 14.0f);
    glPushMatrix(); glTranslatef(0.040f, 0.038f, 0.052f); glRotatef(14.0f, 0.0f, 1.0f, 0.0f); drawBox(0.076f, 0.028f, 0.090f); glPopMatrix();
    setMat(0.18f, 0.16f, 0.15f, 0.10f, 9.0f);
    glPushMatrix(); glTranslatef(0.124f, 0.024f, -0.026f); drawBox(0.068f, 0.014f, 0.072f); glPopMatrix();
    setMat(0.76f, 0.70f, 0.54f, 0.18f, 22.0f);
    glPushMatrix(); glTranslatef(0.112f, 0.048f, -0.024f); glRotatef(90.0f, 0.0f, 0.0f, 1.0f); drawCylinderY(0.010f, 0.010f, 0.064f, 14); glPopMatrix();

    setMat(0.58f, 0.58f, 0.56f, 0.18f, 24.0f);
    glPushMatrix(); glTranslatef(0.070f, 0.042f, -0.050f); glRotatef(90.0f, 0.0f, 0.0f, 1.0f); drawCylinderY(0.020f, 0.020f, 0.100f, 18); glPopMatrix();
    glPushMatrix(); glTranslatef(0.070f, 0.042f, -0.050f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); drawDiskY(0.020f, 1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.170f, 0.042f, -0.050f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); drawDiskY(0.020f, -1.0f); glPopMatrix();

    glPopMatrix();
}

void setupSpotlight(GLenum lightID,
                    float px, float py, float pz,
                    float dx, float dy, float dz,
                    float cutoff, float exponent) {
    glEnable(lightID);
    GLfloat pos[] = { px, py, pz, 1.0f };
    GLfloat dir[] = { dx, dy, dz };
    GLfloat diff[] = { 1.28f, 1.14f, 0.90f, 1.0f };
    GLfloat spec[] = { 0.52f, 0.48f, 0.40f, 1.0f };
    GLfloat amb[]  = { 0.00f, 0.00f, 0.00f, 1.0f };

    glLightfv(lightID, GL_POSITION, pos);
    glLightfv(lightID, GL_SPOT_DIRECTION, dir);
    glLightfv(lightID, GL_DIFFUSE, diff);
    glLightfv(lightID, GL_SPECULAR, spec);
    glLightfv(lightID, GL_AMBIENT, amb);
    glLightf(lightID, GL_SPOT_CUTOFF, cutoff);
    glLightf(lightID, GL_SPOT_EXPONENT, exponent);
    glLightf(lightID, GL_CONSTANT_ATTENUATION, 0.72f);
    glLightf(lightID, GL_LINEAR_ATTENUATION, 0.055f);
    glLightf(lightID, GL_QUADRATIC_ATTENUATION, 0.0105f);
}

void setupLightingRig() {
    GLfloat ambient[] = { 0.18f, 0.165f, 0.145f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    setupSpotlight(GL_LIGHT0, -4.10f, 4.16f,  3.60f,  0.62f, -1.0f, -0.20f, 31.0f, 18.0f);
    setupSpotlight(GL_LIGHT1,  4.10f, 4.16f,  3.60f, -0.62f, -1.0f, -0.20f, 31.0f, 18.0f);
    setupSpotlight(GL_LIGHT2, -3.10f, 4.16f,  1.15f,  0.48f, -1.0f,  0.00f, 31.0f, 18.0f);
    setupSpotlight(GL_LIGHT3,  3.10f, 4.16f,  1.15f, -0.48f, -1.0f,  0.00f, 31.0f, 18.0f);
    setupSpotlight(GL_LIGHT4, -3.10f, 4.16f, -1.45f,  0.48f, -1.0f,  0.12f, 31.0f, 18.0f);
    setupSpotlight(GL_LIGHT5,  3.10f, 4.16f, -1.45f, -0.48f, -1.0f,  0.12f, 31.0f, 18.0f);
    setupSpotlight(GL_LIGHT6,  0.00f, 4.16f,  2.30f,  0.00f, -1.0f, -0.05f, 34.0f, 14.0f);
    setupSpotlight(GL_LIGHT7,  0.00f, 4.16f, -3.65f,  0.00f, -1.0f,  0.08f, 36.0f, 16.0f);
}

void drawShoe(float r, float g, float b) {
    if (shoeMeshLoaded) {
        glPushMatrix();
        // Keep the imported shoe centered near the shelf surface so the repeated store placements stay unchanged.
        glTranslatef(0.0f, -0.005f, 0.060f);
        glScalef(0.82f, 0.36f, 1.10f);
        drawLoadedShoeMesh(r, g, b);
        // Layer a local pool under the shoe so the mesh keeps readable floor contact in the legacy spotlight rig.
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        drawSoftEllipseDecal(0.0f, -0.010f, 0.020f, 0.155f, 0.290f, 0.0f, 0.0f, 0.0f, 0.16f, 0.0f);
        drawSoftEllipseDecal(0.014f, -0.009f, 0.056f, 0.092f, 0.136f, 1.0f, 0.95f, 0.82f, 0.035f, 0.0f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glPopMatrix();
        return;
    }

    glPushMatrix();
    glScalef(1.08f, 1.08f, 1.08f);

    setMat(0.88f, 0.86f, 0.82f, 0.14f, 12.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.012f, 0.008f); drawRoundedBox(0.175f, 0.036f, 0.324f, 0.015f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.020f, 0.145f); drawScaledSphere(0.086f, 0.032f, 0.088f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.018f, -0.126f); drawScaledSphere(0.078f, 0.028f, 0.062f); glPopMatrix();

    setMat(0.12f, 0.12f, 0.11f, 0.05f, 6.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.028f, 0.020f); drawRoundedBox(0.152f, 0.010f, 0.250f, 0.005f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.024f, 0.144f); drawScaledSphere(0.071f, 0.010f, 0.070f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.026f, -0.102f); drawScaledSphere(0.064f, 0.012f, 0.044f); glPopMatrix();

    setMat(std::min(r * 1.02f, 1.0f), std::min(g * 1.02f, 1.0f), std::min(b * 1.00f, 1.0f), 0.20f, 24.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.060f, 0.018f); drawScaledSphere(0.118f, 0.068f, 0.174f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.054f, 0.138f); drawScaledSphere(0.102f, 0.046f, 0.082f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.078f, -0.102f); drawScaledSphere(0.088f, 0.082f, 0.056f); glPopMatrix();

    setMat(r * 0.62f, g * 0.62f, b * 0.64f, 0.16f, 15.0f);
    glPushMatrix(); glTranslatef(-0.062f, 0.062f, 0.034f); glRotatef(16.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.020f, 0.018f, 0.108f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.062f, 0.062f, 0.034f); glRotatef(-16.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.020f, 0.018f, 0.108f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.095f, 0.014f); glRotatef(-18.0f, 1.0f, 0.0f, 0.0f); drawRoundedBox(0.066f, 0.016f, 0.120f, 0.008f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.092f, -0.070f); drawRoundedBox(0.084f, 0.028f, 0.058f, 0.012f); glPopMatrix();

    // Tuck a darker inset under the upper so the shoe silhouette stays legible from shelf distance.
    setMat(r * 0.44f, g * 0.44f, b * 0.46f, 0.05f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.046f, 0.012f); drawScaledSphere(0.090f, 0.014f, 0.150f); glPopMatrix();

    setMat(0.78f, 0.78f, 0.75f, 0.12f, 12.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.034f, 0.160f); drawScaledSphere(0.090f, 0.010f, 0.020f, 18, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.038f, -0.134f); drawScaledSphere(0.066f, 0.010f, 0.016f, 18, 10); glPopMatrix();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawSoftEllipseDecal(0.0f, 0.006f, 0.030f, 0.162f, 0.282f, 0.0f, 0.0f, 0.0f, 0.15f, 0.0f);
    drawSoftEllipseDecal(0.014f, 0.007f, 0.060f, 0.090f, 0.126f, 1.0f, 0.94f, 0.80f, 0.030f, 0.0f);
    glColor3f(0.94f, 0.94f, 0.92f);
    glLineWidth(1.3f);
    glBegin(GL_LINES);
    glVertex3f(-0.040f, 0.114f, 0.056f); glVertex3f(0.040f, 0.114f, 0.020f);
    glVertex3f(-0.040f, 0.114f, 0.020f); glVertex3f(0.040f, 0.114f, 0.056f);
    glVertex3f(-0.038f, 0.117f, -0.020f); glVertex3f(0.038f, 0.117f, 0.018f);
    glVertex3f(-0.038f, 0.117f, 0.018f); glVertex3f(0.038f, 0.117f, -0.020f);
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawShoePair(float r, float g, float b) {
    // Nudge each pair off perfect symmetry so the merchandising reads staged instead of duplicated.
    glPushMatrix(); glTranslatef(-0.066f, 0.002f, -0.014f); glRotatef(-12.0f, 0.0f, 1.0f, 0.0f); drawShoe(r * 0.98f, g * 0.98f, b * 0.99f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.060f, 0.0f,  0.024f); glRotatef( 16.0f, 0.0f, 1.0f, 0.0f); drawShoe(std::min(r * 1.03f, 1.0f), std::min(g * 1.03f, 1.0f), std::min(b * 1.01f, 1.0f)); glPopMatrix();
}

void drawFoldedShirt(float r, float g, float b) {
    glPushMatrix();

    setMat(r * 0.99f, g * 0.99f, b, 0.045f, 6.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.016f, 0.000f); drawRoundedBox(0.332f, 0.028f, 0.248f, 0.010f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.040f, -0.010f); drawRoundedBox(0.276f, 0.015f, 0.204f, 0.007f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.144f, 0.018f, 0.000f); drawScaledSphere(0.044f, 0.015f, 0.160f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.144f, 0.018f, 0.000f); drawScaledSphere(0.044f, 0.015f, 0.160f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.018f, 0.116f); drawScaledSphere(0.290f, 0.017f, 0.032f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.016f, -0.112f); drawScaledSphere(0.275f, 0.013f, 0.028f); glPopMatrix();
    // Extra soft edge volumes
    glPushMatrix(); glTranslatef(0.0f, 0.025f, 0.122f); drawScaledSphere(0.240f, 0.009f, 0.018f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.024f, -0.118f); drawScaledSphere(0.230f, 0.008f, 0.016f); glPopMatrix();

    setMat(r * 0.84f, g * 0.84f, b * 0.86f, 0.035f, 5.0f);
    glPushMatrix(); glTranslatef(-0.064f, 0.050f, -0.040f); glRotatef(24.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.036f, 0.012f, 0.070f, 18, 10); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.064f, 0.050f, -0.040f); glRotatef(-24.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.036f, 0.012f, 0.070f, 18, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.046f, 0.014f); drawScaledSphere(0.028f, 0.009f, 0.118f, 20, 10); glPopMatrix();

    // Sink a soft band below the collar fold so the stacked fabric keeps a readable self-shadow.
    setMat(r * 0.64f, g * 0.64f, b * 0.68f, 0.025f, 4.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.035f, -0.014f); drawScaledSphere(0.150f, 0.006f, 0.080f, 22, 10); glPopMatrix();

    setMat(r * 0.70f, g * 0.70f, b * 0.74f, 0.032f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.053f, 0.034f); drawScaledSphere(0.005f, 0.005f, 0.056f, 16, 8); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.053f, 0.040f); drawScaledSphere(0.108f, 0.004f, 0.005f, 18, 8); glPopMatrix();

    setMat(std::min(r * 1.05f, 1.0f), std::min(g * 1.05f, 1.0f), std::min(b * 1.04f, 1.0f), 0.024f, 4.0f);
    glPushMatrix(); glTranslatef(-0.090f, 0.044f, 0.068f); glRotatef(22.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.022f, 0.007f, 0.048f, 16, 8); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.090f, 0.044f, 0.068f); glRotatef(-22.0f, 0.0f, 1.0f, 0.0f); drawScaledSphere(0.022f, 0.007f, 0.048f, 16, 8); glPopMatrix();

    glPopMatrix();
}

void drawShirtStack(float r, float g, float b, int layers) {
    for (int i = 0; i < layers; ++i) {
        float tint = 1.0f - i * 0.035f;
        glPushMatrix();
        glTranslatef(0.0f, i * 0.030f, 0.0f);
        drawFoldedShirt(r * tint, g * tint, b * tint);
        glPopMatrix();
    }
}

void drawHangingJacket(float r, float g, float b) {
    glPushMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(0.72f, 0.72f, 0.68f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex3f(0.000f, 0.86f, 0.000f); glVertex3f(0.000f, 0.72f, 0.000f);
    glVertex3f(0.000f, 0.72f, 0.000f); glVertex3f(0.000f, 0.80f, -0.165f);
    glVertex3f(0.000f, 0.72f, 0.000f); glVertex3f(0.000f, 0.80f,  0.165f);
    glEnd();
    glEnable(GL_LIGHTING);

    setMat(r, g, b, 0.060f, 7.0f);
    glPushMatrix(); glTranslatef(0.028f, 0.42f, 0.000f); drawScaledSphere(0.058f, 0.350f, 0.222f, 28, 18); glPopMatrix();
    glPushMatrix(); glTranslatef(0.060f, 0.40f, 0.000f); drawScaledSphere(0.032f, 0.300f, 0.188f, 26, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.052f, 0.112f, 0.000f); drawScaledSphere(0.034f, 0.018f, 0.188f, 22, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.022f, 0.63f, -0.182f); drawScaledSphere(0.052f, 0.065f, 0.090f, 20, 14); glPopMatrix();
    glPushMatrix(); glTranslatef(0.022f, 0.63f,  0.182f); drawScaledSphere(0.052f, 0.065f, 0.090f, 20, 14); glPopMatrix();
    glPushMatrix(); glTranslatef(0.028f, 0.40f, -0.220f); glRotatef(4.0f, 1.0f, 0.0f, 0.0f); drawScaledSphere(0.032f, 0.205f, 0.045f, 22, 14); glPopMatrix();
    glPushMatrix(); glTranslatef(0.028f, 0.40f,  0.220f); glRotatef(-4.0f, 1.0f, 0.0f, 0.0f); drawScaledSphere(0.032f, 0.205f, 0.045f, 22, 14); glPopMatrix();

    setMat(r * 0.72f, g * 0.72f, b * 0.74f, 0.048f, 6.0f);
    glPushMatrix(); glTranslatef(0.076f, 0.60f, -0.054f); glRotatef(-22.0f, 0.0f, 0.0f, 1.0f); drawScaledSphere(0.016f, 0.092f, 0.050f, 18, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(0.076f, 0.60f,  0.054f); glRotatef( 22.0f, 0.0f, 0.0f, 1.0f); drawScaledSphere(0.016f, 0.092f, 0.050f, 18, 12); glPopMatrix();

    // Use slightly sunken darker volumes so the front opening keeps some depth as the spotlight angle changes.
    setMat(r * 0.58f, g * 0.58f, b * 0.62f, 0.028f, 4.0f);
    glPushMatrix(); glTranslatef(0.034f, 0.50f, 0.000f); drawScaledSphere(0.018f, 0.220f, 0.034f, 18, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(0.024f, 0.54f, -0.128f); drawScaledSphere(0.018f, 0.050f, 0.048f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.024f, 0.54f,  0.128f); drawScaledSphere(0.018f, 0.050f, 0.048f, 16, 10); glPopMatrix();

    setMat(r * 0.68f, g * 0.68f, b * 0.72f, 0.034f, 5.0f);
    glPushMatrix(); glTranslatef(0.080f, 0.39f, 0.000f); drawScaledSphere(0.006f, 0.240f, 0.008f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.078f, 0.40f, -0.050f); drawScaledSphere(0.005f, 0.135f, 0.007f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.078f, 0.40f,  0.050f); drawScaledSphere(0.005f, 0.135f, 0.007f, 16, 10); glPopMatrix();

    setMat(std::min(r * 1.04f, 1.0f), std::min(g * 1.04f, 1.0f), std::min(b * 1.03f, 1.0f), 0.022f, 4.0f);
    glPushMatrix(); glTranslatef(0.058f, 0.30f, -0.105f); glRotatef(10.0f, 0.0f, 0.0f, 1.0f); drawScaledSphere(0.012f, 0.086f, 0.030f, 14, 8); glPopMatrix();
    glPushMatrix(); glTranslatef(0.058f, 0.30f,  0.105f); glRotatef(-10.0f, 0.0f, 0.0f, 1.0f); drawScaledSphere(0.012f, 0.086f, 0.030f, 14, 8); glPopMatrix();

    glPopMatrix();
}

void drawTrackRail(float x) {
    setMat(0.05f, 0.05f, 0.05f, 0.05f, 4.0f);
    glPushMatrix();
    glTranslatef(x, CEILING_Y - 0.10f, 0.15f);
    drawBox(0.055f, 0.045f, 9.15f);
    glPopMatrix();

    float heads[] = { 4.02f, 2.92f, 1.82f, 0.70f, -0.45f, -1.58f, -2.72f, -3.85f };
    for (int i = 0; i < 8; ++i) {
        glPushMatrix();
        glTranslatef(x, CEILING_Y - 0.19f, heads[i]);
        setMat(0.08f, 0.08f, 0.08f, 0.06f, 5.0f);
        glPushMatrix(); glTranslatef(0.0f, 0.05f, 0.0f); drawBox(0.025f, 0.11f, 0.025f); glPopMatrix();
        glPushMatrix(); glRotatef((x < 0.0f) ? 22.0f : -22.0f, 0.0f, 0.0f, 1.0f); drawBox(0.105f, 0.160f, 0.105f); glPopMatrix();
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.95f, 0.82f);
        glPushMatrix(); glTranslatef(0.0f, -0.10f, 0.0f); drawBox(0.086f, 0.024f, 0.086f); glPopMatrix();
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
}

void drawCeilingPanels() {
    float panelZ[] = { 3.75f, 2.15f, 0.55f, -1.05f, -2.65f, -4.05f };
    for (int i = 0; i < 6; ++i) {
        setMat(0.06f, 0.055f, 0.05f, 0.08f, 6.0f);
        glPushMatrix(); glTranslatef(0.0f, CEILING_Y - 0.02f, panelZ[i]); drawBox(9.75f, 0.026f, 0.50f); glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, CEILING_Y - 0.035f, panelZ[i]);
        drawWarmStrip(9.10f, 0.010f, 0.26f);
        glPopMatrix();
    }

    setMat(0.03f, 0.028f, 0.025f, 0.06f, 5.0f);
    for (float z = -4.55f; z <= 4.30f; z += 0.82f) {
        glPushMatrix(); glTranslatef(0.0f, CEILING_Y - 0.075f, z); drawBox(10.65f, 0.030f, 0.035f); glPopMatrix();
    }
}

void drawFittingMirror(float x, float z, float rotY) {
    glPushMatrix();
    glTranslatef(x, 1.58f, z);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    setMat(0.07f, 0.07f, 0.07f, 0.12f, 10.0f);
    glPushMatrix(); drawBox(1.12f, 2.42f, 0.08f); glPopMatrix();
    setMat(0.16f, 0.13f, 0.10f, 0.14f, 16.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.018f); drawBox(0.98f, 2.28f, 0.038f); glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.040f);
    drawGlassPanel(0.86f, 2.12f, 0.42f, 0.45f, 0.48f, 0.34f);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 0.11f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBegin(GL_QUADS);
    glVertex3f(0.14f, -0.82f, 0.044f);
    glVertex3f(0.24f, -0.82f, 0.044f);
    glVertex3f(0.02f, 0.88f, 0.044f);
    glVertex3f(-0.06f, 0.88f, 0.044f);
    glEnd();

    glColor4f(0.18f, 0.20f, 0.23f, 0.10f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, -0.74f, 0.043f);
    glVertex3f( 0.30f, -0.74f, 0.043f);
    glVertex3f( 0.22f, -0.12f, 0.043f);
    glVertex3f(-0.22f, -0.12f, 0.043f);
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawDisplayPedestal(float x, float z, float topR, float topG, float topB) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    setMat(0.07f, 0.07f, 0.07f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.03f, 0.0f); drawCylinderY(0.28f, 0.32f, 0.06f, 26); glPopMatrix();
    setMat(topR * 0.88f, topG * 0.88f, topB * 0.88f, 0.10f, 14.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.56f, 0.0f); drawCylinderY(0.22f, 0.25f, 0.58f, 24); glPopMatrix();
    setMat(0.13f, 0.12f, 0.11f, 0.18f, 26.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.86f, 0.0f); drawCylinderY(0.34f, 0.34f, 0.05f, 28); glPopMatrix();
    setMat(std::min(topR * 1.02f, 1.0f), std::min(topG * 1.02f, 1.0f), std::min(topB * 1.02f, 1.0f), 0.15f, 22.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.83f, 0.0f); drawCylinderY(0.24f, 0.27f, 0.04f, 24); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.87f, 0.0f); drawAccessoryTray(0.12f, 0.14f, 0.16f); glPopMatrix();

    glPopMatrix();
}

void drawPottedPlant(float scale) {
    glPushMatrix();
    glScalef(scale, scale, scale);

    setMat(0.11f, 0.10f, 0.09f, 0.06f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.10f, 0.0f); drawBox(0.26f, 0.20f, 0.26f); glPopMatrix();

    setMat(0.41f, 0.28f, 0.15f, 0.05f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.28f, 0.0f); drawCylinderY(0.030f, 0.036f, 0.34f, 12); glPopMatrix();

    float leafRot[] = { -52.0f, -26.0f, -8.0f, 16.0f, 36.0f, 58.0f };
    float leafScale[] = { 0.24f, 0.28f, 0.32f, 0.29f, 0.26f, 0.22f };
    for (int i = 0; i < 6; ++i) {
        float y = 0.42f + i * 0.018f;
        float z = -0.02f + i * 0.015f;
        setMat(0.28f, 0.43f + i * 0.025f, 0.18f, 0.05f, 5.0f);
        glPushMatrix();
        glTranslatef(0.0f, y, z);
        glRotatef(leafRot[i], 0.0f, 0.0f, 1.0f);
        glTranslatef(0.0f, leafScale[i] * 0.55f, 0.0f);
        drawScaledSphere(0.038f, leafScale[i], 0.11f, 16, 10);
        glPopMatrix();
    }

    glPopMatrix();
}

void drawFramedPoster(float x, float y, float z, float rotY) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    setMat(0.06f, 0.06f, 0.06f, 0.10f, 10.0f);
    drawBox(0.92f, 1.52f, 0.05f);
    setMat(0.16f, 0.13f, 0.10f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.015f); drawBox(0.82f, 1.42f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.026f); drawGlassPanel(0.74f, 1.34f, 0.86f, 0.88f, 0.92f, 0.16f); glPopMatrix();

    setMat(0.10f, 0.10f, 0.10f, 0.05f, 4.0f);
    glPushMatrix(); glTranslatef(0.0f, -0.02f, 0.024f); drawBox(0.66f, 1.18f, 0.006f); glPopMatrix();
    setMat(0.90f, 0.89f, 0.86f, 0.06f, 6.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.40f, 0.028f); drawScaledSphere(0.10f, 0.17f, 0.04f, 16, 10); glPopMatrix();
    setMat(0.15f, 0.16f, 0.18f, 0.06f, 6.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.02f, 0.028f); drawBox(0.18f, 0.54f, 0.008f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -0.34f, 0.028f); drawBox(0.12f, 0.26f, 0.008f); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(0.18f, 0.18f, 0.18f);
    glLineWidth(1.4f);
    glBegin(GL_LINES);
    glVertex3f(-0.18f, 0.10f, 0.029f); glVertex3f(0.18f, 0.10f, 0.029f);
    glVertex3f(0.0f, 0.10f, 0.029f); glVertex3f(0.0f, -0.48f, 0.029f);
    glVertex3f(-0.12f, -0.48f, 0.029f); glVertex3f(0.0f, -0.72f, 0.029f);
    glVertex3f(0.12f, -0.48f, 0.029f); glVertex3f(0.0f, -0.72f, 0.029f);
    glEnd();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawFrontAccentShelf(float x, float z, bool leftSide) {
    glPushMatrix();
    glTranslatef(x, 1.56f, z);
    if (!leftSide) glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    setMat(0.08f, 0.065f, 0.045f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, -0.02f, 0.0f); drawBox(0.22f, 0.04f, 1.30f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.42f, 0.0f); drawBox(0.20f, 0.04f, 1.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.004f, 0.0f); setMat(0.18f, 0.14f, 0.10f, 0.14f, 16.0f); drawBox(0.205f, 0.010f, 1.22f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.444f, 0.0f); setMat(0.18f, 0.14f, 0.10f, 0.14f, 16.0f); drawBox(0.186f, 0.010f, 1.10f); glPopMatrix();

    glPushMatrix(); glTranslatef(-0.02f, 0.03f, -0.34f); drawFoldedPantsStack(0.88f, 0.87f, 0.84f, 3); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.02f, 0.03f, 0.02f); drawFoldedPantsStack(0.12f, 0.13f, 0.15f, 3); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.02f, 0.03f, 0.38f); drawFoldedPantsStack(0.40f, 0.31f, 0.22f, 3); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.01f, 0.47f, -0.28f); drawShirtStack(0.90f, 0.89f, 0.86f, 2); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.01f, 0.47f, 0.20f); drawShirtStack(0.10f, 0.11f, 0.12f, 2); glPopMatrix();

    glPopMatrix();
}

void drawCenterBench(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.24f, z);

    setMat(0.05f, 0.05f, 0.05f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, -0.18f, 0.0f); drawBox(0.64f, 0.04f, 0.44f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.24f, -0.06f, -0.14f); drawBox(0.035f, 0.24f, 0.035f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.24f, -0.06f, -0.14f); drawBox(0.035f, 0.24f, 0.035f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.24f, -0.06f,  0.14f); drawBox(0.035f, 0.24f, 0.035f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.24f, -0.06f,  0.14f); drawBox(0.035f, 0.24f, 0.035f); glPopMatrix();

    setMat(0.12f, 0.12f, 0.13f, 0.10f, 10.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.0f); drawRoundedBox(0.68f, 0.14f, 0.48f, 0.015f); glPopMatrix();

    glPopMatrix();
}

void drawFloorTileLines() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glLineWidth(1.0f);
    glColor4f(0.68f, 0.62f, 0.54f, 0.14f);

    glBegin(GL_LINES);
    for (float x = -6.25f; x <= 6.25f; x += 1.18f) {
        glVertex3f(x, 0.034f, -5.15f);
        glVertex3f(x, 0.034f, 7.05f);
    }
    for (float z = -5.00f; z <= 7.00f; z += 1.02f) {
        glVertex3f(-6.35f, 0.035f, z);
        glVertex3f( 6.35f, 0.035f, z);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawConcretePanelLines() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glLineWidth(1.0f);
    glColor4f(0.58f, 0.54f, 0.48f, 0.12f);

    glBegin(GL_LINES);
    for (float y = 0.85f; y <= 3.65f; y += 0.72f) {
        glVertex3f(-STORE_HALF_W + 0.28f, y, STORE_BACK_Z + 0.018f);
        glVertex3f( STORE_HALF_W - 0.28f, y, STORE_BACK_Z + 0.018f);
    }
    for (float x = -5.00f; x <= 5.00f; x += 1.25f) {
        glVertex3f(x, 0.15f, STORE_BACK_Z + 0.019f);
        glVertex3f(x, CEILING_Y - 0.36f, STORE_BACK_Z + 0.019f);
    }
    for (float z = STORE_BACK_Z + 0.60f; z <= STORE_FRONT_Z - 0.45f; z += 1.18f) {
        glVertex3f(-STORE_HALF_W + 0.018f, 0.24f, z);
        glVertex3f(-STORE_HALF_W + 0.018f, CEILING_Y - 0.32f, z);
        glVertex3f( STORE_HALF_W - 0.018f, 0.24f, z);
        glVertex3f( STORE_HALF_W - 0.018f, CEILING_Y - 0.32f, z);
    }
    for (float y = 0.95f; y <= 3.55f; y += 0.86f) {
        glVertex3f(-STORE_HALF_W + 0.019f, y, STORE_BACK_Z + 0.18f);
        glVertex3f(-STORE_HALF_W + 0.019f, y, STORE_FRONT_Z - 0.42f);
        glVertex3f( STORE_HALF_W - 0.019f, y, STORE_BACK_Z + 0.18f);
        glVertex3f( STORE_HALF_W - 0.019f, y, STORE_FRONT_Z - 0.42f);
    }
    for (float x = -5.05f; x <= 5.05f; x += 1.18f) {
        glVertex3f(x, CEILING_Y - 0.022f, STORE_BACK_Z + 0.20f);
        glVertex3f(x, CEILING_Y - 0.022f, STORE_FRONT_Z - 0.42f);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawFacadeOpeningFrame() {
    setMat(0.025f, 0.024f, 0.022f, 0.12f, 12.0f);
    glPushMatrix(); glTranslatef(0.0f, 3.02f, STORE_FRONT_Z - 0.11f); drawBox(9.55f, 0.16f, 0.32f); glPopMatrix();
    glPushMatrix(); glTranslatef(-4.62f, 1.53f, STORE_FRONT_Z - 0.05f); drawBox(0.13f, 3.05f, 0.24f); glPopMatrix();
    glPushMatrix(); glTranslatef( 4.62f, 1.53f, STORE_FRONT_Z - 0.05f); drawBox(0.13f, 3.05f, 0.24f); glPopMatrix();

    setMat(0.07f, 0.068f, 0.062f, 0.10f, 10.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.06f, STORE_FRONT_Z - 0.16f); drawBox(9.80f, 0.10f, 0.34f); glPopMatrix();

    glPushMatrix(); glTranslatef(-4.56f, 1.52f, STORE_FRONT_Z + 0.015f); drawWarmStrip(0.018f, 2.78f, 0.018f); glPopMatrix();
    glPushMatrix(); glTranslatef( 4.56f, 1.52f, STORE_FRONT_Z + 0.015f); drawWarmStrip(0.018f, 2.78f, 0.018f); glPopMatrix();
}

void drawFloor() {
    setMat(0.48f, 0.455f, 0.415f, 0.12f, 10.0f);
    glPushMatrix();
    drawTexturedQuadXZ(18.0f, 20.0f, 5.2f, 5.6f, floorTex);
    glPopMatrix();
    setMat(0.30f, 0.285f, 0.255f, 0.14f, 18.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.006f, 0.08f); drawBox(2.56f, 0.004f, 10.10f); glPopMatrix();
    drawFloorTileLines();

    setMat(0.22f, 0.205f, 0.180f, 0.12f, 14.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.014f, 0.05f); drawBox(2.78f, 0.020f, 10.28f); glPopMatrix();
    setMat(0.30f, 0.280f, 0.245f, 0.10f, 10.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.024f, 0.05f); drawBox(2.92f, 0.008f, 10.42f); glPopMatrix();

    setMat(0.34f, 0.315f, 0.270f, 0.06f, 5.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.010f, STORE_FRONT_Z - 0.12f); drawBox(8.85f, 0.020f, 0.36f); glPopMatrix();

    glPushMatrix(); glTranslatef(-3.25f, 0.012f,  3.30f); drawFloorGlow(1.10f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef( 3.25f, 0.012f,  3.30f); drawFloorGlow(1.10f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef(-3.25f, 0.012f,  1.05f); drawFloorGlow(1.06f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef( 3.25f, 0.012f,  1.05f); drawFloorGlow(1.06f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef(-3.25f, 0.012f, -1.15f); drawFloorGlow(1.06f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef( 3.25f, 0.012f, -1.15f); drawFloorGlow(1.06f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef(-3.25f, 0.012f, -3.35f); drawFloorGlow(1.10f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef( 3.25f, 0.012f, -3.35f); drawFloorGlow(1.10f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.00f, 0.012f,  2.05f); drawFloorGlow(1.24f, 1.0f, 0.86f, 0.64f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.00f, 0.012f, -3.45f); drawFloorGlow(1.28f, 1.0f, 0.86f, 0.64f); glPopMatrix();
}

void drawArchitecture() {
    setMat(0.23f, 0.215f, 0.190f, 0.07f, 7.0f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-STORE_HALF_W, 0.0f, STORE_BACK_Z); glVertex3f(STORE_HALF_W, 0.0f, STORE_BACK_Z);
    glVertex3f(STORE_HALF_W, CEILING_Y, STORE_BACK_Z); glVertex3f(-STORE_HALF_W, CEILING_Y, STORE_BACK_Z);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-STORE_HALF_W, 0.0f, STORE_BACK_Z); glVertex3f(-STORE_HALF_W, 0.0f, STORE_FRONT_Z);
    glVertex3f(-STORE_HALF_W, CEILING_Y, STORE_FRONT_Z); glVertex3f(-STORE_HALF_W, CEILING_Y, STORE_BACK_Z);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(STORE_HALF_W, 0.0f, STORE_BACK_Z); glVertex3f(STORE_HALF_W, 0.0f, STORE_FRONT_Z);
    glVertex3f(STORE_HALF_W, CEILING_Y, STORE_FRONT_Z); glVertex3f(STORE_HALF_W, CEILING_Y, STORE_BACK_Z);
    glEnd();

    setMat(0.13f, 0.120f, 0.105f, 0.05f, 5.0f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-STORE_HALF_W, CEILING_Y, STORE_BACK_Z); glVertex3f(STORE_HALF_W, CEILING_Y, STORE_BACK_Z);
    glVertex3f(STORE_HALF_W, CEILING_Y, STORE_FRONT_Z); glVertex3f(-STORE_HALF_W, CEILING_Y, STORE_FRONT_Z);
    glEnd();

    drawConcretePanelLines();

    setMat(0.05f, 0.05f, 0.048f, 0.10f, 8.0f);
    glPushMatrix(); glTranslatef(-5.45f, 1.98f, STORE_FRONT_Z + 0.02f); drawBox(0.86f, 3.96f, 0.22f); glPopMatrix();
    glPushMatrix(); glTranslatef( 5.45f, 1.98f, STORE_FRONT_Z + 0.02f); drawBox(0.86f, 3.96f, 0.22f); glPopMatrix();

    setMat(0.05f, 0.05f, 0.05f, 0.10f, 10.0f);
    glPushMatrix(); glTranslatef(-4.62f, 1.98f, STORE_FRONT_Z - 0.05f); drawBox(0.16f, 3.92f, 0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef( 4.62f, 1.98f, STORE_FRONT_Z - 0.05f); drawBox(0.16f, 3.92f, 0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 3.05f, STORE_FRONT_Z - 0.15f); drawBox(9.55f, 0.12f, 0.38f); glPopMatrix();

    setMat(0.05f, 0.05f, 0.05f, 0.10f, 8.0f);
    glPushMatrix(); glTranslatef(-5.92f, 1.95f, STORE_FRONT_Z - 0.02f); drawBox(0.28f, 3.92f, 0.28f); glPopMatrix();
    glPushMatrix(); glTranslatef( 5.92f, 1.95f, STORE_FRONT_Z - 0.02f); drawBox(0.28f, 3.92f, 0.28f); glPopMatrix();

    setMat(0.03f, 0.03f, 0.028f, 0.12f, 10.0f);
    glPushMatrix(); glTranslatef(0.0f, 3.72f, STORE_FRONT_Z + 0.015f); drawBox(12.35f, 1.12f, 0.26f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 4.22f, STORE_FRONT_Z + 0.045f); drawWarmStrip(11.86f, 0.020f, 0.022f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 3.20f, STORE_FRONT_Z + 0.050f); drawWarmStrip(11.25f, 0.014f, 0.016f); glPopMatrix();

    drawFacadeOpeningFrame();

    drawCeilingPanels();

    drawLayeredSignText("ICON MODE", 0.00f, 3.76f, STORE_FRONT_Z + 0.088f, 0.00455f,
                        1.0f, 0.99f, 0.97f, 0.22f, 0.03f, 0.02f);
    drawLayeredSignText("MEN'S WEAR", 0.00f, 3.42f, STORE_FRONT_Z + 0.088f, 0.00170f,
                        0.84f, 0.83f, 0.81f, 0.16f, 0.03f, 0.02f);

    setMat(0.11f, 0.10f, 0.09f, 0.08f, 6.0f);
    glPushMatrix(); glTranslatef(-4.88f, 2.00f, STORE_FRONT_Z - 0.03f); drawBox(0.10f, 2.00f, 0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef( 4.88f, 2.00f, STORE_FRONT_Z - 0.03f); drawBox(0.10f, 2.00f, 0.18f); glPopMatrix();

    drawFittingMirror(-3.88f, -4.25f, 18.0f);
    drawFittingMirror( 3.88f, -4.25f, -18.0f);
    drawDisplayPedestal(-2.24f, -4.35f, 0.11f, 0.11f, 0.11f);
    drawDisplayPedestal( 2.24f, -4.35f, 0.18f, 0.18f, 0.18f);
    drawFramedPoster(-4.07f, 2.06f, 3.62f, 90.0f);
    drawFramedPoster( 4.07f, 2.06f, 3.62f, -90.0f);
    drawFrontAccentShelf(-4.56f, 3.52f, true);
    drawFrontAccentShelf( 4.56f, 3.52f, false);
    glPushMatrix(); glTranslatef(4.68f, 0.0f, 4.40f); drawPottedPlant(0.68f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.96f, 0.0f, -3.62f); drawPottedPlant(0.58f); glPopMatrix();
}

void drawRearLogoWall() {
    setMat(0.02f, 0.02f, 0.02f, 0.06f, 4.0f);
    glPushMatrix(); glTranslatef(0.0f, 2.82f, STORE_BACK_Z + 0.01f); drawBox(5.10f, 1.46f, 0.02f); glPopMatrix();

    setMat(0.045f, 0.042f, 0.038f, 0.10f, 10.0f);
    glPushMatrix();
    glTranslatef(0.0f, 2.82f, STORE_BACK_Z + 0.025f);
    drawTexturedQuadXY(4.90f, 1.34f, 2.2f, 1.0f, woodTex);
    glPopMatrix();

    glPushMatrix(); glTranslatef(0.0f, 3.52f, STORE_BACK_Z + 0.032f); drawWarmStrip(4.55f, 0.016f, 0.018f); glPopMatrix();

    drawLayeredSignText("ICON MODE", 0.00f, 3.09f, STORE_BACK_Z + 0.075f, 0.00222f,
                        1.0f, 0.99f, 0.97f, 0.14f, 0.01f, 0.015f);
    drawLayeredSignText("MEN'S WEAR", 0.00f, 2.82f, STORE_BACK_Z + 0.075f, 0.00098f,
                        0.82f, 0.81f, 0.79f, 0.10f, 0.01f, 0.015f);
}

void drawWallBay(float sideX, float zCenter, bool leftSide, int mode) {
    glPushMatrix();
    glTranslatef(sideX, 0.0f, zCenter);
    if (!leftSide) glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    setMat(0.06f, 0.06f, 0.055f, 0.06f, 4.0f);
    glPushMatrix(); glTranslatef(0.05f, 2.00f, 0.0f); drawBox(0.08f, 3.95f, 1.90f); glPopMatrix();

    setMat(0.05f, 0.05f, 0.05f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.68f, 1.98f, -0.83f); drawBox(0.04f, 3.70f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.68f, 1.98f,  0.83f); drawBox(0.04f, 3.70f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.36f, 3.63f, 0.0f); drawBox(0.68f, 0.05f, 1.72f); glPopMatrix();

    setMat(0.10f, 0.07f, 0.05f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.36f, 0.40f, 0.0f); drawBox(0.60f, 0.78f, 1.66f); glPopMatrix();
    setMat(0.03f, 0.03f, 0.03f, 0.05f, 4.0f);
    glPushMatrix(); glTranslatef(0.36f, 0.08f, 0.0f); drawBox(0.52f, 0.12f, 1.46f); glPopMatrix();

    setMat(0.13f, 0.095f, 0.065f, 0.07f, 8.0f);
    glPushMatrix(); glTranslatef(0.34f, 3.18f, 0.0f); drawBox(0.56f, 0.07f, 1.58f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.60f, 3.12f, 0.0f); drawWarmStrip(0.02f, 0.012f, 1.45f); glPopMatrix();

    setMat(0.10f, 0.09f, 0.08f, 0.14f, 14.0f);
    glPushMatrix(); glTranslatef(0.36f, 2.02f, -0.79f); drawBox(0.54f, 3.32f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.36f, 2.02f,  0.79f); drawBox(0.54f, 3.32f, 0.02f); glPopMatrix();

    glPushMatrix(); glTranslatef(0.36f, 2.02f, -0.775f); drawGlassPanel(0.46f, 3.10f, 0.83f, 0.88f, 0.92f, 0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.36f, 2.02f,  0.775f); drawGlassPanel(0.46f, 3.10f, 0.83f, 0.88f, 0.92f, 0.12f); glPopMatrix();

    if (mode == 0) {
        setMat(0.18f, 0.18f, 0.17f, 0.10f, 12.0f);
        glPushMatrix();
        glTranslatef(0.36f, 2.36f, -0.68f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinderY(0.014f, 0.014f, 1.36f, 16);
        glPopMatrix();

        float jacketColors[8][3] = {
            {0.86f, 0.84f, 0.80f},
            {0.10f, 0.11f, 0.13f},
            {0.17f, 0.21f, 0.29f},
            {0.42f, 0.34f, 0.22f},
            {0.08f, 0.08f, 0.08f},
            {0.70f, 0.66f, 0.58f},
            {0.92f, 0.90f, 0.84f},
            {0.12f, 0.12f, 0.13f}
        };
        for (int i = 0; i < 8; ++i) {
            glPushMatrix();
            // Offset each hanger slightly so the rail feels hand-merchandised instead of procedurally cloned.
            glTranslatef(0.35f + ((i % 2 == 0) ? -0.012f : 0.010f), 1.48f + ((i % 3 == 1) ? 0.012f : 0.0f), -0.62f + i * 0.18f + ((i % 3 == 0) ? -0.012f : 0.010f));
            glRotatef((i % 2 == 0) ? -4.0f : 3.0f, 0.0f, 1.0f, 0.0f);
            drawHangingJacket(jacketColors[i][0], jacketColors[i][1], jacketColors[i][2]);
            glPopMatrix();
        }

        setMat(0.13f, 0.095f, 0.065f, 0.07f, 8.0f);
        glPushMatrix(); glTranslatef(0.34f, 0.92f, 0.0f); drawBox(0.54f, 0.05f, 1.54f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.34f, 0.86f, 0.0f); drawWarmStrip(0.02f, 0.010f, 1.40f); glPopMatrix();

        glPushMatrix(); glTranslatef(0.18f, 0.96f, -0.36f); drawShoePair(0.08f, 0.08f, 0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.18f, 0.96f,  0.00f); drawShoePair(0.88f, 0.88f, 0.85f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.18f, 0.96f,  0.36f); drawShoePair(0.42f, 0.27f, 0.12f); glPopMatrix();

        glPushMatrix(); glTranslatef(0.48f, 0.93f, -0.64f); glRotatef(16.0f, 0.0f, 1.0f, 0.0f); drawShoppingBag(0.15f, 0.17f, 0.19f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.50f, 0.93f,  0.58f); glRotatef(-14.0f, 0.0f, 1.0f, 0.0f); drawShoppingBag(0.79f, 0.77f, 0.71f); glPopMatrix();
    } else {
        float levels[] = { 1.05f, 1.74f, 2.43f };
        float stackColors[3][3] = {
            {0.90f, 0.89f, 0.86f},
            {0.14f, 0.18f, 0.28f},
            {0.34f, 0.32f, 0.30f}
        };

        for (int i = 0; i < 3; ++i) {
            setMat(0.13f, 0.095f, 0.065f, 0.07f, 8.0f);
            glPushMatrix(); glTranslatef(0.34f, levels[i], 0.0f); drawBox(0.54f, 0.05f, 1.52f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.60f, levels[i] - 0.04f, 0.0f); drawWarmStrip(0.02f, 0.010f, 1.38f); glPopMatrix();

            float zPos[] = { -0.46f, 0.0f, 0.46f };
            for (int c = 0; c < 3; ++c) {
                glPushMatrix();
                glTranslatef(0.34f + (i - 1) * 0.015f, levels[i] + 0.03f, zPos[c] + (c - 1) * 0.018f);
                drawShirtStack(stackColors[c][0], stackColors[c][1], stackColors[c][2], 4);
                glPopMatrix();
            }

            glPushMatrix(); glTranslatef(0.52f, levels[i] + 0.04f, -0.24f - i * 0.012f); glRotatef(-4.0f + i * 2.5f, 0.0f, 1.0f, 0.0f); drawFoldedPantsStack(0.10f, 0.11f, 0.12f, 3); glPopMatrix();
            glPushMatrix(); glTranslatef(0.52f, levels[i] + 0.04f,  0.24f + i * 0.010f); glRotatef(3.0f - i * 2.0f, 0.0f, 1.0f, 0.0f); drawFoldedPantsStack(0.44f, 0.34f, 0.24f, 3); glPopMatrix();
        }

        glPushMatrix(); glTranslatef(0.20f, 3.23f, -0.26f); drawShoePair(0.08f, 0.08f, 0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.20f, 3.23f,  0.26f); drawShoePair(0.90f, 0.89f, 0.84f); glPopMatrix();

        glPushMatrix(); glTranslatef(0.17f, 0.86f, 0.00f); drawAccessoryTray(0.12f, 0.14f, 0.17f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.48f, 0.90f, -0.64f); glRotatef(10.0f, 0.0f, 1.0f, 0.0f); drawShoppingBag(0.18f, 0.18f, 0.18f); glPopMatrix();
    }

    glPopMatrix();
}

void drawCounter() {
    glPushMatrix();
    glTranslatef(0.0f, 0.49f, -4.18f);

    setMat(0.035f, 0.033f, 0.030f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, -0.01f, 0.0f); drawBox(3.55f, 0.88f, 0.74f); glPopMatrix();

    setMat(0.12f, 0.11f, 0.10f, 0.18f, 24.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.46f, 0.0f); drawBox(3.78f, 0.060f, 0.90f); glPopMatrix();
    setMat(0.03f, 0.03f, 0.03f, 0.14f, 14.0f);
    glPushMatrix(); glTranslatef(0.0f, -0.39f, 0.08f); drawBox(3.06f, 0.08f, 0.44f); glPopMatrix();

    setMat(0.12f, 0.09f, 0.06f, 0.06f, 6.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.378f);
    extern GLuint sceneProgram;
    if (sceneProgram) glUseProgram(0);
    drawTexturedQuadXY(3.46f, 0.72f, 3.2f, 1.0f, woodTex);
    if (sceneProgram) glUseProgram(sceneProgram);
    glPopMatrix();

    for (float x = -1.58f; x <= 1.58f; x += 0.090f) {
        setMat(0.25f, 0.18f, 0.12f, 0.05f, 5.0f);
        glPushMatrix(); glTranslatef(x, -0.02f, 0.389f); drawBox(0.042f, 0.74f, 0.028f); glPopMatrix();
    }

    glPushMatrix(); glTranslatef(0.0f, 0.43f, 0.40f); drawWarmStrip(3.10f, 0.016f, 0.016f); glPopMatrix();

    setMat(0.18f, 0.17f, 0.16f, 0.18f, 22.0f);
    glPushMatrix(); glTranslatef(-0.82f, 0.64f, 0.18f); drawRoundedBox(0.42f, 0.03f, 0.26f, 0.010f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.80f, 0.68f, 0.16f); glRotatef(-7.0f, 0.0f, 1.0f, 0.0f); drawAccessoryTray(0.11f, 0.12f, 0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.36f, 0.46f, 0.18f); glRotatef(-9.0f, 0.0f, 1.0f, 0.0f); drawShoppingBag(0.12f, 0.12f, 0.13f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.08f, 0.46f, 0.08f); glRotatef(18.0f, 0.0f, 1.0f, 0.0f); drawShoppingBag(0.80f, 0.78f, 0.72f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.96f, 0.64f, -0.16f); glRotatef(-14.0f, 0.0f, 1.0f, 0.0f); drawShoppingBag(0.18f, 0.19f, 0.20f); glPopMatrix();

    setMat(0.05f, 0.05f, 0.05f, 0.14f, 18.0f);
    glPushMatrix(); glTranslatef(0.78f, 0.67f, 0.02f); glRotatef(-12.0f, 1.0f, 0.0f, 0.0f); drawRoundedBox(0.40f, 0.25f, 0.025f, 0.008f); glPopMatrix();
    setMat(0.10f, 0.13f, 0.16f, 0.18f, 22.0f);
    glPushMatrix(); glTranslatef(0.78f, 0.67f, 0.038f); glRotatef(-12.0f, 1.0f, 0.0f, 0.0f); drawRoundedBox(0.34f, 0.20f, 0.010f, 0.004f); glPopMatrix();
    setMat(0.16f, 0.16f, 0.15f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.78f, 0.53f, -0.005f); drawRoundedBox(0.050f, 0.12f, 0.050f, 0.010f); glPopMatrix();

    glPopMatrix();
}

void drawCenterTable(float z, int variant) {
    glPushMatrix();
    glTranslatef(0.0f, 0.39f, z);

    float width = 2.95f;
    float depth = 1.26f;
    float legInsetX = width * 0.5f - 0.10f;
    float legInsetZ = depth * 0.5f - 0.10f;

    setMat(0.04f, 0.04f, 0.04f, 0.10f, 10.0f);
    float lx[] = { -legInsetX, legInsetX, -legInsetX, legInsetX };
    float lz[] = { -legInsetZ, -legInsetZ, legInsetZ, legInsetZ };
    for (int i = 0; i < 4; ++i) {
        glPushMatrix(); glTranslatef(lx[i], -0.39f, lz[i]); drawCylinderY(0.016f, 0.016f, 0.78f, 14); glPopMatrix();
    }

    glPushMatrix(); glTranslatef(0.0f, -0.04f,  depth * 0.5f); drawBox(width, 0.03f, 0.03f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -0.04f, -depth * 0.5f); drawBox(width, 0.03f, 0.03f); glPopMatrix();
    glPushMatrix(); glTranslatef(-width * 0.5f, -0.04f, 0.0f); drawBox(0.03f, 0.03f, depth); glPopMatrix();
    glPushMatrix(); glTranslatef( width * 0.5f, -0.04f, 0.0f); drawBox(0.03f, 0.03f, depth); glPopMatrix();

    setMat(0.10f, 0.10f, 0.095f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.08f, 0.0f); drawBox(width * 0.92f, 0.02f, depth * 0.70f); glPopMatrix();

    setMat(0.13f, 0.12f, 0.11f, 0.18f, 24.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.39f, 0.0f); drawBox(width, 0.060f, depth); glPopMatrix();
    setMat(0.07f, 0.07f, 0.065f, 0.10f, 10.0f);
    glPushMatrix(); glTranslatef(0.0f, -0.08f, 0.0f); drawBox(width * 0.90f, 0.040f, depth * 0.82f); glPopMatrix();

    if (variant == 0) {
        glPushMatrix(); glTranslatef(-0.55f, 0.44f, -0.18f); drawShirtStack(0.90f, 0.89f, 0.86f, 5); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.55f, 0.44f, -0.18f); drawShirtStack(0.14f, 0.18f, 0.28f, 5); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.00f, 0.44f,  0.20f); drawShirtStack(0.32f, 0.30f, 0.28f, 4); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.48f, -0.02f, 0.08f); glRotatef(-8.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.08f, 0.08f, 0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.46f, -0.02f, 0.02f); glRotatef(9.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.90f, 0.89f, 0.84f); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.02f, -0.03f, -0.08f); glRotatef(-6.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.42f, 0.27f, 0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.06f, -0.01f,  0.28f); glRotatef(16.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.88f, 0.86f, 0.80f); glPopMatrix();
    } else {
        glPushMatrix(); glTranslatef(-0.56f, 0.44f,  0.16f); drawShirtStack(0.88f, 0.87f, 0.84f, 4); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.00f, 0.44f, -0.08f); drawShirtStack(0.10f, 0.11f, 0.12f, 5); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.56f, 0.44f,  0.16f); drawShirtStack(0.45f, 0.34f, 0.22f, 4); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.58f, -0.02f, 0.04f); glRotatef(-7.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.42f, 0.27f, 0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.52f, -0.02f, -0.02f); glRotatef(8.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.08f, 0.08f, 0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.02f, -0.04f, 0.02f); glRotatef(5.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.88f, 0.86f, 0.80f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.06f, -0.02f, -0.30f); glRotatef(-8.0f, 0.0f, 1.0f, 0.0f); drawShoePair(0.90f, 0.89f, 0.84f); glPopMatrix();
    }

    glPopMatrix();
}

void drawProceduralMannequinBody(float jacketR, float jacketG, float jacketB) {
    setMat(0.12f, 0.13f, 0.15f, 0.050f, 10.0f);
    glPushMatrix(); glTranslatef(-0.10f, 0.18f, 0.02f); drawCylinderY(0.058f, 0.048f, 0.54f, 18); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.09f, 0.72f, 0.01f); drawCylinderY(0.078f, 0.060f, 0.50f, 18); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f, 0.18f, 0.02f); drawCylinderY(0.058f, 0.048f, 0.54f, 18); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.09f, 0.72f, 0.01f); drawCylinderY(0.078f, 0.060f, 0.50f, 18); glPopMatrix();

    setMat(0.07f, 0.08f, 0.10f, 0.032f, 6.0f);
    glPushMatrix(); glTranslatef(-0.10f, 0.67f, 0.074f); drawScaledSphere(0.006f, 0.490f, 0.007f, 14, 10); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f, 0.67f, 0.074f); drawScaledSphere(0.006f, 0.490f, 0.007f, 14, 10); glPopMatrix();

    setMat(0.80f, 0.78f, 0.74f, 0.034f, 12.0f);
    glPushMatrix(); glTranslatef(0.0f, 1.18f, 0.00f); drawScaledSphere(0.22f, 0.09f, 0.12f, 24, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.50f, 0.03f); drawScaledSphere(0.17f, 0.32f, 0.10f, 28, 18); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.62f, 0.105f); drawRoundedBox(0.10f, 0.14f, 0.018f, 0.012f); glPopMatrix();

    setMat(0.93f, 0.92f, 0.89f, 0.026f, 7.0f);
    glPushMatrix(); glTranslatef(0.0f, 1.47f, 0.125f); drawRoundedBox(0.11f, 0.18f, 0.016f, 0.012f); glPopMatrix();

    setMat(jacketR * 0.90f, jacketG * 0.90f, jacketB * 0.92f, 0.050f, 7.0f);
    glPushMatrix(); glTranslatef(0.0f, 1.40f, 0.000f); drawScaledSphere(0.126f, 0.255f, 0.064f, 24, 16); glPopMatrix();
    setMat(std::min(jacketR * 1.02f, 1.0f), std::min(jacketG * 1.02f, 1.0f), std::min(jacketB * 1.01f, 1.0f), 0.074f, 10.0f);
    glPushMatrix(); glTranslatef(-0.085f, 1.46f, 0.02f); drawScaledSphere(0.125f, 0.360f, 0.078f, 28, 18); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.085f, 1.46f, 0.02f); drawScaledSphere(0.125f, 0.360f, 0.078f, 28, 18); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.39f, 0.010f); drawScaledSphere(0.138f, 0.250f, 0.082f, 24, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.185f, 1.67f, 0.005f); drawScaledSphere(0.090f, 0.060f, 0.085f, 20, 14); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.185f, 1.67f, 0.005f); drawScaledSphere(0.090f, 0.060f, 0.085f, 20, 14); glPopMatrix();

    setMat(jacketR * 0.72f, jacketG * 0.72f, jacketB * 0.74f, 0.045f, 6.0f);
    glPushMatrix(); glTranslatef(-0.070f, 1.69f, 0.090f); glRotatef(-24.0f, 0.0f, 0.0f, 1.0f); drawScaledSphere(0.016f, 0.092f, 0.014f, 18, 12); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.070f, 1.69f, 0.090f); glRotatef( 24.0f, 0.0f, 0.0f, 1.0f); drawScaledSphere(0.016f, 0.092f, 0.014f, 18, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.53f, 0.118f); drawScaledSphere(0.008f, 0.210f, 0.008f, 16, 10); glPopMatrix();

    // Keep the shirt opening recessed with lit volumes so the jacket reads layered from more than one camera angle.
    setMat(jacketR * 0.58f, jacketG * 0.58f, jacketB * 0.62f, 0.028f, 4.0f);
    glPushMatrix(); glTranslatef(0.0f, 1.53f, 0.108f); drawScaledSphere(0.012f, 0.230f, 0.020f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.082f, 1.47f, 0.126f); drawScaledSphere(0.010f, 0.170f, 0.008f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.082f, 1.47f, 0.126f); drawScaledSphere(0.010f, 0.170f, 0.008f, 16, 10); glPopMatrix();

    setMat(0.11f, 0.11f, 0.12f, 0.20f, 28.0f);
    glPushMatrix(); glTranslatef(0.0f, 1.11f, 0.122f); drawRoundedBox(0.16f, 0.032f, 0.020f, 0.008f); glPopMatrix();
    setMat(0.56f, 0.56f, 0.54f, 0.24f, 32.0f);
    glPushMatrix(); glTranslatef(-0.046f, 1.10f, 0.135f); drawRoundedBox(0.040f, 0.050f, 0.010f, 0.004f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.046f, 1.10f, 0.135f); drawRoundedBox(0.040f, 0.050f, 0.010f, 0.004f); glPopMatrix();

    setMat(jacketR * 0.94f, jacketG * 0.94f, jacketB * 0.95f, 0.060f, 8.0f);
    glPushMatrix();
    glTranslatef(-0.235f, 1.60f, 0.00f);
    glRotatef(191.0f, 0.0f, 0.0f, 1.0f);
    drawCylinderY(0.050f, 0.042f, 0.46f, 18);
    glTranslatef(0.0f, 0.46f, 0.0f);
    drawCylinderY(0.042f, 0.032f, 0.28f, 18);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.235f, 1.60f, 0.00f);
    glRotatef(169.0f, 0.0f, 0.0f, 1.0f);
    drawCylinderY(0.050f, 0.042f, 0.46f, 18);
    glTranslatef(0.0f, 0.46f, 0.0f);
    drawCylinderY(0.042f, 0.032f, 0.28f, 18);
    glPopMatrix();

    setMat(0.80f, 0.78f, 0.74f, 0.034f, 12.0f);
    glPushMatrix(); glTranslatef(-0.285f, 1.04f, 0.03f); drawScaledSphere(0.035f, 0.060f, 0.028f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.285f, 1.04f, 0.03f); drawScaledSphere(0.035f, 0.060f, 0.028f, 16, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.75f, 0.00f); drawCylinderY(0.040f, 0.043f, 0.135f, 18); glPopMatrix();
}

void drawMannequin(float x, float z, float rotY, float jacketR, float jacketG, float jacketB) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    setMat(0.03f, 0.03f, 0.03f, 0.08f, 8.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.02f, 0.0f); drawCylinderY(0.28f, 0.28f, 0.03f, 28); glPopMatrix();
    setMat(0.10f, 0.10f, 0.10f, 0.10f, 12.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.038f, 0.0f); drawCylinderY(0.19f, 0.24f, 0.018f, 24); glPopMatrix();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    // Push a denser heel shadow into the pedestal zone so the figure does not appear to float.
    drawSoftEllipseDecal(0.0f, 0.043f, 0.040f, 0.168f, 0.118f, 0.0f, 0.0f, 0.0f, 0.24f, 0.0f);
    drawSoftEllipseDecal(0.0f, 0.044f, 0.052f, 0.092f, 0.060f, 1.0f, 0.94f, 0.80f, 0.020f, 0.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    if (mannequinMeshLoaded) {
        glPushMatrix();
        // Keep the imported mesh aligned with the existing pedestal and shoe placement.
        glTranslatef(0.0f, 0.046f, -0.023f);
        glScalef(1.0f, 1.0f, 1.0f);
        drawLoadedMannequinMesh();
        glPopMatrix();
    } else {
        glPushMatrix(); glTranslatef(-0.11f, 0.065f, 0.11f); glRotatef(-6.0f, 0.0f, 1.0f, 0.0f); drawShoe(0.90f, 0.90f, 0.86f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.11f, 0.065f, 0.11f); glRotatef( 6.0f, 0.0f, 1.0f, 0.0f); drawShoe(0.90f, 0.90f, 0.86f); glPopMatrix();
        drawProceduralMannequinBody(jacketR, jacketG, jacketB);
    }

    glPopMatrix();
}

struct DummyShader {
    void use() {}
    void setMat4(const char*, const glm::mat4&) {}
    void setInt(const char*, int) {}
};
DummyShader shadowShader, pbrShader;
glm::vec3 lightPos(0.0f, 10.0f, 0.0f);
glm::mat4 lightSpaceGlobal;
void renderScene(DummyShader&) { renderScene(); }
unsigned int shadowFBO, shadowMap;
const int SHADOW_W = 4096, SHADOW_H = 4096;

void init() {
    initOptionalGLFeatures();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_MULTISAMPLE);
    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glDisable(GL_FOG);

    floorTex = loadTexture("floor.jpg");
    woodTex = loadTexture("wood.jpg");
    loadMannequinMesh("mannequin_store.obj");
    loadShoeMesh("shoe_store.obj");

    buildSceneProgram();

    glClearColor(0.022f, 0.020f, 0.018f, 1.0f);
    updateCamera();

    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // PCF soften edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Ma trận ánh sáng
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 50.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));
    glm::mat4 lightSpace = lightProjection * lightView;
    lightSpaceGlobal = lightSpace;
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    updateCamera();
    applyLookAt(camX, camY, camZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);

    setupLightingRig();

    drawFloor();
    drawContactShadows();
    
    if (sceneProgram) glUseProgram(sceneProgram);
    drawArchitecture();
    drawRearLogoWall();
    if (sceneProgram) glUseProgram(0);

    if (sceneProgram) glUseProgram(sceneProgram);

    float bayZ[] = { -4.10f, -1.55f, 1.00f, 3.55f };
    int bayMode[] = { 1, 0, 0, 1 };
    for (int i = 0; i < 4; ++i) {
        drawWallBay(-5.35f, bayZ[i], true, bayMode[i]);
        drawWallBay( 5.35f, bayZ[i], false, bayMode[i]);
    }

    drawTrackRail(-4.05f);
    drawTrackRail(-2.05f);
    drawTrackRail( 0.00f);
    drawTrackRail( 2.05f);
    drawTrackRail( 4.05f);

    drawCenterTable(3.20f, 0);
    glPushMatrix(); glTranslatef( 2.42f, 0.0f, 0.18f); glScalef(0.76f, 0.96f, 0.92f); drawCenterTable(1.62f, 1); glPopMatrix();
    glPushMatrix(); glTranslatef(-2.20f, 0.0f, 0.00f); glScalef(0.70f, 0.94f, 0.88f); drawCenterTable(1.10f, 1); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.00f, 0.0f, -0.20f); glScalef(0.86f, 0.94f, 0.90f); drawCenterTable(-0.70f, 1); glPopMatrix();
    drawCenterBench(-0.68f, -1.95f);
    drawCenterBench( 0.68f, -1.95f);
    drawCounter();

    glPushMatrix(); glTranslatef(-1.82f, 0.0f, 2.45f); drawPottedPlant(0.70f); glPopMatrix();
    glPushMatrix(); glTranslatef( 4.76f, 0.0f, 3.38f); drawPottedPlant(0.50f); glPopMatrix();
    glPushMatrix(); glScalef(1.10f, 1.10f, 1.10f); drawMannequin(-4.30f, 4.68f, 5.0f, 0.08f, 0.08f, 0.08f); glPopMatrix();
    glPushMatrix(); glScalef(1.10f, 1.10f, 1.10f); drawMannequin( 4.30f, 4.68f, -5.0f, 0.12f, 0.15f, 0.22f); glPopMatrix();
    
    if (sceneProgram) glUseProgram(0);
}

void display() {
    int SCR_WIDTH = presentWidth;
    int SCR_HEIGHT = presentHeight;
    glm::mat4 lightSpace = lightSpaceGlobal;
    bool usePresentTarget = presentPathReady && presentFBO && presentProgram;

    // Pass 1: render depth từ góc đèn
    glViewport(0, 0, SHADOW_W, SHADOW_H);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    shadowShader.use();
    shadowShader.setMat4("lightSpaceMatrix", lightSpace);
    renderScene(shadowShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Pass 2: render thật với shadow
    if (usePresentTarget) {
        storeBindFramebuffer(GL_FRAMEBUFFER, presentFBO);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pbrShader.use();
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    pbrShader.setInt("shadowMap", 5);
    pbrShader.setMat4("lightSpaceMatrix", lightSpace);
    glActiveTexture(GL_TEXTURE0);
    renderScene(pbrShader);

    if (usePresentTarget) {
        storeBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, presentWidth, presentHeight);
        drawPresentPass();
    }
    drawStorefrontHeaderOverlay();
}

bool updateMovement(float dt) {
    float forward = 0.0f;
    float right = 0.0f;
    float up = 0.0f;

    if (keyStates['w'] || keyStates['W'] || specialKeyStates[GLFW_KEY_UP]) forward += 1.0f;
    if (keyStates['s'] || keyStates['S'] || specialKeyStates[GLFW_KEY_DOWN]) forward -= 1.0f;
    if (keyStates['d'] || keyStates['D'] || specialKeyStates[GLFW_KEY_RIGHT]) right += 1.0f;
    if (keyStates['a'] || keyStates['A'] || specialKeyStates[GLFW_KEY_LEFT]) right -= 1.0f;
    if (keyStates['q'] || keyStates['Q']) up += 1.0f;
    if (keyStates['e'] || keyStates['E']) up -= 1.0f;

    float len = sqrt(forward * forward + right * right + up * up);
    if (len <= 0.0f) {
        return false;
    }

    forward /= len;
    right /= len;
    up /= len;

    float cy = cos(toRad(yaw));
    float sy = sin(toRad(yaw));
    float step = speed * dt;

    camX += (cy * forward - sy * right) * step;
    camZ += (sy * forward + cy * right) * step;
    camY += up * step;

    if (camY < 0.55f) camY = 0.55f;
    if (camY > 4.00f) camY = 4.00f;

    return true;
}

void idle() {
    double now = glfwGetTime();
    if (lastFrameTime == 0.0) {
        lastFrameTime = now;
    }

    float dt = static_cast<float>(now - lastFrameTime);
    if (dt > 0.05f) {
        dt = 0.05f;
    }
    lastFrameTime = now;

    updateMovement(dt);
}

void keyCallback(GLFWwindow* currentWindow, int key, int scancode, int action, int mods) {
    bool pressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_ESCAPE && pressed) {
        glfwSetWindowShouldClose(currentWindow, GLFW_TRUE);
    }
    if (key >= 0 && key < 256) {
        keyStates[key] = pressed;
        if (key >= 'A' && key <= 'Z') {
            keyStates[key + ('a' - 'A')] = pressed;
        }
    }
    if (key >= 0 && key < 256) {
        specialKeyStates[key] = pressed;
    } else if (key >= 0 && key < 512) {
        specialKeyStates[key] = pressed;
    }
}

void cursorMoveCallback(GLFWwindow* currentWindow, double x, double y) {
    if (isMousePressed) {
        yaw += (static_cast<int>(x) - lastMouseX) * 0.2f;
        pitch -= (static_cast<int>(y) - lastMouseY) * 0.2f;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        lastMouseX = static_cast<int>(x);
        lastMouseY = static_cast<int>(y);
    }
}

void mouseButtonCallback(GLFWwindow* currentWindow, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(currentWindow, &x, &y);
        isMousePressed = true;
        lastMouseX = static_cast<int>(x);
        lastMouseY = static_cast<int>(y);
    } else {
        isMousePressed = false;
    }
}

void reshape(int w, int h) {
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    presentWidth = w;
    presentHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    applyPerspective(47.0f, static_cast<float>(w) / static_cast<float>(h), 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);

    if (presentProgram && !resizePresentTargets(w, h)) {
        destroyPresentResources();
    }
}

void framebufferSizeCallback(GLFWwindow* currentWindow, int width, int height) {
    reshape(width, height);
}

int main(int argc, char** argv) {
    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(1536, 1024, "ICON MODE - MEN'S WEAR", 0, 0);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    init();

    int framebufferW, framebufferH;
    glfwGetFramebufferSize(window, &framebufferW, &framebufferH);
    reshape(framebufferW, framebufferH);
    initPresentPath(framebufferW, framebufferH);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorMoveCallback);

    lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        idle();
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyPresentResources();
    if (floorTex) glDeleteTextures(1, &floorTex);
    if (woodTex) glDeleteTextures(1, &woodTex);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
