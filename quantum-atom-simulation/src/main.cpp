#include <GL/glut.h>
#include "BohrModel.h"
#include "WaveParticleModel.h"
#include "ThreeDRenderModel.h"
#include "ElementData.h"
#include <iostream>

AtomModel* currentModel;
BohrModel bohrModel;
WaveParticleModel waveParticleModel;
ThreeDRenderModel threeDRenderModel;
int currentModelIndex = 0;
bool isWireframe = false;

int windowWidth = 1000;
int windowHeight = 720;

double rotateX = 0.0;
double rotateY = 0.0;
double zoom = 1.0;
double panX = 0.0;
double panY = 0.0;

int lastTime = 0;
int selectedElementIndex = 0;

void applySelectedElement() {
    const auto& element = ElementData::supportedElements()[selectedElementIndex];
    bohrModel.setElementConfiguration(element.atomicNumber, element.neutronNumber);
    waveParticleModel.setAtomicNumber(element.atomicNumber);
    waveParticleModel.setNeutronNumber(element.neutronNumber);
    threeDRenderModel.setAtomicNumber(element.atomicNumber);
    threeDRenderModel.setNeutronNumber(element.neutronNumber);

    std::cout << "Selected element: " << element.name
              << " (" << element.symbol << ")"
              << " Z=" << element.atomicNumber
              << " N=" << element.neutronNumber << std::endl;
}

void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void reshape(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(panX, panY, -10.0 * zoom);
    glRotatef(rotateX, 1.0, 0.0, 0.0);
    glRotatef(rotateY, 0.0, 1.0, 0.0);

    currentModel->render();
    glutSwapBuffers();
}

void update() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    double deltaTime = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    currentModel->update(deltaTime);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    const auto& elements = ElementData::supportedElements();
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
        case 'n':
            selectedElementIndex = (selectedElementIndex + 1) % elements.size();
            applySelectedElement();
            break;
        case 'b':
            selectedElementIndex = (selectedElementIndex + elements.size() - 1) % elements.size();
            applySelectedElement();
            break;
        case 'u':
            bohrModel.triggerTransition(true);
            break;
        case 'j':
            bohrModel.triggerTransition(false);
            break;
        case 't':
            bohrModel.toggleAutoTransition();
            break;
        case 'k':
            threeDRenderModel.nextQuantumPreset();
            break;
        case '[':
            threeDRenderModel.setProbabilityThreshold(0.04);
            break;
        case ']':
            threeDRenderModel.setProbabilityThreshold(0.2);
            break;
        case 'w':
            isWireframe = !isWireframe;
            glPolygonMode(GL_FRONT_AND_BACK, isWireframe ? GL_LINE : GL_FILL);
            break;
        case 'p':
            if (currentModel->getIsPlaying()) currentModel->pause();
            else currentModel->play();
            break;
        case 'r':
            currentModel->reset();
            break;
        case '+':
            currentModel->setSpeed(currentModel->getSpeed() * 1.25);
            break;
        case '-':
            currentModel->setSpeed(currentModel->getSpeed() / 1.25);
            break;
        case 'a':
            panX -= 0.2;
            break;
        case 'd':
            panX += 0.2;
            break;
        case 's':
            panY -= 0.2;
            break;
        case 'x':
            panY += 0.2;
            break;
        case 'q':
            exit(0);
            break;
    }
}

void mouseMove(int x, int y) {
    static int lastX = x;
    static int lastY = y;

    rotateY += (x - lastX) * 0.5;
    rotateX += (y - lastY) * 0.5;

    lastX = x;
    lastY = y;
}

void mouseWheel(int wheel, int direction, int x, int y) {
    (void)wheel;
    (void)x;
    (void)y;

    if (direction > 0) zoom *= 0.9;
    else zoom *= 1.1;

    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 5.0) zoom = 5.0;
}

int main(int argc, char** argv) {
    std::cout << "Initializing GLUT..." << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);

    int windowId = glutCreateWindow("Quantum Atom Simulation (Classical + Quantum)");
    if (windowId == 0) {
        std::cerr << "Error: Failed to create GLUT window" << std::endl;
        return 1;
    }

    initOpenGL();
    glutDisplayFunc(render);
    glutReshapeFunc(reshape);
    glutIdleFunc(update);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(mouseMove);

#ifdef GLUT_WHEEL_UP
    glutMouseWheelFunc(mouseWheel);
#endif

    currentModel = &bohrModel;
    applySelectedElement();

    std::cout << "Controls: 1/2/3 model, n/b element, u/j transition, t auto transition, k quantum preset" << std::endl;
    std::cout << "Camera: drag rotate, wheel zoom, a/d/s/x pan" << std::endl;

    glutMainLoop();
    return 0;
}
