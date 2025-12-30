#pragma once

#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

/**
 * @class ConfigManager
 * @brief 应用程序配置管理器
 *
 * 负责：
 * - 应用程序设置的管理和持久化
 * - Python环境的自动检测和配置
 * - 用户首选项的保存和加载
 * - 默认配置的管理
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return ConfigManager& 配置管理器实例
     */
    static ConfigManager& instance();

    /**
     * @brief 初始化配置管理器
     * @param configFile 配置文件路径（可选）
     */
    void initialize(const QString& configFile = QString());

    /**
     * @brief 获取Python主目录
     * @return QString Python安装目录
     */
    QString getPythonHome() const;

    /**
     * @brief 设置Python主目录
     * @param path Python安装路径
     */
    void setPythonHome(const QString& path);

    /**
     * @brief 自动检测Python安装
     * @return QString 检测到的Python路径或空字符串
     */
    QString autoDetectPython();

    /**
     * @brief 获取Python路径列表
     * @return QStringList Python模块搜索路径
     */
    QStringList getPythonPaths() const;

    /**
     * @brief 添加Python路径
     * @param path 要添加的路径
     */
    void addPythonPath(const QString& path);

    /**
     * @brief 获取编辑器字体
     * @return QString 字体名称
     */
    QString getEditorFont() const;

    /**
     * @brief 设置编辑器字体
     * @param font 字体名称
     */
    void setEditorFont(const QString& font);

    /**
     * @brief 获取编辑器字体大小
     * @return int 字体大小
     */
    int getEditorFontSize() const;

    /**
     * @brief 设置编辑器字体大小
     * @param size 字体大小
     */
    void setEditorFontSize(int size);

    /**
     * @brief 获取自动保存间隔（秒）
     * @return int 间隔秒数
     */
    int getAutoSaveInterval() const;

    /**
     * @brief 设置自动保存间隔
     * @param seconds 间隔秒数
     */
    void setAutoSaveInterval(int seconds);

    /**
     * @brief 获取代码执行延迟（毫秒）
     * @return int 延迟毫秒数
     */
    int getExecutionDelay() const;

    /**
     * @brief 设置代码执行延迟
     * @param delayMs 延迟毫秒数
     */
    void setExecutionDelay(int delayMs);

    /**
     * @brief 获取主题设置
     * @return QString 主题名称
     */
    QString getTheme() const;

    /**
     * @brief 设置主题
     * @param theme 主题名称
     */
    void setTheme(const QString& theme);

    /**
     * @brief 获取窗口几何信息
     * @return QByteArray 窗口几何数据
     */
    QByteArray getWindowGeometry() const;

    /**
     * @brief 设置窗口几何信息
     * @param geometry 窗口几何数据
     */
    void setWindowGeometry(const QByteArray& geometry);

    /**
     * @brief 获取窗口状态
     * @return QByteArray 窗口状态数据
     */
    QByteArray getWindowState() const;

    /**
     * @brief 设置窗口状态
     * @param state 窗口状态数据
     */
    void setWindowState(const QByteArray& state);

    /**
     * @brief 重置为默认设置
     */
    void resetToDefaults();

    /**
     * @brief 保存设置
     */
    void save();

    /**
     * @brief 加载设置
     */
    void load();

    /**
     * @brief 获取配置文件路径
     * @return QString 配置文件路径
     */
    QString getConfigFilePath() const;

    bool initialized() const;

signals:
    /**
     * @brief 配置改变信号
     */
    void configurationChanged();

    /**
     * @brief Python路径改变信号
     */
    void pythonPathsChanged();

    /**
     * @brief 编辑器设置改变信号
     */
    void editorSettingsChanged();

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();

    // 禁用拷贝和赋值
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * @brief 检测Conda环境
     * @return QString Conda Python路径或空字符串
     */
    QString detectCondaPython();

    /**
     * @brief 检测系统Python
     * @return QString 系统Python路径或空字符串
     */
    QString detectSystemPython();

    /**
     * @brief 创建默认配置
     */
    void createDefaultConfiguration();

private:
    QSettings*  m_settings = nullptr;
    QString     m_configFile;
    QString     m_pythonHome;
    QStringList m_pythonPaths;
    QString     m_editorFont;
    QString     m_theme;
    int         m_editorFontSize;
    int         m_autoSaveInterval;
    int         m_executionDelay;
    bool        m_initialized = false;
};
