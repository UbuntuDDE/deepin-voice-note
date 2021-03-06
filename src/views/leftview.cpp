/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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

#include "leftview.h"
#include "leftviewdelegate.h"
#include "leftviewsortfilter.h"
#include "globaldef.h"
#include "moveview.h"

#include "dialog/folderselectdialog.h"

#include "common/actionmanager.h"
#include "common/standarditemcommon.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "common/setting.h"
#include "widgets/vnoterightmenu.h"
#include "db/vnoteitemoper.h"

#include <DApplication>

#include <QMouseEvent>
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#include <QTimer>
#include <QScrollBar>

/**
 * @brief LeftView::LeftView
 * @param parent
 */
LeftView::LeftView(QWidget *parent)
    : DTreeView(parent)
{
    initModel();
    initDelegate();
    initMenu();
    initConnections();
    setContextMenuPolicy(Qt::NoContextMenu);
    this->setDragEnabled(true);
    this->setDragDropMode(QAbstractItemView::DragDrop);
    this->setDropIndicatorShown(true);
    this->setAcceptDrops(true);
    viewport()->installEventFilter(this);
    this->installEventFilter(this);
}

/**
 * @brief LeftView::initModel
 */
void LeftView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);
    m_pSortViewFilter = new LeftViewSortFilter(this);
    m_pSortViewFilter->setDynamicSortFilter(false);
    m_pSortViewFilter->setSourceModel(m_pDataModel);
    this->setModel(m_pSortViewFilter);
}

/**
 * @brief LeftView::initDelegate
 */
void LeftView::initDelegate()
{
    m_pItemDelegate = new LeftViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

/**
 * @brief LeftView::getNotepadRoot
 * @return ?????????????????????
 */
QStandardItem *LeftView::getNotepadRoot()
{
    QStandardItem *pItem = m_pDataModel->item(0);
    if (pItem == nullptr) {
        pItem = StandardItemCommon::createStandardItem(nullptr, StandardItemCommon::NOTEPADROOT);
        m_pDataModel->insertRow(0, pItem);
    }
    return pItem;
}

/**
 * @brief LeftView::getNotepadRootIndex
 * @return ????????????????????????
 */
QModelIndex LeftView::getNotepadRootIndex()
{
    return m_pSortViewFilter->mapFromSource(getNotepadRoot()->index());
}

/**
 * @brief LeftView::mousePressEvent
 * @param event
 */
void LeftView::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();
    if (!m_onlyCurItemMenuEnable) {
        //???????????????
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            //????????????????????????????????????
            m_touchPressPoint = event->pos();
            m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
            //??????????????????
            setTouchState(TouchPressing);
            m_index = indexAt(event->pos());
            m_notepadMenu->setPressPoint(QCursor::pos());
            //??????????????????
            m_selectCurrentTimer->start(250);
            //????????????????????????
            m_popMenuTimer->start(1000);
            return;
        }
        event->setModifiers(Qt::NoModifier);
        setTouchState(TouchState::TouchPressing);
        //?????????????????????
        //        DTreeView::mousePressEvent(event);
        if (event->button() == Qt::RightButton) {
            if (MenuStatus::ReleaseFromMenu == m_menuState) {
                m_menuState = MenuStatus::Normal;
                return;
            }
            QModelIndex index = this->indexAt(event->pos());
            if (StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM
                && (!m_onlyCurItemMenuEnable || index == this->currentIndex())) {
                this->setCurrentIndex(index);
                m_notepadMenu->popup(event->globalPos());
                //????????????????????????????????????????????????????????????hide
                m_notepadMenu->setWindowOpacity(1);
            }
        }
    } else {
        //?????????????????????????????????????????????
        if (currentIndex() == indexAt(event->pos())) {
            if (event->source() == Qt::MouseEventSynthesizedByQt) {
                m_touchPressPoint = event->pos();
                m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
                //??????????????????
                setTouchState(TouchPressing);
                m_notepadMenu->setPressPoint(QCursor::pos());
                m_popMenuTimer->start(1000);
                return;
            } else {
                if (Qt::RightButton == event->button()) {
                    m_notepadMenu->setWindowOpacity(1);
                    m_notepadMenu->popup(event->globalPos());
                }
            }
        }
    }
}

/**
 * @brief LeftView::mouseReleaseEvent
 * @param event
 */
