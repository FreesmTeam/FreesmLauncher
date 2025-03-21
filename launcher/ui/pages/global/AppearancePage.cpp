#include "AppearancePage.h"
#include "ui_AppearancePage.h"

#include <BuildConfig.h>
#include <ui/themes/ITheme.h>
#include <ui/themes/ThemeManager.h>

AppearancePage::AppearancePage(QWidget* parent) : QWidget(parent), m_ui(new Ui::AppearancePage)
{
    m_ui->setupUi(this);

    defaultFormat = new QTextCharFormat(m_ui->fontPreview->currentCharFormat());

    loadSettings();
    connect(m_ui->fontSizeBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AppearancePage::updateFontPreview);
    connect(m_ui->consoleFont, &QFontComboBox::currentFontChanged, this, &AppearancePage::updateFontPreview);
    connect(m_ui->themeCustomizationWidget, &ThemeCustomizationWidget::currentWidgetThemeChanged, this, &AppearancePage::updateFontPreview);
    connect(m_ui->themeCustomizationWidget, &ThemeCustomizationWidget::currentCatChanged, APPLICATION, &Application::currentCatChanged);
}

AppearancePage::~AppearancePage()
{
    delete m_ui;
}

bool AppearancePage::apply()
{
    applySettings();
    return true;
}

void AppearancePage::retranslate()
{
    m_ui->retranslateUi(this);
}

void AppearancePage::applySettings()
{
    SettingsObjectPtr settings = APPLICATION->settings();
    QString consoleFontFamily = m_ui->consoleFont->currentFont().family();
    settings->set("ConsoleFont", consoleFontFamily);
    settings->set("ConsoleFontSize", m_ui->fontSizeBox->value());
}

void AppearancePage::loadSettings()
{
    QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
    QFont consoleFont(fontFamily);
    m_ui->consoleFont->setCurrentFont(consoleFont);

    bool conversionOk = true;
    int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
    if (!conversionOk) {
        fontSize = 11;
    }
    m_ui->fontSizeBox->setValue(fontSize);

    updateFontPreview();
}

void AppearancePage::updateFontPreview()
{
    const LogColors& colors = APPLICATION->themeManager()->getLogColors();

    int fontSize = m_ui->fontSizeBox->value();
    QString fontFamily = m_ui->consoleFont->currentFont().family();
    m_ui->fontPreview->clear();
    defaultFormat->setFont(QFont(fontFamily, fontSize));

    auto print = [this, colors](const QString& message, MessageLevel::Enum level) {
        QTextCharFormat format(*defaultFormat);

        QColor bg = colors.background.value(level);
        QColor fg = colors.foreground.value(level);

        if (bg.isValid())
            format.setBackground(bg);

        if (fg.isValid())
            format.setForeground(fg);

        // append a paragraph/line
        auto workCursor = m_ui->fontPreview->textCursor();
        workCursor.movePosition(QTextCursor::End);
        workCursor.insertText(message, format);
        workCursor.insertBlock();
    };

    print(QString("%1 version: %2 (%3)\n")
              .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString(), BuildConfig.BUILD_PLATFORM),
          MessageLevel::Launcher);

    QDate today = QDate::currentDate();

    if (today.month() == 10 && today.day() == 31)
        print(tr("[Test/ERROR] OOoooOOOoooo! A spooky error!"), MessageLevel::Error);
    else
        print(tr("[Test/ERROR] A spooky error!"), MessageLevel::Error);

    print(tr("[Test/INFO] A harmless message..."), MessageLevel::Info);
    print(tr("[Test/WARN] A not so spooky warning."), MessageLevel::Warning);
    print(tr("[Test/DEBUG] A secret debugging message..."), MessageLevel::Debug);
    print(tr("[Test/FATAL] A terrifying fatal error!"), MessageLevel::Fatal);
}
