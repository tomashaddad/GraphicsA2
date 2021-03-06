#define _USE_MATH_DEFINES
#include <cmath>

#include "GameManager.h"
#include "GlutHeaders.h"
#include "Math/Utility.h"

#include "Transparent/Transparent.h"

#include "Collisions/Collision.h"

#include "Assets/Asset.h"

#include <iostream>
#include <memory>

GameManager::GameManager() :
	dt(0),
	last_time(0),
	ship(std::make_unique<Ship>()),
	keyboard(std::make_unique<Keyboard>()),
	mouse(std::make_unique<Mouse>()),
	window(std::make_unique<Window>()),
	camera(std::make_unique<Camera>()),
	arena(std::make_unique<Arena>()),
	asteroid_field(std::make_unique<AsteroidField>()),
	explosion_manager(std::make_unique<ExplosionManager>()) {}

void GameManager::start() {
	init();
	glutMainLoop();
}

void GameManager::init() {
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	// small amount of ambient light
	float worldAmbient[] = { 0.1, 0.1, 0.1, 1.0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, worldAmbient);

	// directional light source
	float ambient0[] = { 0.0, 0.0, 0.0, 1.0 };
	float diffuse0[] = { 1.0, 1.0, 1.0, 1.0 };
	float specular0[] = { 1.0, 1.0, 1.0, 1.0 };
	float position0[] = { 1.0, 0.0, 0.0, 0.0 };

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glEnable(GL_LIGHT0);

	// satellite light source orbiting arena
	float ambient1[] = { 0.0, 0.0, 0.0, 1.0 };
	float diffuse1[] = { 1.0, 1.0, 1.0, 1.0 };
	float specular1[] = { 1.0, 1.0, 1.0, 1.0 };

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specular1);
	glEnable(GL_LIGHT1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



// Draw everything
void GameManager::onDisplay() {
	updateCamera();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);

	// Update camera and draw skybox
	glLoadIdentity();
	camera->rotate();
	arena->drawSkybox();
	camera->translate();

	float position0[] = { 1.0, 0.0, 0.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, position0);

	// Drawing scene objects
	ship->draw();

	arena->drawArena();
	arena->drawSatellite();
	asteroid_field->drawAsteroids();

	Transparent::drawAll(); // draws bullets and explosions (if any)

	int err;
	while ((err = glGetError()) != GL_NO_ERROR)
		printf("display: %s\n", gluErrorString(err));

	glutSwapBuffers();
}

// Calculations and updates that occur in the background
void GameManager::onIdle() {
	calculateTimeDelta();

	updateEntities();
	handleCollisions();
	
	handleKeyboardInput();
	handleMouseInput();
}

void GameManager::onReshape(const int w, const int h) {
	window->width = w;
	window->height = h;

	const float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);

	camera->setAspect(aspect_ratio);

	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(camera->getFov(), aspect_ratio, camera->getZNear(), camera->getZFar());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// The camera is placed depending on which key in the set {I, J, K, L, M} is pressed:
//	I: The camera moves above the ship and looks below it
//	J: The camera moves to the left of the ship and looks to its right
//	K: The camera moves in front of the ship and looks behind it
//	L: The camera moves to the right of the ship and looks to its left
//	M: The camera moves below the ship and looks above it

