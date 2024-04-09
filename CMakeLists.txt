cmake_minimum_required(VERSION 3.5)

project(psip_switch VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(libtins REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED context)

set(PROJECT_SOURCES
    main.cpp
)

set(TEST_SOURCES
    test.cpp
)

set(COMMON_SOURCES
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    shared_storage.cpp
    shared_storage.h
    shared_storage_handle.cpp
    shared_storage_handle.h
    network_handle.cpp
    network_handle.h
    network_switch.cpp
    network_switch.h
    rest_handle.cpp
    rest_handle.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(psip_switch
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${COMMON_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET psip_switch APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(psip_switch SHARED
            ${PROJECT_SOURCES}
            ${COMMON_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(psip_switch
            ${PROJECT_SOURCES}
            ${COMMON_SOURCES}
        )
    endif()
endif()

add_custom_target(
    symbols_generation
    COMMAND li_symbol_generator ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_psip_switch
    ${COMMON_SOURCES}
    ${TEST_SOURCES}
)

add_dependencies(psip_switch symbols_generation)
add_dependencies(test_psip_switch symbols_generation)

set(LIBS
    Qt${QT_VERSION_MAJOR}::Widgets
    tins
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
)
set(INSTALLED_LIBS
)

target_link_libraries(psip_switch PRIVATE ${LIBS})
target_link_libraries(psip_switch PRIVATE ${INSTALLED_LIBS})
target_link_libraries(test_psip_switch PRIVATE ${LIBS})
target_link_libraries(test_psip_switch PRIVATE ${INSTALLED_LIBS})

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.psip_switch)
endif()
set_target_properties(psip_switch PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS psip_switch
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(psip_switch)
endif()
