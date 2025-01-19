// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2024-2024 Freesm Launcher Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
#include "FreesmLightTheme.h"

#include <QObject>

QString FreesmLightTheme::id()
{
    return "freesm-light";
}

QString FreesmLightTheme::name()
{
    return QObject::tr("Freesm Light");
}

QPalette FreesmLightTheme::colorScheme()
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(250, 244, 237));
    darkPalette.setColor(QPalette::WindowText, QColor(65, 62, 87));
    darkPalette.setColor(QPalette::Base, QColor(250, 244, 237));
    darkPalette.setColor(QPalette::AlternateBase, QColor(250, 244, 237));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(144, 122, 169));
    darkPalette.setColor(QPalette::ToolTipText, QColor(144, 122, 169));
    darkPalette.setColor(QPalette::Text, QColor(87, 82, 121));
    darkPalette.setColor(QPalette::Button, QColor(215, 130, 126));
    darkPalette.setColor(QPalette::ButtonText, QColor(87, 82, 121));
    darkPalette.setColor(QPalette::BrightText, QColor(65, 62, 87));
    darkPalette.setColor(QPalette::Link, QColor(215, 130, 126));
    darkPalette.setColor(QPalette::Highlight, QColor(215, 130, 126));
    darkPalette.setColor(QPalette::HighlightedText, QColor(206, 202, 205));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(206, 202, 205));
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double FreesmLightTheme::fadeAmount()
{
    return 0.5;
}

QColor FreesmLightTheme::fadeColor()
{
    return QColor(255, 255, 255);
}

bool FreesmLightTheme::hasStyleSheet()
{
    return true;
}

