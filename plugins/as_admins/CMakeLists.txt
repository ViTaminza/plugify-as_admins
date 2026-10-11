# as_admins — Plugify C++ plugin: the "Admins" module of the Pisex admin_system (Metamod plugin
# as_admins) on top of the admin_system plugin and pisexcompat.
#
#   plugify-as_admins  the plugin shared library (entry "bin/as_admins" -> bin/libas_admins.so)

set(AS_ADMINS_VERSION "1.0.3")

# Binding globals filled by the plugify module from our manifest dependencies:
#   __s2sdk_*        (s2sdk_bindings, compiled into every plugin)
#   __admin_system_* (generated consumer bindings of the admin_system plugin)
set(AS_ADMINS_BINDING_OBJECTS "")
if(TARGET admin_system_bindings_obj)
    list(APPEND AS_ADMINS_BINDING_OBJECTS $<TARGET_OBJECTS:admin_system_bindings_obj>)
else()
    message(WARNING "as_admins: admin_system_bindings_obj is missing (build plugins/admin_system first)")
endif()

add_library(plugify-as_admins SHARED
        src/plugin.cpp
        $<TARGET_OBJECTS:s2sdk_bindings>
        ${AS_ADMINS_BINDING_OBJECTS}
)
target_include_directories(plugify-as_admins PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
# The consumer side of the admin_system API: admin_api.hpp plus the generated bindings. SYSTEM:
# warnings from the generated/third-party headers must not fail -Werror on our sources.
target_include_directories(plugify-as_admins SYSTEM PRIVATE
        "${CMAKE_SOURCE_DIR}/plugins/admin_system/include"
        "${CMAKE_SOURCE_DIR}/plugins/admin_system/include/external/plugify/include"
        "${CMAKE_SOURCE_DIR}/sdk/include")
target_link_libraries(plugify-as_admins PRIVATE
        pisexcompat
        -static-libstdc++ -static-libgcc
)
target_compile_options(plugify-as_admins PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(plugify-as_admins PRIVATE
        _GLIBCXX_USE_CXX11_ABI=1
        PLUGIFY_MODULE="as_admins"
        PLUGIFY_IS_DEBUG=0
        PLUGIFY_IS_RELEASE=1
)
set_target_properties(plugify-as_admins PROPERTIES OUTPUT_NAME as_admins)

if(UNIX AND NOT APPLE)
    target_link_options(plugify-as_admins PRIVATE
            "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/sym/version_script.lds")
endif()

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/as_admins.pplugin.in"
        "${CMAKE_CURRENT_BINARY_DIR}/as_admins.pplugin"
)
