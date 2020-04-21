// Assignment 2 Question 1

#include "common.h"
#include "raytracer.h"

#include <iostream>
#define M_PI 3.14159265358979323846264338327950288
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const char *WINDOW_TITLE = "Ray Tracing";
const double FRAME_RATE_MS = 1;

colour3 texture[1<<16]; // big enough for a row of pixels
point3 vertices[2]; // xy+u for start and end of line
GLuint Window;
int vp_width, vp_height;
float drawing_y = 0;

point3 eye;
float d = 1;

// added variables for moving camera
float rotationX, rotationY = 0;
float move_step = 0.5;
float rotate_step = M_PI / 8;
point3 facing, camera_right, camera_up;
void setFacing() {
	facing = point3(-sin(rotationY) * cos(rotationX), sin(rotationX), -cos(rotationY) * cos(rotationX));

	for (int i = 0; i < 3; i++) {
		if (abs(facing[i]) < 1e-5)
			facing[i] = 0;
	}

	float aspect_ratio = (float)vp_width / vp_height;
	float h = d * (float)tan((M_PI * fov) / 180.0 / 2.0);
	float w = h * aspect_ratio;

	camera_right = glm::normalize(glm::cross(point3(-sin(rotationY), 0, -cos(rotationY)), point3(0, 1, 0))) * w;
	camera_up = glm::normalize(glm::cross(camera_right, facing)) * h;
}

// added variable for anti-aliasing
bool antialias = false;

//----------------------------------------------------------------------------

point3 s(int x, int y) {
	return eye + facing + camera_right * (2 * ((x + 0.5f) / vp_width - 0.5f)) + camera_up * (2 * ((y + 0.5f) / vp_height - 0.5f));
}

point3 s_aa(int x, int y, int num) {
	if (num == 0)
		return eye + facing + camera_right * (2 * ((x + 0.25f) / vp_width - 0.5f)) + camera_up * (2 * ((y + 0.25f) / vp_height - 0.5f));
	if (num == 1)
		return eye + facing + camera_right * (2 * ((x + 0.75f) / vp_width - 0.5f)) + camera_up * (2 * ((y + 0.25f) / vp_height - 0.5f));
	if (num == 2)
		return eye + facing + camera_right * (2 * ((x + 0.25f) / vp_width - 0.5f)) + camera_up * (2 * ((y + 0.75f) / vp_height - 0.5f));
	else
		return eye + facing + camera_right * (2 * ((x + 0.75f) / vp_width - 0.5f)) + camera_up * (2 * ((y + 0.75f) / vp_height - 0.5f));
}

//----------------------------------------------------------------------------

// OpenGL initialization
void init(char *fn) {
	choose_scene(fn);
   
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW );

	// Load shaders and use the resulting shader program
	GLuint program = InitShader( "v.glsl", "f.glsl" );
	glUseProgram( program );

	// set up vertex arrays
	GLuint vPos = glGetAttribLocation( program, "vPos" );
	glEnableVertexAttribArray( vPos );
	glVertexAttribPointer( vPos, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	Window = glGetUniformLocation( program, "Window" );

	// glClearColor( background_colour[0], background_colour[1], background_colour[2], 1 );
	glClearColor( 0.7, 0.7, 0.8, 1 );

	// set up a 1D texture for each scanline of output
	GLuint textureID;
	glGenTextures( 1, &textureID );
	glBindTexture( GL_TEXTURE_1D, textureID );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
}

//----------------------------------------------------------------------------