void LeftView::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDraging = false;
    //???????????????
    m_selectCurrentTimer->stop();
    m_popMenuTimer->stop();
    m_menuState = MenuStatus::Normal;
    //??????????????????????????????drop?????????????????????????????????
    if (m_touchState == TouchState::TouchDraging) {
        setTouchState(TouchState::TouchNormal);
        return;
    }
    if (m_onlyCurItemMenuEnable) {
        return;
    }
    //?????????????????????????????????????????????
    QModelIndex index = indexAt(event->pos());
    if (index.row() != currentIndex().row() && m_touchState == TouchState::TouchPressing) {
        if (index.isValid()) {
            setCurrentIndex(index);
        }
        setTouchState(TouchState::TouchNormal);
        return;
    }
    setTouchState(TouchState::TouchNormal);
    DTreeView::mouseReleaseEvent(event);
}

/**
 * @brief LeftView::setTouchState ???????????????????????????
 * @param touchState
 */
void LeftView::setTouchState(const TouchState &touchState)
{
    m_touchState = touchState;
}

/**
 * @brief LeftView::mouseDoubleClickEvent
 * @param event
 */
void LeftView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DTreeView::mouseDoubleClickEvent(event);
    }
}

/**
 * @brief LeftView::mouseMoveEvent
 * @param event
 */
void LeftView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_onlyCurItemMenuEnable) {
        return;
    }
    //???????????????????????????
    if ((event->source() == Qt::MouseEventSynthesizedByQt && event->buttons() & Qt::LeftButton)) {
        if (TouchState::TouchOutVisibleRegion != m_touchState) {
            doTouchMoveEvent(event);
        }
        return;
    }
    //????????????????????????
    else if ((event->buttons() & Qt::LeftButton) && m_touchState == TouchState::TouchPressing) {
        if (!m_isDraging && indexAt(event->pos()).isValid()) {
            setCurrentIndex(indexAt(event->pos()));
            //?????????????????????
            if (qAbs(event->pos().x() - m_touchPressPoint.x()) > 3
                || qAbs(event->pos().y() - m_touchPressPoint.y()) > 3) {
                handleDragEvent(false);
            }
        }
    } else {
        DTreeView::mouseMoveEvent(event);
    }
}

/**
 * @brief LeftView::doTouchMoveEvent  ???????????????move??????????????????????????????????????????????????????
 * @param event
 */
void LeftView::doTouchMoveEvent(QMouseEvent *event)
{
    //?????????????????????move????????????????????????????????????
    //    m_pItemDelegate->setDraging(false);
    double distX = event->pos().x() - m_touchPressPoint.x();
    double distY = event->pos().y() - m_touchPressPoint.y();
    //??????????????????
    QDateTime current = QDateTime::currentDateTime();
    qint64 timeParam = current.toMSecsSinceEpoch() - m_touchPressStartMs;

    switch (m_touchState) {
    //????????????
    case TouchState::TouchPressing:
        //250ms-1000ms??????????????????10px??????????????????
        if ((timeParam > 250 && timeParam < 1000) && (qAbs(distY) > 10 || qAbs(distX) > 10)) {
            if (!m_isDraging)
                handleDragEvent();
        }
        //250ms?????????????????????????????????10px???????????????????????????5px??????????????????
        else if (timeParam <= 250 && (qAbs(distY) > 10 || qAbs(distX) > 5)) {
            setTouchState(TouchState::TouchMoving);
            handleTouchSlideEvent(timeParam, distY, event->pos());
        }
        break;
    //??????????????????
    case TouchState::TouchDraging:
        if (!m_isDraging)
            handleDragEvent();
        break;
    //??????????????????
    case TouchState::TouchMoving:
        if (!viewport()->visibleRegion().contains(event->pos())) {
            setTouchState(TouchState::TouchOutVisibleRegion);
        } else if (qAbs(distY) > 5) {
            handleTouchSlideEvent(timeParam, distY, event->pos());
        }
        break;
    default:
        break;
    }
}

/**
 * @brief LeftView::handleDragEvent
 * @param isTouch ????????????
 */
void LeftView::handleDragEvent(bool isTouch)
{
    if (m_onlyCurItemMenuEnable)
        return;
    if (isTouch) {
        setTouchState(TouchState::TouchDraging);
    }
    m_popMenuTimer->stop();
    m_notepadMenu->setWindowOpacity(0.0);
    triggerDragFolder();
}

/**
 * @brief LeftView::keyPressEvent
 * @param e
 */
