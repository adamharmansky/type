#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>

#include "type.h"
#include "type_glfw.h"

const char text[] = "The quick brown fox jumps over a lazy dog.";

static void
die(const char *text) {
    fputs(text, stdout);
    exit(1);
}

GLFWwindow *window;
TypeFont *font, *font1;
int window_width, window_height;
int number_of_lines;

float scale = 0.5;

void
show_fps() {
    static double last_time = 0;
    char txt[64];
    double time = glfwGetTime();
    snprintf(txt, 64, "%.1f FPS, %d objects [press UP]", 1/(time-last_time), number_of_lines);
    glPushMatrix();
        glTranslatef(0, type_font_ascent(font), 0);
        type_draw_text(font, txt);
    glPopMatrix();
    last_time = time;
}

void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_UP:
                number_of_lines++;
                break;
            case GLFW_KEY_DOWN:
                if (number_of_lines)
                    number_of_lines--;
                break;
            case GLFW_KEY_RIGHT:
                scale += 0.1;
                break;
            case GLFW_KEY_LEFT:
                scale -= 0.1;
                break;
        }
    }
}

void
draw_some_funny_stuff() {
    unsigned int i;
    srandom(0xDEADBEEF);
    for (i = 0; i < number_of_lines; i++) {
        glPushMatrix();
            glColor4f((float)random()/RAND_MAX, (float)random()/RAND_MAX, (float)random()/RAND_MAX, 1);
            glTranslatef(random()%window_width, random()%window_height, 0);
            glRotatef(glfwGetTime()*i, 0, 0, 1);
            glScalef((4+sin(glfwGetTime()*2))/5, (4+sin(glfwGetTime()*2))/5, (4+sin(glfwGetTime()*2))/5);
            glTranslatef(-type_text_width(i%2?font:font1, text)/2, 0, 0);
            type_draw_text(i%2?font:font1, text);
            glColor4f(0,0,0,0.2);
            glBegin(GL_QUADS);
                glVertex2f(0, 0);
                glVertex2f(type_text_width(i%2?font:font1, text), 0);
                glVertex2f(type_text_width(i%2?font:font1, text), -type_font_ascent(i%2?font:font1));
                glVertex2f(0, -type_font_ascent(i%2?font:font1));
            glEnd();
            glColor4f(0,0,0,0.5);
            glBegin(GL_QUADS);
                glVertex2f(0, 0);
                glVertex2f(type_text_width(i%2?font:font1, text), 0);
                glVertex2f(type_text_width(i%2?font:font1, text), type_font_descent(i%2?font:font1));
                glVertex2f(0, type_font_descent(i%2?font:font1));
            glEnd();
        glPopMatrix();
    }
}

int
main() {
    unsigned int i;

    if (!glfwInit()) die("Cannot initialize GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(800, 600, "Text", NULL, NULL);

    if (!window) die("Cannot create window");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);

    glfwSetKeyCallback(window, key_callback);

    type_set_dpi_for_glfw();
    font = type_load_font("Ubuntu:size=14");
    font1 = type_load_font("Mononoki:pixelsize=30");

    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &window_width, &window_height);
        glViewport(0, 0, window_width, window_height);

        glLoadIdentity();
        glOrtho(0, window_width, window_height, 0, -10000, 10000);

        glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT);

        draw_some_funny_stuff();

        glColor4f(0,0,0,1);
        show_fps();
        glTranslatef(0, type_font_height(font), 0);
        glPushMatrix();
            glScalef(scale, scale, scale);
            glTranslatef(0, type_font_ascent(font), 0);
            glPushMatrix();
                for (i = 32; i < 128; i++) {
                    type_draw_char(font, i);
                }
            glPopMatrix();
            glTranslatef(0, type_font_height(font), 0);
            glPushMatrix();
                for (i = 128; i < 256; i++) {
                    type_draw_char(font, i);
                }
            glPopMatrix();
        glPopMatrix();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
