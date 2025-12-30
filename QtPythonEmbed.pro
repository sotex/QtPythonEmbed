QT += core gui widgets


HEADERS += \
    CodeRunner.h \
    ConfigManager.h \
    PyEditor.h \
    PyWindow.h \
    PythonInterpreterManager.h

SOURCES += \
    CodeRunner.cpp \
    ConfigManager.cpp \
    PyEditor.cpp \
    PyWindow.cpp \
    PythonInterpreterManager.cpp \
    main.cpp

PYTHON_ROOT=C:/Users/w/.conda/pkgs/python-3.10.19-h981015d_0

INCLUDEPATH += $$PWD/3rd/include
INCLUDEPATH += $$PYTHON_ROOT/include

win32:CONFIG(release, debug|release): LIBS += -L$$PYTHON_ROOT/libs/ -lpython310
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PYTHON_ROOT/libs/ -lpython310_d
