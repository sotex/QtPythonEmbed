#pragma once

#include <QObject>
#include <QString>
#include <QSettings>

#define PYBIND11_NO_ASSERT_GIL_HELD_INCREF_DECREF 1

#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

/**
 * @class PythonInterpreterManager
 * @brief 负责Python解释器的生命周期管理和配置
 * 
 * 这个类提供：
 * - Python解释器的安全初始化和销毁
 * - Python路径和环境的配置管理
 * - 嵌入式模块注册
 * - 全局Python设置管理
 */
class PythonInterpreterManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return PythonInterpreterManager& 单例引用
     */
    static PythonInterpreterManager& instance();

    /**
     * @brief 初始化Python解释器
     * @param configFile 配置文件路径（可选）
     * @return bool 初始化是否成功
     */
    bool initialize(const QString& configFile = QString());

    /**
     * @brief 清理Python解释器资源
     */
    void cleanup();

    /**
     * @brief 检查Python解释器是否已初始化
     * @return bool 是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取Python版本信息
     * @return QString Python版本字符串
     */
    QString getPythonVersion() const;

    /**
     * @brief 设置Python主目录
     * @param path Python安装路径
     */
    void setPythonHome(const QString& path);

    /**
     * @brief 添加Python路径
     * @param path 要添加的路径
     */
    void addPythonPath(const QString& path);

    /**
     * @brief 获取当前Python路径列表
     * @return QStringList Python路径列表
     */
    QStringList getPythonPaths() const;

    /**
     * @brief 注册嵌入式C++模块
     * @param moduleName 模块名称
     * @param initFunc 初始化函数
     */
    void registerEmbeddedModule(const char* moduleName, PyObject* (*initFunc)(void));
    
    /**
     * @brief 注册嵌入式C++模块（重载版本，接受PyModuleDef*）
     * @param moduleName 模块名称
     * @param moduleDef 模块定义
     */
    void registerEmbeddedModule(const char* moduleName, PyModuleDef* moduleDef);

    /**
     * @brief 执行Python代码
     * @param code Python代码字符串
     * @param globalDict 全局字典（可选）
     * @param localDict 局部字典（可选）
     * @return py::object 执行结果
     */
    py::object executeCode(const QString& code, 
                          py::object* globalDict = nullptr, 
                          py::object* localDict = nullptr);

    /**
     * @brief 重定向Python输出到C++回调
     * @param callback 输出回调函数
     */
    void redirectPythonOutput(const std::function<void(const QString&)>& callback);

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

    /**
     * @brief 初始化完成信号
     */
    void initialized();

    /**
     * @brief 清理完成信号
     */
    void cleaned();

private:
    explicit PythonInterpreterManager(QObject* parent = nullptr);
    ~PythonInterpreterManager();

    // 禁用拷贝和赋值
    PythonInterpreterManager(const PythonInterpreterManager&) = delete;
    PythonInterpreterManager& operator=(const PythonInterpreterManager&) = delete;

    /**
     * @brief 从配置文件加载设置
     * @param configFile 配置文件路径
     */
    void loadConfiguration(const QString& configFile);

    /**
     * @brief 设置环境变量
     */
    void setupEnvironment();

    /**
     * @brief 配置Python路径
     */
    void setupPythonPaths();

    /**
     * @brief 初始化嵌入式模块
     */
    void initializeEmbeddedModules();

    /**
     * @brief 安装Python追踪钩子
     * @param traceFunc 追踪函数
     */
    void installTraceHook(Py_tracefunc traceFunc);

private:
    bool m_initialized = false;
    QString m_pythonHome;
    QStringList m_pythonPaths;
    QString m_configFile;
    std::function<void(const QString&)> m_outputCallback;

    // Python线程状态管理
    PyThreadState* m_mainThreadState = nullptr;
};