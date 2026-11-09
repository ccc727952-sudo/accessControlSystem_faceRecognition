# 门禁人脸识别系统 —— CMake 构建脚本
# 基于 Qt 6.8 + CMake，链接 Quick（QML 界面）与 Mqtt 模块；
# 同时把原生 sqlite3.c 一起编译进可执行文件，因为 Qt 内置 SQLite
# 缺少 load_extension，无法加载 sqlite-vec 向量检索扩展。
cmake_minimum_required(VERSION 3.16)

# 只有 `CXX` 时 CMake **不会报错**，但会静默跳过 `.c` 文件不编译；
project(accessControlSystem_faceRecognition VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Mqtt)

qt_standard_project_setup(REQUIRES 6.8)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appaccessControlSystem_faceRecognition
    main.cpp
    sqlite3.c
)

qt_add_qml_module(appaccessControlSystem_faceRecognition
    URI accessControlSystem_faceRecognition
    QML_FILES
        Main.qml
        SOURCES mqttutil.cpp mqttutil.h
        SOURCES facedata.cpp facedata.h facedatadao.cpp facedatadao.h
SOURCES sqlite3.h
RESOURCES sqlite3.c
SOURCES facedatalogic.h facedatalogic.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appaccessControlSystem_faceRecognition PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appaccessControlSystem_faceRecognition
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appaccessControlSystem_faceRecognition
    PRIVATE Qt6::Quick
    Qt6::Mqtt
)

install(TARGETS appaccessControlSystem_faceRecognition
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
