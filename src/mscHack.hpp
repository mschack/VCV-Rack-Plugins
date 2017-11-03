#include "rack.hpp"
#include "CLog.h"

using namespace rack;

extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct Seq_3x16x16_Widget : ModuleWidget {
	Seq_3x16x16_Widget();
};

struct Mix_4x4_Stereo_Widget : ModuleWidget {
	Mix_4x4_Stereo_Widget();
};

struct Seq_Triad_Widget : ModuleWidget {
	Seq_Triad_Widget();
};

/*struct WaveShaper1_Widget : ModuleWidget {
	WaveShaper1_Widget();
};*/

