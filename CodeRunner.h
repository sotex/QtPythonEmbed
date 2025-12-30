#pragma once

#include <QObject>
#include <QString>

#include <Python.h>

/**
 * @class CodeRunner
 * @brief 负责在独立线程中安全执行Python代码
 *
 * 这个类解决了原有的线程安全问题：
 * - 正确的GIL管理
 * - 线程隔离的Python执行环境
 * - 完善的异常处理
 * - 代码执行状态跟踪
 */
class CodeRunner : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CodeRunner(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CodeRunner();

signals:
    /**
     * @brief 代码执行开始信号
     */
    void executionStarted();

    /**
     * @brief 代码执行完成信号
     */
    void executionFinished();

    /**
     * @brief 行执行信号（用于高亮显示）
     * @param lineNumber 行号
     */
    void lineExecuted(int lineNumber);

    /**
     * @brief 接收Python输出信号
     * @param text 输出文本
     */
    void outputReceived(const QString& text);

    /**
     * @brief 错误发生信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);

    /**
     * @brief 执行进度信号
     * @param current 当前行
     * @param total 总行数
     */
    void progressUpdated(int current, int total);

public slots:
    /**
     * @brief 运行Python代码
     * @param code Python代码
     */
    void runCode(const QString& code);

    /**
     * @brief 中止代码执行
     */
    void abortExecution();

    /**
     * @brief 设置执行速度（用于调试）
     * @param delayMs 延迟毫秒数
     */
    void setExecutionDelay(int delayMs);

private:
    /**
     * @brief Python追踪函数
     * @param obj 调用对象
     * @param frame Python栈帧
     * @param event 事件类型
     * @param arg 事件参数
     * @return int 执行结果
     */
    static int pythonTraceFunction(PyObject* obj, PyFrameObject* frame, int event, PyObject* arg);

    /**
     * @brief 获取行号
     * @param frame Python栈帧
     * @return int 行号
     */
    int getLineNumber(PyFrameObject* frame) const;

    /**
     * @brief 获取文件名
     * @param frame Python栈帧
     * @return QString 文件名
     */
    QString getFileName(PyFrameObject* frame) const;

    /**
     * @brief 安全的Python代码执行
     * @param code Python代码
     */
    void executePythonCodeSafely(const QString& code);

    /**
     * @brief 处理Python异常
     * @param exc Python异常对象
     */
    void handlePythonException(void* exc);

private:
    bool m_isExecuting    = false;
    bool m_shouldAbort    = false;
    int  m_executionDelay = 0;
    int  m_currentLine    = -1;
};
