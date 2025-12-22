cmake_minimum_required(VERSION 3.10)
project(lab14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Установите пути для поиска библиотек
set(CMAKE_PREFIX_PATH "/opt/homebrew;/opt/homebrew/opt")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/opt/homebrew/share/cmake/Modules")

# Найдите необходимые библиотеки
find_library(SFML_GRAPHICS sfml-graphics)
find_library(SFML_WINDOW sfml-window)
find_library(SFML_SYSTEM sfml-system)
find_library(GLEW_LIBRARY GLEW)

# Настройте include директории
include_directories(
    /opt/homebrew/include
    /opt/homebrew/opt/sfml/include
    /opt/homebrew/opt/glew/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Проверьте, что все библиотеки найдены
if(NOT SFML_GRAPHICS OR NOT SFML_WINDOW OR NOT SFML_SYSTEM OR NOT GLEW_LIBRARY)
    message(FATAL_ERROR "Не удалось найти все необходимые библиотеки!")
endif()

add_executable(lab14 main.cpp)

# Ссылки на библиотеки
target_link_libraries(lab14
    ${SFML_GRAPHICS}
    ${SFML_WINDOW}
    ${SFML_SYSTEM}
    ${GLEW_LIBRARY}
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreFoundation"
    "-framework CoreVideo"
)

# Для macOS
if(APPLE)
    set_target_properties(lab14 PROPERTIES
        LINK_FLAGS "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreVideo"
    )
endif()