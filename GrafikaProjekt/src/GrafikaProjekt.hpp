﻿#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"
#include "SOIL/SOIL.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

const unsigned int SHADOW_WIDTH = 8024, SHADOW_HEIGHT = 8024;

int WIDTH = 500, HEIGHT = 500;

namespace models {
	// Bed
	Core::RenderContext bedContext;
	Core::RenderContext bedFrameContext;
	Core::RenderContext bedBlanketContext;
	Core::RenderContext bedMattressContext;
	Core::RenderContext bedLegsContext;

	// Room
	Core::RenderContext roomContext;
	Core::RenderContext planeContext;

	// Table
	Core::RenderContext tableContext;

	// Doors
	Core::RenderContext doorsFrameContext;
	Core::RenderContext doorsPanelContext;

	// Monitor
	Core::RenderContext monitorContext;

	// Snow globe
	Core::RenderContext snowGlobeContext;

	// Windows
	Core::RenderContext windowContext1;
	Core::RenderContext windowContext2;
	Core::RenderContext windowContext3;
	Core::RenderContext floorContext;

	// Skybox
	Core::RenderContext skyboxContext;

	// Mirror
	Core::RenderContext mirrorFrameContext;
	Core::RenderContext mirrorGlassContext;
	
	Core::RenderContext spaceshipContext;
	Core::RenderContext sphereContext;
	Core::RenderContext boxContext;

}

namespace texture {
	GLuint skyboxTexture;
	GLuint box;
	GLuint woodPlanks;
}

namespace frameBuffers {
	GLuint box;
}

GLuint FramebufferName = 0;
GLuint renderedTexture;
GLuint RBO;
GLuint depthrenderbuffer;
GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

GLuint depthMapFBO;
GLuint depthMap;

GLuint framebufferProgram;
GLuint program;
GLuint programDepth;
GLuint programSun;
GLuint programTest;
GLuint programTex;
GLuint programSkybox;
Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;

glm::vec3 sunPos = glm::vec3(4.740971f, 2.149999f, 0.369280f);
glm::vec3 sunDir = glm::vec3(0.93633f, 0.351106, 0.003226f);
glm::vec3 sunColor = glm::vec3(0.9f, 0.9f, 0.7f)*5;

glm::vec3 cameraPos = glm::vec3(0.479490f, 1.250000f, -2.124680f);
glm::vec3 cameraDir = glm::vec3(-0.354510f, 0.000000f, 0.935054f);


glm::vec3 spaceshipPos = glm::vec3(0.065808f, 1.250000f, -2.189549f);
glm::vec3 spaceshipDir = glm::vec3(-0.490263f, 0.000000f, 0.871578f);
GLuint VAO,VBO;

float aspectRatio = 1.f;

float exposition = 3.f;

glm::vec3 pointlightPos = glm::vec3(0, 3.8, 0);
glm::vec3 pointlightColor = glm::vec3(0.9, 0.6, 0.6);

glm::vec3 spotlightPos = glm::vec3(0, 0, 0);
glm::vec3 spotlightConeDir = glm::vec3(0, 0, 0);
glm::vec3 spotlightColor = glm::vec3(0.4, 0.4, 0.9)*3;
float spotlightPhi = 3.14 / 4;

//glm::mat4 lightVP = glm::ortho(-3.f, 2.2f, -2.f, 3.5f, 1.f, 30.0f) * glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));
//glm::mat4 lightVP = glm::ortho(-8.f, 7.2f, -15.f, 8.5f, -3.f, 35.0f) * glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));
glm::mat4 lightVP = glm::ortho(-10.f, 10.2f, -25.f, 20.5f, -4.f, 30.0f) * glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));

float lastTime = -1.f;
float deltaTime = 0.f;

float rectangleVertices[] =
{
	//Cords		//texCoords
	1.0f, -1.0f, 1.0f, 0.0f,
   -1.0f, -1.0f, 0.0f, 0.0f,
   -1.0f,  1.0f, 0.0f, 1.0f,

	1.0f,  1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
   -1.0f,  1.0f, 0.0f, 1.0f

};

unsigned int rectVAO, rectVBO;

void updateDeltaTime(float time) {
	if (lastTime < 0) {
		lastTime = time;
		return;
	}

	deltaTime = time - lastTime;
	if (deltaTime > 0.1) deltaTime = 0.1;
	lastTime = time;
}

void initDepthMap() {
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir,glm::vec3(0.f,1.f,0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide,cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);

	return cameraMatrix;
}

glm::mat4 createPerspectiveMatrix()
{

	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 20.;
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		1,0.,0.,0.,
		0.,aspectRatio,0.,0.,
		0.,0.,(f+n) / (n - f),2*f * n / (n - f),
		0.,0.,-1.,0.,
		});

	
	perspectiveMatrix=glm::transpose(perspectiveMatrix);

	return perspectiveMatrix;
}

