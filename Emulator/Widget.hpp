#pragma once

#include "glad/glad.h"


class Widget {
public:
	struct Vertex {
		float x, y;
	};

	// bounding box
	float x1;
	float y1;
	float x2;
	float y2;
	
	Widget(const char *fragmentShaderSource);

	virtual ~Widget();

	/**
	 * check if widget contains the given point
	 */
	bool contains(float x, float y);

	/**
	 * called when the widget is "touched" at given position (mouse is pressed/dragged)
	 */
	virtual void touch(bool first, float x, float y);

	/**
	 * called when the widget is released from a "touch"
	 */
	virtual void release();

	/**
	 * draw component
	 */
	virtual void draw();
	
	void setRect(float x, float y, float w, float h) {
		this->x1 = x;
		this->y1 = y;
		this->x2 = x + w;
		this->y2 = y + h;
	}
	
protected:
	GLuint getUniformLocation(const char *name);
	void setTextureIndex(const char *name, int index);
	
	struct Matrix4x4 {
		float m[16];
	};
	
	// derived classes can modify the matrix, e.g add rotation
	virtual Matrix4x4 getMatrix();
	
	// derived classes can set additional render state
	virtual void setState();

	// derived classes can reset additional render state
	virtual void resetState();
	
	static GLuint createShader(GLenum type, const char* code);

	GLuint program;
	//GLuint scaleLocation;
	//GLuint offsetLocation;
	GLuint matLocation;
	GLuint vertexBuffer;
	GLuint vertexArray;

	static const Vertex quad[6];
};
