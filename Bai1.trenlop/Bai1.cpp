#include "glut.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// Khởi tạo ánh sáng Spotlight giống ảnh mẫu
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // Đèn môi trường (Ambient) tối để tạo không gian sang trọng
    GLfloat ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    // Spotlight chiếu từ trần xuống
    GLfloat lightPos[] = { 0.0f, 4.0f, 0.0f, 1.0f };
    GLfloat spotDir[] = { 0.0f, -1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f); // Độ tỏa của đèn
}

void drawStore() {
    // Vẽ Sàn nhà (Màu tối)
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, -5); glVertex3f(5, 0, -5);
    glVertex3f(5, 0, 5);  glVertex3f(-5, 0, 5);
    glEnd();

    // Vẽ Bức tường phía sau (Nơi đặt Logo)
    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, -5); glVertex3f(5, 0, -5);
    glVertex3f(5, 4, -5);  glVertex3f(-5, 4, -5);
    glEnd();

    // Vẽ Kệ đồ đơn giản bên trái (Dùng khối hộp)
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(-3.5f, 1.0f, -2.0f);
    glScalef(0.1f, 2.0f, 4.0f);
    glutSolidCube(1.0);
    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(0, 2, 8, 0, 1, 0, 0, 1, 0); // Camera nhìn từ cửa vào

    initLighting();
    drawStore();

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Cua hang ICON MODE");
    glEnable(GL_DEPTH_TEST);
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}