void GameManager::updateCamera() {
	Vector3D position;
	Quaternion rotation;

	if (camera->look_at == Look::AHEAD) {
		rotation = ship->getRotation();
		position = ship->getPosition() + camera->distanceFromShip() * (ship->getRotation() * Vector3D::forward());
	}
	else if (camera->look_at == Look::LEFT) {
		rotation = ship->getRotation() * Quaternion(Vector3D::up(), -90);
		position = ship->getPosition() + camera->distanceFromShip() * (ship->getRotation() * Vector3D::right());
	}
	else if (camera->look_at == Look::RIGHT) {
		rotation = ship->getRotation() * Quaternion(Vector3D::up(), 90);
		position = ship->getPosition() - camera->distanceFromShip() * (ship->getRotation() * Vector3D::right());
	}
	else if (camera->look_at == Look::BEHIND) {
		rotation = ship->getRotation() * Quaternion(Vector3D::up(), 180);
		position = ship->getPosition() - camera->distanceFromShip() * (ship->getRotation() * Vector3D::forward());
	}
	else if (camera->look_at == Look::ABOVE) {
		rotation = ship->getRotation() * Quaternion(Vector3D::right(), -90);
		position = ship->getPosition() - camera->distanceFromShip() * (ship->getRotation() * Vector3D::up());
	}
	else if (camera->look_at == Look::BELOW) {
		rotation = ship->getRotation() * Quaternion(Vector3D::right(), 90);
		position = ship->getPosition() + camera->distanceFromShip() * (ship->getRotation() * Vector3D::up());
	}

	// raise the position of the camera slightly up
	position += 10.0f * (ship->getRotation() * Vector3D::up());

	camera->lerpPositionTo(position);
	camera->lerpRotationTo(rotation);

	glutPostRedisplay();
}

void GameManager::updateEntities() {
	Transparent::sort(camera->getPosition()); // sort transparent entities after updating everything
	updateShip();
	updateAsteroids();
	updateBullets();
	updateSatellite();
	updateExplosions();
}

void GameManager::updateShip() {
	ship->update(dt);
}

void GameManager::updateAsteroids() {
	asteroid_field->updateAsteroids(dt);

	if (asteroid_field->isEmpty() || asteroid_field->levellingUp()) {
		// Reset timer if empty to prevent two waves spawning back to back
		if (asteroid_field->isEmpty()) {
			asteroid_field->resetTimer();
		}
		asteroid_field->increaseAsteroidCountBy(1);
		asteroid_field->launchAsteroidsAtShip(ship->getPosition());
	}
}

void GameManager::updateBullets() {
	ship->updateBullets(dt);
}

void GameManager::updateSatellite() {
	arena->updateSatellite(dt);
}

void GameManager::updateExplosions() {
	explosion_manager->updateExplosions(dt);
}

void GameManager::handleCollisions() {
	handleWallCollisions();
	handleBulletCollisions();
	handleAsteroidCollisions();
}

// Ship -> Wall
void GameManager::handleWallCollisions() {
	for (Wall& wall : arena->getWalls()) {
		if (collision::withWall(wall, ship->getPosition(), ship->getWarningRadius())) {
			wall.setColour(Colour::RED);
		}
		else {
			wall.setColour(Colour::WHITE);
		}

		if (collision::withWall(wall, ship->getPosition(), ship->getCollisionRadius())) {
			resetGame();
			break;
		}
	}
}

// Asteroid -> Ship
// Asteroid -> Wall
// Asteroid -> Asteroid
void GameManager::handleAsteroidCollisions() {
	for (Asteroid& a1 : asteroid_field->getAsteroids()) {
		if (!a1.isInArena()) {
			continue;
		}

		if (collision::withAsteroid(a1.getPosition(), a1.getRadius(), ship->getPosition(), ship->getCollisionRadius())) {
			// Persist ship explosions after resetting the game
			Vector3D ship_position = ship->getPosition();
			resetGame();
			explosion_manager->populate(ship_position);
		}

		for (const Wall& wall : arena->getWalls()) {
			if (collision::withWall(wall, a1.getPosition(), a1.getRadius())) {
				collision::resolve(wall, a1);
			}
		}

		// ASTEROID->ASTEROID COLLISIONS ///////////////////////////
		for (Asteroid& a2 : asteroid_field->getAsteroids()) {
			if (a1.id() != a2.id()) {
				if (collision::withAsteroid(a1.getPosition(), a1.getRadius(), a2.getPosition(), a2.getRadius())) {
					// Calculate new velocities, then move slightly apart
					collision::resolve(a1, a2);
					a1.update(dt);
					a2.update(dt);
				}
			}
		}
	}
}

