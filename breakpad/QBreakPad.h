/*
 * QEstEidBreakpad
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <QtCore/QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QWizard>
#else
#include <QtGui/QWizard>
#endif

class QPlainTextEdit;
class QProgressBar;
class QTextStream;
namespace google_breakpad { class CallStack; class ExceptionHandler; }

class QBreakPad: public QObject
{
	Q_OBJECT
public:
	explicit QBreakPad();
	~QBreakPad();

	static bool isCrashReport(int argc, char *argv[] );

private slots:
	void sendReports();

private:
	QString path() const;

	google_breakpad::ExceptionHandler *d;
};

class QBreakPadDialog: public QWizard
{
	Q_OBJECT
public:
	explicit QBreakPadDialog( const QString &name, const QString &path = QString() );
	~QBreakPadDialog();

private slots:
	void toggleComments();
	void toggleStack();
	void updateProgress( qint64 value, qint64 range );

private:
	QString parseStack() const;
	void printStack( const google_breakpad::CallStack *stack, QTextStream &s ) const;
	bool validateCurrentPage();

	QPlainTextEdit *edit, *stack;
	QString id, file;
	QProgressBar *progress;
};
