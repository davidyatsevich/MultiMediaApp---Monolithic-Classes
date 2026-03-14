cmake_minimum_required(VERSION 3.16)
project(MultiMediaAppTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Qt ────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Multimedia
    MultimediaWidgets
    Sql
    Test          # QtTest framework
)

# ── Include paths (same as main project) ─────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../Backend
    ${CMAKE_CURRENT_SOURCE_DIR}/../GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

# ── Shared backend sources (no main.cpp) ──────────────────────
set(BACKEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../Backend/SQLite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Backend/Recorders.cpp
)

# ── Helper macro: one executable per test file ───────────────
# Usage: add_qt_test(TestFoo TestFoo.cpp [extra sources])
macro(add_qt_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${ARGN})

    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        Qt6::Sql
        Qt6::Test
    )

    # Register with CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    # Pass Qt plugin path so tests find SQL/multimedia drivers at runtime
    set_tests_properties(${TEST_NAME} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endmacro()

# ── Enable CTest ─────────────────────────────────────────────
enable_testing()

# ── Test targets ─────────────────────────────────────────────

# File class — pure C++, no Qt I/O needed
add_qt_test(TestFile
    TestFile.cpp
)

# SQLite class — uses QSql, writes to a temp DB
add_qt_test(TestSQLite
    TestSQLite.cpp
    ${BACKEND_SOURCES}
)

# AudioRecorder — requires a microphone; skips gracefully if absent
add_qt_test(TestAudioRecorder
    TestAudioRecorder.cpp
    ${BACKEND_SOURCES}
)

# VideoRecorder — requires a webcam; skips gracefully if absent
add_qt_test(TestVideoRecorder
    TestVideoRecorder.cpp
    ${BACKEND_SOURCES}
)

# Integration: SQLite + File round-trip
add_qt_test(TestSQLiteFileIntegration
    TestSQLiteFileIntegration.cpp
    ${BACKEND_SOURCES}
)
