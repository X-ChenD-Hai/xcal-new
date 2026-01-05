#include "opengl_support.hpp"

#include <print>

#include "opengl_wrapper/opengl_api.hpp"  // IWYU pragma: keep

namespace opengl_support {

void OpenGLSupport::cleanup() {
    if (!initialized_) {
        return;
    }
    initialized_ = false;
}

void OpenGLSupport::load(GetProcAddressFunc get_proc_address) {
    if (initialized_) {
        return;
    }
#ifdef USE_GLBINDING
    glbinding::initialize(get_proc_address, false);
    std::println("opengl init");
#else
    std::println("get_proc_address {}", (void*)get_proc_address);
    if (!gladLoadGLLoader((GLADloadproc)get_proc_address)) {
        std::println("gladLoadGLLoader failed");
        throw(std::runtime_error("gladLoadGLLoader failed"));
    }
#endif
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    initialized_ = true;
}
OpenGLSupport* OpenGLSupport::install(ecs::World& world,
                                      GetProcAddressFunc get_proc_address) {
    auto plugin = new OpenGLSupport(world);
    plugin->load(get_proc_address);

    return plugin;
}
void OpenGLSupport::uninstall(ecs::World& world, OpenGLSupport* plugin) {
    plugin->cleanup();
    delete plugin;
}
}  // namespace opengl_support
