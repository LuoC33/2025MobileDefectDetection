cmake_minimum_required(VERSION 3.20)
project(qrsds)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt6组件（添加Multimedia）
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    Charts
    QuickControls2
    Widgets
    Network
    Multimedia
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    loginmanager.h
    loginmanager.cpp
    src.qrc
    TcpServer.h
    TcpServer.cpp
    UdpServer.h
    UdpServer.cpp
    ChartDataManager.h
    ChartDataManager.cpp
)

# 链接库（添加Multimedia）
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Charts
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