void LeftView::keyPressEvent(QKeyEvent *e)
{
    if (m_onlyCurItemMenuEnable || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        e->ignore();
    } else {
        if (0 == this->currentIndex().row() && e->key() == Qt::Key_Up) {
            e->ignore();
        } else if (e->key() == Qt::Key_Home) {
            //????????????????????????home????????????????????????index????????????????????????????????????????????????????????????????????????
            this->setCurrentIndex(m_pSortViewFilter->index(0, 0, getNotepadRootIndex()));
        } else {
            DTreeView::keyPressEvent(e);
        }
    }
}

/**
 * @brief LeftView::restoreNotepadItem
 * @return ???????????????
 */
QModelIndex LeftView::restoreNotepadItem()
{
    QModelIndex index = this->currentIndex();
    QItemSelectionModel *model = this->selectionModel();

    if (index.isValid() && StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM) {
        if (!model->isSelected(index)) {
            this->setCurrentIndex(index);
        }
    } else {
        index = setDefaultNotepadItem();
    }

    return index;
}

/**
 * @brief LeftView::setDefaultNotepadItem
 * @return ???????????????
 */
QModelIndex LeftView::setDefaultNotepadItem()
{
    QModelIndex index = m_pSortViewFilter->index(0, 0, getNotepadRootIndex());
    this->setCurrentIndex(index);
    return index;
}

/**
 * @brief LeftView::addFolder
 * @param folder
 */
void LeftView::addFolder(VNoteFolder *folder)
{
    if (nullptr != folder) {
        QStandardItem *pItem = StandardItemCommon::createStandardItem(
            folder, StandardItemCommon::NOTEPADITEM);

        QStandardItem *root = getNotepadRoot();
        root->appendRow(pItem);
        QModelIndex index = m_pSortViewFilter->mapFromSource(pItem->index());
        setCurrentIndex(index);
    }
    this->scrollToTop();
}

/**
 * @brief LeftView::eventFilter
 * @param o
 * @param e
 * @return false ??????????????????????????????
 */
bool LeftView::eventFilter(QObject *o, QEvent *e)
{
    if (o == this) {
        if (e->type() == QEvent::FocusIn) {
            QFocusEvent *event = dynamic_cast<QFocusEvent *>(e);
            m_pItemDelegate->setTabFocus(event->reason() == Qt::TabFocusReason);
            if (m_pItemDelegate->isTabFocus()) {
                scrollTo(currentIndex());
            }
        } else if (e->type() == QEvent::FocusOut) {
            m_pItemDelegate->setTabFocus(false);
        }
    } else {
        if (e->type() == QEvent::DragLeave) {
            m_pItemDelegate->setDrawHover(false);
            update();
        } else if (e->type() == QEvent::DragEnter) {
            m_pItemDelegate->setDrawHover(true);
            update();
        }
    }
    return false;
}

/**
 * @brief LeftView::appendFolder
 * @param folder
 */
void LeftView::appendFolder(VNoteFolder *folder)
{
    if (nullptr != folder) {
        QStandardItem *pItem = StandardItemCommon::createStandardItem(
            folder, StandardItemCommon::NOTEPADITEM);

        QStandardItem *root = getNotepadRoot();

        if (nullptr != root) {
            root->appendRow(pItem);
        }
    }
}

/**
 * @brief LeftView::editFolder
 */
void LeftView::editFolder()
{
    edit(currentIndex());
}

/**
 * @brief LeftView::removeFolder
 * @return ????????????????????????
 */
VNoteFolder *LeftView::removeFolder()
{
    QModelIndex index = currentIndex();

    if (StandardItemCommon::getStandardItemType(index) != StandardItemCommon::NOTEPADITEM) {
        return nullptr;
    }

    VNoteFolder *data = reinterpret_cast<VNoteFolder *>(
        StandardItemCommon::getStandardItemData(index));

    m_pSortViewFilter->removeRow(index.row(), index.parent());

    return data;
}

/**
 * @brief LeftView::folderCount
 * @return ???????????????
 */
int LeftView::folderCount()
{
    int count = 0;

    QStandardItem *root = getNotepadRoot();

    if (nullptr != root) {
        count = root->rowCount();
    }

    return count;
}

/**
 * @brief LeftView::initMenu
 */
void LeftView::initMenu()
{
    m_notepadMenu = ActionManager::Instance()->notebookContextMenu();
}

/**
 * @brief LeftView::initConnections
 */
