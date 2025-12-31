#pragma once

#include <QPlainTextEdit>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QSettings>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

class CodeRunner;
class ConfigManager;
class LineNumberArea;
class PythonHighlighter;

class PyEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PyEditor(QWidget* parent = nullptr);
    ~PyEditor() override;

    /**
     * @brief 设置代码执行器
     * @param runner 代码执行器指针
     */
    void setCodeRunner(CodeRunner* runner);

    /**
     * @brief 获取当前行号
     * @return int 当前光标所在行号（1-based）
     */
    int currentLineNumber() const;

    /**
     * @brief 设置当前高亮行
     * @param line 行号（1-based）
     */
    void setCurrentLine(int line);

    /**
     * @brief 行号区域绘制事件
     */
    void lineNumberAreaPaintEvent(QPaintEvent* event);

    /**
     * @brief 计算行号区域宽度
     * @return int 行号区域像素宽度
     */
    int lineNumberAreaWidth() const;

    /**
     * @brief 加载代码文件
     * @param filePath 文件路径
     * @return bool 加载成功返回true
     */
    bool loadFromFile(const QString& filePath);

    /**
     * @brief 保存代码到文件
     * @param filePath 文件路径
     * @return bool 保存成功返回true
     */
    bool saveToFile(const QString& filePath) const;

    /**
     * @brief 格式化代码（缩进对齐）
     */
    void formatCode();

signals:
    /**
     * @brief 代码改变信号
     */
    void codeChanged();

    /**
     * @brief 行号改变信号
     */
    void currentLineChanged(int lineNumber);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect& rect, int dy);
    void highlightCurrentLine();
    void onCodeChanged();

private:
    void setupEditor();
    void updateExtraSelections();
    void setupLineNumberArea();
    void setupAutoSave();
    void setupSyntaxHighlighting();

private:
    LineNumberArea* lineNumberArea = nullptr;
    CodeRunner* codeRunner = nullptr;
    ConfigManager* configManager = nullptr;
    PythonHighlighter* syntaxHighlighter = nullptr;
    int currentLine = -1;
    QTimer* changeTimer = nullptr;
    QString currentFilePath;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(PyEditor* editor)
        : QWidget(editor)
        , codeEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    PyEditor* codeEditor;
};
