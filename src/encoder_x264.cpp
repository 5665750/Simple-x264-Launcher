///////////////////////////////////////////////////////////////////////////////
// Simple x264 Launcher
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "encoder_x264.h"

#include "model_options.h"
#include "model_status.h"
#include "binaries.h"

#include <QStringList>
#include <QDir>
#include <QRegExp>

//x264 version info
static const unsigned int X264_VERSION_X264_MINIMUM_REV = 2380;
static const unsigned int X264_VERSION_X264_CURRENT_API = 142;

// ------------------------------------------------------------
// Helper Macros
// ------------------------------------------------------------

#define REMOVE_CUSTOM_ARG(LIST, ITER, FLAG, PARAM) do \
{ \
	if(ITER != LIST.end()) \
	{ \
		if((*ITER).compare(PARAM, Qt::CaseInsensitive) == 0) \
		{ \
			log(tr("WARNING: Custom parameter \"" PARAM "\" will be ignored in Pipe'd mode!\n")); \
			ITER = LIST.erase(ITER); \
			if(ITER != LIST.end()) \
			{ \
				if(!((*ITER).startsWith("--", Qt::CaseInsensitive))) ITER = LIST.erase(ITER); \
			} \
			FLAG = true; \
		} \
	} \
} \
while(0)

#define X264_UPDATE_PROGRESS(X) do \
{ \
	bool ok = false; qint64 size_estimate = 0; \
	unsigned int progress = (X)->cap(1).toUInt(&ok); \
	setStatus((pass == 2) ? JobStatus_Running_Pass2 : ((pass == 1) ? JobStatus_Running_Pass1 : JobStatus_Running)); \
	if(ok) \
	{ \
		setProgress(progress); \
		size_estimate = estimateSize(m_outputFile, progress); \
	} \
	setDetails(tr("%1, est. file size %2").arg(line.mid(offset).trimmed(), sizeToString(size_estimate))); \
} \
while(0)

// ------------------------------------------------------------
// Constructor & Destructor
// ------------------------------------------------------------

X264Encoder::X264Encoder(JobObject *jobObject, const OptionsModel *options, const SysinfoModel *const sysinfo, const PreferencesModel *const preferences, JobStatus &jobStatus, volatile bool *abort, volatile bool *pause, QSemaphore *semaphorePause, const QString &sourceFile, const QString &outputFile)
:
	AbstractEncoder(jobObject, options, sysinfo, preferences, jobStatus, abort, pause, semaphorePause, sourceFile, outputFile),
	m_binaryFile(ENC_BINARY(sysinfo, options))
{
	if(options->encType() != OptionsModel::EncType_X264)
	{
		throw "Invalid encoder type!";
	}
}

X264Encoder::~X264Encoder(void)
{
	/*Nothing to do here*/
}

// ------------------------------------------------------------
// Check Version
// ------------------------------------------------------------

void X264Encoder::checkVersion_init(QList<QRegExp*> &patterns, QStringList &cmdLine)
{
	cmdLine << "--version";
	patterns << new QRegExp("\\bx264\\s(\\d)\\.(\\d+)\\.(\\d+)\\s([a-f0-9]{7})", Qt::CaseInsensitive);
	patterns << new QRegExp("\\bx264 (\\d)\\.(\\d+)\\.(\\d+)", Qt::CaseInsensitive);
}

void X264Encoder::checkVersion_parseLine(const QString &line, QList<QRegExp*> &patterns, unsigned int &coreVers, unsigned int &revision, bool &modified)
{
	int offset = -1;

	if((offset = patterns[0]->lastIndexIn(line)) >= 0)
	{
		bool ok1 = false, ok2 = false;
		unsigned int temp1 = patterns[0]->cap(2).toUInt(&ok1);
		unsigned int temp2 = patterns[0]->cap(3).toUInt(&ok2);
		if(ok1 && ok2)
		{
			coreVers = temp1;
			revision = temp2;
		}
	}
	else if((offset = patterns[1]->lastIndexIn(line)) >= 0)
	{
		bool ok1 = false, ok2 = false;
		unsigned int temp1 = patterns[1]->cap(2).toUInt(&ok1);
		unsigned int temp2 = patterns[1]->cap(3).toUInt(&ok2);
		if(ok1 && ok2)
		{
			coreVers = temp1;
			revision = temp2;
		}
		modified = true;
	}

	if(!line.isEmpty())
	{
		log(line);
	}
}

void X264Encoder::printVersion(const unsigned int &revision, const bool &modified)
{
	log(tr("\nx264 revision: %1 (core #%2)\n").arg(QString::number(revision % REV_MULT), QString::number(revision / REV_MULT)).append(modified ? tr(" - with custom patches!") : QString()));
}

bool X264Encoder::isVersionSupported(const unsigned int &revision, const bool &modified)
{
	if((revision % REV_MULT) < X264_VERSION_X264_MINIMUM_REV)
	{
		log(tr("\nERROR: Your revision of x264 is too old! (Minimum required revision is %1)").arg(QString::number(X264_VERSION_X264_MINIMUM_REV)));
		return false;
	}
	
	if((revision / REV_MULT) != X264_VERSION_X264_CURRENT_API)
	{
		log(tr("\nWARNING: Your revision of x264 uses an unsupported core (API) version, take care!"));
		log(tr("This application works best with x264 core (API) version %2.").arg(QString::number(X264_VERSION_X264_CURRENT_API)));
	}

	return true;
}

