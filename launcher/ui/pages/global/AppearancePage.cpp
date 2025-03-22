#include "AppearancePage.h"
#include "ui_AppearancePage.h"

#include <DesktopServices.h>
#include <QGraphicsOpacityEffect>
#include "BuildConfig.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"

AppearancePage::AppearancePage(QWidget* parent) : QWidget(parent), m_ui(new Ui::AppearancePage)
{
    m_ui->setupUi(this);

    m_ui->catPreview->setGraphicsEffect(new QGraphicsOpacityEffect(this));

    defaultFormat = new QTextCharFormat(m_ui->consolePreview->currentCharFormat());

    loadSettings();
    loadThemeSettings();

    updateConsolePreview();
    updateCatPreview();

    connect(m_ui->fontSizeBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AppearancePage::updateConsolePreview);
    connect(m_ui->consoleFont, &QFontComboBox::currentFontChanged, this, &AppearancePage::updateConsolePreview);

    connect(m_ui->iconsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppearancePage::applyIconTheme);
    connect(m_ui->widgetStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppearancePage::applyWidgetTheme);
    connect(m_ui->catPackComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppearancePage::applyCatTheme);
    connect(m_ui->catOpacitySlider, &QAbstractSlider::valueChanged, this, &AppearancePage::updateCatPreview);

    connect(m_ui->iconsFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getIconThemesFolder().path()); });
    connect(m_ui->widgetStyleFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getApplicationThemesFolder().path()); });
    connect(m_ui->catPackFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getCatPacksFolder().path()); });
    connect(m_ui->reloadThemesButton, &QPushButton::pressed, this, &AppearancePage::loadThemeSettings);
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
    settings->set("CatOpacity", m_ui->catOpacitySlider->value());
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

    m_ui->catOpacitySlider->setValue(APPLICATION->settings()->get("CatOpacity").toInt());
}

void AppearancePage::applyIconTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalIconTheme = settings->get("IconTheme").toString();
    auto newIconTheme = m_ui->iconsComboBox->itemData(index).toString();
    if (originalIconTheme != newIconTheme) {
        settings->set("IconTheme", newIconTheme);
        APPLICATION->themeManager()->applyCurrentlySelectedTheme();
    }
}

void AppearancePage::applyWidgetTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalAppTheme = settings->get("ApplicationTheme").toString();
    auto newAppTheme = m_ui->widgetStyleComboBox->itemData(index).toString();
    if (originalAppTheme != newAppTheme) {
        settings->set("ApplicationTheme", newAppTheme);
        APPLICATION->themeManager()->applyCurrentlySelectedTheme();
    }

    updateConsolePreview();
}

void AppearancePage::applyCatTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalCat = settings->get("BackgroundCat").toString();
    auto newCat = m_ui->catPackComboBox->itemData(index).toString();
    if (originalCat != newCat) {
        settings->set("BackgroundCat", newCat);
    }

    APPLICATION->currentCatChanged(index);
    updateCatPreview();
}

void AppearancePage::loadThemeSettings()
{
    APPLICATION->themeManager()->refresh();

    m_ui->iconsComboBox->blockSignals(true);
    m_ui->widgetStyleComboBox->blockSignals(true);
    m_ui->catPackComboBox->blockSignals(true);

    m_ui->iconsComboBox->clear();
    m_ui->widgetStyleComboBox->clear();
    m_ui->catPackComboBox->clear();

    const SettingsObjectPtr settings = APPLICATION->settings();

    const QString currentIconTheme = settings->get("IconTheme").toString();
    const auto iconThemes = APPLICATION->themeManager()->getValidIconThemes();

    for (int i = 0; i < iconThemes.count(); ++i) {
        const IconTheme* theme = iconThemes[i];

        QIcon iconForComboBox = QIcon(theme->path() + "/scalable/settings");
        m_ui->iconsComboBox->addItem(iconForComboBox, theme->name(), theme->id());

        if (currentIconTheme == theme->id())
            m_ui->iconsComboBox->setCurrentIndex(i);
    }

    const QString currentTheme = settings->get("ApplicationTheme").toString();
    auto themes = APPLICATION->themeManager()->getValidApplicationThemes();
    for (int i = 0; i < themes.count(); ++i) {
        ITheme* theme = themes[i];

        m_ui->widgetStyleComboBox->addItem(theme->name(), theme->id());

        if (!theme->tooltip().isEmpty())
            m_ui->widgetStyleComboBox->setItemData(i, theme->tooltip(), Qt::ToolTipRole);

        if (currentTheme == theme->id())
            m_ui->widgetStyleComboBox->setCurrentIndex(i);
    }

    const QString currentCat = settings->get("BackgroundCat").toString();
    const auto cats = APPLICATION->themeManager()->getValidCatPacks();
    for (int i = 0; i < cats.count(); ++i) {
        const CatPack* cat = cats[i];

        QIcon catIcon = QIcon(QString("%1").arg(cat->path()));
        m_ui->catPackComboBox->addItem(catIcon, cat->name(), cat->id());

        if (currentCat == cat->id())
            m_ui->catPackComboBox->setCurrentIndex(i);
    }

    m_ui->iconsComboBox->blockSignals(false);
    m_ui->widgetStyleComboBox->blockSignals(false);
    m_ui->catPackComboBox->blockSignals(false);
}

void AppearancePage::updateConsolePreview()
{
    const LogColors& colors = APPLICATION->themeManager()->getLogColors();

    int fontSize = m_ui->fontSizeBox->value();
    QString fontFamily = m_ui->consoleFont->currentFont().family();
    m_ui->consolePreview->clear();
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
        auto workCursor = m_ui->consolePreview->textCursor();
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

void AppearancePage::updateCatPreview()
{
    QIcon catPackIcon(APPLICATION->themeManager()->getCatPack());
    m_ui->catPreview->setIcon(catPackIcon);

    auto effect = dynamic_cast<QGraphicsOpacityEffect*>(m_ui->catPreview->graphicsEffect());
    if (effect)
        effect->setOpacity(m_ui->catOpacitySlider->value() / 100.0);
}
