/*
 *  Qactus - A Qt based OBS notifier
 *
 *  Copyright (C) 2016-2018 Javier Llorente <javier@opensuse.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "obsresult.h"

OBSResult::OBSResult()
{
    status = new OBSStatus();
}

OBSResult::~OBSResult()
{
    delete status;
    status = nullptr;
}

QString OBSResult::getProject() const
{
    return status->getProject();
}

void OBSResult::setProject(const QString &value)
{
    status->setProject(value);
}

QString OBSResult::getRepository() const
{
    return repository;
}

void OBSResult::setRepository(const QString &value)
{
    repository = value;
}

QString OBSResult::getArch() const
{
    return arch;
}

void OBSResult::setArch(const QString &value)
{
    arch = value;
}

QString OBSResult::getCode() const
{
    return code;
}

void OBSResult::setCode(const QString &value)
{
    code = value;
}

QString OBSResult::getState() const
{
    return state;
}

void OBSResult::setState(const QString &value)
{
    state = value;
}

OBSStatus *OBSResult::getStatus() const
{
    return status;
}
