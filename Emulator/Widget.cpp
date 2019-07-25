#include "Widget.hpp"
#include <vector>
#include <stdexcept>


Widget::Widget(const char* fragmentShaderSource) {
	// create vertex shader
	GLuint vertexShader = createShader(GL_VERTEX_SHADER,
		"#version 330\n"
		"uniform mat4 mat;\n"
		"in vec2 position;\n"
		"out vec2 uv;\n"
		"void main() {\n"
		"	gl_Position = mat * vec4(position, 0.0, 1.0);\n"
		"	uv = position;\n"
		"}\n");
	
	// create fragment shader
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
	
	// create and link program
	this->program = glCreateProgram();
	glAttachShader(this->program, vertexShader);
	glAttachShader(this->program, fragmentShader);
	glLinkProgram(this->program);

	// get uniform locations
	this->matLocation = getUniformLocation("mat");

	// create vertex buffer containing a quad
	glGenBuffers(1, &this->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	// create vertex array objct
	GLuint positionLocation = glGetAttribLocation(this->program, "position");
	glGenVertexArrays(1, &this->vertexArray);
	glBindVertexArray(this->vertexArray);
	glEnableVertexAttribArray(positionLocation);
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

Widget::~Widget() {
}

bool Widget::contains(float x, float y) {
	return x >= this->x1 && x <= this->x2 && y >= this->y1 && y <= this->y2;
}

void Widget::touch(bool first, float x, float y) {
}

void Widget::draw() {
	// use program
	glUseProgram(this->program);
	
	// set uniforms
	Matrix4x4 mat = getMatrix();
	glUniformMatrix4fv(this->matLocation, 1, true, mat.m);
	
	// set vertex array
	glBindVertexArray(this->vertexArray);

	// set additional render state, e.g. uniforms or textures
	setState();
	
	// draw
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	// reset additional render state
	resetState();
	
	// reset
	glBindVertexArray(0);
}

Widget::Matrix4x4 Widget::getMatrix() {
	Matrix4x4 mat = {{
		(this->x2 - this->x1) * 2.0f, 0, 0, this->x1 * 2.0f - 1.0f,
		0, (this->y2 - this->y1) * 2.0f, 0, this->y1 * 2.0f - 1.0f,
		0, 0, 0, 0,
		0, 0, 0, 1}};
	return mat;
}

void Widget::setState() {
}

void Widget::resetState() {
}

GLuint Widget::createShader(GLenum type, const char* code) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);
	
	// check status
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		// get length of log (including trailing null character)
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	
		// get log
		std::vector<char> errorLog(length);
		glGetShaderInfoLog(shader, length, &length, (GLchar*)errorLog.data());
		
		throw std::runtime_error(errorLog.data());
	}
	return shader;
}

GLuint Widget::getUniformLocation(const char *name) {
	return glGetUniformLocation(this->program, name);
}

void Widget::setTextureIndex(const char *name, int index) {
	glUseProgram(this->program);
	glUniform1i(glGetUniformLocation(this->program, name), index);
	glUseProgram(0);
}

Widget::Vertex const Widget::quad[6] = {
	{0.0f, 0.0f},
	{1.0f, 0.0f},
	{1.0f, 1.0f},
	{0.0f, 0.0f},
	{1.0f, 1.0f},
	{0.0f, 1.0f},
};
