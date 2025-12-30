#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QTimer>

class PyEditor;
class QTextEdit;
class CodeRunner;
class PythonInterpreterManager;

/**
 * @class PyWindow
 * @brief 主窗口类，负责Python代码编辑器和运行器的集成
 *
 * 重构后的特点：
 * - 使用单例模式的Python解释器管理器
 * - 更好的异常处理和用户反馈
 * - 可配置的Python环境设置
 * - 改进的UI响应性
 */
class PyWindow : public QMainWindow
{
    Q_OBJECT

public:
    PyWindow(QWidget* parent = nullptr);

    ~PyWindow();

signals:
    /**
     * @brief Python输出信号
     * @param text 输出文本
     */
    void pythonOutput(const QString& text);

    /**
     * @brief Python错误信号
     * @param error 错误文本
     */
    void pythonError(const QString& error);

private slots:
    /**
     * @brief 运行Python代码
     */
    void runPythonCode();

    /**
     * @brief 追加输出文本
     * @param text 输出文本
     */
    void appendOutput(const QString& text);

    /**
     * @brief 追加错误文本
     * @param text 错误文本
     */
    void appendError(const QString& text);

    /**
     * @brief 执行开始处理
     */
    void onExecutionStart();

    /**
     * @brief 执行完成处理
     */
    void onExecutionFinish();

    /**
     * @brief Python解释器状态变化处理
     */
    void onPythonInitialized();

    /**
     * @brief 显示设置对话框
     */
    void showSettings();

    /**
     * @brief 应用设置
     */
    void applySettings();

    /**
     * @about 加载示例代码
     */
    void loadExampleCode();

    /**
     * @brief 保存当前代码
     */
    void saveCurrentCode();

    /**
     * @brief 加载保存的代码
     */
    void loadSavedCode();

private:
    /**
     * @brief 初始化UI界面
     */
    void initializeUI();

    /**
     * @brief 初始化Python解释器
     */
    void initializePython();

    /**
     * @brief 连接信号和槽
     */
    void connectSignals();

    /**
     * @brief 加载窗口设置
     */
    void loadWindowSettings();

    /**
     * @brief 保存窗口设置
     */
    void saveWindowSettings();

    /**
     * @brief 更新执行按钮状态
     */
    void updateExecutionButtons();

    /**
     * @brief 清除输出
     */
    void clearOutput();

    /**
     * @brief 关闭事件
     */
    void closeEvent(QCloseEvent* event);

private:
    // UI组件
    PyEditor*    m_codeEditor     = nullptr;
    QTextEdit*   m_logOutput      = nullptr;
    QPushButton* m_runButton      = nullptr;
    QPushButton* m_clearButton    = nullptr;
    QPushButton* m_settingsButton = nullptr;
    QPushButton* m_saveButton     = nullptr;

    // 核心组件
    CodeRunner*               m_runner        = nullptr;
    QThread*                  m_runnerThread  = nullptr;
    PythonInterpreterManager* m_pythonManager = nullptr;

    // 状态管理
    bool      m_isExecuting = false;
    QSettings m_settings;
    QString   m_lastSavedCode;
    QString   m_settingsFile;

    // 示例代码
    QString m_exampleCode;
};
