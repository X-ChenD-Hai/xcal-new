//line.vs
#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 model;
// layout(std140, binding = 1) uniform RenderGlobal {
//     mat4 projection_view;
//     vec4 global_ambient_light;
// };
layout(std140, binding = 1) uniform RenderGlobal {
    mat4 projection_view_matrix_;
    // vec4 global_ambient_light_;
};

out vec4 ourColor;
void main() {
    // gl_Position = projection_view * model * vec4(aPos, 1.0);
    gl_Position = projection_view_matrix_ * model * vec4(aPos, 1.0);
    ourColor = vec4(aColor, 1.0);
}