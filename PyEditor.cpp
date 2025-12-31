#include "PyEditor.h"
#include "CodeRunner.h"
#include "ConfigManager.h"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTextBlock>
#include <QTextStream>
#include <QTimer>

// Python语法高亮器类
class PythonHighlighter : public QSyntaxHighlighter
{
public:
    explicit PythonHighlighter(QTextDocument* parent = nullptr)
        : QSyntaxHighlighter(parent)
    {
        setupHighlightingRules();
    }

protected:
    void highlightBlock(const QString& text) override
    {
        // 应用所有高亮规则
        for (const HighlightingRule& rule : m_highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        // 处理多行字符串
        setCurrentBlockState(0);
        processMultiLineStrings(text);

        // 处理单行注释
        processSingleLineComments(text);
    }

private:
    // 高亮规则结构体
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    // 设置高亮规则
    void setupHighlightingRules()
    {
        // 关键字
        QStringList keywordPatterns = {
            QStringLiteral("def"),    QStringLiteral("class"),   QStringLiteral("import"),
            QStringLiteral("from"),   QStringLiteral("if"),      QStringLiteral("elif"),
            QStringLiteral("else"),   QStringLiteral("while"),   QStringLiteral("for"),
            QStringLiteral("return"), QStringLiteral("break"),   QStringLiteral("continue"),
            QStringLiteral("pass"),   QStringLiteral("raise"),   QStringLiteral("try"),
            QStringLiteral("except"), QStringLiteral("finally"), QStringLiteral("with"),
            QStringLiteral("as"),     QStringLiteral("global"),  QStringLiteral("nonlocal"),
            QStringLiteral("True"),   QStringLiteral("False"),   QStringLiteral("None"),
            QStringLiteral("and"),    QStringLiteral("or"),      QStringLiteral("not"),
            QStringLiteral("in"),     QStringLiteral("is"),      QStringLiteral("lambda")};

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(127, 0, 85));
        keywordFormat.setFontWeight(QFont::Bold);

        for (const QString& pattern : keywordPatterns) {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(QStringLiteral("\\b%1\\b").arg(pattern));
            rule.format  = keywordFormat;
            m_highlightingRules.append(rule);
        }

        // 内置函数
        QStringList builtinFunctions = {
            QStringLiteral("print"),    QStringLiteral("input"),  QStringLiteral("len"),
            QStringLiteral("type"),     QStringLiteral("int"),    QStringLiteral("float"),
            QStringLiteral("str"),      QStringLiteral("list"),   QStringLiteral("tuple"),
            QStringLiteral("dict"),     QStringLiteral("set"),    QStringLiteral("range"),
            QStringLiteral("abs"),      QStringLiteral("max"),    QStringLiteral("min"),
            QStringLiteral("sum"),      QStringLiteral("sorted"), QStringLiteral("enumerate"),
            QStringLiteral("zip"),      QStringLiteral("map"),    QStringLiteral("filter"),
            QStringLiteral("reversed"), QStringLiteral("any"),    QStringLiteral("all"),
            QStringLiteral("open"),     QStringLiteral("os"),     QStringLiteral("math")};

        QTextCharFormat builtinFunctionFormat;
        builtinFunctionFormat.setForeground(QColor(0, 0, 255));
        builtinFunctionFormat.setFontWeight(QFont::Bold);

        for (const QString& pattern : builtinFunctions) {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(QStringLiteral("\\b%1\\b").arg(pattern));
            rule.format  = builtinFunctionFormat;
            m_highlightingRules.append(rule);
        }

        // 字符串
        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(0, 128, 0));

        // 匹配单引号和双引号字符串
        HighlightingRule stringRule1;
        stringRule1.pattern = QRegularExpression(QStringLiteral("'[^\\']*'"));
        stringRule1.format  = stringFormat;
        m_highlightingRules.append(stringRule1);

        HighlightingRule stringRule2;
        stringRule2.pattern = QRegularExpression(QStringLiteral("\"[^\\\"]*\""));
        stringRule2.format  = stringFormat;
        m_highlightingRules.append(stringRule2);

        // 数字
        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(255, 140, 0));

        HighlightingRule numberRule;
        numberRule.pattern = QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*\\b"));
        numberRule.format  = numberFormat;
        m_highlightingRules.append(numberRule);
    }

    // 处理多行字符串
    void processMultiLineStrings(const QString& text)
    {
        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(0, 128, 0));

        int startIndex = 0;
        if (previousBlockState() != 1) startIndex = text.indexOf("\"\"\"");

        while (startIndex >= 0) {
            int endIndex   = text.indexOf("\"\"\"", startIndex + 3);
            int blockState = 0;

            if (endIndex == -1) {
                setCurrentBlockState(1);
                blockState = 1;
            }

            int commentLength =
                (endIndex == -1) ? text.length() - startIndex : endIndex - startIndex + 3;
            setFormat(startIndex, commentLength, stringFormat);
            startIndex = text.indexOf("\"\"\"", startIndex + commentLength);
        }
    }

    // 处理单行注释
    void processSingleLineComments(const QString& text)
    {
        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(128, 128, 128));
        commentFormat.setFontItalic(true);

        // 查找#号
        int hashIndex = text.indexOf("#");
        if (hashIndex >= 0) {
            // 检查是否在字符串内部
            bool inString = false;
            for (int i = 0; i < hashIndex; ++i) {
                if (text[i] == '"' || text[i] == '\'') {
                    // 检查是否是转义的引号
                    if (i > 0 && text[i - 1] == '\\') continue;
                    inString = !inString;
                }
            }

            if (!inString) {
                setFormat(hashIndex, text.length() - hashIndex, commentFormat);
            }
        }
    }

    QVector<HighlightingRule> m_highlightingRules;
};

