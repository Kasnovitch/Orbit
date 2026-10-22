cmake_minimum_required(VERSION 3.21)
project(Orbit LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

qt_add_executable(Orbit WIN32
    third_party/libsodium/compat_mingw.c
    src/app/bootstrap/Main.cpp
    src/app/Application.cpp
    src/ui/auth/LoginScreen.cpp
    src/ui/main_window/MainWindow.cpp
    src/ui/dashboard/Dashboard.cpp
    src/ui/dashboard/project_selector/ProjectSelector.cpp
    src/ui/dashboard/countdown_panel/CountdownPanel.cpp
    src/ui/dashboard/task_panel/TaskPanel.cpp
    src/ui/dashboard/links_panel/LinksPanel.cpp
    src/ui/dashboard/files_panel/FilesPanel.cpp
    src/ui/projects/ProjectsScreen.cpp
    src/ui/settings/SettingsScreen.cpp
    src/ui/widgets/Widgets.cpp
    src/ui/styles/Theme.cpp
    src/infrastructure/config/AppConfig.cpp
    src/data/database/Database.cpp
    src/data/auth/AuthService.cpp
    src/data/auth/SessionWatch.cpp
    src/services/projects/ProjectService.cpp
    src/services/permissions/PermissionService.cpp
    src/services/tasks/TaskService.cpp
    src/services/countdowns/CountdownService.cpp
    src/services/links/LinkService.cpp
    src/services/files/FileService.cpp
    src/services/settings/SettingsService.cpp
    src/services/audit/AuditService.cpp
    src/services/notifications/NotificationService.cpp
    src/cache/Cache.cpp
    src/ui/resources/Resources.qrc
)

target_include_directories(Orbit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsodium/include)

target_link_libraries(Orbit PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsodium/lib/libsodium.a bcrypt)

# console shell: banner + "request users" style commands, same postgres db
qt_add_executable(OrbitConsole
    src/console/ConsoleMain.cpp
    src/infrastructure/config/AppConfig.cpp
    src/data/database/Database.cpp
)

target_include_directories(OrbitConsole PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(OrbitConsole PRIVATE Qt6::Core Qt6::Sql)
