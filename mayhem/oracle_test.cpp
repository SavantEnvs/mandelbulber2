// Behavioural oracle: loads Mandelbulber's shipped example settings through the real parser and
// asserts known parameter values, a known fractal distance estimate, a text round trip and the
// v1.21 importer. Prints one "PASS|FAIL <name>" line per check; exit status is the failure count.
#include <cmath>
#include <cstdio>
#include <memory>

#include <QString>

#include "harness_init.hpp"
#include "src/animation_frames.hpp"
#include "src/calculate_distance.hpp"
#include "src/fractal_container.hpp"
#include "src/fractparams.hpp"
#include "src/hybrid_fractal_sequences.h"
#include "src/keyframes.hpp"
#include "src/objects_tree.h"
#include "src/old_settings.hpp"
#include "src/parameters.hpp"
#include "src/render_data.hpp"
#include "src/settings.hpp"
#include "src/system_directories.hpp"

namespace
{
int failures = 0;

void Check(const char *name, bool ok, double actual = 0.0)
{
	if (ok)
		std::printf("PASS %s\n", name);
	else
	{
		std::printf("FAIL %s (actual %.12g)\n", name, actual);
		failures++;
	}
}

bool Near(double a, double b, double tol) { return std::fabs(a - b) <= tol * (1.0 + std::fabs(b)); }

double DistanceAt(CVector3 point, std::shared_ptr<cParameterContainer> par,
	std::shared_ptr<cFractalContainer> fract)
{
	std::shared_ptr<sParamRender> params(new sParamRender(par));
	cObjectsTree objectsTree;
	objectsTree.CreateNodeDataFromParameters(par);
	std::vector<cObjectsTree::sNodeDataForRendering> nodes = objectsTree.GetNodeDataListForRendering();
	cHybridFractalSequences fractals;
	fractals.CreateSequences(par, fract, nodes);
	sDistanceIn in(point, 0, false);
	sDistanceOut out;
	std::shared_ptr<sRenderData> renderData(new sRenderData());
	renderData->nodesDataForRendering = nodes;
	renderData->hybridFractalSequences = fractals;
	std::vector<ObjectTreeStackFrame> stack(GetObjectTreeRequiredStackDepth(nodes));
	return CalculateDistance(*params, fractals, in, &out, renderData.get(), &stack);
}

bool LoadExample(const QString &name, std::shared_ptr<cParameterContainer> &par,
	std::shared_ptr<cFractalContainer> &fract)
{
	MbMakeContainers(par, fract);
	cSettings settings(cSettings::formatFullText);
	settings.BeQuiet(true);
	if (!settings.LoadFromFile(systemDirectories.sharedDir + "examples/" + name)) return false;
	return settings.Decode(par, fract);
}
} // namespace

int main()
{
	MbHarnessInit();

	std::shared_ptr<cParameterContainer> par;
	std::shared_ptr<cFractalContainer> fract;
	Check("load mandelbulb001.fract", LoadExample("mandelbulb001.fract", par, fract));

	Check("image_width", par->Get<int>("image_width") == 1920, par->Get<int>("image_width"));
	Check("image_height", par->Get<int>("image_height") == 1080, par->Get<int>("image_height"));
	Check("DE_factor decoded from comma decimal", Near(par->Get<double>("DE_factor"), 0.645654, 1e-9),
		par->Get<double>("DE_factor"));
	Check("fov", Near(par->Get<double>("fov"), 57.62, 1e-9), par->Get<double>("fov"));
	CVector3 camera = par->Get<CVector3>("camera");
	Check("camera.x", Near(camera.x, -0.1588900287502842, 1e-9), camera.x);
	Check("camera.y", Near(camera.y, -1.18529433988786, 1e-9), camera.y);
	Check("camera.z", Near(camera.z, 0.1644950620698425, 1e-9), camera.z);
	Check("iteration_fog_enable", par->Get<bool>("iteration_fog_enable"));

	// distance estimate of the loaded fractal at fixed points (regression values from this build)
	double dInside = DistanceAt(CVector3(0.3, 0.2, 0.1), par, fract);
	double dCamera = DistanceAt(camera, par, fract);
	double dFar = DistanceAt(CVector3(3.0, 0.0, 0.0), par, fract);
	Check("distance at (0.3,0.2,0.1)", Near(dInside, 0.0, 1e-6), dInside);
	Check("distance at camera", Near(dCamera, 0.126112911974178, 1e-6), dCamera);
	Check("distance at (3,0,0)", Near(dFar, 1.64816709450547, 1e-6), dFar);
	Check("distance grows with range", dFar > dCamera && dCamera > dInside, dFar);

	// text round trip: encode, decode into fresh containers, same values
	cSettings writer(cSettings::formatFullText);
	writer.CreateText(par, fract);
	QString text = writer.GetSettingsText();
	std::shared_ptr<cParameterContainer> par2;
	std::shared_ptr<cFractalContainer> fract2;
	MbMakeContainers(par2, fract2);
	cSettings reader(cSettings::formatFullText);
	reader.BeQuiet(true);
	Check("round trip load", reader.LoadFromString(text) && reader.Decode(par2, fract2));
	Check("round trip DE_factor", Near(par2->Get<double>("DE_factor"), 0.645654, 1e-9),
		par2->Get<double>("DE_factor"));
	CVector3 camera2 = par2->Get<CVector3>("camera");
	Check("round trip camera", Near(camera2.x, camera.x, 1e-9) && Near(camera2.z, camera.z, 1e-9),
		camera2.x);
	Check("round trip distance", Near(DistanceAt(camera2, par2, fract2), dCamera, 1e-6));

	// garbage is rejected without crashing
	std::shared_ptr<cParameterContainer> par3;
	std::shared_ptr<cFractalContainer> fract3;
	MbMakeContainers(par3, fract3);
	cSettings junk(cSettings::formatFullText);
	junk.BeQuiet(true);
	junk.LoadFromString("this is not a settings file\n[main_parameters]\nimage_width notanumber;\n");
	junk.Decode(par3, fract3);
	Check("junk keeps default image_width", par3->Get<int>("image_width") == 800,
		par3->Get<int>("image_width"));

	// v1.21 importer
	oldSettings::cOldSettings oldSettings;
	std::shared_ptr<cParameterContainer> par4;
	std::shared_ptr<cFractalContainer> fract4;
	MbMakeContainers(par4, fract4);
	bool loaded = oldSettings.LoadSettings(systemDirectories.sharedDir + "data/mandelbulber_1.21_defaults.fract");
	Check("old settings load", loaded);
	if (loaded) oldSettings.ConvertToNewContainer(par4, fract4);
	Check("old settings image_width", par4->Get<int>("image_width") == 800, par4->Get<int>("image_width"));
	Check("old settings image_height", par4->Get<int>("image_height") == 600, par4->Get<int>("image_height"));
	Check("old settings N", par4->Get<int>("N") == 250, par4->Get<int>("N"));

	std::printf("%d failure(s)\n", failures);
	return failures;
}
