#include "ConfigManager.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

void ConfigManager::initialize(const QString& configFile)
{
    if (m_initialized) {
        return;
    }

    // 确定配置文件路径
    if (!configFile.isEmpty()) {
        m_configFile = configFile;
    } else {
        QString appName = QApplication::applicationName();
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        m_configFile = configDir + "/" + appName + ".ini";
    }

    // 创建设置对象
    m_settings = new QSettings(m_configFile, QSettings::IniFormat, this);
    m_settings->setParent(this);

    // 加载配置
    load();

    m_initialized = true;
    qDebug() << "ConfigManager initialized with config file:" << m_configFile;
}

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_editorFont("Consolas")
    , m_editorFontSize(12)
    , m_autoSaveInterval(30)
    , m_executionDelay(100)
    , m_theme("light")
{
}

ConfigManager::~ConfigManager()
{
    if (m_settings) {
        m_settings->sync();
    }
}

QString ConfigManager::getPythonHome() const
{
    return m_pythonHome;
}

void ConfigManager::setPythonHome(const QString& path)
{
    if (m_pythonHome != path) {
        m_pythonHome = path;
        m_settings->setValue("Python/home", m_pythonHome);
        emit configurationChanged();
        emit pythonPathsChanged();
    }
}

QString ConfigManager::autoDetectPython()
{
    QString detectedPath;

    // 首先尝试Conda环境
    detectedPath = detectCondaPython();
    if (detectedPath.isEmpty()) {
        // 然后尝试系统Python
        detectedPath = detectSystemPython();
    }

    if (!detectedPath.isEmpty()) {
        setPythonHome(detectedPath);
        qDebug() << "Auto-detected Python at:" << detectedPath;
    }

    return detectedPath;
}

QStringList ConfigManager::getPythonPaths() const
{
    return m_pythonPaths;
}

void ConfigManager::addPythonPath(const QString& path)
{
    if (!m_pythonPaths.contains(path)) {
        m_pythonPaths.append(path);
        m_settings->setValue("Python/paths", m_pythonPaths);
        emit pythonPathsChanged();
    }
}

QString ConfigManager::getEditorFont() const
{
    return m_editorFont;
}

void ConfigManager::setEditorFont(const QString& font)
{
    if (m_editorFont != font) {
        m_editorFont = font;
        m_settings->setValue("Editor/font", m_editorFont);
        emit editorSettingsChanged();
    }
}

int ConfigManager::getEditorFontSize() const
{
    return m_editorFontSize;
}

void ConfigManager::setEditorFontSize(int size)
{
    if (m_editorFontSize != size && size > 5 && size < 72) {
        m_editorFontSize = size;
        m_settings->setValue("Editor/fontSize", m_editorFontSize);
        emit editorSettingsChanged();
    }
}

int ConfigManager::getAutoSaveInterval() const
{
    return m_autoSaveInterval;
}

void ConfigManager::setAutoSaveInterval(int seconds)
{
    if (m_autoSaveInterval != seconds && seconds > 0) {
        m_autoSaveInterval = seconds;
        m_settings->setValue("Editor/autoSaveInterval", m_autoSaveInterval);
        emit configurationChanged();
    }
}

int ConfigManager::getExecutionDelay() const
{
    return m_executionDelay;
}

void ConfigManager::setExecutionDelay(int delayMs)
{
    if (m_executionDelay != delayMs && delayMs >= 0) {
        m_executionDelay = delayMs;
        m_settings->setValue("Application/executionDelay", m_executionDelay);
        emit configurationChanged();
    }
}

QString ConfigManager::getTheme() const
{
    return m_theme;
}

void ConfigManager::setTheme(const QString& theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        m_settings->setValue("Application/theme", m_theme);
        emit configurationChanged();
    }
}

QByteArray ConfigManager::getWindowGeometry() const
{
    return m_settings->value("Window/geometry").toByteArray();
}

void ConfigManager::setWindowGeometry(const QByteArray& geometry)
{
    m_settings->setValue("Window/geometry", geometry);
}

QByteArray ConfigManager::getWindowState() const
{
    return m_settings->value("Window/state").toByteArray();
}

void ConfigManager::setWindowState(const QByteArray& state)
{
    m_settings->setValue("Window/state", state);
}

