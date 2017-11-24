#include "mscHack.hpp"
#include "mscHack_Controls.hpp"

Plugin *plugin;

void init(rack::Plugin *p) 
{
	plugin = p;
	plugin->slug = "mscHack";
	plugin->website = "https://github.com/mschack/VCV-Rack-Plugins";

    p->addModel( createModel<MasterClockx4_Widget>   ( "mscHack", "MasterClockx4", "Master CLOCK x 4", CLOCK_TAG, QUAD_TAG ) );
    p->addModel( createModel<Seq_3x16x16_Widget>     ( "mscHack", "Seq_3ch_16step", "SEQ 3 x 16", SEQUENCER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<SEQ_6x32x16_Widget>     ( "mscHack", "Seq_6ch_32step", "SEQ 6 x 32", SEQUENCER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<Seq_Triad_Widget>       ( "mscHack", "TriadSeq", "SEQ Triad", SEQUENCER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<Seq_Triad2_Widget>      ( "mscHack", "TriadSeq2", "SEQ Triad 2", SEQUENCER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<SynthDrums_Widget>      ( "mscHack", "SynthDrums", "SYNTH Drums", DRUM_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<XFade_Widget>           ( "mscHack", "XFade", "MIXER Cross Fader 3 Channel", MIXER_TAG, MULTIPLE_TAG) );
    p->addModel( createModel<Mix_1x4_Stereo_Widget>  ( "mscHack", "Mix_1x4_Stereo", "MIXER 1x4 Stereo/Mono", MIXER_TAG, EQUALIZER_TAG, PANNING_TAG, AMPLIFIER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<Mix_2x4_Stereo_Widget>  ( "mscHack", "Mix_2x4_Stereo", "MIXER 2x4 Stereo/Mono", MIXER_TAG, EQUALIZER_TAG, DUAL_TAG, PANNING_TAG, AMPLIFIER_TAG, MULTIPLE_TAG ) );
    //p->addModel( createModel<Mix_2x4_Stereo_Chained_Widget>  (plugin, "Mix_2x4_Stereo_Chained", "MIXER Chained 2x4 Stereo/Mono") );
    p->addModel( createModel<Mix_4x4_Stereo_Widget> ( "mscHack", "Mix_4x4_Stereo(2)", "MIXER 4x4 Stereo/Mono", MIXER_TAG, EQUALIZER_TAG, QUAD_TAG, PANNING_TAG, AMPLIFIER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<Mix_4x4_Stereo_Widget_old>  ( "mscHack", "Mix_4x4_Stereo", "MIXER 4x4 (old)", MIXER_TAG, QUAD_TAG, PANNING_TAG, AMPLIFIER_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<PingPong_Widget>        ( "mscHack", "PingPong_Widget", "DELAY Ping Pong", DELAY_TAG, PANNING_TAG ) );
    p->addModel( createModel<Osc_3Ch_Widget>         ( "mscHack", "Osc_3Ch_Widget", "OSC 3 Channel", SYNTH_VOICE_TAG, MULTIPLE_TAG ) );
    p->addModel( createModel<Compressor_Widget>      ( "mscHack", "Compressor1", "COMP Basic Compressor", DYNAMICS_TAG ) );
}



