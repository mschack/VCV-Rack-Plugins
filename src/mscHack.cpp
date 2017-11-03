#include "mscHack.hpp"

Plugin *plugin;

void init(rack::Plugin *p) 
{
	plugin = p;
	plugin->slug = "mscHack";
	plugin->name = "mscHack";
	plugin->homepageUrl = "";

    createModel<Seq_3x16x16_Widget>(plugin, "Seq_3ch_16step", "SEQ 3 x 16 Programmable");
    createModel<Mix_4x4_Stereo_Widget>(plugin, "Mix_4x4_Stereo", "MIXER 4x4 Stereo");
    createModel<Mix_4x4_Stereo2_Widget>(plugin, "Mix_4x4_Stereo(2)", "MIXER 4x4 Stereo 2 with EQ");
    createModel<Seq_Triad_Widget>(plugin, "TriadSeq", "Triad Sequencer Programmable");
}
