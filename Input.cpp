#include "Input.h"

#include <cstring>
#include <GLFW/glfw3.h>

Input::Input() {

    memset(m_prev, 0, sizeof(m_prev));
    memset(m_curr, 0, sizeof(m_curr));

}

Input::~Input() = default;

void Input::Initialize() {



}

void Input::Shutdown() {



}

void Input::BeginProcessInput(GLFWwindow* window) {

    for (int i = 0; i < GLFW_KEY_LAST; ++i) {
        m_curr[i] = glfwGetKey(window, i) == GLFW_PRESS;
    }

}

void Input::EndProcessInput() {

    memcpy(m_prev, m_curr, sizeof(m_curr));

}

bool Input::IsPressed(int key) {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return m_curr[key] && !m_prev[key];
}

bool Input::IsHeld(int key) {
    return m_curr[key];
}

bool Input::IsReleased(int key) {
    return !m_curr[key] && m_prev[key];
}


