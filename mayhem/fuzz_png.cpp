// Fuzzes LoadPNG(), the 16-bit PNG loader behind Mandelbulb textures and displacement maps.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <QString>

#include "harness_init.hpp"
#include "src/files.h"

static std::string gInputPath;

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	MbHarnessInit();
	const char *tmp = getenv("TMPDIR");
	std::string tmpl = std::string(tmp && *tmp ? tmp : "/tmp") + "/mb_png_XXXXXX";
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

	int width = 0, height = 0;
	LoadPNG(QString::fromLocal8Bit(gInputPath.c_str()), width, height);
	return 0;
}
