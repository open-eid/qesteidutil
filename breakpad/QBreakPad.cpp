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

/*
 * dump_syms qbreakpad > qbreakpad.sym
 * head -n1 qbreakpad.sym
 * "MODULE mac x86_64 1F902FC241353FE28AA37FAEDB23131E0 qbreakpad"
 * mkdir -p sympath/qbreakpad/1F902FC241353FE28AA37FAEDB23131E0/
 * mv qbreakpad.sym sympath/qbreakpad/1F902FC241353FE28AA37FAEDB23131E0/
 *
 * Debug
 * minidump_stackwalk 219F15E5-1B1F-4F08-84D8-28031706FD81.dmp ./symbols/ 2>&1
 */

#include "QBreakPad.h"

#include <common/Common.h>
#include <common/Settings.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QTranslator>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>
#else
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QProgressBar>
#include <QtGui/QVBoxLayout>
#endif

#if defined(Q_OS_MAC)
#include "client/mac/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined(Q_OS_WIN)
#include "client/windows/handler/exception_handler.h"
#endif
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/stack_frame.h"

using namespace google_breakpad;

#ifdef Q_OS_WIN
static bool QBreakPadDump( const wchar_t *dump_path, const wchar_t *minidump_id, void *,
	EXCEPTION_POINTERS *, MDRawAssertionInfo *, bool succeeded )
{
	qDebug() << "Application crashed, starting crashreport application";
	QProcess::startDetached( qApp->applicationFilePath(), QStringList() << "-crashreport"
		<< QString::fromUtf16( (const ushort*)dump_path )
		<< QString::fromUtf16( (const ushort*)minidump_id ) );
	return succeeded;
}
#elif defined(Q_OS_MAC)
static bool QBreakPadDump( const char *dump_path, const char *minidump_id, void *, bool succeeded )
{
	qDebug() << "Application crashed, saving report to " << dump_path << "/" << minidump_id;
	return succeeded;
}
#else
static bool QBreakPadDump( const MinidumpDescriptor &descriptor, void *, bool succeeded )
{
	qDebug() << "Application crashed, starting crashreport application" << descriptor.path();
	QProcess::startDetached( qApp->applicationFilePath(), QStringList() << "-crashreport"
		<< descriptor.path() );
	return succeeded;
}
#endif

QBreakPad::QBreakPad()
	: d( 0 )
{
	Q_INIT_RESOURCE(breakpad);
#if defined(Q_OS_WIN)
	QString tmp = QDir::tempPath();
	d = new ExceptionHandler( LPCWSTR(tmp.utf16()), 0, QBreakPadDump, 0,
		ExceptionHandler::HANDLER_ALL );
#elif defined(Q_OS_MAC)
	QDir().mkpath( path() );
	QByteArray tmp = path().toLocal8Bit();
	qDebug() << tmp;
	d = new google_breakpad::ExceptionHandler( tmp.constData(), 0, QBreakPadDump, 0, true, 0 );
	QTimer::singleShot( 0, this, SLOT(sendReports()) );
#else
	QByteArray tmp = QDir::tempPath().toLocal8Bit();
	d = new ExceptionHandler(
		MinidumpDescriptor( tmp.constData() ), 0, QBreakPadDump, 0, true, -1 );
#endif
	qDebug() << "Enabling crashreporting" << tmp;
}

QBreakPad::~QBreakPad()
{
	delete d;
}

bool QBreakPad::isCrashReport( int argc, char *argv[] )
{
	QString crash( "-crashreport" );
	for( int i = 0; i < argc; ++i )
		if( crash == argv[i] )
			return true;
	return false;
}

QString QBreakPad::path() const
{
	return QDir::tempPath() + "/ee.ria.crashreport";
}

void QBreakPad::sendReports()
{
	qWarning() << "Looking for crash reports" << path();
	Common *app = static_cast<Common*>(QCoreApplication::instance());
	Q_FOREACH( const QString &file, QDir( path() ).entryList( QStringList() << "*.dmp" ) )
	{
		qWarning() << "Processing crash report" << file;
		QBreakPadDialog diag( app->applicationName(), path() + "/" + file );
		diag.setProperty( "User-Agent", QString( "%1/%2 (%3)" )
			.arg( app->applicationName(), app->applicationVersion(), app->applicationOs() ).toUtf8() );
		diag.exec();
	}
}

