#pragma once
#include <memory>

class cParameterContainer;
class cFractalContainer;

// One-time process setup shared by the harnesses and the oracle: a headless QCoreApplication,
// the formula list, locale and data directories. Safe to call repeatedly.
void MbHarnessInit();

// Fresh default-initialised parameter containers, as the application builds them at startup.
void MbMakeContainers(std::shared_ptr<cParameterContainer> &par, std::shared_ptr<cFractalContainer> &fract);
