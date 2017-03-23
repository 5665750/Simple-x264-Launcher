///////////////////////////////////////////////////////////////////////////////
// Simple x264 Launcher
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#pragma once

//MUtils
#include <MUtils/OSSupport.h>

//Qt
#include <QValidator>
#include <QLabel>
#include <QToolTip>

class StringValidator : public QValidator
{
public:
	StringValidator(QLabel *notifier, QLabel *icon);
	virtual State validate(QString &input, int &pos) const = 0;
	virtual void fixup(QString &input) const;

protected:
	QLabel *const m_notifier, *const m_icon;

	bool checkParam(const QStringList &input, const char *const params[], const bool &doubleMinus) const;
	bool checkPrefix(const QStringList &input, const bool &doubleMinus) const;
	bool checkCharacters(const QStringList &input) const;
	const bool &setStatus(const bool &flag, const QString &toolName) const;
};

class StringValidatorEncoder : public StringValidator
{
public:
	StringValidatorEncoder(QLabel *notifier, QLabel *icon) : StringValidator(notifier, icon) {}
	virtual State validate(QString &input, int &pos) const;
};

class StringValidatorSource : public StringValidator
{
public:
	StringValidatorSource(QLabel *notifier, QLabel *icon) : StringValidator(notifier, icon) {}
	virtual State validate(QString &input, int &pos) const;
};