void LeftView::initConnections()
{
    //??????????????????
    connect(m_notepadMenu, &VNoteRightMenu::menuTouchMoved, this, &LeftView::handleDragEvent);
    //??????????????????
    connect(m_notepadMenu, &VNoteRightMenu::menuTouchReleased, this, [=] {
        m_touchState = TouchState::TouchNormal;
        m_menuState = MenuStatus::ReleaseFromMenu;
    });
    //???????????????????????????????????????
    m_selectCurrentTimer = new QTimer(this);
    connect(m_selectCurrentTimer, &QTimer::timeout, [=] {
        if (m_touchState == TouchState::TouchPressing && m_index.isValid())
            this->setCurrentIndex(m_index);
        m_selectCurrentTimer->stop();
    });
    //???????????????????????????????????????
    m_popMenuTimer = new QTimer(this);
    connect(m_popMenuTimer, &QTimer::timeout, [=] {
        if (m_touchState == TouchState::TouchPressing && m_index.isValid()) {
            m_notepadMenu->setWindowOpacity(1);
            m_notepadMenu->exec(QCursor::pos());
        }
        m_popMenuTimer->stop();
    });
}

/**
 * @brief LeftView::setOnlyCurItemMenuEnable
 * @param enable
 */
void LeftView::setOnlyCurItemMenuEnable(bool enable)
{
    m_onlyCurItemMenuEnable = enable;
    m_pItemDelegate->setEnableItem(!enable);
    this->update();
}

/**
 * @brief LeftView::sort
 */
void LeftView::sort()
{
    return m_pSortViewFilter->sort(0, Qt::DescendingOrder);
}

/**
 * @brief LeftView::closeEditor
 * @param editor
 * @param hint
 */
void LeftView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    Q_UNUSED(hint);
    DTreeView::closeEditor(editor, QAbstractItemDelegate::NoHint);
}

/**
 * @brief LeftView::closeMenu
 */
void LeftView::closeMenu()
{
    m_notepadMenu->close();
}

/**
 * @brief LeftView::doNoteMove
 * @param src
 * @param dst
 * @return
 */
bool LeftView::doNoteMove(const QModelIndexList &src, const QModelIndex &dst)
{
    if (src.size() && StandardItemCommon::getStandardItemType(dst) == StandardItemCommon::NOTEPADITEM) {
        VNoteFolder *selectFolder = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(dst));
        VNoteItem *tmpData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));
        if (selectFolder && tmpData->folderId != selectFolder->id) {
            VNoteItemOper noteOper;
            VNOTE_ITEMS_MAP *srcNotes = noteOper.getFolderNotes(tmpData->folderId);
            VNOTE_ITEMS_MAP *destNotes = noteOper.getFolderNotes(selectFolder->id);
            for (auto it : src) {
                tmpData = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(it));
                //??????????????????
                srcNotes->lock.lockForWrite();
                srcNotes->folderNotes.remove(tmpData->noteId);
                srcNotes->lock.unlock();

                destNotes->lock.lockForWrite();
                tmpData->folderId = selectFolder->id;
                destNotes->folderNotes.insert(tmpData->noteId, tmpData);
                destNotes->lock.unlock();
                //???????????????
                noteOper.updateFolderId(tmpData);
            }

            //????????????????????????????????????maxid
            if (src.count() == m_notesNumberOfCurrentFolder) {
                VNoteFolder *folder = reinterpret_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex()));
                folder->maxNoteIdRef() = 0;
            } else {
                selectFolder->maxNoteIdRef() += src.size();
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief LeftView::selectMoveFolder
 * @param src
 * @return
 */
QModelIndex LeftView::selectMoveFolder(const QModelIndexList &src)
{
    QModelIndex index;
    if (src.size()) {
        VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(src[0]));
        QString elideText = data->noteTitle;
        if (m_folderSelectDialog == nullptr) {
            m_folderSelectDialog = new FolderSelectDialog(m_pDataModel, this);
            m_folderSelectDialog->resize(VNOTE_SELECTDIALOG_W, 372);
        }
        m_folderSelectDialog->setFocus();
        QList<VNoteFolder *> folders;
        folders.push_back(static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex())));
        m_folderSelectDialog->setFolderBlack(folders);
        m_folderSelectDialog->setNoteContextInfo(elideText, src.size());
        m_folderSelectDialog->clearSelection();

        m_folderSelectDialog->exec();
        if (m_folderSelectDialog->result() == FolderSelectDialog::Accepted) {
            index = m_folderSelectDialog->getSelectIndex();
        }
    }
    return index;
}

/**
 * @brief LeftView::getFolderSort
 * @return ?????????????????????????????????????????????
 * ?????????????????????
 */
QString LeftView::getFolderSort()
{
    QString tmpQstr = "";
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex currentIndex;
    for (int i = 0; i < folderCount(); i++) {
        currentIndex = m_pSortViewFilter->index(i, 0, rootIndex);
        if (!currentIndex.isValid()) {
            break;
        }
        VNoteFolder *data = reinterpret_cast<VNoteFolder *>(
            StandardItemCommon::getStandardItemData(currentIndex));
        if (tmpQstr.isEmpty()) {
            tmpQstr = QString::number(data->id);
        } else {
            tmpQstr = tmpQstr + "," + QString::number(data->id);
        }
    }
    return tmpQstr;
}

/**
 * @brief LeftView::setFolderSort
 * @return true ???????????????false ????????????
 * ???????????????????????????
 */
bool LeftView::setFolderSort()
{
    bool sortResult = false;
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex currentIndex;
    int rowCount = folderCount();
    for (int i = 0; i < rowCount; i++) {
        currentIndex = m_pSortViewFilter->index(i, 0, rootIndex);
        if (!currentIndex.isValid()) {
            break;
        }
        VNoteFolder *data = reinterpret_cast<VNoteFolder *>(
            StandardItemCommon::getStandardItemData(currentIndex));
        if (nullptr != data) {
            reinterpret_cast<VNoteFolder *>(
                StandardItemCommon::getStandardItemData(currentIndex))
                ->sortNumber = rowCount - i;
        }
        sortResult = true;
    }
    return sortResult;
}

/**
 * @brief LeftView::getFirstFolder
 * @return ??????????????????
 */
VNoteFolder *LeftView::getFirstFolder()
{
    QModelIndex rootIndex = getNotepadRootIndex();
    QModelIndex child = m_pSortViewFilter->index(0, 0, rootIndex);
    if (child.isValid()) {
        VNoteFolder *vnotefolder = reinterpret_cast<VNoteFolder *>(
            StandardItemCommon::getStandardItemData(child));
        return vnotefolder;
    }
    return nullptr;
}

/**
 * @brief LeftView::handleTouchSlideEvent  ???????????????move??????????????????????????????????????????????????????
 * @param timeParam ????????????
 * @param distY ??????????????????
 * @param point ????????????????????????
 */
void LeftView::handleTouchSlideEvent(qint64 timeParam, double distY, QPoint point)
{
    //????????????????????????????????????????????????????????????????????????
    double param = ((qAbs(distY)) / timeParam) + 0.3;
    verticalScrollBar()->setSingleStep(static_cast<int>(20 * param));
    verticalScrollBar()->triggerAction((distY > 0) ? QScrollBar::SliderSingleStepSub : QScrollBar::SliderSingleStepAdd);
    m_touchPressStartMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_touchPressPoint = point;
}

/**
 * @brief LeftView::dragEnterEvent
 * @param event
 * ????????????????????????
 */
void LeftView::dragEnterEvent(QDragEnterEvent *event)
{
    // ??????????????????????????????????????????????????????NOTES_DRAG_KEY???????????????NOTEPAD_DRAG_KEY???
    if (!event->mimeData()->hasFormat(NOTES_DRAG_KEY)
        && !event->mimeData()->hasFormat(NOTEPAD_DRAG_KEY)) {
        event->ignore();
        return DTreeView::dragEnterEvent(event);
    }

    if (m_folderDraing) {
        m_pItemDelegate->setDragState(true);
        this->update();
    }

    event->accept();
}

/**
 * @brief LeftView::dragMoveEvent
 * @param event
 * ??????????????????
 */
void LeftView::dragMoveEvent(QDragMoveEvent *event)
{
    DTreeView::dragMoveEvent(event);
    this->update();
    event->accept();
}

/**
 * @brief LeftView::dragLeaveEvent
 * @param event
 * ????????????????????????
 */
void LeftView::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (m_folderDraing) {
        m_pItemDelegate->setDragState(false);
        m_pItemDelegate->setDrawHover(false);
        this->update();
    }
    event->accept();
}

/**
 * @brief LeftView::doDragMove
 * @param src
 * @param dst
 * ????????????
 */
