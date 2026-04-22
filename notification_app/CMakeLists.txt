# Build Zep Notification App
# Requires: Zep library, SDL2, ImGui, OpenGL, Freetype

IF (BUILD_IMGUI)

set(APP_ROOT ${ZEP_ROOT})
set(APP_NAME ZepNotificationApp)

project(ZepNotificationApp VERSION 0.1.0.0)

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

set(APP_SOURCE
    ${APP_ROOT}/notification_app.cpp
    ${APP_ROOT}/notification_app.h
)

add_executable(${APP_NAME} ${APP_SOURCE})

target_compile_definitions(${APP_NAME} PUBLIC -DZEP_USE_SDL)

target_include_directories(${APP_NAME}
    PRIVATE
    ${ZEP_ROOT}/include
    ${FREETYPE_INCLUDE_DIRS}
    ${SDL2_ROOT_DIR}
    ${CMAKE_BINARY_DIR})

target_link_libraries(${APP_NAME}
    PRIVATE
    Zep::Zep
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    imgui::imgui
    unofficial::gl3w::gl3w
    ${PLATFORM_LINKLIBS}
    ${FREETYPE_LIBRARIES}
)

install(TARGETS ${APP_NAME}
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

endif()