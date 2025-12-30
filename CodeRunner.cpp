#include "CodeRunner.h"
#include "PythonInterpreterManager.h"

#include <QDebug>
#include <QThread>
#include <QMetaObject>
#include <QCoreApplication>

#include <Python.h>
#include <frameobject.h>

// 全局变量，用于在静态追踪函数中访问CodeRunner实例
static CodeRunner* g_currentRunner = nullptr;

CodeRunner::CodeRunner(QObject* parent)
    : QObject(parent)
{
    g_currentRunner = this;
}

CodeRunner::~CodeRunner()
{
    abortExecution();
}

void CodeRunner::runCode(const QString& code)
{
    if (m_isExecuting) {
        qWarning() << "Code execution already in progress";
        return;
    }

    m_shouldAbort = false;
    m_isExecuting = true;

    // 在事件循环中异步执行，避免阻塞UI线程
    QMetaObject::invokeMethod(this, [this, code]() {
        executePythonCodeSafely(code);
    }, Qt::QueuedConnection);
}

void CodeRunner::abortExecution()
{
    m_shouldAbort = true;
}

void CodeRunner::setExecutionDelay(int delayMs)
{
    m_executionDelay = delayMs;
}

int CodeRunner::pythonTraceFunction(PyObject* obj, PyFrameObject* frame, int event, PyObject* arg)
{
    Q_UNUSED(obj);
    Q_UNUSED(event);
    Q_UNUSED(arg);
    
    if (g_currentRunner && !g_currentRunner->m_shouldAbort && frame) {
        // 在追踪函数中只获取行号，不发出信号
        int lineNumber = PyFrame_GetLineNumber(frame);
        
        // 使用Qt的invokeMethod在主线程中发出信号
        QMetaObject::invokeMethod(g_currentRunner, [lineNumber]() {
            emit g_currentRunner->lineExecuted(lineNumber);
        }, Qt::QueuedConnection);
    }
    
    // 追踪函数必须返回0，否则会导致Python解释器崩溃
    return 0;
}

int CodeRunner::getLineNumber(PyFrameObject* frame) const
{
    if (!frame) {
        return -1;
    }

    int lineNumber = PyFrame_GetLineNumber(frame);
    
    return lineNumber;
}

QString CodeRunner::getFileName(PyFrameObject* frame) const
{
    if (!frame) {
        return QString();
    }

    PyObject* filename = frame->f_code->co_filename;
    
    if (filename && PyUnicode_Check(filename)) {
        return QString::fromUtf8(PyUnicode_AsUTF8(filename));
    }
    
    return QString();
}

void CodeRunner::executePythonCodeSafely(const QString& code)
{
    emit executionStarted();

    try {
        // 获取Python解释器管理器实例
        PythonInterpreterManager& pyManager = PythonInterpreterManager::instance();
        
        if (!pyManager.isInitialized()) {
            throw std::runtime_error("Python interpreter not initialized");
        }

        // 获取GIL - 在子线程中执行Python代码时必须获取GIL
        PyGILState_STATE gstate = PyGILState_Ensure();
        
        try {
            // 设置Python追踪函数
            PyEval_SetTrace(pythonTraceFunction, nullptr);

            // 重定向Python输出
            pyManager.redirectPythonOutput([this](const QString& text) {
                // 在UI线程中发出信号
                QMetaObject::invokeMethod(this, [this, text]() {
                    emit outputReceived(text);
                });
            });

            // 执行代码
            py::object result = pyManager.executeCode(code);

            // 清除追踪函数
            PyEval_SetTrace(nullptr, nullptr);
            
            // 检查是否需要中止
            if (m_shouldAbort) {
                PyErr_SetString(PyExc_KeyboardInterrupt, "User aborted execution");
            }

        } catch (...) {
            // 释放GIL
            PyGILState_Release(gstate);
            
            // 处理Python异常
            if (PyErr_Occurred()) {
                PyObject *exc_type, *exc_value, *exc_tb;
                PyErr_Fetch(&exc_type, &exc_value, &exc_tb);

                QString errorMsg;
                if (exc_value && PyUnicode_Check(exc_value)) {
                    errorMsg = QString::fromUtf8(PyUnicode_AsUTF8(exc_value));
                } else {
                    errorMsg = "Unknown Python error";
                }

                QMetaObject::invokeMethod(this, [this, errorMsg]() {
                    emit errorOccurred(errorMsg);
                });
            } else {
                QMetaObject::invokeMethod(this, [this]() {
                    emit errorOccurred("Unknown error during execution");
                });
            }

            m_isExecuting = false;
            emit executionFinished();
            return;
        }

        // 释放GIL
        PyGILState_Release(gstate);

        // 发送完成信号
        QMetaObject::invokeMethod(this, [this]() {
            emit executionFinished();
        });

    } catch (const std::exception& e) {
        QString errorMsg = QString("Execution error: %1").arg(e.what());
        QMetaObject::invokeMethod(this, [this, errorMsg]() {
            emit errorOccurred(errorMsg);
        });
    } catch (...) {
        QMetaObject::invokeMethod(this, [this]() {
            emit errorOccurred("Unknown error occurred");
        });
    }

    m_isExecuting = false;
}

void CodeRunner::handlePythonException(void* exc)
{
    // 这个函数暂时未使用，保留用于将来扩展
    Q_UNUSED(exc)
}