void LeftView::doDragMove(const QModelIndex &src, const QModelIndex &dst)
{
    if (src.isValid() && dst.isValid() && src != dst) {
        VNoteFolder *tmpFolder = reinterpret_cast<VNoteFolder *>(
            StandardItemCommon::getStandardItemData(dst));
        if (nullptr == tmpFolder) {
            return;
        }

        // ??????????????????????????????????????????
        VNoteFolder *firstFolder = getFirstFolder();
        if (firstFolder && -1 == firstFolder->sortNumber) {
            setFolderSort();
        }

        int tmpRow = qAbs(src.row() - dst.row());
        int dstNum = tmpFolder->sortNumber;
        QModelIndex tmpIndex;
        QModelIndex rootIndex = getNotepadRootIndex();

        // ???????????????????????????????????????????????????????????????????????????
        for (int i = 0; i < tmpRow; i++) {
            // ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????1??????????????????1
            if (dst.row() > src.row()) {
                tmpIndex = m_pSortViewFilter->index(dst.row() - i, 0, rootIndex);
                if (!tmpIndex.isValid()) {
                    break;
                }
                tmpFolder = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(tmpIndex));
                tmpFolder->sortNumber += 1;

            } else {
                tmpIndex = m_pSortViewFilter->index(dst.row() + i, 0, rootIndex);
                if (!tmpIndex.isValid()) {
                    break;
                }
                tmpFolder = reinterpret_cast<VNoteFolder *>(
                    StandardItemCommon::getStandardItemData(tmpIndex));
                tmpFolder->sortNumber -= 1;
            }
        }

        tmpFolder = reinterpret_cast<VNoteFolder *>(
            StandardItemCommon::getStandardItemData(src));
        tmpFolder->sortNumber = dstNum;

        sort();

        // ???????????????????????????????????????????????????????????????????????????
        QString folderSortData = getFolderSort();
        setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);
    }
}

void LeftView::setNumberOfNotes(int numberOfNotes)
{
    m_notesNumberOfCurrentFolder = numberOfNotes;
}

/**
 * @brief LeftView::triggerDragFolder
 * ??????????????????
 */
void LeftView::triggerDragFolder()
{
    QModelIndex dragIndex = this->indexAt(mapFromGlobal(QCursor::pos()));
    if (!dragIndex.isValid()) {
        m_isDraging = true;
        return;
    }

    VNoteFolder *folder = reinterpret_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(currentIndex()));
    // ?????????????????????????????????????????????????????????????????????????????????????????????
    if (folder) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(NOTEPAD_DRAG_KEY, QByteArray());

        if (nullptr == m_MoveView) {
            m_MoveView = new MoveView(this);
        }
        m_MoveView->setFixedSize(224, 91);
        m_MoveView->setFolder(folder);
        drag->setPixmap(m_MoveView->grab());
        drag->setMimeData(mimeData);
        m_folderDraing = true;
        //????????????????????????????????????????????????????????????????????????
        drag->setHotSpot(QPoint(21, 25));
        drag->exec(Qt::MoveAction);
        drag->deleteLater();
        m_folderDraing = false;
        m_pItemDelegate->setDragState(false);
        m_pItemDelegate->setDrawHover(true);
        //????????????
        m_notepadMenu->hide();
    }
}

/**
 * @brief LeftView::dropEvent
 * @param event
 * ??????????????????
 */
void LeftView::dropEvent(QDropEvent *event)
{
    // ????????????????????????????????????????????????NOTES_DRAG_KEY???????????????NOTEPAD_DRAG_KEY???
    if (event->mimeData()->hasFormat(NOTES_DRAG_KEY)) {
        //???????????????????????????????????????
        QModelIndex index = indexAt(event->pos());
        //???????????????????????????????????????????????????????????????
        bool isDragCancelled = currentIndex().row() == index.row() || !index.isValid() ? true : false;
        //?????????????????????
        emit dropNotesEnd(isDragCancelled);
    } else if (event->mimeData()->hasFormat(NOTEPAD_DRAG_KEY)) {
        doDragMove(currentIndex(), indexAt(mapFromGlobal(QCursor::pos())));
    }
}

/**
 * @brief LeftView::popupMenu
 */
void LeftView::popupMenu()
{
    QModelIndexList selectIndexes = selectedIndexes();
    if (selectIndexes.count()) {
        QRect curRect = visualRect(selectIndexes.first());
        //????????????????????????
        if (!viewport()->visibleRegion().contains(curRect.center())) {
            scrollTo(selectIndexes.first());
            curRect = visualRect(selectIndexes.first());
        }
        bool tabFocus = m_pItemDelegate->isTabFocus();
        m_notepadMenu->setWindowOpacity(1);
        m_notepadMenu->exec(mapToGlobal(curRect.center()));
        if (hasFocus()) {
            m_pItemDelegate->setTabFocus(tabFocus);
        }
    }
}
