cmake_minimum_required(VERSION 3.16)
project(contentmanipulation LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/webenginewidgets/contentmanipulation")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS WebEngineWidgets)

qt_add_executable(contentmanipulation
    main.cpp
    mainwindow.cpp mainwindow.h
    rendererkiller.h rendererkiller.cpp
)
set_target_properties(contentmanipulation PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(contentmanipulation PUBLIC
    Qt::Core
    Qt::Gui
    Qt::WebEngineWidgets
)

add_subdirectory(resources)


install(TARGETS contentmanipulation
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
