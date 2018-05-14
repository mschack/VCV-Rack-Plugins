#include "rack.hpp"

using namespace rack;
extern Plugin *plugin;

#include "CLog.h"
#include "mscHack_Controls.hpp"

#define CV_MAX (10.0f)
#define AUDIO_MAX (6.0f)
#define VOCT_MAX (6.0f)
#define AMP_MAX (2.0f)

#define TOJSON true
#define FROMJSON false

void JsonDataInt( bool bTo, std::string strName, json_t *root, int *pdata, int len );
void JsonDataBool( bool bTo, std::string strName, json_t *root, bool *pdata, int len );

////////////////////
// module widgets
////////////////////

extern Model *modelMasterClockx4;
extern Model *modelSeq_3x16x16;
extern Model *modelSEQ_6x32x16;
extern Model *modelSeq_Triad2;
extern Model *modelSEQ_Envelope_8;
extern Model *modelMaude_221;
extern Model *modelARP700;
extern Model *modelSynthDrums;
extern Model *modelXFade;
extern Model *modelMix_1x4_Stereo;
extern Model *modelMix_2x4_Stereo;
extern Model *modelMix_4x4_Stereo;
extern Model *modelMix_24_4_4;
extern Model *modelStepDelay;
extern Model *modelPingPong;
extern Model *modelOsc_3Ch;
extern Model *modelCompressor;

