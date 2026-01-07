#include "./shader.hpp"
namespace xcal_opengl_render {
namespace static_shader_source {

const char* kVertexWithPosUniformColorShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;
uniform vec3 color;
out vec3 ourColor;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = color;
}
)glsl";

const char* kVertexWithPosColorShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 3) in vec3 aNormel;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;
out vec3 ourColor;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = aColor;
}
)glsl";
const char* kVertexWhitPosNormalLightShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aNormal;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;
uniform vec3 color;
out vec3 ourColor;
out vec3 normal;
out vec3 vpos;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = color;
    normal = mat3(transpose(inverse(transform))) * aNormal;;
    vpos = vec3(transform * vec4(aPos, 1.0));
}
)glsl";
const char* kFragmentShaderSource = R"glsl(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)glsl";
const char* kFragmentWithLightShaderSource = R"glsl(
#version 330 core
uniform vec3 light_color;
uniform vec3 light_pos;
uniform mat4 transform;
uniform float ambientStrength;
in vec3 ourColor;
in vec3 vpos;
in vec3 normal;
out vec4 FragColor;
void main()
{
    vec3 norm = normalize(normal);
    vec3 light_direction = normalize(light_pos-vpos);
    float diff = max(dot(norm, light_direction), 0.0);
    FragColor = vec4(ourColor*(ambientStrength+diff), 1.0);
}
)glsl";
}  // namespace static_shader_source
}  // namespace xcal_opengl_render