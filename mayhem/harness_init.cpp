#include "harness_init.hpp"

#include <QCoreApplication>
#include <QLocale>
#include <QtGlobal>

#include "formula/definition/all_fractal_list.hpp"
#include "src/fractal_container.hpp"
#include "src/initparameters.hpp"
#include "src/parameters.hpp"
#include "src/system.hpp"
#include "src/system_data.hpp"
#include "src/system_directories.hpp"

namespace
{
int gArgc = 1;
char gArg0[] = "mandelbulber2";
char *gArgv[] = {gArg0, nullptr};
bool gInitialised = false;

void quietHandler(QtMsgType, const QMessageLogContext &, const QString &) {}
} // namespace

void MbHarnessInit()
{
	if (gInitialised) return;
	gInitialised = true;

	static QCoreApplication app(gArgc, gArgv);
	qInstallMessageHandler(quietHandler);

	systemData.locale = QLocale::c();
	systemData.locale.setNumberOptions(QLocale::OmitGroupSeparator);
	QLocale::setDefault(systemData.locale);
	systemData.decimalPoint = ".";
	systemData.numberOfThreads = 1;
	systemData.noGui = true;
	systemData.silent = true;
	systemData.loggingVerbosity = 0;
	systemData.terminalWidth = 80;
	systemData.useColor = false;
	systemData.enableConsoleOutput = false;
	systemData.globalTimer.start();

	systemDirectories.sharedDir = QString(SHARED_DIR) + "/";
	systemDirectories.docDir = QString(SHARED_DOC_DIR) + "/";
	systemDirectories.homeDir = "/tmp/";
	systemDirectories.SetDataDirectoryHidden("/tmp/.mandelbulber/");
	systemDirectories.SetDataDirectoryPublic("/tmp/mandelbulber/");

	DefineFractalList(&newFractalList);
}

void MbMakeContainers(std::shared_ptr<cParameterContainer> &par, std::shared_ptr<cFractalContainer> &fract)
{
	// Defaults are built once and never modified; each call hands out value copies (QMap is
	// copy-on-write), so no input can see another's edits.
	static std::shared_ptr<cParameterContainer> defaultPar;
	static std::shared_ptr<cFractalContainer> defaultFract;
	if (!defaultPar)
	{
		defaultPar.reset(new cParameterContainer);
		defaultFract.reset(new cFractalContainer);
		defaultPar->SetContainerName("main");
		InitParams(defaultPar);
		InitNodeParams(1, defaultPar);
		for (int i = 0; i < defaultFract->size(); i++)
		{
			defaultFract->at(i)->SetContainerName(QString("fractal") + QString::number(i));
			InitFractalParams(defaultFract->at(i));
		}
	}
	par.reset(new cParameterContainer(*defaultPar));
	fract.reset(new cFractalContainer(*defaultFract));

	// upstream's main() sets these before any settings are decoded; compatibility migrations
	// (cSettings::Compatibility3 -> RemoveAnimatedParameter) fall back to gPar
	gPar = par;
	gParFractal = fract;
}
