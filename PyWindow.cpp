#include "PyWindow.h"
#include "CodeRunner.h"
#include "PyEditor.h"
#include "PythonInterpreterManager.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

PyWindow::PyWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_settings("QtPythonEmbedTest2", "Settings")
{
    // 设置窗口属性
    setWindowTitle("Python Code Editor & Runner - 重构版");
    setMinimumSize(800, 600);
    resize(1000, 700);

    // 初始化组件
    initializeUI();
    initializePython();
    connectSignals();

    // 加载设置和示例代码
    loadWindowSettings();
    loadSavedCode();
    loadExampleCode();

    // 更新UI状态
    updateExecutionButtons();

    qDebug() << "PyWindow initialized successfully";
}

PyWindow::~PyWindow()
{
    // 保存设置
    saveWindowSettings();
    saveCurrentCode();

    // 清理资源
    if (m_runnerThread) {
        m_runnerThread->quit();
        m_runnerThread->wait();
        delete m_runnerThread;
    }

    if (m_runner) {
        delete m_runner;
    }

    // Python解释器清理由管理器负责
    qDebug() << "PyWindow destroyed";
}

void PyWindow::initializeUI()
{
    // 创建中央部件
    QWidget* centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    // 创建主要布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 创建工具栏
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    // 创建按钮
    m_runButton = new QPushButton("运行代码 (F5)");
    m_runButton->setShortcut(QKeySequence::Refresh);
    m_runButton->setToolTip("运行当前Python代码");

    m_clearButton = new QPushButton("清除输出");
    m_clearButton->setToolTip("清除输出窗口中的所有文本");

    m_saveButton = new QPushButton("保存代码");
    m_saveButton->setToolTip("保存当前代码到文件");

    m_settingsButton = new QPushButton("设置");
    m_settingsButton->setToolTip("打开Python环境设置");

    // 添加按钮到工具栏
    toolbar->addWidget(m_runButton);
    toolbar->addSeparator();
    toolbar->addWidget(m_clearButton);
    toolbar->addWidget(m_saveButton);
    toolbar->addSeparator();
    toolbar->addWidget(m_settingsButton);

    // 创建调试工具栏
    QToolBar* debugToolbar = addToolBar("Debug");
    debugToolbar->setMovable(false);

    // 创建调试按钮
    m_continueButton = new QPushButton("继续");
    m_continueButton->setToolTip("继续执行代码");
    m_continueButton->setEnabled(false);

    m_stepIntoButton = new QPushButton("逐语句");
    m_stepIntoButton->setToolTip("执行当前行，进入函数");
    m_stepIntoButton->setEnabled(false);

    m_stepOverButton = new QPushButton("逐过程");
    m_stepOverButton->setToolTip("执行当前行，不进入函数");
    m_stepOverButton->setEnabled(false);

    m_stepOutButton = new QPushButton("跳出");
    m_stepOutButton->setToolTip("执行完当前函数，返回调用者");
    m_stepOutButton->setEnabled(false);

    // 添加调试按钮到调试工具栏
    debugToolbar->addWidget(m_continueButton);
    debugToolbar->addWidget(m_stepIntoButton);
    debugToolbar->addWidget(m_stepOverButton);
    debugToolbar->addWidget(m_stepOutButton);

    // 创建代码编辑器和输出窗口
    m_codeEditor = new PyEditor;
    m_logOutput  = new QTextEdit;

    // 设置输出窗口属性
    m_logOutput->setReadOnly(true);
    m_logOutput->setStyleSheet(
        "background-color: #f8f8f8; font-family: 'Consolas', 'Courier New', monospace;");
    m_logOutput->setPlaceholderText("Python代码输出将显示在这里...\n"
                                    "错误信息将以红色显示。");

    // 创建分割器
    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_codeEditor);
    splitter->addWidget(m_logOutput);
    splitter->setSizes({400, 200});
    splitter->setChildrenCollapsible(false);

    mainLayout->addWidget(splitter);

    // 创建状态栏
    statusBar()->showMessage("就绪");
}

void PyWindow::initializePython()
{
    m_pythonManager = &PythonInterpreterManager::instance();

    // 初始化Python解释器
    bool success = m_pythonManager->initialize();

    if (!success) {
        QMessageBox::critical(this,
                              "初始化错误",
                              "Python解释器初始化失败！\n"
                              "请检查Python安装和环境配置。");
    }
}

