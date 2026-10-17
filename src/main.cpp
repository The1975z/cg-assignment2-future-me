#undef GLFW_DLL

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <vector>
#include <iostream>

#include "Libs/Mesh.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>

const GLint WIDTH = 800;
const GLint HEIGHT = 600;

std::vector<Mesh*> meshList;
std::vector<Shader*> shaderList;

// initial yaw = -90 bc 0 = right
float yaw = -90.0f;
float pitch = 0.0f;

const std::filesystem::path shaderDirectory =
	std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);
const std::filesystem::path textureDirectory =
	std::filesystem::u8path(OPENGL_STARTER_TEXTURE_DIR);
const std::filesystem::path modelDirectory =
	std::filesystem::u8path(OPENGL_STARTER_MODEL_DIR);
void CreateTriangle()
{
	GLfloat vertices[] =
	{
		// x, y, z				u, v
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		 0.0f, -1.0f, 1.0f,		0.5f, 0.0f,
		 1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		 0.0f, 1.0f, 0.0f,		0.5f, 1.0f	
	};

	unsigned int indices[] =
	{
		0, 3, 1, 
		1, 3, 2,
		2, 3, 0,
		0, 1, 2
	};

	Mesh* obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, 20, 12);
	for (int i = 0; i < 10; i++)
	{
		meshList.push_back(obj1);
	}
}

void CreateOBJ(const std::filesystem::path& modelPath){
	Mesh *obj = new Mesh();
	bool loaded = obj->CreateMeshFromOBJ(modelPath.u8string().c_str());
	if(!loaded){
		std::cerr << "Failed to load model :" << modelPath << std::endl;
		return;
	}

	meshList.push_back(obj);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(
		shaderDirectory / "shader.vert",
		shaderDirectory / "shader.frag"
	);
	shaderList.push_back(shader1);
}