QString FreesmLightTheme::appStyleSheet()
{
    return "QMainWindow, QDockWidget {background-color: #ffffff;}QWidget {border-color: rgba(255, 255, 255, 8);font-family: \"Segoe UI "
           "Variable Text Semibold\", serif;border-radius: 4px;}QObject, QObject::separator, QObject::handle, QObject::tab-bar, "
           "QObject::tab, QObject::section {outline: 0;selection-color: #ffffff;border-radius: 2px;selection-background-color: rgba(203, "
           "166, 247, 200);alternate-background-color: transparent;}QObject::handle {margin: 2px;background: rgb(215, 130, "
           "126);}QObject::handle:active {margin: 2px;background: rgba(203, 166, 247, 200);}QDialog, QWizard *, QCalendarWidget *, "
           "#qswPages > QObject {background-color: #ffffff;color: #ffffff;}QToolBar {background-color: transparent;spacing: 0;padding: "
           "8px;}QToolBar::separator {background: rgba(255, 255, 255, 20);height: 0;margin: 0;}QToolButton {background-color: rgba(255, "
           "255, 255, 0);border: 1px solid transparent;border-radius: 6px;padding: 2px;margin-left: 8px;}QToolButton[popupMode=\"2\"] "
           "{padding-right: 20px;}QToolButton:on {background-color: rgba(205, 214, 244, 18);}QToolButton:hover {background-color: "
           "rgba(203, 166, 247, 23);}QToolButton:pressed {background-color: rgba(205, 214, 244, 7);border: 1px solid rgba(205, 214, 244, "
           "8);color: #575279;}QToolButton::menu-indicator {image: url(theme:ComboBox.png);width: 32px;height: "
           "38px;}QToolButton::menu-arrow {image: url(theme:ComboBox.png);width: 20px;height: 100%;border-left: 1px solid rgba(255, 255, "
           "255, 8);}QToolButton::menu-button {background: rgba(0,0,0,0%);border: none;}QMenuBar {background-color: transparent;color: "
           "#ffffff;padding: 5px;}QMenuBar::item {background-color: transparent;padding: 6px 10px;border-radius: "
           "4px;}QMenuBar::item:selected {background-color: rgba(255, 255, 255, 20);}QMenuBar::item:pressed {background-color: rgba(255, "
           "255, 255, 13);color: #333333;}QMenu {background-color: #ffffff;padding-left: 1px;padding-top: 1px;border: 1px solid rgba(255, "
           "255, 255, 8);border-radius: 0px;}QMenu::indicator, QMenu::icon {left: 4px;}QMenu::item {background-color: transparent;padding: "
           "5px 15px;border-radius: 5px;min-width: 60px;margin: 3px;border: 1px solid transparent;}QMenu::item:selected {background-color: "
           "rgba(203, 166, 247, 16);}QMenu::item:pressed {background-color: rgba(203, 166, 247, 8);border: 1px solid rgba(205, 214, 244, "
           "8);}QMenu::right-arrow {image: url(theme:TreeViewClose.png);min-width: 40px;min-height: 18px;}QMenuBar:disabled {color: "
           "rgba(150, 150, 150);}QMenu::item:disabled {color: rgba(150, 150, 150);background-color: transparent;}QPushButton "
           "{background-color: rgba(203, 166, 247, 18);border-radius: 5px;color: #ffffff;padding: 7px 20px;margin-right: 5px;border: 1px "
           "solid transparent;}QPushButton:hover {background-color: rgba(203, 166, 247, 40);}QPushButton:pressed, "
           "QPushButton:pressed:focus {background-color: rgba(255, 255, 255, 7);border: 1px solid rgba(255, 255, 255, 8);color: "
           "#333333;border: 1px solid rgba(205, 214, 244, 8);}QPushButton:disabled {color: #9893a5;background-color: rgba(150, 150, 150, "
           "23);}QGroupBox {border-radius: 5px;border: 1px solid rgba(255, 255, 255, 3);margin-top: 24px;margin-bottom: "
           "4px;background-color: rgba(255, 255, 255, 5);}QGroupBox::title {subcontrol-origin: margin;subcontrol-position: top "
           "left;padding-bottom: 4px;padding-left: 4px;padding-right: 4px;border-bottom: 1px solid #d7827e;}QGroupBox::title::disabled "
           "{color: rgba(150, 150, 150) }QListView {}QListView::indicator, QListView::text, QListView::icon {left: 4px;}QListView::item "
           "{padding-top: 5px;padding-bottom: 5px;margin-bottom: 5px;border-radius: 5px;}QListView::item:hover {background-color: "
           "rgba(255, 255, 255, 18);}QListView::item:selected {border-radius: 5px;outline: 0;background-color: rgba(255, 255, 255, "
           "13);border-color: rgba(255, 255, 255, 0);}QTreeView {background-color: transparent;color: #ffffff;outline: "
           "0;selection-background-color: rgba(255, 255, 255, 100);}QTreeView::indicator, QTreeView::icon {margin: 6px;}QTreeView::item "
           "{color: #ffffff;min-width: 60px;padding: 5px 4px;background-color: transparent;outline: 0;}QTreeView::item:hover, "
           "QTreeView::item:focus {background-color: rgba(255, 255, 255, 8);outline: 0;}QTreeView::item:selected {color: "
           "#ffffff;background-color: transparent;outline: 0;}QTreeView::item:selected:active {background-color: rgba(255, 255, 255, "
           "10);outline: 0;}QTreeView::branch:has-children:closed {image: "
           "url(theme:TreeViewClose.png);}QTreeView::branch:has-children:open {image: "
           "url(theme:TreeViewOpen.png);}QTreeView::branch:has-children:open:hover {image: url(theme:TreeViewOpen.png);}QHeaderView "
           "{font-family: \"Segoe UI Variable Small\", serif;font-size: 12px;background-color: transparent;text-align: "
           "center;qproperty-defaultAlignment: AlignVCenter;}QHeaderView::section {background-color: rgba(255, 255, 255, 0.05);color: "
           "#9893a5;padding: 4px;padding-left: 8px;padding-right: 20px;border-radius: 0;}QHeaderView::section:hover {background-color: "
           "rgba(255, 255, 255, 14);}QHeaderView::up-arrow {margin-top: 2px;margin-bottom: 2px;margin-right: 5px;width: 19px;height: "
           "13px;}QHeaderView::down-arrow {margin-bottom: 4px;margin-right: 5px;width: 19px;height: 13px;}QHeaderView::up-arrow {image: "
           "url(theme:SpinBoxDown.png);}QHeaderView::down-arrow {image: url(theme:SpinBoxUp.png);}QTabWidget::pane {border: 0px solid "
           "rgba(255, 255, 255, 40);border-radius: 5px;}QTabWidget::tab-bar {left: 8px;}QTabBar::tab {background-color: rgba(255, 255, "
           "255, 0);padding: 8px 15px;margin-right: 8px;margin-bottom: 2px;border-radius: 5px;}QTabBar::tab:hover {background-color: "
           "rgba(255, 255, 255, 13);}QTabBar::tab:selected {background-color: rgba(255, 255, 255, 16);}QTabBar::tab:disabled {color: "
           "rgba(150, 150, 150);}QScrollBar:vertical {border: 6px solid rgba(255, 255, 255, 0);background-color: transparent;margin: 0px "
           "0px 0px 0px;width: 16px;}QScrollBar:vertical:hover {border: 5px solid rgba(255, 255, 255, 0);}QScrollBar::handle:vertical "
           "{background-color: rgba(255, 255, 255, 130);border-radius: 2px;min-height: 25px;}QScrollBar::sub-line:vertical {image: "
           "url(theme:ScrollTop.png);subcontrol-position: top;subcontrol-origin: margin;}QScrollBar::sub-line:vertical:hover {image: "
           "url(theme:ScrollTopHover.png);}QScrollBar::sub-line:vertical:pressed {image: "
           "url(theme:ScrollTopPressed.png);}QScrollBar::add-line:vertical {image: url(theme:ScrollBottom.png);subcontrol-position: "
           "bottom;subcontrol-origin: margin;}QScrollBar::add-line:vertical:hover {image: "
           "url(theme:ScrollBottomHover.png);}QScrollBar::add-line:vertical:pressed {image: "
           "url(theme:ScrollBottomPressed.png);}QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {background: "
           "none;}QScrollBar:horizontal {border: 6px solid rgba(255, 255, 255, 0);background-color: transparent;margin: 0px 0px 0px "
           "0px;height: 16px;}QScrollBar:horizontal:hover {border: 5px solid rgba(255, 255, 255, 0);}QScrollBar::handle:horizontal "
           "{background-color: rgba(255, 255, 255, 130);border-radius: 2px;min-width: 25px;}QScrollBar::sub-line:horizontal {image: "
           "url(theme:ScrollLeft.png);subcontrol-position: left;subcontrol-origin: margin;}QScrollBar::sub-line:horizontal:hover {image: "
           "url(theme:ScrollLeftHover.png);}QScrollBar::sub-line:horizontal:pressed {image: "
           "url(theme:ScrollLeftPressed.png);}QScrollBar::add-line:horizontal {image: url(theme:ScrollRight.png);subcontrol-position: "
           "right;subcontrol-origin: margin;}QScrollBar::add-line:horizontal:hover {image: "
           "url(theme:ScrollRightHover.png);}QScrollBar::add-line:horizontal:pressed {image: "
           "url(theme:ScrollRightPressed.png);}QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {background: "
           "none;}QAbstractScrollArea {background-color: transparent;}QProgressBar {text-align: right;background-color: "
           "qlineargradient(spread:reflect, x1:0.5, y1:0.5, x2:0.5, y2:1, stop:0.119403 rgba(255, 255, 255, 20), stop:0.273632 rgba(0, 0, "
           "0, 0));border-radius: 1px;margin-top: 14px;margin-bottom: 14px;}QProgressBar::chunk {background-color: "
           "qlineargradient(spread:reflect, x1:0.5, y1:0.5, x2:0.5, y2:1, stop:0.119403 rgba(255, 255, 255, 250), stop:0.273632 rgba(0, 0, "
           "0, 0));border-radius: 2px;}QSpinBox {background-color: rgba(255, 255, 255, 10);border: 1px solid rgba(255, 255, 255, "
           "8);border-radius: 5px;padding-left: 10px;padding-top: 6px;padding-bottom: 6px;min-width: 60px;border-bottom: 1px solid "
           "#e0e0e0;}QSpinBox:hover {background-color: rgba(255, 255, 255, 16);border: 1px solid rgba(255, 255, 255, 10);border-bottom: "
           "1px solid #e0e0e0;}QSpinBox::focus {background-color: rgba(255, 255, 255, 5);border: 1px solid rgba(255, 255, 255, 8);color: "
           "rgba(255, 255, 255, 200);border-bottom: 2px solid #d7827e;}QSpinBox::up-button {image: "
           "url(theme:SpinBoxUp.png);background-color: rgba(0, 0, 0, 0);border: 1px solid rgba(0, 0, 0, 0);border-radius: 4px;margin-top: "
           "1px;margin-bottom: 1px;margin-right: 3px;min-width: 25px;max-width: 25px;min-height: 15px;}QSpinBox::up-button:hover "
           "{background-color: rgba(255, 255, 255, 13);}QSpinBox::up-button:pressed {background-color: rgba(255, 255, 255, "
           "5);}QSpinBox::down-button {image: url(theme:SpinBoxDown.png);background-color: rgba(0, 0, 0, 0);border: 1px solid rgba(0, 0, "
           "0, 0);border-radius: 4px;margin-top: 1px;margin-bottom: 1px;margin-right: 3px;min-width: 25px;max-width: 25px;min-height: "
           "15px;}QSpinBox::down-button:hover {background-color: rgba(255, 255, 255, 13);}QSpinBox::down-button:pressed {background-color: "
           "rgba(255, 255, 255, 5);}QSpinBox::drop-down {background-color: transparent;width: 50px;}QSpinBox:disabled {color: rgba(150, "
           "150, 150);background-color: rgba(255, 255, 255, 13);border: 1px solid rgba(255, 255, 255, 5);}QSpinBox::up-button:disabled "
           "{image: url(theme:SpinBoxUpDisabled.png);}QSpinBox::down-button:disabled {image: "
           "url(theme:SpinBoxDownDisabled.png);}QDoubleSpinBox {background-color: rgba(255, 255, 255, 10);border: 1px solid rgba(255, 255, "
           "255, 8);border-radius: 5px;padding-left: 10px;padding-top: 6px;padding-bottom: 6px;min-width: 70px;border-bottom: 1px solid "
           "#e0e0e0;}QDoubleSpinBox:hover {background-color: rgba(255, 255, 255, 16);border: 1px solid rgba(255, 255, 255, "
           "13);border-bottom: 1px solid #e0e0e0;}QDoubleSpinBox::focus {background-color: rgba(255, 255, 255, 5);border: 1px solid "
           "rgba(255, 255, 255, 13);color: rgba(255, 255, 255, 200);border-bottom: 2px solid #d7827e;}QDoubleSpinBox::up-button {image: "
           "url(theme:SpinBoxUp.png);background-color: rgba(0, 0, 0, 0);border: 1px solid rgba(0, 0, 0, 0);border-radius: 4px;margin-top: "
           "1px;margin-bottom: 1px;margin-right: 3px;min-width: 25px;max-width: 25px;min-height: 15px;}QDoubleSpinBox::up-button:hover "
           "{background-color: rgba(255, 255, 255, 13);}QDoubleSpinBox::up-button:pressed {background-color: rgba(255, 255, 255, "
           "5);}QDoubleSpinBox::down-button {image: url(theme:SpinBoxDown.png);background-color: rgba(0, 0, 0, 0);border: 1px solid "
           "rgba(0, 0, 0, 0);border-radius: 4px;margin-top: 1px;margin-bottom: 1px;margin-right: 3px;min-width: 25px;max-width: "
           "25px;min-height: 15px;}QDoubleSpinBox::down-button:hover {background-color: rgba(255, 255, 255, 13);}"
           "QDoubleSpinBox::down-button:pressed {background-color: rgba(255, 255, 255, 5);}QDoubleSpinBox::drop-down {background-color: "
           "transparent;width: 50px;}QDoubleSpinBox:disabled {color: rgba(150, 150, 150);background-color: rgba(255, 255, 255, 13);border: "
           "1px solid rgba(255, 255, 255, 5);}QDoubleSpinBox::up-button:disabled {image: "
           "url(theme:SpinBoxUpDisabled.png);}QDoubleSpinBox::down-button:disabled {image: "
           "url(theme:SpinBoxDownDisabled.png);}QDateTimeEdit {background-color: rgba(255, 255, 255, 10);border: 1px solid rgba(255, 255, "
           "255, 8);border-radius: 5px;padding-left: 10px;padding-top: 6px;padding-bottom: 6px;min-width: 100px;border-bottom: 1px solid "
           "#e0e0e0;}QDateTimeEdit:hover {background-color: rgba(255, 255, 255, 16);border: 1px solid rgba(255, 255, 255, "
           "13);border-bottom: 1px solid #e0e0e0;}QDateTimeEdit::focus {background-color: rgba(255, 255, 255, 5);border: 1px solid "
           "rgba(255, 255, 255, 13);color: rgba(255, 255, 255, 200);border-bottom: 2px solid #e0e0e0;}QDateTimeEdit::up-button {image: "
           "url(theme:SpinBoxUp.png);background-color: rgba(0, 0, 0, 0);border: 1px solid rgba(0, 0, 0, 0);border-radius: 4px;margin-top: "
           "1px;margin-bottom: 1px;margin-right: 2px;min-width: 25px;max-width: 25px;min-height: 15px;}QDateTimeEdit::up-button:hover "
           "{background-color: rgba(255, 255, 255, 13);}QDateTimeEdit::up-button:pressed {background-color: rgba(255, 255, 255, "
           "5);}QDateTimeEdit::down-button {image: url(theme:SpinBoxDown.png);background-color: rgba(0, 0, 0, 0);border: 1px solid rgba(0, "
           "0, 0, 0);border-radius: 4px;margin-top: 1px;margin-bottom: 1px;margin-right: 2px;min-width: 25px;max-width: 25px;min-height: "
           "15px;}QDateTimeEdit::down-button:hover {background-color: rgba(255, 255, 255, 13);}QDateTimeEdit::down-button:pressed "
           "{background-color: rgba(255, 255, 255, 5);}QDateTimeEdit::drop-down {background-color: transparent;width: "
           "50px;}QDateTimeEdit:disabled {color: rgba(150, 150, 150);background-color: rgba(255, 255, 255, 13);border: 1px solid rgba(255, "
           "255, 255, 5);}QDateTimeEdit::up-button:disabled {image: url(theme:SpinBoxUpDisabled.png);}QDateTimeEdit::down-button:disabled "
           "{image: url(theme:SpinBoxDownDisabled.png);}QComboBox {background-color: rgba(255, 255, 255, 18);border-radius: "
           "5px;padding-left: 10px;padding-top: 5px;padding-bottom: 6px;}QGroupBox QComboBox {padding-top: 5px;padding-bottom: "
           "6px;}QComboBox:hover {background-color: rgba(255, 255, 255, 23);}QComboBox::down-arrow {image: url(theme:ComboBox.png);width: "
           "40px;height: 40px;margin-left: 15px;}QGroupBox QComboBox::down-arrow {width: 38px;height: 40px;}QComboBox::drop-down "
           "{background-color: transparent;min-width: 50px;}QComboBox:disabled {color: rgba(150, 150, 150);background-color: rgba(255, "
           "255, 255, 13);}QComboBox::down-arrow:disabled {image: url(theme:ComboBoxDisabled.png);}QComboBox QAbstractItemView "
           "{background-color: #ffffff;border-radius: 0px;border: 1px solid rgba(255, 255, 255, 8);selection-color: "
           "#ffffff;selection-background-color: rgba(255, 255, 255, 16);padding: 4px;}QTextEdit, QPlainTextEdit {background-color: "
           "rgba(255, 255, 255, 16);border: 1px solid rgba(255, 255, 255, 5);border-bottom: 1px solid #e0e0e0;}QTextEdit:hover, QPlainTextEdit:hover "
           "{background-color: #fffdfa;}QTextEdit:focus, QPlainTextEdit:focus {border-bottom: 1px solid #d7827e;background-color: #fffdfa; "
           "border-top: 1px solid #d7827e;border-left: 1px solid #d7827e; "
           "border-right: 1px solid #d7827e;}QTextEdit:disabled, QPlainTextEdit:disabled {color: rgba(150, "
           "150, 150);background-color: rgba(255, 255, 255, 13);border: 1px solid rgba(255, 255, 255, 5);}QLineEdit {background-color: "
           "rgba(255, 255, 255, 16);border: 1px solid rgba(255, 255, 255, 5);color: #ffffff;border-bottom: 1px solid "
           "#e0e0e0;padding-left: 5px;}QLineEdit:hover {background-color: #fffdfa; "
           "border: 1px solid rgba(255, 255, 255, 10);border-bottom: 1px solid #e0e0e0;}QLineEdit:focus {border-bottom: 1px solid "
           "#d7827e;background-color: #fffdfa;border-top: 1px solid rgba(255, 255, 255, 7);border-left: 1px solid rgba(255, "
           "255, 255, 7);border-right: 1px solid rgba(255, 255, 255, 7);}QLineEdit:disabled {color: rgba(150, 150, 150);background-color: "
           "rgba(255, 255, 255, 13);border: 1px solid rgba(255, 255, 255, 5);}QLineEdit QToolButton, QLineEdit:hover QToolButton:hover, "
           "QLineEdit:pressed QToolButton:pressed {background-color: transparent;}QRadioButton {min-height: 30px;max-height: "
           "30px;}QRadioButton::indicator {width: 15px;height: 15px;border-radius: 8px;border: 1px solid #848484;background-color: "
           "rgba(255, 255, 255, 0);margin-right: 5px;}QRadioButton::indicator:hover {background-color: rgba(255, 255, 255, "
           "16);}QRadioButton::indicator:pressed {background-color: rgba(255, 255, 255, 20);border: 1px solid #adadad;image: "
           "url(theme:RadioButton.png);}QRadioButton::indicator:checked {background-color: #d7827e;border: 1px solid #d7827e;image: "
           "url(theme:RadioButton.png);}QRadioButton::indicator:checked:hover {image: "
           "url(theme:RadioButtonHover.png);}QRadioButton::indicator:checked:pressed {image: "
           "url(theme:RadioButtonPressed.png);}QRadioButton:disabled {color: rgba(150, 150, 150);}QRadioButton::indicator:disabled "
           "{border: 1px solid #9c9c9c;background-color: rgba(255, 255, 255, 0);}QSlider:vertical {min-width: 30px;min-height: "
           "100px;}QSlider::groove:vertical {width: 5px;background-color: rgba(255, 255, 255, 150);border-radius: "
           "2px;}QSlider::handle:vertical {background-color: #d7827e;border: 4px solid #cfcfcf;height: 9px;min-width: 13px;margin: 0px "
           "-7px;border-radius: 8px;}QSlider::handle:vertical:hover {background-color: #d7827e;border: 3px solid #cfcfcf;height: "
           "11px;min-width: 13px;margin: 0px -7px;border-radius: 8px;}QSlider::handle:vertical:pressed {background-color: #d7827e;border: "
           "5px solid #cfcfcf;height: 7px;min-width: 13px;margin: 0px -7px;border-radius: 8px;}QSlider::groove:vertical:disabled "
           "{background-color: rgba(255, 255, 255, 75);}QSlider::handle:vertical:disabled {background-color: #cfcfcf;border: 6px solid "
           "#d9d9d9;}QSlider:horizontal {min-width: 100px;min-height: 30px;}QSlider::groove:horizontal {height: 3px;background-color: "
           "rgba(255, 255, 255, 150);border-radius: 1px;padding-left: 0px;padding-right: 0px;}QSlider::handle:horizontal "
           "{background-color: #d7827e;border: 4px solid #cfcfcf;width: 9px;min-height: 13px;margin: -7px 0;border-radius: "
           "8px;}QSlider::handle:horizontal:hover {background-color: #d7827e;border: 3px solid #cfcfcf;width: 11px;min-height: "
           "13px;margin: -7px 0;border-radius: 8px;}QSlider::handle:horizontal:pressed {background-color: #d7827e;border: 5px solid "
           "#cfcfcf;width: 7px;min-height: 13px;margin: -7px 0;border-radius: 8px;}QSlider::groove:horizontal:disabled {background-color: "
           "rgba(255, 255, 255, 75);}QSlider::handle:horizontal:disabled {background-color: #cfcfcf;border: 6px solid #d9d9d9;}QCheckBox, "
           "QLabel, QCheckBox, QAbstractCheckBox, QTreeView::indicator, QRadioButton {min-height: 20px;font-style: normal;}QLabel "
           "{min-height: fit-content;}QGroupBox::indicator, QMenu::indicator, QCheckBox::indicator, QTreeView::indicator {width: "
           "15px;height: 15px;border-radius: 4px;border: 1px solid #b3b3b3;background-color: rgba(255, 255, 255, 0);margin-right: "
           "5px;}QGroupBox::indicator:hover, QMenu::indicator:hover, QCheckBox::indicator:hover, QTreeView::indicator:hover "
           "{background-color: rgba(255, 255, 255, 16);}QGroupBox::indicator:pressed, QMenu::indicator:pressed, "
           "QCheckBox::indicator:pressed, QTreeView::indicator:pressed {background-color: rgba(255, 255, 255, 20);border: 1px solid "
           "#c2c2c2;}QGroupBox::indicator:checked, QMenu::indicator:checked, QCheckBox::indicator:checked, QTreeView::indicator:checked "
           "{background-color: #d7827e;border: 1px solid #d7827e;image: url(theme:CheckBox.png);}QGroupBox::indicator:checked:hover, "
           "QMenu::indicator:checked:hover, QCheckBox::indicator:checked:hover, QTreeView::indicator:checked:hover {background-color: "
           "#d7827e;border: 1px solid #d7827e;}QGroupBox::indicator:checked:pressed, QMenu::indicator:checked:pressed, "
           "QCheckBox::indicator:checked:pressed, QTreeView::indicator:checked:pressed {background-color: #d7827e;border: 1px solid "
           "#d7827e;image: url(theme:CheckBoxPressed.png);}QCheckBox::indicator:indeterminate, QGroupBox::indicator:indeterminate, "
           "QTreeView::indicator:indeterminate, QTableView::indicator:indeterminate {background-color: #d7827e;border: 1px solid "
           "#d7827e;image: url(theme:CheckBoxInterminate.png);}QCheckBox::indicator:indeterminate:hover, "
           "QGroupBox::indicator:indeterminate:hover, QTreeView::indicator:indeterminate:hover, QTableView::indicator:indeterminate:hover "
           "{background-color: #d7827e;border: 1px solid #d7827e;}QCheckBox::indicator:indeterminate:pressed, "
           "QGroupBox::indicator:indeterminate:pressed, QTreeView::indicator:indeterminate:pressed, "
           "QTableView::indicator:indeterminate:pressed {background-color: #d7827e;border: 1px solid #d7827e;image: "
           "url(theme:CheckBoxInterminatePressed.png);}QCheckBox::indicator:indeterminate:disabled, "
           "QGroupBox::indicator:indeterminate:disabled, QTreeView::indicator:indeterminate:disabled, "
           "QTableView::indicator:indeterminate:disabled {border: 1px solid #d1d1d1;background-color: #d1d1d1;image: "
           "url(theme:CheckBoxInterminateDisabled.png);}QGroupBox::indicator:checked:disabled, QMenu::indicator:checked:disabled, "
           "QCheckBox::indicator:checked:disabled, QTreeView::indicator:checked:disabled {border: 1px solid #d1d1d1;background-color: "
           "#d1d1d1;image: url(theme:CheckBoxDisabled.png);}QGroupBox::indicator:disabled, QMenu::indicator:disabled, "
           "QCheckBox::indicator:disabled, QTreeView::indicator:disabled {border: 1px solid #d1d1d1;background-color: rgba(255, 255, 255, "
           "0);}QStatusBar::item {background: transparent;}QTableView, QColumnView {gridline-color: rgba(255, 255, 255, 0);}QGroupBox "
           "QToolButton {padding: 6px;}QDialog QListView::icon, QDialog QListView::text {left: 0px;}InstanceView {padding: "
           "5px;background-color: rgba(0,0,0,30%);}#mainToolBar {background: rgba(0,0,0,0%);}#instanceToolBar {background: "
           "rgba(0,0,0,40%);}#mainToolBar QToolButton {padding: 5px;}#mainToolBar QToolButton[popupMode=\"2\"] {padding-right: "
           "20px;}#statusBar, #newsToolBar {background: rgba(0,0,0,50%);}#statusBar QLabel {padding: 20px;padding-top: 0px;padding-bottom: "
           "15px;}QSizeGrip {height: 0px;width: 0px;}QToolTip {border: 1px solid #ffffff;color: white;padding: 2px;border-radius: "
           "3px;background: #ffffff;}QListView {padding: 5px;}";
}
QString FreesmLightTheme::tooltip()
{
    return "";
}
