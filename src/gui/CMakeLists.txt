# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)

# Qt5Keychain requires Qt5DBus, but doesn't quite set this up right
find_package(Qt5DBus REQUIRED)
find_package(Qt5Keychain REQUIRED)

set(QACTUS_SRC
    main.cpp
    mainwindow.cpp
    trayicon.cpp
    configure.cpp
    login.cpp
    roweditor.cpp
    requeststateeditor.cpp
    autotooltipdelegate.cpp
    requesttreewidgetitem.cpp
    utils.cpp
    credentials.cpp
    syntaxhighlighter.cpp
    browserfilter.cpp
    createdialog.cpp
    filetreewidget.cpp
    buildlogviewer.cpp
    projecttreewidget.cpp
    monitortreewidget.cpp
    packagetreewidget.cpp
    buildresulttreewidget.cpp
    searchwidget.cpp
    loghighlighter.cpp)

set(QACTUS_HDR
    mainwindow.h
    trayicon.h
    configure.h
    login.h
    roweditor.h
    requeststateeditor.h
    autotooltipdelegate.h
    requesttreewidgetitem.h
    utils.h
    credentials.h
    syntaxhighlighter.h
    browserfilter.h
    createdialog.h
    filetreewidget.h
    buildlogviewer.h
    projecttreewidget.h
    monitortreewidget.h
    packagetreewidget.h
    buildresulttreewidget.h
    searchwidget.h
    loghighlighter.h)

set(QACTUS_UI
    mainwindow.ui
    configure.ui
    login.ui
    roweditor.ui
    requeststateeditor.ui
    browserfilter.ui
    createdialog.ui
    buildlogviewer.ui
    searchwidget.ui)

set(QACTUS_RC
    ../../application.qrc)

qt5_wrap_ui(QACTUS_UI_SRC ${QACTUS_UI})

qt5_add_resources(QACTUS_RC_SRC ${QACTUS_RC})

add_executable(qactus ${QACTUS_SRC} ${QACTUS_UI_SRC} ${QACTUS_RC_SRC})

add_dependencies(qactus libqobs)

target_include_directories(qactus PRIVATE ../qobs)

target_link_libraries(qactus libqobs Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network ${QTKEYCHAIN_LIBRARIES})

install(TARGETS qactus RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES ${CMAKE_SOURCE_DIR}/qactus.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES ${CMAKE_SOURCE_DIR}/icons/qactus.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps)