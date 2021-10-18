/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_vnotedbmanager.h"
#include "vnotedbmanager.h"
#include "vnotefolderoper.h"
#include "vnoteforlder.h"
#include "vnoteitemoper.h"
#include "vnoteitem.h"

ut_vnotedbmanager_test::ut_vnotedbmanager_test()
{
}

TEST_F(ut_vnotedbmanager_test, initVNoteDb)
{
    VNoteDbManager::instance()->initVNoteDb();
}

TEST_F(ut_vnotedbmanager_test, getVNoteDb)
{
    VNoteDbManager::instance()->getVNoteDb();
}
