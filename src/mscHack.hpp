#include "rack.hpp"
#include "CLog.h"

using namespace rack;

extern Plugin *plugin;

#define CV_MAX (15.0f)
#define AUDIO_MAX (6.0f)
#define VOCT_MAX (6.0f)

#define TOJSON true
#define FROMJSON false

void JsonDataInt( bool bTo, std::string strName, json_t *root, int *pdata, int len );
void JsonDataBool( bool bTo, std::string strName, json_t *root, bool *pdata, int len );



extern Model *modelMasterClockx4_Widget;
extern Model *modelSeq_3x16x16_Widget;
extern Model *modelSEQ_6x32x16_Widget;
extern Model *modelSeq_Triad_Widget;
extern Model *modelSeq_Triad2_Widget;
extern Model *modelARP700_Widget;
extern Model *modelSynthDrums_Widget;
extern Model *modelXFade_Widget;
extern Model *modelMix_1x4_Stereo_Widget;
extern Model *modelMix_2x4_Stereo_Widget;
extern Model *modelMix_4x4_Stereo_Widget;
extern Model *modelMix_4x4_Stereo_Widget_old;
extern Model *modelPingPong_Widget;
extern Model *modelOsc_3Ch_Widget;
extern Model *modelCompressor_Widget;

