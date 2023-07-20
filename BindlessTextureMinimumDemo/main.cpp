#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <vector>

#define BINDING_POINT_TEXTURE_SSBO 2

const char* vertexShaderSource = R"(
#version 460
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(pos, 1.0);
    TexCoords = texCoords;
}
)";

const char* fragmentShaderSource = R"(
#version 460
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader5 : require 
#extension GL_ARB_gpu_shader_int64 : require

#define texture_handle_t uint64_t

#define BINDING_POINT_TEXTURE_SSBO 2

layout (std430, binding = BINDING_POINT_TEXTURE_SSBO) coherent buffer TextureBlock
{
	texture_handle_t TextureSSBO[];
};

layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

texture_handle_t GetCurrentTextureHandle()
{
	return TextureSSBO[4];
}

void main()
{
	texture_handle_t handle = GetCurrentTextureHandle();
	uvec2 uvechandle = uvec2(uint(handle), uint(handle >> 32));
	sampler2D texSampler = sampler2D(uvechandle);
    FragColor = texture(texSampler, TexCoords);
}
)";

// Function for compiling a shader
GLuint compileShader(const char* shaderSource, GLenum shaderType)
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
	}

	return shader;
}


int main()
{
	// Initialize GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Create a windowed mode window and its OpenGL context
	GLFWwindow* window = glfwCreateWindow(800, 600, "Bindless Texture Example", NULL, NULL);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	// Check if GL_ARB_bindless_texture is supported
	if (!glewIsSupported("GL_ARB_bindless_texture"))
	{
		std::cerr << "GL_ARB_bindless_texture not supported" << std::endl;
		return -1;
	}

	// Check if GL_ARB_gpu_shader_int64 is supported
	if (!glewIsSupported("GL_ARB_gpu_shader_int64"))
	{
		std::cerr << "GL_ARB_gpu_shader_int64 not supported" << std::endl;
		return -1;
	}

	// Vertex data
	GLfloat vertices[] = {
		// Positions          // Texture Coords
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // Bottom Left
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // Bottom Right
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // Top Right
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // Top Left 
	};

	GLuint indices[] = {
		0, 1, 2, // First Triangle
		2, 3, 0  // Second Triangle
	};


	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0); // Unbind VAO

	// Create and compile shaders, create program, etc...
	// Compile the vertex shader
	GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);

	// Compile the fragment shader
	GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

	// Link shaders
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		std::cerr << "Program linking error:\n" << infoLog << std::endl;
	}

	// Delete the shaders as they're linked into program now and no longer necessery
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	const char* textureName[] = { "Aatrox.png", "Ahri.png" , "Akali.png" , "Akshan.png" , "Yuumi.png" };
	std::vector<GLuint64> textureHandles;

	for (int i = 0; i < _countof(textureName); ++i)
	{
		// Load texture using stb_image
		int width, height, nrChannels;
		unsigned char* data = stbi_load(textureName[i], &width, &height, &nrChannels, 0);
		if (!data)
		{
			std::cerr << "Failed to load texture" << std::endl;
			return -1;
		}

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		// Set texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Create texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free image data
		stbi_image_free(data);

		GLuint64 textureHandle = glGetTextureHandleARB(texture);
		if (!glIsTextureHandleResidentARB(textureHandle))
		{
			glMakeTextureHandleResidentARB(textureHandle);
		}
		textureHandles.emplace_back(textureHandle);
	}

	GLuint TextureSSBO;
	glGenBuffers(1, &TextureSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, TextureSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * textureHandles.size(), textureHandles.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// Use our shader
		glUseProgram(program);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT_TEXTURE_SSBO, TextureSSBO);

		// Draw the quad
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup and exit
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(program);

	glfwTerminate();

	return 0;
}