void drawObjectPBR(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color, float roughness, float metallic) {
	glUseProgram(program);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	glUniform1f(glGetUniformLocation(program, "exposition"), exposition);

	glUniform1f(glGetUniformLocation(program, "roughness"), roughness);
	glUniform1f(glGetUniformLocation(program, "metallic"), metallic);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glUniformMatrix4fv(glGetUniformLocation(program, "lightVP"), 1, GL_FALSE, (float*)&lightVP);

	glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);

	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUniform3f(glGetUniformLocation(program, "sunDir"), sunDir.x, sunDir.y, sunDir.z);
	glUniform3f(glGetUniformLocation(program, "sunColor"), sunColor.x, sunColor.y, sunColor.z);

	glUniform3f(glGetUniformLocation(program, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(program, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	glUniform3f(glGetUniformLocation(program, "spotlightConeDir"), spotlightConeDir.x, spotlightConeDir.y, spotlightConeDir.z);
	glUniform3f(glGetUniformLocation(program, "spotlightPos"), spotlightPos.x, spotlightPos.y, spotlightPos.z);
	glUniform3f(glGetUniformLocation(program, "spotlightColor"), spotlightColor.x, spotlightColor.y, spotlightColor.z);
	glUniform1f(glGetUniformLocation(program, "spotlightPhi"), spotlightPhi);
	Core::DrawContext(context);
}

void drawObjectDepth(Core::RenderContext& context, glm::mat4 viewProjectionMatrix, glm::mat4 modelMatrix) {
	glUniformMatrix4fv(glGetUniformLocation(programDepth, "viewProjectionMatrix"), 1, GL_FALSE, (float*)&viewProjectionMatrix);
	glUniformMatrix4fv(glGetUniformLocation(programDepth, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	Core::DrawContext(context);
}

void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID) {
	glUseProgram(programTex);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programTex, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programTex, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programTex, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	Core::SetActiveTexture(textureID, "colorTexture", programTex, 0);
	Core::DrawContext(context);
}

void drawSkybox(glm::mat4 modelMatrix) {
	glDisable(GL_DEPTH_TEST);

	glUseProgram(programSkybox);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "transformation"), 1, GL_FALSE, (float*)&transformation);

	Core::SetActiveTexture(texture::skyboxTexture, "skybox", programSkybox, 0);
	Core::DrawContext(models::skyboxContext);

	glEnable(GL_DEPTH_TEST);
}

void renderShadowapSun() {
	// dodać argument który będzie macierzą widoku
	float time = glfwGetTime();
	//uzupelnij o renderowanie glebokosci do tekstury
	glUseProgram(programDepth);

	//ustawianie przestrzeni rysowania 
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	//bindowanie FBO
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	//czyszczenie mapy głębokości 
	glClear(GL_DEPTH_BUFFER_BIT);
	//ustawianie programu
	glUseProgram(programDepth);
	//zmienić ortho na persp
	glm::mat4 viewProjection = lightVP;
	//glm::mat4 viewProjection = createPerspectiveMatrix() * glm::lookAt(sunPos, sunPos - sunDir, glm::vec3(0, 1, 0));
	drawObjectDepth(sphereContext, glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 3) * glm::translate(glm::vec3(4.f, 0, 0)) * glm::scale(glm::vec3(0.3f)), glm::mat4());
	drawObjectDepth(sphereContext,
		glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 3) * glm::translate(glm::vec3(4.f, 0, 0)) * glm::eulerAngleY(time) * glm::translate(glm::vec3(1.f, 0, 0)) * glm::scale(glm::vec3(0.1f)), glm::mat4());

	// Bed
	drawObjectDepth(models::bedContext, viewProjection, glm::mat4());
	drawObjectDepth(models::bedFrameContext, viewProjection, glm::mat4());
	drawObjectDepth(models::bedBlanketContext, viewProjection, glm::mat4());
	drawObjectDepth(models::bedMattressContext, viewProjection, glm::mat4());
	drawObjectDepth(models::bedLegsContext, viewProjection, glm::mat4());

	// Doors
	drawObjectDepth(models::doorsFrameContext, viewProjection, glm::mat4());
	drawObjectDepth(models::doorsPanelContext, viewProjection, glm::mat4());

	// Table
	drawObjectDepth(models::tableContext, viewProjection, glm::mat4());

	// Room
	drawObjectDepth(models::planeContext, viewProjection, glm::mat4());
	drawObjectDepth(models::roomContext, viewProjection, glm::mat4());

	// Windows
	drawObjectDepth(models::windowContext1, viewProjection, glm::mat4());
	drawObjectDepth(models::windowContext2, viewProjection, glm::mat4());
	drawObjectDepth(models::windowContext3, viewProjection, glm::mat4());

	// Skybox
	drawObjectDepth(models::skyboxContext, viewProjection, glm::mat4());

	// Snow globe
	drawObjectDepth(models::snowGlobeContext, viewProjection, glm::mat4());

	// Monitor
	drawObjectDepth(models::monitorContext, viewProjection, glm::mat4());

	drawObjectDepth(models::spaceshipContext, viewProjection, glm::mat4());
	drawObjectDepth(models::sphereContext, viewProjection, glm::mat4());
	drawObjectDepth(models::boxContext, viewProjection, glm::mat4());

	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
	glm::mat4 specshipCameraRotrationMatrix = glm::mat4({
		spaceshipSide.x,spaceshipSide.y,spaceshipSide.z,0,
		spaceshipUp.x,spaceshipUp.y,spaceshipUp.z ,0,
		-spaceshipDir.x,-spaceshipDir.y,-spaceshipDir.z,0,
		0.,0.,0.,1.,
		});


	//drawObjectColor(shipContext,
	//	glm::translate(cameraPos + 1.5 * cameraDir + cameraUp * -0.5f) * inveseCameraRotrationMatrix * glm::eulerAngleY(glm::pi<float>()),
	//	glm::vec3(0.3, 0.3, 0.5)
	//	);
	drawObjectDepth(shipContext,
		viewProjection, glm::translate(spaceshipPos) * specshipCameraRotrationMatrix * glm::eulerAngleY(glm::pi<float>()) * glm::scale(glm::vec3(0.03f)));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
}

