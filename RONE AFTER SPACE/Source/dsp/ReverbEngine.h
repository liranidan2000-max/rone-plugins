#pragma once

// The AFTERSPACE late-field reverb is the bundle's shared FDN (Shared/RoneReverbEngine,
// also used by RONE Rise). This file kept a copy that was byte-identical apart from the
// class name; the name stays so the processor and the DSP tests read as before.
#include "../../../Shared/RoneReverbEngine.h"

using ReverbEngine = RoneReverbEngine;
