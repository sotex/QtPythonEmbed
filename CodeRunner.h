#pragma once

#include <QMutex>
#include <QObject>
#include <QSet>
#include <QString>
#include <QWaitCondition>

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
     * @brief 调试状态枚举
     */
    enum DebugState
    {
        Running,    // 正常运行
        Paused,     // 暂停
        StepInto,   // 逐语句
        StepOver,   // 逐过程
        StepOut     // 跳出
    };
    Q_ENUM(DebugState)

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

    /**
     * @brief 调试状态变化信号
     * @param state 新的调试状态
     */
    void debugStateChanged(int state);

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

    /**
     * @brief 继续执行
     */
    void continueExecution();

    /**
     * @brief 逐语句执行
     */
    void stepInto();

    /**
     * @brief 逐过程执行
     */
    void stepOver();

    /**
     * @brief 跳出当前函数
     */
    void stepOut();

    /**
     * @brief 设置断点列表
     * @param breakpoints 断点行号集合
     */
    void setBreakpoints(const QSet<int>& breakpoints);

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

    /**
     * @brief 检查当前行是否是断点
     * @param lineNumber 行号
     * @return bool 是否是断点
     */
    bool isBreakpoint(int lineNumber) const;

private:
    bool           m_isExecuting    = false;
    bool           m_shouldAbort    = false;
    int            m_executionDelay = 0;
    int            m_currentLine    = -1;
    DebugState     m_debugState     = Running;
    QSet<int>      m_breakpoints;     // 断点行号集合
    int            m_callDepth = 0;   // 当前调用深度，用于逐过程和跳出
    QMutex         m_debugMutex;
    QWaitCondition m_debugCondition;
};
