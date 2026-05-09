cmake_minimum_required(VERSION 3.16)
project(MultiMediaApp)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# --------------------------------------------------
# Sanitizer Option
# --------------------------------------------------
option(ENABLE_SANITIZERS "Enable Address/UB sanitizers" OFF)


if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
   message(STATUS "Sanitizers enabled")
   add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g)
   add_link_options(-fsanitize=address -fsanitize=undefined)
endif()


# --------------------------------------------------
# Qt Configuration
# --------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets Sql)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


include_directories(
   ${PROJECT_SOURCE_DIR}
   ${PROJECT_SOURCE_DIR}/Backend
   ${PROJECT_SOURCE_DIR}/GUI
)


set(BACKEND_SOURCES
   Backend/SQLite.cpp
   Backend/Recorders.cpp
)


set(GUI_SOURCES
   GUI/MainWindow.cpp
   GUI/MainTabs.cpp
   GUI/NestedRecordingTabs.cpp
   GUI/NestedEditingTabs.cpp
   GUI/VideoPlayerWindow.cpp
)


# Single main.cpp works on all platforms.
# Platform guards inside it (#ifdef __APPLE__ / #ifdef __linux__)
# handle the Darwin permission plugin imports at compile time.
add_executable(MultiMediaApp
   main.cpp
   ${BACKEND_SOURCES}
   ${GUI_SOURCES}
)


# --------------------------------------------------
# macOS
# --------------------------------------------------
if(APPLE)
   set_target_properties(MultiMediaApp PROPERTIES
       MACOSX_BUNDLE TRUE
       MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
   )


   target_link_libraries(MultiMediaApp PRIVATE
       Qt6::Core
       Qt6::Widgets
       Qt6::Multimedia
       Qt6::MultimediaWidgets
       Qt6::Sql
       $ENV{HOME}/Qt/6.11.0/macos/plugins/permissions/libqdarwincamerapermission.a
       $ENV{HOME}/Qt/6.11.0/macos/plugins/permissions/libqdarwinmicrophonepermission.a
       "-framework AVFoundation"
       "-framework CoreMedia"
       "-framework CoreVideo"
       "-framework CoreFoundation"
   )


   target_link_options(MultiMediaApp PRIVATE
       "-Wl,-u,_QDarwinCameraPermissionRequest"
       "-Wl,-u,_QDarwinMicrophonePermissionRequest"
   )


   add_custom_command(TARGET MultiMediaApp POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
           "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
           "$<TARGET_BUNDLE_CONTENT_DIR:MultiMediaApp>/Info.plist"
       COMMENT "Copying Info.plist into bundle"
       VERBATIM
   )


# --------------------------------------------------
# Linux (Ubuntu, Fedora, any distro)
# --------------------------------------------------
elseif(UNIX AND NOT APPLE)
   message(STATUS "Configuring for Linux")


   target_link_libraries(MultiMediaApp PRIVATE
       Qt6::Core
       Qt6::Widgets
       Qt6::Multimedia
       Qt6::MultimediaWidgets
       Qt6::Sql
   )


   install(TARGETS MultiMediaApp RUNTIME DESTINATION bin)
endif()