GLuint LoadTexture(const std::filesystem::path& texturePath)
{
	int width = 0;
	int height = 0;
	int channels = 0;

	unsigned char * data = stbi_load(texturePath.u8string().c_str(),
									&width, &height, &channels, 0);

	if (data == nullptr)
	{
		std::cerr << "Failed to load texture." << stbi_failure_reason() << "\n";
		return 0;
	}

	GLenum format = GL_RGB;

	switch (channels)
	{
		case 4: format = GL_RGBA; // 4 bytes
			break;
		case 3: format = GL_RGB;
			break;
		case 2: format = GL_RG;
			break;
		case 1: format = GL_RED;
			break;
		default:
			std::cerr << "Unsupported texture format.\n";
			stbi_image_free(data);
			return 0;
	}

	// GL_TEXTURE_2D : similar to whiteboard for texture infos

	GLuint texture = 0; // memory address
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // s-axis wrap-around
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // t-axis wrap_around
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // magnify texture using linear filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // minification

	// Allow image rows whose byte width is not a multiple of four.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

void Cleanup()
{
	if (!meshList.empty() && meshList[0] != nullptr)
	{
		delete meshList[0];
	}
	meshList.clear();

	for (Shader* shader : shaderList)
		delete shader;
	shaderList.clear();
}

void updateMouseLook(Window &window)
{
	double mouseX, mouseY;
	glfwGetCursorPos(window.getWindow(), &mouseX, &mouseY);

	// pull first mouseX
	static double lastX = mouseX;
	static double lastY = mouseY;

	float xOffset = static_cast<float> (mouseX - lastX);
	float yOffset = static_cast<float> (mouseY - lastY);

	float sensitivity = 0.1f;

	xOffset *= sensitivity;
	yOffset *= sensitivity;

	// update each loop
	lastX = mouseX;
	lastY = mouseY;

	yaw += xOffset;
	// picth(+) เงย ทำให้ cursorY ตรงกันข้าม 
	// picth(-) ก้ม
	pitch -= yOffset;

	// limit character head up-down
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

int main()
{
	Window mainWindow(WIDTH, HEIGHT, 3, 3);
	if (mainWindow.initialise() != 0)
		return 1;

	//CreateTriangle();
	CreateOBJ(modelDirectory / "suzanne.obj");
	CreateShaders();

	GLuint texture_data = LoadTexture(textureDirectory / "uvmap.png");

	if (texture_data == 0)
	{
		glDeleteTextures(1, &texture_data);
		return 1;
	}

	GLint uniformModel = shaderList[0]->GetUniformLocation("model");
	GLint uniformView = shaderList[0]->GetUniformLocation("view");
	GLint uniformProjection = shaderList[0]->GetUniformLocation("projection");
	GLint uniformTextureData = shaderList[0]->GetUniformLocation("texture_data");
	GLint uniformLightColour = shaderList[0]->GetUniformLocation("lightColour");
	GLint uniformLightPos = shaderList[0]->GetUniformLocation("lightPos");
	GLint uniformViewPos = shaderList[0]->GetUniformLocation("viewPos");

	glEnable(GL_DEPTH_TEST);

	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		(GLfloat)mainWindow.getBufferWidth() / (GLfloat)mainWindow.getBufferHeight(),
		0.1f,
		100.0f
	);

	// z=5 ถอยหลัง 
	glm::vec3 cameraPos = glm::vec3 (0.0f, 0.0f, 5.0f);
	glm::vec3 lightPos = glm::vec3(5.0f, 5.0f, 0.0f);
	glm::vec3 lightColour = glm::vec3(0.0f, 1.0f, 1.0f);
	// มองไปที่ -z
	glm::vec3 cameraTarget = glm::vec3 (0.0f, 0.0f, -1.0f);
	glm::vec3 cameraDirection = glm::normalize(cameraTarget - cameraPos);

	cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraDirection.y = sin(glm::radians(pitch));
	cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	glm::vec3 up = glm::vec3 (0.0f, 1.0f, 0.0f);
	// กฎมือขวา เอานิ้วโป้งชี้ไปทางขวา กลาง=Direction ชี้=Up
	glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDirection, up));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraDirection));

	float lastTime = static_cast<float> (glfwGetTime());

	while (!mainWindow.getShouldClose())
	{
		float currentTime = static_cast<float> (glfwGetTime());
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		// poll inputs
		glfwPollEvents();
		updateMouseLook(mainWindow);

		// inputs
		float moveSpeed = 5.0f;
		if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_W) == GLFW_PRESS)
		{
			cameraPos = cameraPos + cameraDirection * moveSpeed * deltaTime;
		}
		if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_S) == GLFW_PRESS)
		{
			cameraPos = cameraPos - cameraDirection * moveSpeed * deltaTime;
		}
		if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_A) == GLFW_PRESS)
		{
			cameraPos = cameraPos - cameraRight * moveSpeed * deltaTime;
		}
		if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_D) == GLFW_PRESS)
		{
			cameraPos = cameraPos + cameraRight * moveSpeed * deltaTime;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderList[0]->UseShader();

		glUniform1i(uniformTextureData, 0);
		glUniform3fv(uniformLightColour, 1, glm::value_ptr(lightColour));
		glUniform3fv(uniformLightPos, 1, glm::value_ptr(lightPos));
		glUniform3fv(uniformViewPos, 1, glm::value_ptr(cameraPos));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_data);
		
		glm::mat4 view (1.0f);
		glm::mat4 cameraRotateMat (1.0f);
		glm::mat4 cameraPosMat (1.0f);

		cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraDirection.y = sin(glm::radians(pitch));
		cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDirection, up));
		glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraDirection));

		// manual marix calculation ================================================================

		// cpp uses [col][row]
		cameraPosMat[3][0] = -cameraPos.x;
		cameraPosMat[3][1] = -cameraPos.y;
		cameraPosMat[3][2] = -cameraPos.z;

		cameraRotateMat[0] = glm::vec4 (cameraRight.x, cameraUp.x, -cameraDirection.x, 0.0f);
		cameraRotateMat[1] = glm::vec4 (cameraRight.y, cameraUp.y, -cameraDirection.y, 0.0f);
		cameraRotateMat[2] = glm::vec4 (cameraRight.z, cameraUp.z, -cameraDirection.z, 0.0f);

		view = cameraRotateMat * cameraPosMat;

		// ==========================================================================================

		// with glm::lookAt
		// view = glm::lookAt(cameraPos, cameraPos + cameraDirection, cameraUp);

		glm::vec3 pyramidPositions[] = {
			glm::vec3(0.0f, 0.0f, -2.5f),
			glm::vec3(2.0f, 5.0f, -15.0f),
			glm::vec3(-1.5f, -2.2f, -2.5f),
			glm::vec3(-3.8f, -2.0f, -12.3f),
			glm::vec3(2.4f, -0.4f, -3.5f),
			glm::vec3(-1.7f, 3.0f, -7.5f),
			glm::vec3(1.3f, -2.0f, -2.5f),
			glm::vec3(1.5f, 2.0f, -2.5f),
			glm::vec3(1.5f, 0.2f, -1.5f),
			glm::vec3(-1.3f, 1.0f, -1.5f)
		};

		for (int i = 0; i < 10; i++)
		{
			glUniform1i(uniformTextureData, 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_data);

			glm::mat4 model(1.0f);

			model = glm::translate(model, pyramidPositions[i]);
			model = glm::rotate(model, glm::radians(2.0f * i), glm::vec3(1.0f, 0.3f, 0.5f));
			model = glm::scale(model, glm::vec3(0.8f, 0.8f, 1.0f));

			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));

			meshList[0]->RenderMesh();
		}
		
		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(0);
		mainWindow.swapBuffers();
	}

	Cleanup();
	return 0;
}