QBreakPadDialog::QBreakPadDialog( const QString &name, const QString &path )
	: QWizard()
	, edit(0)
	, stack(0)
	, progress(0)
{
	if( path.isEmpty() )
	{
		QStringList args = qApp->arguments();
		int pos = args.indexOf( "-crashreport" );
		if( pos == -1 )
			return;
		file = QString( "%1/%2.dmp" ).arg( args[pos + 1], id = args[pos + 2] );
	}
	else
	{
		file = path;
		id = QFileInfo( path ).baseName();
	}

	QTranslator *qt = new QTranslator( this );
	QTranslator *bp = new QTranslator( this );
	qt->load( ":/translations/qt_" + Settings::language() );
	bp->load( ":/translations/breakpad_" + Settings::language() );
	qApp->installTranslator( qt );
	qApp->installTranslator( bp );

	qDebug() << "Crashreport application started";
	Q_UNUSED(QT_TR_NOOP("qdigidocclient"));
	Q_UNUSED(QT_TR_NOOP("qesteidutil"));
	setWindowTitle( tr("%1 has stopped working").arg( tr(name.toUtf8()) ) );
	setButtonLayout( QList<WizardButton>()
		<< Stretch << CancelButton << FinishButton );
	setButtonText( FinishButton, tr("Send report") );
	setButtonText( CancelButton, path.isEmpty() ? tr("Close the program") : tr("Close") );

	QWizardPage *crash = new QWizardPage( this );
	crash->setTitle( tr("Error Report") );
	crash->setSubTitle( path.isEmpty() ?
		tr("Please send us this error report to "
			"help fix the problem and improve this software.") :
		tr("We detected that last time this application was used, it stopped unexpectedly. "
			"Do You wish to send a crash report so we can diagnose and fix the problem?") );
	addPage( crash );

	QLabel *privacy = new QLabel( QString( "<a href='%1'>%2</a>" )
		.arg( tr("https://installer.id.ee/Privacy_Policy_ENG.html") )
		.arg( tr("ID software error reporting principles") ) );
	privacy->setOpenExternalLinks( true );
	QLabel *link = new QLabel( QString( "<a href='show'>%1</a>" ).arg(
		tr("Provide additional info about the problem (recommended)") ), crash );
	QLabel *stackLabel = new QLabel( QString( "<a href='show'>%1</a>" )
		.arg( tr("Details") ), crash );
	edit = new QPlainTextEdit( crash );
	stack = new QPlainTextEdit( crash );
	edit->hide();
	stack->hide();
	stack->setReadOnly( true );
	connect( link, SIGNAL(linkActivated(QString)), SLOT(toggleComments()) );
	connect( stackLabel, SIGNAL(linkActivated(QString)), SLOT(toggleStack()) );

	progress = new QProgressBar( crash );
	progress->hide();

	QVBoxLayout *l = new QVBoxLayout( crash );
	l->addWidget( privacy );
	l->addWidget( link );
	l->addWidget( edit, 1 );
	l->addWidget( stackLabel );
	l->addWidget( stack, 1 );
	l->addStretch();
	l->addWidget( progress );
}

QBreakPadDialog::~QBreakPadDialog()
{
	QFile::remove( file );
}

QString QBreakPadDialog::parseStack() const
{
	QString st;
	QTextStream s( &st );
	s << "File: " << file << endl << endl;

	BasicSourceLineResolver resolver;
	MinidumpProcessor minidump_processor(0, &resolver);

	ProcessState process_state;
	if( minidump_processor.Process(file.toLocal8Bit().constData(), &process_state) != PROCESS_OK )
		return st;

	// Print OS and CPU information.
	s << "Operating system: " << process_state.system_info()->os.c_str() << " "
	  << process_state.system_info()->os_version.c_str() << endl
	  << "CPU: " << process_state.system_info()->cpu.c_str();
	std::string cpu_info = process_state.system_info()->cpu_info;
	if( !cpu_info.empty() )
		s << " " << cpu_info.c_str();
	s << " CPU " << process_state.system_info()->cpu_count << (process_state.system_info()->cpu_count != 1 ? "s" : "") << endl << endl;

	// Print crash information.
	if( process_state.crashed() )
	{
		s << "Crash reason:  " << process_state.crash_reason().c_str() << endl
		  << "Crash address: 0x" << hex << process_state.crash_address() << endl;
	}
	else
		s << "No crash" << endl;

	std::string assertion = process_state.assertion();
	if( !assertion.empty() )
		s << "Assertion: " << assertion.c_str() << endl;

	// If the thread that requested the dump is known, print it first.
	int requesting_thread = process_state.requesting_thread();
	if( requesting_thread != -1 )
	{
		s << endl << "Thread " << requesting_thread << " ("
		  << (process_state.crashed() ? "crashed" : "requested dump, did not crash") << ")" << endl;
		printStack( process_state.threads()->at(requesting_thread), s );
	}

	// Print all of the threads in the dump.
	size_t thread_count = process_state.threads()->size();
	for( size_t thread_index = 0; thread_index < thread_count; ++thread_index )
	{
		if( thread_index == size_t(requesting_thread) )
			continue;

		s << endl << "Thread " << dec << thread_index << endl;
		printStack( process_state.threads()->at(thread_index), s );
	}

	if( process_state.modules() )
	{
		s << endl << "Loaded modules:" << endl;

		uint64_t main_address = 0;
		const CodeModule *main_module = process_state.modules()->GetMainModule();
		if (main_module)
			main_address = main_module->base_address();

		unsigned int module_count = process_state.modules()->module_count();
		for (unsigned int module_sequence = 0; module_sequence < module_count; ++module_sequence)
		{
			const CodeModule *module = process_state.modules()->GetModuleAtSequence(module_sequence);
			uint64_t base_address = module->base_address();
			s << "0x" << hex << base_address << " - 0x" << hex << base_address + module->size() - 1 << "  "
			  << QFileInfo( module->code_file().c_str() ).fileName() << " "
			  << (module->version().empty() ? "???" : module->version().c_str())
			  << (main_module != NULL && base_address == main_address ? "  (main)" : "") << endl;
		}
	}

	return st;
}

