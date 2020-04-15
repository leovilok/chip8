#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "media.h"

#define PIXEL_RADIUS 16

#define SCREEN_WIDTH (64*PIXEL_RADIUS)
#define SCREEN_HEIGHT (32*PIXEL_RADIUS)

#define WINDOW_NAME "CHIP-8 emulator"

#define NUM_PIXELS (32*64)

#define SOUND_DEV_FREQ 48000
#define SOUND_SAMPLES 1024
#define BUZZER_FREQ 440
#define BUZZER_VOL .05

unsigned char pixels[NUM_PIXELS];

GLFWwindow *window;

const float vertices[] = {
	-1.0f, -1.0f,
	-1.0f, +1.0f,
	+1.0f, -1.0f,
	+1.0f, +1.0f,
};

const char *v_shader_src =
"#version 100\n"
"in vec2 coords;\n"
"varying vec2 tex_coords;\n"
"void main(){\n"
"	gl_Position = vec4(coords.x, coords.y, 0.0, 1.0);\n"
"	tex_coords.x = (1.0 + coords.x)/2.0;\n"
"	tex_coords.y = (1.0 - coords.y)/2.0;\n"
"}";

const char *f_shader_src =
"#version 100\n"
"in lowp vec2 tex_coords;\n"
"uniform sampler2D tex;\n"
"void main(){\n"
"	gl_FragColor = vec4(vec3(texture2D(tex, tex_coords).a), 1.0);\n"
"}";


unsigned short input;


/* sound state*/
int buzzer_state = 0;

/*
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ C │
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │
├───┼───┼───┼───┤
│ A │ 0 │ B │ F │
└───┴───┴───┴───┘
*/

int keys[16] = {
	GLFW_KEY_V,
	GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5,
	GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_T,
	GLFW_KEY_D, GLFW_KEY_F, GLFW_KEY_G,
	GLFW_KEY_C, GLFW_KEY_B,
	GLFW_KEY_6, GLFW_KEY_Y, GLFW_KEY_H, GLFW_KEY_N
};

static void buzzer_callback(void *userdata, unsigned char *stream, int len){
	static long sample_num = 0;
	(void) userdata;
	for(int i=0 ; i<len ; i++){
		if(buzzer_state == 0)
			stream[i] = 0;
		else /* saw waves at the moment */
			stream[i] =
				((sample_num++*256*BUZZER_FREQ/SOUND_DEV_FREQ)
				 %256)*BUZZER_VOL;
	}
}

static void key_callback(GLFWwindow* window, int key, int s, int action, int m){
	(void) s, (void)m, (void) window;
	for (int i=0 ; i<16 ; i++) {
		if (key == keys[i]){
			if (action == GLFW_PRESS)
				input |= 1<<i;
			else if (action == GLFW_RELEASE)
				input ^= 1<<i;
		}
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
	(void) window;
	glViewport(0, 0, width, height);
}

static void check_shader_log(unsigned shader){
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success){
		char log[512];
		glGetShaderInfoLog(shader, 512, NULL, log);
		puts("Shader compile log:");
		puts(log);
	}
}

static void setup_shaders(void){
	/* Create vertex shader */
	unsigned v_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v_shader, 1, &v_shader_src, NULL);
	glCompileShader(v_shader);
	check_shader_log(v_shader);

	/* Create fragment shader */
	unsigned f_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f_shader, 1, &f_shader_src, NULL);
	glCompileShader(f_shader);
	check_shader_log(f_shader);

	/* Create shader program */
	unsigned program = glCreateProgram();
	glAttachShader(program, v_shader);
	glAttachShader(program, f_shader);
	glLinkProgram(program);
	glUseProgram(program);

	/* Remove shaders */
	glDeleteShader(v_shader);
	glDeleteShader(f_shader);

	/* Create vertex buffer */
	unsigned VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Setup vertex input array */
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

static void setup_texture(void){
	unsigned texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 64, 32, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
}

int m_init(int argc, char **argv){
	(void)argc, (void)argv;
	/*TODO: check every init */

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_NAME, NULL, NULL);
	glfwMakeContextCurrent(window);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwSwapInterval(1);
	
	glfwSetKeyCallback(window, key_callback);

	setup_shaders();
	setup_texture();
	
	glClear(GL_COLOR_BUFFER_BIT);

	return 0;
}

void m_quit(void){
	glfwDestroyWindow(window);

	glfwTerminate();
}

unsigned short get_input(unsigned short old_input){
	(void) old_input;
	if (glfwWindowShouldClose(window))
		return -1;

	return input;
}
unsigned short wait_input(unsigned short input){
	int new_input;
	
	while((new_input = get_input(input)) == input)
		frame();

	return new_input;
}

void clear_screen(void){
	memset(pixels,0,NUM_PIXELS);
	glClear(GL_COLOR_BUFFER_BIT);
}

void draw(int x, int y, int value){
	assert(0<=x && x<64);
	assert(0<=y && y<32);

	pixels[y*64+x] = value ? 255 : 0;
}

void frame(void){
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 64, 32, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glfwSwapBuffers(window);
	glfwPollEvents();
}

void set_buzzer_state(int state){
	buzzer_state = state;
}
