//GAME CLIENT
/*
////////////////////////////////////

Jessica Le - 100555079
Gil Robern - 100651824

////////////////////////////////////
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>

#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> 

///// Networking //////
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")

#define MSG_SIZE 512

GLFWwindow* window;

float UPDATE_INTERVAL = 100.0f;

unsigned char* image;
int width, height; 

void loadImage() {
	int channels;
	stbi_set_flip_vertically_on_load(true);
	image = stbi_load("box.jpg",
		&width,
		&height,
		&channels,
		0);
	if (image) {
		std::cout << "Image LOADED" << width << " " << height << std::endl;
	}
	else {
		std::cout << "Failed to load image!" << std::endl;
	}

}

bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		std::cout << "Failed to Initialize GLFW" << std::endl;
		return false;
	}

	//Create a new GLFW window
	window = glfwCreateWindow(800, 800, "Window", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	return true;
}

bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		std::cout << "Failed to initialize Glad" << std::endl;
		return false;
	}
}
GLuint shader_program;

bool loadShaders() {
	// Read Shaders from file
	std::string vert_shader_str;
	std::ifstream vs_stream("vertex_shader.glsl", std::ios::in);
	if (vs_stream.is_open()) {
		std::string Line = "";
		while (getline(vs_stream, Line))
			vert_shader_str += "\n" + Line;
		vs_stream.close();
	}
	else {
		printf("Could not open vertex shader!!\n");
		return false;
	}
	const char* vs_str = vert_shader_str.c_str();

	std::string frag_shader_str;
	std::ifstream fs_stream("frag_shader.glsl", std::ios::in);
	if (fs_stream.is_open()) {
		std::string Line = "";
		while (getline(fs_stream, Line))
			frag_shader_str += "\n" + Line;
		fs_stream.close();
	}
	else {
		printf("Could not open fragment shader!!\n");
		return false;
	}
	const char* fs_str = frag_shader_str.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_str, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_str, NULL);
	glCompileShader(fs);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);

	return true;
}

//INPUT handling
float tx = 0.0001f;
float ty = 0.0001f;
float temptx = 0.0001f;
float tempty = 0.0001f;
float tx2 = 0.0f;
float ty2 = 0.0f;
float deadX = 0.0f;
float deadY = 0.0f;
GLuint filter_mode = GL_LINEAR;
bool keyEnter = false;

void keyboard() {
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		ty += 0.1;
		keyEnter = true;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		ty -= 0.1;
		keyEnter = true;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		tx += 0.1;
		keyEnter = true;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		tx -= 0.1;
		keyEnter = true;
	}
	

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		UPDATE_INTERVAL += 25;
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		UPDATE_INTERVAL -= 25;
		if (UPDATE_INTERVAL <= 0.0f)
		{
			UPDATE_INTERVAL = 25.0f;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		if (filter_mode == GL_LINEAR) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			filter_mode = GL_NEAREST;
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			filter_mode = GL_LINEAR;
		}
	}



}

//PREDICT MOVEMENT FOR THE OTHER BOX
void DeadReckoning(float xi, float yi, float xf, float yf)
{
	
	//P=pk + v*t
	//pk is last known position (have to do this for both x and y)
	//v is velocity, the change
	//t is a constnat: 500MS

	float velocity = (xf - xi) / (yf - yi);
	//std::cout << "vel:" << velocity;
	float t = 0.0005L; //500MS constant
	//std::cout << "t:" << t << std::endl;
	tx2 = xi + velocity * t;
	ty2 = yi + velocity * t;
	//std::cout << "xf:" << tx2 << "xy" << ty2 << std::endl;

}
int main()
{
	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	WSADATA WsaData;
	
	//intalize out winsock
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0 )
	{
		std::cout << "WSA failed! \n";
		WSACleanup();
		return 0;
	}


	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //it is TCP
	if (Socket == INVALID_SOCKET)
	{
		std::cout << "Socket creation error\n";
		WSACleanup();
		return 0;
	}

	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons(8888);
	SockAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &SockAddr.sin_addr.s_addr);

	if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
	{
		std::cout << "Failed to connect to server\n";
		return 0;
	}

	//send and recieve infromation so we nee 2 buffers

	//can also use memsize of strcpy
	char recBuf[MSG_SIZE] = { "" };
	char sendBuf[MSG_SIZE] = { "" };

	//change to non blocking mode
	u_long mode = 1;
	ioctlsocket(Socket, FIONBIO, &mode);
	// Cube data
	static const GLfloat points[] = {//front face, 2 triangles
		-0.5f, -0.5f, 0.5f,//0  front face
		0.5f, -0.5f, 0.5f, //3
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3
		0.5f, 0.5f, 0.5f, //2
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3 Right face
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, 0.5f, //2
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, -0.5f, //6
		0.5f, 0.5f, 0.5f,  //2
		-0.5f, -0.5f, -0.5f, //4 Left face
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, 0.5f,  //1
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, 0.5f, 0.5f,  //1 Top face
		0.5f, 0.5f, 0.5f,  //2
		-0.5f, 0.5f, -0.5f,//5
		0.5f, 0.5f, 0.5f,   //2
		0.5f, 0.5f, -0.5f, //6
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, -0.5f, //4 Bottom face
		0.5f, -0.5f, -0.5f, //7
		-0.5f, -0.5f, 0.5f, //0
		0.5f, -0.5f, -0.5f, //7
		0.5f, -0.5f, 0.5f, //3
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, -0.5f, //5 Back face
		0.5f, 0.5f, -0.5f, //6
		-0.5f, -0.5f, -0.5f, //4
		0.5f, 0.5f, -0.5f, //6
		0.5f, -0.5f, -0.5f, //7
		-0.5f, -0.5f, -0.5f, //4
	};

	// Color data
	static const GLfloat colors[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f
	};

	///////// TEXTURES ///////
	static const GLfloat uv[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};


	// Cube 2
	static const GLfloat points2[] = {//front face, 2 triangles
		-0.5f, -0.5f, 0.5f,//0  front face
		0.5f, -0.5f, 0.5f, //3
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3
		0.5f, 0.5f, 0.5f, //2
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3 Right face
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, 0.5f, //2
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, -0.5f, //6
		0.5f, 0.5f, 0.5f,  //2
		-0.5f, -0.5f, -0.5f, //4 Left face
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, 0.5f,  //1
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, 0.5f, 0.5f,  //1 Top face
		0.5f, 0.5f, 0.5f,  //2
		-0.5f, 0.5f, -0.5f,//5
		0.5f, 0.5f, 0.5f,   //2
		0.5f, 0.5f, -0.5f, //6
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, -0.5f, //4 Bottom face
		0.5f, -0.5f, -0.5f, //7
		-0.5f, -0.5f, 0.5f, //0
		0.5f, -0.5f, -0.5f, //7
		0.5f, -0.5f, 0.5f, //3
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, -0.5f, //5 Back face
		0.5f, 0.5f, -0.5f, //6
		-0.5f, -0.5f, -0.5f, //4
		0.5f, 0.5f, -0.5f, //6
		0.5f, -0.5f, -0.5f, //7
		-0.5f, -0.5f, -0.5f, //4
	};

	// Color data
	static const GLfloat colors2[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f
	};

	///////// TEXTURES ///////
	static const GLfloat uv2[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	//VBO
	GLuint pos_vbo = 0;
	glGenBuffers(1, &pos_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

	GLuint color_vbo = 1;
	glGenBuffers(1, &color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

	//VBO2
	GLuint pos_vbo2 = 0;
	glGenBuffers(1, &pos_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points2), points2, GL_STATIC_DRAW);

	GLuint color_vbo2 = 1;
	glGenBuffers(1, &color_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, color_vbo2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors2), colors2, GL_STATIC_DRAW);


	// VAO
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	GLuint uv_vbo = 2;
	glGenBuffers(1, &uv_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, uv_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, uv_vbo);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	loadImage();

	GLuint textureHandle;

	glGenTextures(1, &textureHandle);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Release the space used for your image once you're done
	stbi_image_free(image);

	//Cube2
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, color_vbo2);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

	GLuint uv_vbo2 = 5;
	glGenBuffers(1, &uv_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, uv_vbo2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv2), uv2, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, uv_vbo2);

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	loadImage();

	GLuint textureHandle2;

	glGenTextures(1, &textureHandle2);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureHandle2);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Release the space used for your image once you're done
	stbi_image_free(image);

	// Load your shaders
	if (!loadShaders())
		return 1;

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	glm::mat4 Projection =
		glm::perspective(glm::radians(45.0f),
		(float)width / (float)height, 0.0001f, 100.0f);

	// Camera matrix
	glm::mat4 View = glm::lookAt(
		glm::vec3(0, 0, 3), // Camera is at (0,0,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::mat4(1.0f);
	// create individual matrices glm::mat4 T R and S, then multiply them
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

	///Model 2///
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model2 = glm::mat4(1.0f);
	// create individual matrices glm::mat4 T R and S, then multiply them
	Model2 = glm::translate(Model2, glm::vec3(0.0f, 0.0f, 0.0f));


	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 mvp2 = Projection * View * Model2; // Remember, matrix multiplication is the other way around


	// Get a handle for our "MVP" uniform
	// Only during initialisation
	GLuint MatrixID =
		glGetUniformLocation(shader_program, "MVP");

	GLuint MatrixID2 =
		glGetUniformLocation(shader_program, "MVP");

	// Face culling
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);


	/////// TEXTURE
	glUniform1i(glGetUniformLocation(shader_program, "myTextureSampler"), 0);

	// Timer variables for sending network updates
	float time = 0.0;
	float previous = glfwGetTime();


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		//clear sendBuf
		strcpy(sendBuf, "");

		//create a message of our positions
		std::string msg = std::to_string(tx) + "@" + std::to_string(ty);
		
		//put our message into sendBuf so we can send it to the server
		strcpy(sendBuf, (char*)msg.c_str());
	
		Sleep(UPDATE_INTERVAL);
		

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);

		Model = glm::mat4(1.0f);
		Model2 = glm::mat4(1.0f);


		Model = glm::translate(Model, glm::vec3(tx, ty, -2.0f));
		mvp = Projection * View * Model;
		
		Model2 = glm::translate(Model2, glm::vec3(tx2, ty2, -2.0f)); //Updates position values
		mvp2 = Projection * View * Model2;

		glBindVertexArray(vao);

		glUniformMatrix4fv(MatrixID, 1,
			GL_FALSE, &mvp[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, 36);


		glUniformMatrix4fv(MatrixID2, 1,
			GL_FALSE, &mvp2[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);

		//getting the most recent positions
		keyboard();
		
		//tx and ty is getting positions from the keyboard inputs
		//temptx, tempty is getting last positions (from the server)
		DeadReckoning(temptx, tempty, tx, ty);
		
		//RECIEVE STUFF FROM SERVER
		if (recv(Socket, recBuf, sizeof(recBuf), 0) > 0)
		{
			std::cout << "RECEIVED: " <<recBuf << std::endl;
			
			std::string tmp = recBuf;
			std::size_t pos = tmp.find("@"); //creates a position to separate data
			tmp = tmp.substr(0, pos - 1); //Pos - 1 is tx
			temptx = std::strtof((tmp).c_str(), 0); //Convert to float
			tmp = recBuf; //Reset the temp variable 
			tmp = tmp.substr(pos + 1); //pos + 1 is ty
			tempty = std::strtof((tmp).c_str(), 0);
		
		}

		//ONLY SEND INFO WHEN POSITION HAS CHANGED
		if (keyEnter == true)
		{
			std::cout << "SENT:" << sendBuf << std::endl;
			if (send(Socket, sendBuf, sizeof(sendBuf), 0) == 0)
			{
				std::cout << "Failed to send!\n";
				closesocket(Socket);
				WSACleanup();
				break;
			}
		}

		keyEnter = false;
	}//end while

	shutdown(Socket, SD_BOTH);
	closesocket(Socket);
	WSACleanup();
	return 1; 
	 
}
