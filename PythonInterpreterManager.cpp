#include "PythonInterpreterManager.h"
#include "CodeRunner.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include <iostream>
#include <stdexcept>

namespace py = pybind11;

// C++测试函数，用于嵌入式模块
int testCppFunction(const std::string& input, std::string* output)
{
    std::cout << "C++ test called with input: " << input << std::endl;
    *output = "Processed by C++: " + input;
    return static_cast<int>(input.size());
}

// Pybind11嵌入式模块定义
PYBIND11_EMBEDDED_MODULE(cpp_module, m)
{
    m.def(
        "test",
        [](const std::string& input) {
            std::string output;
            int         result = testCppFunction(input, &output);
            return py::make_tuple(result, output);
        },
        py::arg("input"));

    m.def("get_version", []() { return "1.0.0"; });
}

PythonInterpreterManager& PythonInterpreterManager::instance()
{
    static PythonInterpreterManager instance;
    return instance;
}

PythonInterpreterManager::PythonInterpreterManager(QObject* parent)
    : QObject(parent)
{}

PythonInterpreterManager::~PythonInterpreterManager()
{
    if (m_initialized) {
        cleanup();
    }
}

bool PythonInterpreterManager::initialize(const QString& configFile)
{
    if (m_initialized) {
        qWarning() << "Python interpreter is already initialized";
        return true;
    }

    try {
        // 设置程序名称
        Py_SetProgramName(L"QtPythonEmbedTest2");

        // 加载配置
        m_configFile = configFile;
        loadConfiguration(configFile);

        // 设置环境
        setupEnvironment();

        // 初始化Python解释器
        py::initialize_interpreter();

        // 保存主线程状态
        m_mainThreadState = PyEval_SaveThread();

        m_initialized = true;
        emit initialized();

        qDebug() << "Python interpreter initialized successfully";
        qDebug() << "Python version:" << getPythonVersion();

        return true;
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to initialize Python interpreter:" << e.what();
        return false;
    }
    catch (...) {
        qCritical() << "Failed to initialize Python interpreter: Unknown error";
        return false;
    }
}

void PythonInterpreterManager::cleanup()
{
    if (!m_initialized) {
        return;
    }

    try {
        // 恢复线程状态
        if (m_mainThreadState) {
            PyEval_RestoreThread(m_mainThreadState);
            m_mainThreadState = nullptr;
        }

        // 清理嵌入式模块
        // Py_Finalize() 会自动清理模块

        // 最终化解释器
        py::finalize_interpreter();

        m_initialized = false;
        emit cleaned();

        qDebug() << "Python interpreter cleaned up successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Error during Python cleanup:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown error during Python cleanup";
    }
}

QString PythonInterpreterManager::getPythonVersion() const
{
    if (!m_initialized) {
        return QString("Not initialized");
    }

    try {
        py::gil_scoped_acquire acquire;
        PyObject*              version = PySys_GetObject("version");
        if (version && PyUnicode_Check(version)) {
            return QString::fromUtf8(PyUnicode_AsUTF8(version));
        }
    }
    catch (...) {
        // 忽略异常
    }

    return QString("Unknown");
}

void PythonInterpreterManager::setPythonHome(const QString& path)
{
    m_pythonHome = path;
    if (m_initialized) {
        Py_SetPythonHome(path.toStdWString().c_str());
    }
}

void PythonInterpreterManager::addPythonPath(const QString& path)
{
    if (!m_pythonPaths.contains(path)) {
        m_pythonPaths.append(path);
    }
}

QStringList PythonInterpreterManager::getPythonPaths() const
{
    if (!m_initialized) {
        return m_pythonPaths;
    }

    try {
        py::gil_scoped_acquire acquire;
        PyObject*              sysPath = PySys_GetObject("path");
        if (sysPath && PyList_Check(sysPath)) {
            QStringList paths;
            for (Py_ssize_t i = 0; i < PyList_Size(sysPath); ++i) {
                PyObject* item = PyList_GetItem(sysPath, i);
                if (item && PyUnicode_Check(item)) {
                    paths.append(QString::fromUtf8(PyUnicode_AsUTF8(item)));
                }
            }
            return paths;
        }
    }
    catch (...) {
        // 忽略异常
    }

    return m_pythonPaths;
}

void PythonInterpreterManager::registerEmbeddedModule(const char* moduleName,
                                                      PyObject* (*initFunc)(void))
{
    if (!m_initialized) {
        qWarning() << "Cannot register module" << moduleName << "- Python not initialized";
        return;
    }

    try {
        py::gil_scoped_acquire acquire;
        PyImport_AppendInittab(moduleName, initFunc);
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to register module" << moduleName << ":" << e.what();
    }
}

void PythonInterpreterManager::registerEmbeddedModule(const char*  moduleName,
                                                      PyModuleDef* moduleDef)
{
    Q_UNUSED(moduleDef);

    if (!m_initialized) {
        qWarning() << "Cannot register module" << moduleName << "- Python not initialized";
        return;
    }

    try {
        py::gil_scoped_acquire acquire;
        // 嵌入式模块已经通过PYBIND11_EMBEDDED_MODULE自动注册，不需要手动添加到init tab
        // 或者可以使用PyModule_Create来创建模块，但需要正确的初始化函数
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to register module" << moduleName << ":" << e.what();
    }
}