void ConfigManager::resetToDefaults()
{
    createDefaultConfiguration();
    emit configurationChanged();
    emit editorSettingsChanged();
    emit pythonPathsChanged();
}

void ConfigManager::save()
{
    if (m_settings) {
        m_settings->sync();
    }
}

void ConfigManager::load()
{
    if (!m_settings) {
        return;
    }

    // 加载Python配置
    m_pythonHome = m_settings->value("Python/home").toString();
    m_pythonPaths = m_settings->value("Python/paths").toStringList();

    // 加载编辑器配置
    m_editorFont = m_settings->value("Editor/font", "Consolas").toString();
    m_editorFontSize = m_settings->value("Editor/fontSize", 12).toInt();
    m_autoSaveInterval = m_settings->value("Editor/autoSaveInterval", 30).toInt();

    // 加载应用配置
    m_executionDelay = m_settings->value("Application/executionDelay", 100).toInt();
    m_theme = m_settings->value("Application/theme", "light").toString();

    // 如果没有配置，则创建默认配置
    if (!m_settings->contains("Python/home")) {
        createDefaultConfiguration();
    }
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFile;
}

QString ConfigManager::detectCondaPython()
{
#ifdef Q_OS_WINDOWS
    QString condaPath = QDir::homePath() + "/miniconda3";
    QStringList possiblePaths = {
        QDir::homePath() + "/miniconda3",
        QDir::homePath() + "/anaconda3",
        "C:/miniconda3",
        "C:/anaconda3"
    };
    
    for (const QString& path : possiblePaths) {
        if (QDir(path).exists()) {
            QString pythonPath = path + "/python.exe";
            if (QFileInfo(pythonPath).exists()) {
                return QFileInfo(pythonPath).canonicalPath();
            }
        }
    }
#else
    // Linux/macOS Conda detection
    QString condaPath = QDir::homePath() + "/miniconda3";
    QStringList possiblePaths = {
        QDir::homePath() + "/miniconda3",
        QDir::homePath() + "/anaconda3"
    };
    
    for (const QString& path : possiblePaths) {
        if (QDir(path).exists()) {
            QString pythonPath = path + "/bin/python";
            if (QFileInfo(pythonPath).exists()) {
                return QFileInfo(pythonPath).canonicalPath();
            }
        }
    }
#endif

    return QString();
}

QString ConfigManager::detectSystemPython()
{
    // 尝试从环境变量PATH中查找Python
    QStringList pythonExecutables;
#ifdef Q_OS_WINDOWS
    pythonExecutables << "python.exe" << "python3.exe";
#else
    pythonExecutables << "python3" << "python";
#endif

    QString pathEnv = qEnvironmentVariable("PATH");
    QStringList pathDirs = pathEnv.split(QDir::listSeparator());

    for (const QString& dir : pathDirs) {
        for (const QString& pythonExe : pythonExecutables) {
            QString pythonPath = QDir(dir).filePath(pythonExe);
            if (QFileInfo(pythonPath).exists()) {
                // 验证是否是有效的Python解释器
                QProcess pythonCheck;
                pythonCheck.start(pythonPath, QStringList() << "--version");
                if (pythonCheck.waitForFinished(2000)) {
                    QString version = pythonCheck.readAllStandardOutput();
                    if (version.contains("Python")) {
                        return QFileInfo(pythonPath).canonicalPath();
                    }
                }
            }
        }
    }

    return QString();
}

void ConfigManager::createDefaultConfiguration()
{
    // 自动检测Python
    m_pythonHome = autoDetectPython();
    m_pythonPaths.clear();

    // 设置默认值
    m_editorFont = "Consolas";
    m_editorFontSize = 12;
    m_autoSaveInterval = 30;
    m_executionDelay = 100;
    m_theme = "light";

    // 保存默认值
    m_settings->setValue("Python/home", m_pythonHome);
    m_settings->setValue("Python/paths", m_pythonPaths);
    m_settings->setValue("Editor/font", m_editorFont);
    m_settings->setValue("Editor/fontSize", m_editorFontSize);
    m_settings->setValue("Editor/autoSaveInterval", m_autoSaveInterval);
    m_settings->setValue("Application/executionDelay", m_executionDelay);
    m_settings->setValue("Application/theme", m_theme);

    m_settings->sync();
}

bool ConfigManager::initialized() const
{
    return m_initialized;
}
