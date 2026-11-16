// Fuzzes the importer for settings files of Mandelbulber v1.21 (cOldSettings): the
// "name value;" line reader and the conversion into the current parameter containers.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

#include <QString>

#include "harness_init.hpp"
#include "src/fractal_container.hpp"
#include "src/old_settings.hpp"
#include "src/parameters.hpp"

static std::string gInputPath;

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	MbHarnessInit();
	const char *tmp = getenv("TMPDIR");
	std::string tmpl = std::string(tmp && *tmp ? tmp : "/tmp") + "/mb_old_settings_XXXXXX";
	int fd = mkstemp(&tmpl[0]);
	if (fd < 0) abort();
	close(fd);
	gInputPath = tmpl;
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	FILE *f = fopen(gInputPath.c_str(), "wb");
	if (!f) abort();
	fwrite(data, 1, size, f);
	fclose(f);

	std::shared_ptr<cParameterContainer> par;
	std::shared_ptr<cFractalContainer> fract;
	MbMakeContainers(par, fract);

	oldSettings::cOldSettings oldSettings;
	if (oldSettings.LoadSettings(QString::fromLocal8Bit(gInputPath.c_str())))
		oldSettings.ConvertToNewContainer(par, fract);
	return 0;
}