// Bullet -> Wall
// Bullet -> Asteroid
void GameManager::handleBulletCollisions() {
	for (std::shared_ptr<Bullet>& bullet : ship->getBullets()) {
		// Bullet->Wall
		for (Wall& wall : arena->getWalls()) {
			if (collision::withWall(wall, bullet->getPosition())) {
				bullet->markForDeletion();
			}
		}

		// Why check asteroids if bullet died on a wall?
		if (bullet->markedForDeletion()) {
			continue;
		}

		for (Asteroid& asteroid : asteroid_field->getAsteroids()) {
			if (collision::withAsteroid(asteroid.getPosition(), asteroid.getRadius(), bullet->getPosition())) {
				bullet->markForDeletion();
				asteroid.decrementHealthBy(1);
				if (asteroid.getHealth() <= 0) {
					explosion_manager->populate(asteroid.getPosition());
				}
				break;
			}
		}
	}
}

// glutKeyboardFunc(keyboardDownCallback);
void GameManager::onKeyDown(const unsigned char key, int x, int y) {
	keyboard->setPressed(key, true);
}

// glutKeyboardUpFunc(keyboardUpCallback);
void GameManager::onKeyUp(const unsigned char key, int x, int y) {
	keyboard->setPressed(key, false);
}

void GameManager::handleKeyboardInput() {

	if (keyboard->isPressed('w')) {
		ship->move(Direction::forward, dt);
	}
	else if (keyboard->isPressed('s')) {
		ship->move(Direction::backward, dt);
	}
	else {
		ship->setAccelerationToZero();
	}

	if (keyboard->isPressed('a')) {
		ship->roll(Axis::z, -dt);
	}

	if (keyboard->isPressed('d')) {
		ship->roll(Axis::z, dt);
	}

	if (keyboard->isPressed(' ')) {
		ship->shoot(dt);
	}

	if (keyboard->isPressed('r')) {
		resetGame();
	}

	if (keyboard->isPressed('i')) {
		camera->look(Look::ABOVE);
	}
	else if (keyboard->isPressed('m')) {
		camera->look(Look::BELOW);
	}
	else if (keyboard->isPressed('j')) {
		camera->look(Look::LEFT);
	}
	else if (keyboard->isPressed('l')) {
		camera->look(Look::RIGHT);
	}
	else if (keyboard->isPressed('k')) {
		camera->look(Look::BEHIND);
	}
	else {
		camera->look(Look::AHEAD);
	}

	if (keyboard->isAnyKeyPressed()) {
		glutPostRedisplay();
	}
}

void GameManager::onMouseClick(int button, int state, int x, int y) {
	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {
			mouse->setHoldingLeftClick(true);
		}
		else {
			mouse->setHoldingLeftClick(false);
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN) {
			mouse->setHoldingRightClick(true);
		}
		else {
			mouse->setHoldingRightClick(false);
		}
		break;
	default:
		break;
	}

	mouse->setPosition(x, y);
}

void GameManager::onMouseMovement(int x, int y) {
	mouse->X = x;
	mouse->Y = y;
}

void GameManager::onMouseClickDrag(int x, int y) {
	onMouseMovement(x, y);
}

void GameManager::handleMouseInput() {
	if (mouse->isHoldingLeftClick()) {
		float map_x = utility::mapToRange(
			mouse->X,
			0, window->width,
			camera->getAspect(), -camera->getAspect());

		float map_y = utility::mapToRange(
			mouse->Y,
			0, window->height,
			camera->getAspect(), -camera->getAspect());

		ship->rotate(Axis::y, dt, map_x);
		ship->rotate(Axis::x, dt, map_y);

		glutPostRedisplay();
	}
}

void GameManager::calculateTimeDelta() {
	// gives delta time in seconds
	const float cur_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	dt = cur_time - last_time;
	last_time = cur_time;
}

void GameManager::resetGame() {
	ship->reset();
	asteroid_field->reset();
	Transparent::reset();
}