void renderToTexture() 
{
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	//The texture we will render to
	glGenTextures(1, &renderedTexture);

	//"bind" newly created texture - all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	//Give an empty image to OpenGL (the last "0")
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	//poor filtering. Needed!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//set "renderedTexture" as our colour attachment #0
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

	//Set the list of draw buffers
	glDrawBuffers(1, DrawBuffers); //"1" is the size of DrawBuffers

	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
}

void drawObjectPBRMirror(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color, float roughness, float metallic) {
	//renderToTexture();
	glUseProgram(framebufferProgram);
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 view = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(framebufferProgram, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(framebufferProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniform3f(glGetUniformLocation(framebufferProgram, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	Core::DrawContext(context);
}


void renderScene(GLFWwindow* window)
{
	glClearColor(0.4f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float time = glfwGetTime();
	updateDeltaTime(time);
	renderShadowapSun();

 	drawSkybox(glm::translate(cameraPos));

	//space lamp
	glUseProgram(programSun);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * glm::translate(pointlightPos) * glm::scale(glm::vec3(0.6));
	glUniformMatrix4fv(glGetUniformLocation(programSun, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniform3f(glGetUniformLocation(programSun, "color"), sunColor.x / 2, sunColor.y / 2, sunColor.z / 2);
	glUniform1f(glGetUniformLocation(programSun, "exposition"), exposition);
	Core::DrawContext(sphereContext);

	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
	glm::mat4 specshipCameraRotrationMatrix = glm::mat4({
		spaceshipSide.x,spaceshipSide.y,spaceshipSide.z,0,
		spaceshipUp.x,spaceshipUp.y,spaceshipUp.z ,0,
		-spaceshipDir.x,-spaceshipDir.y,-spaceshipDir.z,0,
		0.,0.,0.,1.,
		});

	drawObjectPBR(shipContext,
		glm::translate(spaceshipPos) * specshipCameraRotrationMatrix * glm::eulerAngleY(glm::pi<float>()) * glm::scale(glm::vec3(0.03f)),
		glm::vec3(0.3, 0.3, 0.5),
		0.2,1.0
	);

	// Bed
	drawObjectPBR(models::bedFrameContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBR(models::bedBlanketContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBR(models::bedMattressContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBR(models::bedLegsContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	// Room
	/*drawObjectPBR(models::floorContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);*/
	drawObjectTexture(models::floorContext, glm::mat4(), texture::woodPlanks);
	drawObjectPBR(models::roomContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	// Windows
	drawObjectPBR(models::windowContext1, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBR(models::windowContext2, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBR(models::windowContext3, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	// Table
	drawObjectPBR(models::tableContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	/*drawObjectPBRTexture(models::tableContext, glm::mat4(), texture::box, 0.2f, 0.f, 10.0f);*/

	// Snow globe
	drawObjectPBR(models::snowGlobeContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	// Mirror
	drawObjectPBR(models::mirrorFrameContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBRMirror(models::mirrorGlassContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	// Table
	drawObjectPBR(models::tableContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	// Monitor
	drawObjectPBR(models::monitorContext, glm::mat4(), glm::vec3(0.17f, 0.17f, 0.17f), 0.2f, 0.f);

	// Doors
	drawObjectPBR(models::doorsFrameContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);
	drawObjectPBR(models::doorsPanelContext, glm::mat4(), glm::vec3(1.f, 1.f, 1.f), 0.2f, 0.f);

	spotlightPos = spaceshipPos + 0.2 * spaceshipDir;
	spotlightConeDir = spaceshipDir;

	//test depth buffer
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glUseProgram(programTest);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, depthMap);
	//Core::DrawContext(models::testContext);

	glUseProgram(0);
	glfwSwapBuffers(window);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
	WIDTH = width;
	HEIGHT = height;
}
void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	context.initFromAssimpMesh(scene->mMeshes[0]);
}

void loadCubemap(std::vector<std::string> faces) {
	glGenTextures(1, &texture::skyboxTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture::skyboxTexture);

	int w, h;
	unsigned char* image;
	for (unsigned int i = 0; i < 6; i++)
	{
		unsigned char* image = SOIL_load_image(faces[i].c_str(), &w, &h, 0, SOIL_LOAD_RGBA);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image
		);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void init(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	initDepthMap();

	glEnable(GL_DEPTH_TEST);
	programDepth = shaderLoader.CreateProgram("shaders/shader_new1.vert", "shaders/shader_new1.frag");
	program = shaderLoader.CreateProgram("shaders/shader_pbr.vert", "shaders/shader_pbr.frag");
	programTex = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programTest = shaderLoader.CreateProgram("shaders/test.vert", "shaders/test.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_8_sun.vert", "shaders/shader_8_sun.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	framebufferProgram = shaderLoader.CreateProgram("shaders/framebuffer.vert", "shaders/framebuffer.frag");

	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);
	loadModelToContext("./models/cube.obj", models::skyboxContext);

	// Bed
	loadModelToContext("./models/bed/frame.obj", models::bedFrameContext);
	loadModelToContext("./models/bed/blanket.obj", models::bedBlanketContext);
	loadModelToContext("./models/bed/mattress.obj", models::bedMattressContext);
	loadModelToContext("./models/bed/legs.obj", models::bedLegsContext);

	// Windows
	loadModelToContext("./models/window/window.obj", models::windowContext1);
	loadModelToContext("./models/window/window2.obj", models::windowContext2);
	loadModelToContext("./models/window/window3.obj", models::windowContext3);

	// Room
	loadModelToContext("./models/room/floor.obj", models::floorContext);
	loadModelToContext("./models/room/room.obj", models::roomContext);

	// Table
	loadModelToContext("./models/table/table.obj", models::tableContext);

	// Mirror
	loadModelToContext("./models/mirror/frame.obj", models::mirrorFrameContext);
	loadModelToContext("./models/mirror/glass.obj", models::mirrorGlassContext);

	// Table
	loadModelToContext("./models/snow-globe/snow-globe.obj", models::snowGlobeContext);

	// Monitor
	loadModelToContext("./models/monitor/monitor.obj", models::monitorContext);

	// Doors
	loadModelToContext("./models/doors/frame.obj", models::doorsFrameContext);
	loadModelToContext("./models/doors/panel.obj", models::doorsPanelContext);

	//texture::box = Core::LoadTexture("textures/moon.jpg");
	texture::box = Core::LoadTexture("textures/grid.png");
	texture::woodPlanks = Core::LoadTexture("textures/wood_planks_2.jpg");

	std::vector<std::string> faces
	{
		"textures/skybox/right.jpg",
		"textures/skybox/left.jpg",
		"textures/skybox/top.jpg",
		"textures/skybox/bottom.jpg",
		"textures/skybox/front.jpg",
		"textures/skybox/back.jpg"
	};
	loadCubemap(faces);
}

void shutdown(GLFWwindow* window)
{
	shaderLoader.DeleteProgram(program);
}

//obsluga wejscia
void processInput(GLFWwindow* window)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f,1.f,0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.05f * deltaTime * 60;
	float moveSpeed = 0.05f * deltaTime * 60;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		spaceshipPos += spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		spaceshipPos -= spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		spaceshipPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		spaceshipPos -= spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));

	cameraPos = spaceshipPos - 0.5 * spaceshipDir + glm::vec3(0, 1, 0) * 0.2f;
	cameraDir = spaceshipDir;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		exposition -= 0.05;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		exposition += 0.05;

	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		printf("spaceshipPos = glm::vec3(%ff, %ff, %ff);\n", spaceshipPos.x, spaceshipPos.y, spaceshipPos.z);
		printf("spaceshipDir = glm::vec3(%ff, %ff, %ff);\n", spaceshipDir.x, spaceshipDir.y, spaceshipDir.z);
	}

	//cameraDir = glm::normalize(-cameraPos);

}

// funkcja jest glowna petla
void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		renderScene(window);
		glfwPollEvents();
	}
}
//}