PyEditor::PyEditor(QWidget* parent)
    : QPlainTextEdit(parent)
    , lineNumberArea(nullptr)
    , codeRunner(nullptr)
    , configManager(&ConfigManager::instance())
    , currentLine(-1)
    , changeTimer(nullptr)
    , currentFilePath()
{
    // 获取配置管理器实例
    if (!configManager->initialized()) {
        configManager->initialize();
    }

    setupEditor();
    setupLineNumberArea();
    setupAutoSave();

    // 连接配置变更信号
    connect(configManager,
            &ConfigManager::editorSettingsChanged,
            this,
            &PyEditor::updateExtraSelections);
}

PyEditor::~PyEditor()
{
    if (changeTimer) {
        changeTimer->stop();
        delete changeTimer;
        changeTimer = nullptr;
    }

    if (syntaxHighlighter) {
        delete syntaxHighlighter;
        syntaxHighlighter = nullptr;
    }
}

void PyEditor::setCodeRunner(CodeRunner* runner)
{
    if (codeRunner != runner) {
        // 断开旧的连接
        if (codeRunner) {
            disconnect(codeRunner, nullptr, this, nullptr);
        }

        codeRunner = runner;

        // 连接新的信号
        if (codeRunner) {
            connect(codeRunner, &CodeRunner::lineExecuted, this, &PyEditor::setCurrentLine);
        }
    }
}

int PyEditor::currentLineNumber() const
{
    return textCursor().blockNumber() + 1;
}

void PyEditor::setCurrentLine(int line)
{
    if (line != currentLine && line >= 0) {
        currentLine = line;
        highlightCurrentLine();
        emit currentLineChanged(line);
    }
}

void PyEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(245, 245, 245));

    QTextBlock block       = firstVisibleBlock();
    int        blockNumber = block.blockNumber();
    int top    = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int     currentLineNumber = blockNumber + 1;
            QString number            = QString::number(currentLineNumber);

            // 绘制断点
            if (breakpoints.contains(currentLineNumber)) {
                painter.setPen(QColor(Qt::red));
                painter.setBrush(QColor(Qt::red));
                int x = 5;
                int y = top + fontMetrics().height() / 2 - 4;
                painter.drawEllipse(x, y, 8, 8);
            }

            // 设置当前执行行颜色
            if (currentLineNumber == currentLine) {
                painter.setPen(QColor(Qt::blue));
                painter.setFont(
                    QFont(painter.font().family(), painter.font().pointSize(), QFont::Bold));
            }
            else {
                painter.setPen(Qt::black);
                painter.setFont(
                    QFont(painter.font().family(), painter.font().pointSize(), QFont::Normal));
            }

            painter.drawText(15,
                             top,
                             lineNumberArea->width() - 15,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             number);
        }

        block  = block.next();
        top    = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}


void PyEditor::lineNumberAreaMousePressEvent(const QPoint& pos)
{
    // 计算点击位置对应的行号
    QTextCursor cursor     = cursorForPosition(pos);
    int         lineNumber = cursor.blockNumber() + 1;

    // 切换断点
    toggleBreakpoint(lineNumber);
}


int PyEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max    = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // 增加断点符号占用的空间（15像素）
    int space = 15 + 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

bool PyEditor::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    setPlainText(in.readAll());
    file.close();

    currentFilePath = filePath;
    emit codeChanged();
    return true;
}

bool PyEditor::saveToFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << toPlainText();
    file.close();

    return true;
}

void PyEditor::formatCode()
{
    // 简单的代码格式化：移除多余空行和统一缩进
    QString     text  = toPlainText();
    QStringList lines = text.split('\n');
    QStringList formattedLines;

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (!trimmedLine.isEmpty()) {
            // 计算缩进
            int indentLevel = 0;
            for (int i = 0; i < line.length(); ++i) {
                if (line[i] == ' ') {
                    indentLevel++;
                }
                else if (line[i] == '\t') {
                    indentLevel += 4;
                }
                else {
                    break;
                }
            }

            // 统一缩进到4个空格
            QString indentedLine = QString(indentLevel / 4, ' ');
            if (indentLevel % 4 != 0) {
                indentedLine += QString(indentLevel % 4, ' ');
            }
            indentedLine += trimmedLine;
            formattedLines.append(indentedLine);
        }
        else if (!formattedLines.isEmpty()) {
            // 保持最多一个空行
            if (!formattedLines.last().isEmpty()) {
                formattedLines.append("");
            }
        }
    }

    setPlainText(formattedLines.join('\n'));
    emit codeChanged();
}

void PyEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void PyEditor::keyPressEvent(QKeyEvent* event)
{
    // Tab键自动插入4个空格
    if (event->key() == Qt::Key_Tab) {
        insertPlainText("    ");
        event->accept();
        return;
    }

    // Shift+Tab减少缩进
    if (event->key() == Qt::Key_Backtab) {
        QTextCursor cursor   = textCursor();
        QString     lineText = cursor.block().text();
        int         pos      = cursor.positionInBlock();

        if (pos >= 4) {
            cursor.beginEditBlock();
            cursor.movePosition(QTextCursor::StartOfBlock);
            for (int i = 0; i < 4; ++i) {
                if (cursor.charFormat().background() != QBrush()) {
                    // 不处理语法高亮区域的字符
                    break;
                }
                if (cursor.atBlockEnd()) {
                    break;
                }
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                if (cursor.selectedText() == " ") {
                    cursor.removeSelectedText();
                }
                else {
                    break;
                }
            }
            cursor.endEditBlock();
        }
        event->accept();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void PyEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void PyEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy != 0) {
        lineNumberArea->scroll(0, dy);
    }
    else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void PyEditor::highlightCurrentLine()
{
    updateExtraSelections();
}

void PyEditor::onCodeChanged()
{
    emit codeChanged();
}

void PyEditor::setupEditor()
{
    // 设置编辑器属性
    setWordWrapMode(QTextOption::NoWrap);
    setTabStopDistance(fontMetrics().horizontalAdvance(" ") * 4);

    // 应用配置
    setFont(QFont(configManager->getEditorFont(), configManager->getEditorFontSize()));

    // 连接信号
    connect(this, &PyEditor::blockCountChanged, this, &PyEditor::updateLineNumberAreaWidth);
    connect(this, &PyEditor::updateRequest, this, &PyEditor::updateLineNumberArea);
    connect(this, &PyEditor::cursorPositionChanged, this, &PyEditor::highlightCurrentLine);

    // 设置语法高亮
    setupSyntaxHighlighting();

    updateLineNumberAreaWidth();
}

void PyEditor::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // 当前编辑行高亮
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection currentLineSelection;
        currentLineSelection.format.setBackground(QColor(255, 255, 0, 30));
        currentLineSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
        currentLineSelection.cursor = textCursor();
        currentLineSelection.cursor.clearSelection();
        extraSelections.append(currentLineSelection);
    }

    // 当前执行行高亮
    if (currentLine > 0 && currentLine <= blockCount()) {
        QTextEdit::ExtraSelection executionLineSelection;
        executionLineSelection.format.setBackground(QColor(0, 255, 255, 50));
        executionLineSelection.format.setProperty(QTextFormat::FullWidthSelection, true);

        QTextCursor cursor(document()->findBlockByLineNumber(currentLine - 1));
        executionLineSelection.cursor = cursor;
        executionLineSelection.cursor.clearSelection();
        extraSelections.append(executionLineSelection);
    }

    setExtraSelections(extraSelections);
}

void PyEditor::setupLineNumberArea()
{
    lineNumberArea = new LineNumberArea(this);
    updateLineNumberAreaWidth();
}

void PyEditor::setupAutoSave()
{
    changeTimer = new QTimer(this);
    changeTimer->setSingleShot(true);

    // 根据配置设置自动保存间隔
    int interval = configManager->getAutoSaveInterval() * 1000;
    changeTimer->setInterval(interval);

    connect(this, &PyEditor::textChanged, [this]() {
        if (changeTimer) {
            changeTimer->start();
        }
    });

    connect(changeTimer, &QTimer::timeout, [this]() {
        // 自动保存逻辑（如果设置了自动保存路径）
        if (!currentFilePath.isEmpty()) {
            saveToFile(currentFilePath);
            qDebug() << "Auto-saved to" << currentFilePath;
        }
    });
}

void PyEditor::setupSyntaxHighlighting()
{
    // 创建语法高亮器实例
    syntaxHighlighter = new PythonHighlighter(document());
}

void PyEditor::toggleBreakpoint(int lineNumber)
{
    if (breakpoints.contains(lineNumber)) {
        breakpoints.remove(lineNumber);
        emit breakpointChanged(lineNumber, false);
    }
    else {
        breakpoints.insert(lineNumber);
        emit breakpointChanged(lineNumber, true);
    }
    emit breakpointsChanged(breakpoints);
    // 更新行号区域
    update();
    updateLineNumberAreaWidth();
}

int PyEditor::lineNumberAtPosition(const QPoint& pos) const
{
    QTextCursor cursor = cursorForPosition(pos);
    return cursor.blockNumber() + 1;
}


void PyEditor::mousePressEvent(QMouseEvent* event)
{
    QPlainTextEdit::mousePressEvent(event);
}
