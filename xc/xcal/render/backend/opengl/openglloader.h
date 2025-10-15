#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#define _gl gl::
#endif
#ifdef USE_GLAD
#include <glad/glad.h>
#define _gl
#endif