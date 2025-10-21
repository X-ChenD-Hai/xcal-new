//line.vs
#version 430 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
layout(std140, binding = 1) uniform RenderGlobal {
    mat4 projection_view_matrix_;
};

void main() {
    gl_Position = projection_view_matrix_ * model * vec4(aPos, 1.0);
}