void PyWindow::connectSignals()
{
    // 按钮连接
    connect(m_runButton, &QPushButton::clicked, this, &PyWindow::runPythonCode);
    connect(m_clearButton, &QPushButton::clicked, this, &PyWindow::clearOutput);
    connect(m_saveButton, &QPushButton::clicked, this, &PyWindow::saveCurrentCode);
    connect(m_settingsButton, &QPushButton::clicked, this, &PyWindow::showSettings);

    // CodeRunner连接
    m_runner       = new CodeRunner;
    m_runnerThread = new QThread;
    m_runner->moveToThread(m_runnerThread);
    m_runnerThread->start();

    connect(m_runner, &CodeRunner::executionStarted, this, &PyWindow::onExecutionStart);
    connect(m_runner, &CodeRunner::executionFinished, this, &PyWindow::onExecutionFinish);
    connect(m_runner, &CodeRunner::outputReceived, this, &PyWindow::appendOutput);
    connect(m_runner, &CodeRunner::errorOccurred, this, &PyWindow::appendError);
    connect(m_runner, &CodeRunner::lineExecuted, m_codeEditor, &PyEditor::setCurrentLine);
    connect(m_runner, &CodeRunner::debugStateChanged, this, &PyWindow::onDebugStateChanged);

    // 调试按钮连接
    Qt::ConnectionType ct = Qt::DirectConnection;
    connect(m_continueButton, &QPushButton::clicked, m_runner, &CodeRunner::continueExecution, ct);
    connect(m_stepIntoButton, &QPushButton::clicked, m_runner, &CodeRunner::stepInto, ct);
    connect(m_stepOverButton, &QPushButton::clicked, m_runner, &CodeRunner::stepOver, ct);
    connect(m_stepOutButton, &QPushButton::clicked, m_runner, &CodeRunner::stepOut, ct);

    // PyEditor连接
    connect(m_codeEditor, &PyEditor::breakpointsChanged, m_runner, &CodeRunner::setBreakpoints, ct);

    // Python管理器连接
    if (m_pythonManager) {
        connect(m_pythonManager,
                &PythonInterpreterManager::pythonOutput,
                this,
                &PyWindow::appendOutput);
        connect(
            m_pythonManager, &PythonInterpreterManager::pythonError, this, &PyWindow::appendError);
        connect(m_pythonManager,
                &PythonInterpreterManager::initialized,
                this,
                &PyWindow::onPythonInitialized);
    }
}

void PyWindow::runPythonCode()
{
    if (m_isExecuting) {
        // 如果正在执行，则停止执行
        m_runner->abortExecution();
        return;
    }

    QString code = m_codeEditor->toPlainText().trimmed();

    if (code.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入要执行的Python代码！");
        return;
    }

    // 清空输出窗口
    clearOutput();

    // 启动执行
    QMetaObject::invokeMethod(m_runner, "runCode", Qt::QueuedConnection, Q_ARG(QString, code));
}

void PyWindow::appendOutput(const QString& text)
{
    m_logOutput->setTextColor(Qt::black);
    m_logOutput->append(text);
}

void PyWindow::appendError(const QString& text)
{
    m_logOutput->setTextColor(Qt::red);
    m_logOutput->append("错误: " + text);
}

void PyWindow::onExecutionStart()
{
    m_isExecuting = true;
    updateExecutionButtons();
    statusBar()->showMessage("正在执行Python代码...");

    // 禁用编辑器
    m_codeEditor->setEnabled(false);
    m_saveButton->setEnabled(false);
}

void PyWindow::onExecutionFinish()
{
    m_isExecuting = false;
    updateExecutionButtons();
    statusBar()->showMessage("执行完成");

    // 启用编辑器
    m_codeEditor->setEnabled(true);
    m_saveButton->setEnabled(true);
}

void PyWindow::onPythonInitialized()
{
    statusBar()->showMessage("Python解释器已初始化: " + m_pythonManager->getPythonVersion());
}

void PyWindow::onDebugStateChanged(int state)
{
    CodeRunner::DebugState dbgState = (CodeRunner::DebugState)state;
    // 根据调试状态更新按钮状态
    switch (dbgState) {
    case CodeRunner::Running:
        // 运行中，禁用所有调试按钮和编辑器
        m_continueButton->setEnabled(false);
        m_stepIntoButton->setEnabled(false);
        m_stepOverButton->setEnabled(false);
        m_stepOutButton->setEnabled(false);
        m_codeEditor->setEnabled(false);
        break;
    case CodeRunner::Paused:
        // 暂停，启用所有调试按钮和编辑器
        m_continueButton->setEnabled(true);
        m_stepIntoButton->setEnabled(true);
        m_stepOverButton->setEnabled(true);
        m_stepOutButton->setEnabled(true);
        m_codeEditor->setEnabled(true);
        break;
    case CodeRunner::StepInto:
    case CodeRunner::StepOver:
    case CodeRunner::StepOut:
        // 单步执行中，禁用所有调试按钮和编辑器
        m_continueButton->setEnabled(false);
        m_stepIntoButton->setEnabled(false);
        m_stepOverButton->setEnabled(false);
        m_stepOutButton->setEnabled(false);
        m_codeEditor->setEnabled(false);
        break;
    }
}

