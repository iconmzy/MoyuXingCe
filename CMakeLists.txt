cmake_minimum_required(VERSION 3.25)

project(KeepGongLearning VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS
    Network
    Widgets
)
qt_standard_project_setup()

qt_add_executable(KeepGongLearning WIN32
    apps/desktop/main.cpp
    src/domain/entities/OnlineQuestion.h
    src/domain/repositories/INotesRepository.h
    src/application/controllers/MainController.cpp
    src/application/controllers/MainController.h
    src/infrastructure/logging/AppLogger.cpp
    src/infrastructure/logging/AppLogger.h
    src/infrastructure/network/BalaApiClient.cpp
    src/infrastructure/network/BalaApiClient.h
    src/infrastructure/storage/JsonFileStorage.cpp
    src/infrastructure/storage/JsonFileStorage.h
    src/infrastructure/storage/JsonNotesRepository.cpp
    src/infrastructure/storage/JsonNotesRepository.h
    src/presentation/views/MainWindow.cpp
    src/presentation/views/MainWindow.h
)

target_include_directories(KeepGongLearning PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(KeepGongLearning PRIVATE
    Qt6::Network
    Qt6::Widgets
)

if(MSVC)
    target_compile_options(KeepGongLearning PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(KeepGongLearning PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
install(TARGETS KeepGongLearning
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