// ------------------------------------------------------------
// Encoding Functions
// ------------------------------------------------------------

void X264Encoder::buildCommandLine(QStringList &cmdLine, const bool &usePipe, const unsigned int &frames, const QString &indexFile, const int &pass, const QString &passLogFile)
{
	double crf_int = 0.0, crf_frc = 0.0;

	switch(m_options->rcMode())
	{
	case OptionsModel::RCMode_CQ:
		cmdLine << "--qp" << QString::number(qRound(m_options->quantizer()));
		break;
	case OptionsModel::RCMode_CRF:
		crf_frc = modf(m_options->quantizer(), &crf_int);
		cmdLine << "--crf" << QString("%1.%2").arg(QString::number(qRound(crf_int)), QString::number(qRound(crf_frc * 10.0)));
		break;
	case OptionsModel::RCMode_2Pass:
	case OptionsModel::RCMode_ABR:
		cmdLine << "--bitrate" << QString::number(m_options->bitrate());
		break;
	default:
		throw "Bad rate-control mode !!!";
		break;
	}
	
	if((pass == 1) || (pass == 2))
	{
		cmdLine << "--pass" << QString::number(pass);
		cmdLine << "--stats" << QDir::toNativeSeparators(passLogFile);
	}

	cmdLine << "--preset" << m_options->preset().toLower();

	if(m_options->tune().compare("none", Qt::CaseInsensitive))
	{
		cmdLine << "--tune" << m_options->tune().toLower();
	}

	if(m_options->profile().compare("auto", Qt::CaseInsensitive) != 0)
	{
		if((m_options->encType() == OptionsModel::EncType_X264) && (m_options->encVariant() == OptionsModel::EncVariant_LoBit))
		{
			cmdLine << "--profile" << m_options->profile().toLower();
		}
	}

	if(!m_options->customEncParams().isEmpty())
	{
		QStringList customArgs = splitParams(m_options->customEncParams(), m_sourceFile, m_outputFile);
		if(usePipe)
		{
			QStringList::iterator i = customArgs.begin();
			while(i != customArgs.end())
			{
				bool bModified = false;
				REMOVE_CUSTOM_ARG(customArgs, i, bModified, "--fps");
				REMOVE_CUSTOM_ARG(customArgs, i, bModified, "--frames");
				if(!bModified) i++;
			}
		}
		cmdLine.append(customArgs);
	}

	cmdLine << "--output" << QDir::toNativeSeparators(m_outputFile);
	
	if(usePipe)
	{
		if(frames < 1) throw "Frames not set!";
		cmdLine << "--frames" << QString::number(frames);
		cmdLine << "--demuxer" << "y4m";
		cmdLine << "--stdin" << "y4m" << "-";
	}
	else
	{
		cmdLine << "--index" << QDir::toNativeSeparators(indexFile);
		cmdLine << QDir::toNativeSeparators(m_sourceFile);
	}
}

void X264Encoder::runEncodingPass_init(QList<QRegExp*> &patterns)
{
	patterns << new QRegExp("\\[(\\d+)\\.(\\d+)%\\].+frames");
	patterns << new QRegExp("indexing.+\\[(\\d+)\\.(\\d+)%\\]");
	patterns << new QRegExp("^(\\d+) frames:");
	patterns << new QRegExp("\\[\\s*(\\d+)\\.(\\d+)%\\]\\s+(\\d+)/(\\d+)\\s(\\d+).(\\d+)\\s(\\d+).(\\d+)\\s+(\\d+):(\\d+):(\\d+)\\s+(\\d+):(\\d+):(\\d+)"); //regExpModified
}

void X264Encoder::runEncodingPass_parseLine(const QString &line, QList<QRegExp*> &patterns, const int &pass)
{
	int offset = -1;
	if((offset = patterns[0]->lastIndexIn(line)) >= 0)
	{
		X264_UPDATE_PROGRESS(patterns[0]);
	}
	else if((offset = patterns[1]->lastIndexIn(line)) >= 0)
	{
		bool ok = false;
		unsigned int progress = patterns[1]->cap(1).toUInt(&ok);
		setStatus(JobStatus_Indexing);
		if(ok)
		{
			setProgress(progress);
		}
		setDetails(line.mid(offset).trimmed());
	}
	else if((offset = patterns[2]->lastIndexIn(line)) >= 0)
	{
		setStatus((pass == 2) ? JobStatus_Running_Pass2 : ((pass == 1) ? JobStatus_Running_Pass1 : JobStatus_Running));
		setDetails(line.mid(offset).trimmed());
	}
	else if((offset = patterns[3]->lastIndexIn(line)) >= 0)
	{
		X264_UPDATE_PROGRESS(patterns[3]);
	}
	else if(!line.isEmpty())
	{
		log(line);
	}
}