void PyWindow::showSettings()
{
    QString pythonHome = QInputDialog::getText(this,
                                               "Python设置",
                                               "请输入Python安装路径:",
                                               QLineEdit::Normal,
                                               m_pythonManager->getPythonVersion());

    if (!pythonHome.isEmpty()) {
        m_pythonManager->setPythonHome(pythonHome);
        m_settings.setValue("Python/pythonHome", pythonHome);
        QMessageBox::information(this, "设置", "Python路径已更新！请重启应用以应用新设置。");
    }
}

void PyWindow::applySettings()
{
    // 应用保存的设置
    QString pythonHome = m_settings.value("Python/pythonHome", "").toString();
    if (!pythonHome.isEmpty() && m_pythonManager) {
        m_pythonManager->setPythonHome(pythonHome);
    }
}

void PyWindow::loadExampleCode()
{
    m_exampleCode =
        "# 重构后的示例代码\n"
        "import time\n"
        "import cpp_module\n"
        "\n"
        "def fibonacci(n):\n"
        "    \"\"\"计算斐波那契数列\"\"\"\n"
        "    if n <= 1:\n"
        "        return n\n"
        "    else:\n"
        "        return fibonacci(n-1) + fibonacci(n-2)\n"
        "\n"
        "def main():\n"
        "    print(\"=== 斐波那契数列计算器 (重构版) ===\")\n"
        "    print(\"Python版本:\", __import__('sys').version)\n"
        "    \n"
        "    # 测试C++模块\n"
        "    test_input = \"Hello from Python!\"\n"
        "    result, output = cpp_module.test(test_input)\n"
        "    print(f\"C++模块测试: 输入='{test_input}', 结果={result}, 输出='{output}'\")\n"
        "    \n"
        "    print(\"\\n开始计算斐波那契数列...\")\n"
        "    for i in range(10):\n"
        "        result = fibonacci(i)\n"
        "        print(f\"Fibonacci({i}) = {result}\")\n"
        "        time.sleep(0.3)  # 暂停以便观察执行过程\n"
        "    \n"
        "    print(\"计算完成！\")\n"
        "    print(\"C++模块版本:\", cpp_module.get_version())\n"
        "\n"
        "if __name__ == \"__main__\":\n"
        "    main()";

    m_codeEditor->setPlainText(m_exampleCode);
}

void PyWindow::saveCurrentCode()
{
    QString code    = m_codeEditor->toPlainText();
    m_lastSavedCode = code;

    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir    dir(appDataDir);
    if (!dir.exists()) {
        dir.mkpath(appDataDir);
    }

    QString filePath = dir.filePath("last_code.py");
    QFile   file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << code;
        file.close();
        statusBar()->showMessage("代码已保存", 2000);
    }
    else {
        QMessageBox::warning(this, "保存失败", "无法保存代码文件！");
    }
}

void PyWindow::loadSavedCode()
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath   = QDir(appDataDir).filePath("last_code.py");
    QFile   file(filePath);

    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString     code = in.readAll();
        file.close();

        if (!code.isEmpty()) {
            m_codeEditor->setPlainText(code);
            m_lastSavedCode = code;
        }
    }
}

void PyWindow::loadWindowSettings()
{
    // 恢复窗口大小和位置
    restoreGeometry(m_settings.value("geometry").toByteArray());
    restoreState(m_settings.value("windowState").toByteArray());
}

void PyWindow::saveWindowSettings()
{
    // 保存窗口大小和位置
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("windowState", saveState());
}

void PyWindow::updateExecutionButtons()
{
    if (m_isExecuting) {
        m_runButton->setText("停止执行");
        m_runButton->setStyleSheet("background-color: #ff4444; color: white;");
        m_runButton->setToolTip("停止当前正在执行的代码");
    }
    else {
        m_runButton->setText("运行代码 (F5)");
        m_runButton->setStyleSheet("");
        m_runButton->setToolTip("运行当前Python代码");
    }
}

void PyWindow::clearOutput()
{
    m_logOutput->clear();
}

void PyWindow::closeEvent(QCloseEvent* event)
{
    if (m_isExecuting) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "确认退出", "代码正在执行中，确定要退出吗？", QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_runner->abortExecution();
            event->accept();
        }
        else {
            event->ignore();
        }
    }
    else {
        saveWindowSettings();
        saveCurrentCode();
        event->accept();
    }
}
