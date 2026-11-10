cmake_minimum_required(VERSION 3.16)

# 只有 `CXX` 时 CMake **不会报错**，但会静默跳过 `.c` 文件不编译；
project(accessControlSystem_faceRecognition VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
# 需要 C++17
set(CMAKE_CXX_STANDARD 17)
# 设置
set(OpenCV_ROOT_DIR "D:/WorkProgram/opencv/opencv/opencv-4.8.0/build/install")
find_package(OpenCV QUIET)
#QuickControls2供了一整套现成的 UI 控件（如按钮、滑块、菜单等），用来在 Qt Quick（QML）中快速构建完整的用户界面
# TextToSpeech文字转语音
# Multimedia多媒体
# Network：请求易客天气 HTTP 接口
find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Mqtt TextToSpeech QuickControls2 Network)
#dlib路径
set(dlib_DIR "D:/WorkProgram/dlib/build/install/lib/cmake/dlib")
find_package(dlib REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)
# 坑：中文注释会"吃掉"代码MSVC 默认按系统 ANSI 代码页（中文机器 = GBK）解析源码。
# 我们写的注释是 UTF-8 的，某些汉字的字节在 GBK 里会被当成反斜杠 + 换行的转义 —— 后面的代码就被当成注释吞掉了，
# 报错还报在莫名其妙的地方。
# 解决办法（CMakeLists.txt 里加一行）：
if(MSVC)
 add_compile_options(/utf-8)
endif()
# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

# ===== 部署说明=====
# 1) 把编译好的 sqlite-vec.dll 放到 appaccessControlSystem_faceRecognition.exe 同级目录；
# 2) face.db 会在 exe 同级目录自动创建（FaceDataDao 用 applicationDirPath() 拼路径）；
# 3) 若要查询 vec0 虚拟表，可用 sqlite3.exe 先执行 .load sqlite-vec.dll sqlite3_vec_init，
#    或直接用本程序写入后查 sqlite_master / 阴影表。
# 本工程不链接 Qt SQL，而是直接编译原生 sqlite3.c，用 C API 加载 sqlite-vec 扩展
# （与 doc 3.2 / 3.3 一致：Qt 内置 SQLite 禁用了 load_extension）。
# 编译配置：Qt 6.8.3 msvc2022_64 + CMake + Ninja，统一 -DCMAKE_BUILD_TYPE=Release
# （录入端链了本机 OpenCV 的 Release 库，Debug 会报 LNK2019；识别端虽不链 OpenCV，
#  也保持同一套配置，方便四个工程共用同一套产物目录）。
#sqlite3.c与 `main.cpp` 并列
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
SOURCES cameraworker.cpp cameraworker.h facerecognitionmanager.cpp facerecognitionmanager.h modelpaths.cpp modelpaths.h
SOURCES facerecognitionhandleworker.h facerecognitionhandleworker.cpp
SOURCES facedetector.cpp facedetector.h
SOURCES livenessdetector.cpp livenessdetector.h
SOURCES dlibutil.cpp dlibutil.h
SOURCES speechutils.cpp speechutils.h
# 实时天气服务（HTTP 请求 + JSON 解析 + 图标路径）
SOURCES weatherservice.cpp weatherservice.h
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
# 【模型目录】编译进程序（modelpaths.cpp 里再用环境变量 FACE_MODEL_DIR 覆盖）
target_compile_definitions(appaccessControlSystem_faceRecognition PRIVATE
    FACE_MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/model"
    # 天气图标目录，运行时可被环境变量 WEATHER_IMG_DIR 覆盖
    WEATHER_IMG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/weather"
)
target_link_libraries(appaccessControlSystem_faceRecognition
    PRIVATE Qt6::Quick
    Qt6::Mqtt
    Qt6::Multimedia
    Qt6::TextToSpeech
    Qt6::QuickControls2
    Qt6::Network#请求易客天气 HTTP 接口//Network是Qt6的网络模块，用于发送HTTP请求
    dlib::dlib
)

# 【接 OpenCV】兼容"标准安装"和本机"扁平安装"两种情况（沿用 Day3 写法）
if(OpenCV_FOUND)
    message(STATUS "OpenCV: 使用 find_package 的结果 (版本 ${OpenCV_VERSION})")
    target_include_directories(appaccessControlSystem_faceRecognition PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(appaccessControlSystem_faceRecognition PRIVATE ${OpenCV_LIBS})
else()
    message(STATUS "OpenCV: find_package 没找到，改用手动指定路径")
    target_include_directories(appaccessControlSystem_faceRecognition PRIVATE "${OpenCV_ROOT_DIR}/include")
    target_link_directories(appaccessControlSystem_faceRecognition PRIVATE "${OpenCV_ROOT_DIR}/lib")
    target_link_libraries(appaccessControlSystem_faceRecognition PRIVATE
        opencv_core480 opencv_imgproc480 opencv_imgcodecs480
        opencv_videoio480 opencv_highgui480 opencv_dnn480)
endif()
# 运行时能找到 OpenCV 的 DLL
set(ENV{PATH} "${OpenCV_ROOT_DIR}/bin;$ENV{PATH}")

install(TARGETS appaccessControlSystem_faceRecognition
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