void display( void ) {
	// draw one scanline at a time, to each buffer; only clear when we draw the first scanline
	// (when fract(drawing_y) == 0.0, draw one buffer, when it is 0.5 draw the other)
	
	if (drawing_y <= 0.5) {
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glFlush();
		glFinish();
		glutSwapBuffers();

		setFacing();

		drawing_y += 0.5;

	} else if (drawing_y >= 1.0 && drawing_y <= vp_height + 0.5) {
		//int y = int(drawing_y) - 1;
		// draw every 16th row, making 16 passes to fill the screen
		int y = ((int(drawing_y) - 1) * 16 % vp_height) + (((int(drawing_y) - 1) * 16) / vp_height) * 7 % 16;

		// only recalculate if this is a new scanline
		if (drawing_y == int(drawing_y)) {

			for (int x = 0; x < vp_width; x++) {
				if (antialias) {
					colour3 totalColour(0.0, 0.0, 0.0);
					for (int i = 0; i < 4; i++) {
						colour3 colour;
						if (!trace(eye, s_aa(x, y, i), colour, false)) {
							colour = background_colour;
						}
						totalColour += colour;
					}
					texture[x] = totalColour / 4.0f;
				}
				else {
					if (!trace(eye, s(x, y), texture[x], false)) {
						texture[x] = background_colour;
					}
				}
			}

			// to ensure a power-of-two texture, get the next highest power of two
			// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
			unsigned int v; // compute the next highest power of 2 of 32-bit v
			v = vp_width;
			v--;
			v |= v >> 1;
			v |= v >> 2;
			v |= v >> 4;
			v |= v >> 8;
			v |= v >> 16;
			v++;
			
			glTexImage1D( GL_TEXTURE_1D, 0, GL_RGB, v, 0, GL_RGB, GL_FLOAT, texture );
			vertices[0] = point3(0, y, 0);
			vertices[1] = point3(v, y, 1);
			glBufferSubData( GL_ARRAY_BUFFER, 0, 2 * sizeof(point3), vertices);
		}

		glDrawArrays( GL_LINES, 0, 2 );
		
		glFlush();
		glFinish();
		glutSwapBuffers();
		
		drawing_y += 0.5;
	}
}

//----------------------------------------------------------------------------

void keyboard( unsigned char key, int x, int y ) {
	switch( key ) {
	case 033: // Escape Key
		exit( EXIT_SUCCESS );
		break;
	case ' ':
		drawing_y = 1;
		break;

	// camera controls
	case 'w':
		eye += facing * move_step;
		drawing_y = 0;
		break;
	case 's':
		eye -= facing * move_step;
		drawing_y = 0;
		break;
	case 'a':
		eye -= glm::normalize(camera_right) * move_step;
		drawing_y = 0;
		break;
	case 'd':
		eye += glm::normalize(camera_right) * move_step;
		drawing_y = 0;
		break;
	case 'q':
		rotationY += rotate_step;
		drawing_y = 0;
		break;
	case 'e':
		rotationY -= rotate_step;
		drawing_y = 0;
		break;
	case 'r':
		if (rotationX < M_PI / 2) {
			rotationX += rotate_step;
			drawing_y = 0;
		}
		break;
	case 'f':
		if (rotationX > -M_PI / 2) {
			rotationX -= rotate_step;
			drawing_y = 0;
		}
		break;
	case 't':
		eye += point3(0, move_step, 0);
		drawing_y = 0;
		break;
	case 'g':
		eye += point3(0, -move_step, 0);
		drawing_y = 0;
		break;
	case 'p':
		std::cout << "camera: (" << eye.x << ", " << eye.y << ", " << eye.z << ")" << std::endl;
		std::cout << "facing: <" << facing.x << ", " << facing.y << ", " << facing.z << ">" << std::endl << std::endl;
		break;
	// anti-aliasing toggle
	case 'l':
		antialias = !antialias;
		if (antialias)
			std::cout << "Anti-aliasing ON" << std::endl;
		else
			std::cout << "Anti-aliasing OFF" << std::endl;
		drawing_y = 0;
		break;
	}
}

//----------------------------------------------------------------------------

void mouse( int button, int state, int x, int y ) {
	y = vp_height - y - 1;
	if ( state == GLUT_DOWN ) {
		switch( button ) {
		case GLUT_LEFT_BUTTON:
			colour3 c;
			point3 uvw = s(x, y);
			std::cout << std::endl;
			if (trace(eye, uvw, c, true)) {
				std::cout << "HIT @ ( " << uvw.x << "," << uvw.y << "," << uvw.z << " )\n";
				std::cout << "      colour = ( " << c.r << "," << c.g << "," << c.b << " )\n";
			} else {
				std::cout << "MISS @ ( " << uvw.x << "," << uvw.y << "," << uvw.z << " )\n";
			}
			std::cout << std::endl;
			break;
		}
	}
}

//----------------------------------------------------------------------------

void update( void ) {
}

//----------------------------------------------------------------------------

void reshape( int width, int height ) {
	glViewport( 0, 0, width, height );

	// GLfloat aspect = GLfloat(width)/height;
	// glm::mat4  projection = glm::ortho( -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f );
	// glUniformMatrix4fv( Projection, 1, GL_FALSE, glm::value_ptr(projection) );
	vp_width = width;
	vp_height = height;
	glUniform2f( Window, width, height );
	drawing_y = 0;
}
