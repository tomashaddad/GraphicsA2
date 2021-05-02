#include "GlutHeaders.h"
#include "GameManager.h"

#include <iostream>
#include <memory>

// Global game manager pointer
std::unique_ptr<GameManager> game = std::make_unique<GameManager>();

void initGlut(int argc, char** argv);
void initCallbacks();
void initFeatures();

// Callback functions
void reshape_callback(int w, int h);
void keyboard_down_callback(unsigned char key, int x, int y);
void keyboard_up_callback(unsigned char key, int x, int y);
void mouseclick_callback(int button, int state, int x, int y);
void mousedrag_callback(int x, int y);
void display_callback();
void idle_callback();

int main(int argc, char** argv)
{
	initGlut(argc, argv);
	initCallbacks();
	initFeatures();

	game->startGameLoop();

	return EXIT_SUCCESS;
}

void initGlut(int argc, char** argv) {
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("Asteroid Arena");
	//glutFullScreen();
	glutIgnoreKeyRepeat(GLUT_KEY_REPEAT_OFF);
}

void initCallbacks() {
	glutReshapeFunc(reshape_callback);
	glutKeyboardFunc(keyboard_down_callback);
	glutKeyboardUpFunc(keyboard_up_callback);
	glutMouseFunc(mouseclick_callback);
	glutMotionFunc(mousedrag_callback);
	glutDisplayFunc(display_callback);
	glutIdleFunc(idle_callback);
}

void initFeatures() {
	//glShadeModel(GL_SMOOTH);

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0, 0, 0, 0);
}

void reshape_callback(int w, int h) {
	game->onReshape(w, h);
}

void keyboard_down_callback(unsigned char key, int x, int y) {
	game->onKeyDown(key, x, y);
}

void keyboard_up_callback(unsigned char key, int x, int y) {
	game->onKeyUp(key, x, y);
}

void mouseclick_callback(int button, int state, int x, int y) {
	game->onMouseClick(button, state, x, y);
}

void mousedrag_callback(int x, int y) {
	game->onMouseClickDrag(x, y);
}

void display_callback() {
	game->onDisplay();
}

void idle_callback() {
	game->calculateTimeDelta();
	game->handleKeyboardInput();
	game->handleMouseInput();
}