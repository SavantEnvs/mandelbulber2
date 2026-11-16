// Fuzzes cSettings, the parser of Mandelbulber's .fract settings and animation files:
// header, [main_parameters], [fractal_N], [frames] and [keyframes] sections, plus the
// compatibility migrations for older file versions.
#include <cstddef>
#include <cstdint>
#include <memory>

#include <QString>

#include "harness_init.hpp"
#include "src/animation_frames.hpp"
#include "src/fractal_container.hpp"
#include "src/keyframes.hpp"
#include "src/parameters.hpp"
#include "src/settings.hpp"

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	MbHarnessInit();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	std::shared_ptr<cParameterContainer> par;
	std::shared_ptr<cFractalContainer> fract;
	MbMakeContainers(par, fract);
	std::shared_ptr<cAnimationFrames> frames(new cAnimationFrames);
	std::shared_ptr<cKeyframes> keyframes(new cKeyframes);

	cSettings settings(cSettings::formatFullText);
	settings.BeQuiet(true);
	settings.LoadFromString(QString::fromUtf8(reinterpret_cast<const char *>(data), qsizetype(size)));
	settings.Decode(par, fract, frames, keyframes);
	return 0;
}
