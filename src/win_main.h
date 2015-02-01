///////////////////////////////////////////////////////////////////////////////
// Simple x264 Launcher
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Internal
#include "global.h"

//Qt
#include <QMainWindow>

class IPC;
class JobListModel;
class OptionsModel;
class SysinfoModel;
class QFile;
class QLibrary;
class PreferencesModel;
class RecentlyUsed;
class InputEventFilter;
class QModelIndex;
class QLabel;
class QSystemTrayIcon;
enum JobStatus;

namespace Ui
{
	class MainWindow;
}

namespace MUtils
{
	namespace CPUFetaures
	{
		struct _stuctName;
		typedef struct _cpu_info_t cpu_info_t;
	}
}

class MainWindow: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(const MUtils::CPUFetaures::cpu_info_t &cpuFeatures);
	~MainWindow(void);

protected:
	virtual void closeEvent(QCloseEvent *e);
	virtual void showEvent(QShowEvent *e);
	virtual void resizeEvent(QResizeEvent *e);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dropEvent(QDropEvent *event);
	virtual bool winEvent(MSG *message, long *result);

private:
	Ui::MainWindow *const ui;

	bool m_initialized;
	QLabel *m_label;
	QTimer *m_fileTimer;

	IPC *const m_ipc;
	QSystemTrayIcon *const m_sysTray;

	InputEventFilter *m_inputFilter_jobList;
	InputEventFilter *m_inputFilter_version;
	InputEventFilter *m_inputFilter_checkUp;

	JobListModel *m_jobList;
	OptionsModel *m_options;
	QStringList *m_pendingFiles;
	QList<QFile*> m_toolsList;
	
	SysinfoModel *m_sysinfo;
	PreferencesModel *m_preferences;
	RecentlyUsed *m_recentlyUsed;
	
	bool createJob(QString &sourceFileName, QString &outputFileName, OptionsModel *options, bool &runImmediately, const bool restart = false, int fileNo = -1, int fileTotal = 0, bool *applyToAll = NULL);
	bool createJobMultiple(const QStringList &filePathIn);

	bool appendJob(const QString &sourceFileName, const QString &outputFileName, OptionsModel *options, const bool runImmediately);
	void updateButtons(JobStatus status);
	void updateTaskbar(JobStatus status, const QIcon &icon);
	unsigned int countPendingJobs(void);
	unsigned int countRunningJobs(void);

	bool parseCommandLineArgs(void);

private slots:
	void addButtonPressed();
	void openActionTriggered();
	void abortButtonPressed(void);
	void browseButtonPressed(void);
	void deleteButtonPressed(void);
	void copyLogToClipboard(bool checked);
	void checkUpdates(void);
	void handlePendingFiles(void);
	void init(void);
	void handleCommand(const int &command, const QStringList &args, const quint32 &flags = 0);
	void jobSelected(const QModelIndex &current, const QModelIndex &previous);
	void jobChangedData(const  QModelIndex &top, const  QModelIndex &bottom);
	void jobLogExtended(const QModelIndex & parent, int start, int end);
	void jobListKeyPressed(const int &tag);
	void launchNextJob();
	void moveButtonPressed(void);
	void pauseButtonPressed(bool checked);
	void restartButtonPressed(void);
	void saveLogFile(const QModelIndex &index);
	void showAbout(void);
	void showPreferences(void);
	void showWebLink(void);
	void shutdownComputer(void);
	void startButtonPressed(void);
	void sysTrayActived(void);
	void updateLabelPos(void);
	void versionLabelMouseClicked(const int &tag);
};