void QBreakPadDialog::printStack( const CallStack *stack, QTextStream &s ) const
{
	size_t frame_count = stack->frames()->size();
	for( size_t frame_index = 0; frame_index < frame_count; ++frame_index )
	{
		s << dec << frame_index << " ";
		const StackFrame *frame = stack->frames()->at(frame_index);
		if( frame->module )
		{
			s << QFileInfo( frame->module->code_file().c_str() ).fileName();
			if( !frame->function_name.empty() )
				s << "!" << frame->function_name.c_str()
				  << " + 0x" << hex << frame->instruction - frame->function_base;
			else
				s << " + 0x" << hex << frame->instruction - frame->module->base_address();
		}
		else
			s << "0x" << hex << frame->instruction;

		s << endl << "    Found by: " << frame->trust_description().c_str() << endl;
	}
}

void QBreakPadDialog::toggleComments()
{
	edit->setVisible( !edit->isVisible() );
}

void QBreakPadDialog::toggleStack()
{
	if( stack->toPlainText().isEmpty() )
		stack->setPlainText( parseStack() );
	stack->setVisible( !stack->isVisible() );
}

void QBreakPadDialog::updateProgress( qint64 value, qint64 range )
{
	progress->setMaximum( range );
	progress->setValue( value );
}

bool QBreakPadDialog::validateCurrentPage()
{
	QFile f( file );
	if( !f.open( QFile::ReadOnly ) )
		return true;

	QHttpPart textPart;
	textPart.setHeader( QNetworkRequest::ContentDispositionHeader, "form-data; name=\"comment\"" );
	textPart.setBody( edit->toPlainText().toUtf8() );

	QHttpPart dumpPart;
	dumpPart.setHeader( QNetworkRequest::ContentDispositionHeader, "form-data; name=\"dump\"; filename=\"dump\"" );
	dumpPart.setHeader( QNetworkRequest::ContentTypeHeader, "application/octet-stream" );
	dumpPart.setBodyDevice( &f );

	QHttpMultiPart multiPart( QHttpMultiPart::FormDataType );
	multiPart.append( textPart );
	multiPart.append( dumpPart );

	QNetworkRequest req( QUrl( BREAKPAD ) );
	req.setRawHeader( "User-Agent", property("User-Agent").toByteArray() );
	req.setRawHeader( "X-Dump-ID", id.toUtf8() );

	qDebug() << "uploading crash dump file" << f.fileName();
	QNetworkAccessManager manager;
	QEventLoop e;
	QNetworkReply *repl = manager.post( req, &multiPart );
	progress->show();
	connect( repl, SIGNAL(uploadProgress(qint64,qint64)), SLOT(updateProgress(qint64,qint64)) );
	connect( repl, SIGNAL(finished()), &e, SLOT(quit()) );
	e.exec();
	progress->hide();

	if( repl->error() == QNetworkReply::NoError )
	{
#ifdef TESTING
		QByteArray result = repl->readAll();
		if( !result.isEmpty() )
			QMessageBox::warning( this, tr("Error Report"), result );
#endif
		return true;
	}
	else
	{
		QMessageBox::warning( this, tr("Error Report"), repl->errorString() );
		return false;
	}
}
