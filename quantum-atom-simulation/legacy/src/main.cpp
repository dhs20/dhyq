#include <GL/glut.h>
#include "BohrModel.h"
#include "WaveParticleModel.h"
#include "ThreeDRenderModel.h"
#include <iostream>

// Global variables
AtomModel* currentModel;
BohrModel bohrModel;
WaveParticleModel waveParticleModel;
ThreeDRenderModel threeDRenderModel;
int currentModelIndex = 0;
bool isWireframe = false;

// Window parameters
int windowWidth = 800;
int windowHeight = 600;

// Rotation parameters
double rotateX = 0.0;
double rotateY = 0.0;
double zoom = 1.0;

// Time parameters
int lastTime = 0;

// Initialize OpenGL
void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

// Resize window
void reshape(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Render function
void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Apply camera transformation
    glTranslatef(0.0, 0.0, -10.0 * zoom);
    glRotatef(rotateX, 1.0, 0.0, 0.0);
    glRotatef(rotateY, 0.0, 1.0, 0.0);
    
    // Render current model
    currentModel->render();
    
    glutSwapBuffers();
}

// Animation update function
void update() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    double deltaTime = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;
    
    currentModel->update(deltaTime);
    glutPostRedisplay();
}

// Keyboard event handling
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case '1':
            currentModelIndex = 0;
            currentModel = &bohrModel;
            break;
        case '2':
            currentModelIndex = 1;
            currentModel = &waveParticleModel;
            break;
        case '3':
            currentModelIndex = 2;
            currentModel = &threeDRenderModel;
            break;
        case 'w':
            isWireframe = !isWireframe;
            if (isWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            break;
        case 'p':
            if (currentModel->getIsPlaying()) {
                currentModel->pause();
            } else {
                currentModel->play();
            }
            break;
        case 'r':
            currentModel->reset();
            break;
        case '+':
            currentModel->setSpeed(currentModel->getSpeed() * 1.5);
            break;
        case '-':
            currentModel->setSpeed(currentModel->getSpeed() / 1.5);
            break;
        case 'q':
            exit(0);
            break;
    }
}

// Mouse event handling
void mouse(int button, int state, int x, int y) {
    // Can add mouse interaction functionality
}

// Mouse move event handling
void mouseMove(int x, int y) {
    static int lastX = x;
    static int lastY = y;
    
    rotateY += (x - lastX) * 0.5;
    rotateX += (y - lastY) * 0.5;
    
    lastX = x;
    lastY = y;
}

// Mouse wheel event handling
void mouseWheel(int wheel, int direction, int x, int y) {
    if (direction > 0) {
        zoom *= 0.9;
    } else {
        zoom *= 1.1;
    }
    
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 5.0) zoom = 5.0;
}

// Main function
int main(int argc, char** argv) {
    std::cout << "Initializing GLUT..." << std::endl;
    
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    
    std::cout << "Creating window..." << std::endl;
    int windowId = glutCreateWindow("Quantum Atom Simulation");
    
    if (windowId == 0) {
        std::cerr << "Error: Failed to create GLUT window" << std::endl;
        return 1;
    }
    
    std::cout << "Window created successfully. ID: " << windowId << std::endl;
    
    // Initialize OpenGL
    std::cout << "Initializing OpenGL..." << std::endl;
    initOpenGL();
    
    // Set callback functions
    std::cout << "Setting callback functions..." << std::endl;
    glutDisplayFunc(render);
    glutReshapeFunc(reshape);
    glutIdleFunc(update);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMove);
    
    // Set mouse wheel callback if available
    #ifdef GLUT_WHEEL_UP
    glutMouseWheelFunc(mouseWheel);
    std::cout << "Mouse wheel callback set" << std::endl;
    #endif
    
    // Set default model
    std::cout << "Setting default model to Bohr model..." << std::endl;
    currentModel = &bohrModel;
    
    // Start main loop
    std::cout << "Starting GLUT main loop..." << std::endl;
    glutMainLoop();
    
    return 0;
}
