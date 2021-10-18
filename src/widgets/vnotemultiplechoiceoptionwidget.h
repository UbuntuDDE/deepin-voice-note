/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     ningyuchuang <ningyuchuang@uniontech.com>
*
* Maintainer: ningyuchuang <ningyuchuang@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VNOTEMULTIPLECHOICERIGHTWIDGET_H
#define VNOTEMULTIPLECHOICERIGHTWIDGET_H

#include <DToolButton>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DWidget>
#include <DLabel>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDomDocument>
#include <QDomElement>
#include <QSvgRenderer>
#include <QFile>

DWIDGET_USE_NAMESPACE
//多选操作页面
class VnoteMultipleChoiceOptionWidget : public DWidget
{
    Q_OBJECT
public:
    //按钮id
    enum ButtonValue {
        Move = 1,
        SaveAsTxT = 2,
        SaveAsVoice = 3,
        Delete = 4
    };
    explicit VnoteMultipleChoiceOptionWidget(QWidget *parent = nullptr);
    //初始化ui
    void initUi();
    //更新笔记数量
    void setNoteNumber(int number);
    //设置按钮是否置灰
    void enableButtons(bool saveAsTxtButtonStatus = true, bool saveAsVoiceButtonStatus = true, bool moveButtonStatus = true);
    //press更新svg
    void buttonPressed(ButtonValue value);

private:
    //获得svg
    QPixmap setSvgColor(QString svgPath, QString color);
    //设置svg颜色属性
    void setSVGBackColor(QDomElement &elem, QString attr, QString val);
signals:
    //请求多选操作
    void requestMultipleOption(int id);

protected:
    //触发多选操作
    void trigger(int id);
    //初始化槽连接
    void initConnections();
    //根据主题设置图标与删除按钮文本颜色
    void changeFromThemeType();
    //根据字体大小调整按钮UI
    void onFontChanged();
    //窗口大小重设
    void resizeEvent(QResizeEvent *event) override;

    bool eventFilter(QObject *o, QEvent *e) override;

private:
    DLabel *m_tipsLabel {nullptr};
    DToolButton *m_moveButton {nullptr};
    DToolButton *m_saveVoiceButton {nullptr};
    DToolButton *m_saveTextButton {nullptr};
    DToolButton *m_deleteButton {nullptr};
    int m_noteNumber = 0;
};

#endif // VNOTEMULTIPLECHOICERIGHTWIDGET_H