py::object PythonInterpreterManager::executeCode(const QString& code, py::object* globalDict,
                                                 py::object* localDict)
{
    if (!m_initialized) {
        throw std::runtime_error("Python interpreter not initialized");
    }

    try {
        py::gil_scoped_acquire acquire;

        if (globalDict && localDict) {
            py::exec(code.toStdString().c_str(), *globalDict, *localDict);
        }
        else if (globalDict) {
            py::exec(code.toStdString().c_str(), *globalDict);
        }
        else if (localDict) {
            py::exec(code.toStdString().c_str(), py::globals(), *localDict);
        }
        else {
            py::exec(code.toStdString().c_str());
        }

        return py::none();
    }
    catch (const py::error_already_set& e) {
        QString errorMsg = QString("Python execution error: %1").arg(QString::fromUtf8(e.what()));
        emit    pythonError(errorMsg);
        throw std::runtime_error(errorMsg.toStdString());
    }
    catch (const std::exception& e) {
        QString errorMsg = QString("C++ exception during Python execution: %1").arg(e.what());
        emit    pythonError(errorMsg);
        throw;
    }
}

void PythonInterpreterManager::redirectPythonOutput(
    const std::function<void(const QString&)>& callback)
{
    m_outputCallback = callback;

    if (!m_initialized) {
        qWarning() << "Cannot redirect output - Python not initialized";
        return;
    }

    try {
        py::gil_scoped_acquire acquire;

        // 导入sys模块
        py::module_ sys = py::module_::import("sys");

        // 创建一个简单的重定向类
        py::exec(R"(
class OutputRedirector:
    def __init__(self, callback):
        self.callback = callback
    
    def write(self, text):
        self.callback(text)
    
    def flush(self):
        pass
)");

        // 获取OutputRedirector类
        py::object redirectorClass = py::module_::import("__main__").attr("OutputRedirector");

        // 创建一个可调用对象，包装我们的C++回调
        py::object pyCallback = py::cpp_function([this](const std::string& text) {
            if (m_outputCallback) {
                m_outputCallback(QString::fromStdString(text));
            }
        });

        // 创建一个OutputRedirector实例
        py::object redirector = redirectorClass(pyCallback);

        // 设置sys.stdout和sys.stderr
        sys.attr("stdout") = redirector;
        sys.attr("stderr") = redirector;

        qDebug() << "Python output redirection configured successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to redirect Python output:" << e.what();
    }
}

void PythonInterpreterManager::loadConfiguration(const QString& configFile)
{
    QString configPath = configFile;
    if (configPath.isEmpty()) {
        // 使用默认配置文件位置
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        configPath         = QDir(appDataDir).filePath("python_config.ini");
    }

    QSettings settings(configPath, QSettings::IniFormat);

    // 加载Python路径配置
    m_pythonHome  = settings.value("Python/pythonHome", "").toString();
    m_pythonPaths = settings.value("Python/pythonPaths", "").toStringList();

    // 如果没有配置，尝试自动检测
    if (m_pythonHome.isEmpty()) {
        // 尝试从conda环境检测
        QString condaPath = QDir::homePath() + "/.conda/envs/py310";
        if (QDir(condaPath).exists()) {
            m_pythonHome = condaPath;
        }
    }

    qDebug() << "Loaded Python configuration:";
    qDebug() << "  Python Home:" << m_pythonHome;
    qDebug() << "  Python Paths:" << m_pythonPaths;
}

void PythonInterpreterManager::setupEnvironment()
{
    // 设置Python Home
    if (!m_pythonHome.isEmpty()) {
        Py_SetPythonHome(m_pythonHome.toStdWString().c_str());

        // 更新PATH环境变量
        QByteArray path = qgetenv("PATH");
        if (!path.contains(m_pythonHome.toUtf8())) {
            path.insert(0, (m_pythonHome + ";").toUtf8());
            path.insert(0, (m_pythonHome + "\\Library\\bin;").toUtf8());
            path.insert(0, (m_pythonHome + "\\Scripts;").toUtf8());
            path.insert(0, (m_pythonHome + "\\bin;").toUtf8());
            qputenv("PATH", path);
        }
    }
}

void PythonInterpreterManager::setupPythonPaths()
{
    if (!m_initialized || m_pythonPaths.isEmpty()) {
        return;
    }

    try {
        py::gil_scoped_acquire acquire;
        py::module_            sys  = py::module_::import("sys");
        py::object             path = sys.attr("path");

        for (const QString& pythonPath : m_pythonPaths) {
            if (QDir(pythonPath).exists()) {
                path.attr("append")(pythonPath.toStdString());
            }
        }
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to setup Python paths:" << e.what();
    }
}

void PythonInterpreterManager::initializeEmbeddedModules()
{
    // cpp_module 已经在文件顶部通过 PYBIND11_EMBEDDED_MODULE 自动注册
    qDebug() << "Embedded modules initialized";
}

void PythonInterpreterManager::installTraceHook(Py_tracefunc traceFunc)
{
    if (!m_initialized) {
        return;
    }

    try {
        py::gil_scoped_acquire acquire;
        PyEval_SetTrace(traceFunc, nullptr);
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to install trace hook:" << e.what();
    }
}
