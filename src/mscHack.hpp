#include "rack.hpp"

using namespace rack;
extern Plugin *thePlugin;

#include "mscHack_Controls.hpp"
#include "mscHack_Knob.hpp"
#include "mscHack_Image.hpp"
#include "mscHack_LineImage.hpp"

#define CV_MAX10 (10.0f)
#define CV_MAXn5 (5.0f)
#define AUDIO_MAX (6.0f)
#define VOCT_MAX (8.0f)
#define AMP_MAX (2.0f)

#define TOJSON true
#define FROMJSON false

void JsonDataInt( bool bTo, std::string strName, json_t *root, int *pdata, int len );
void JsonDataBool( bool bTo, std::string strName, json_t *root, bool *pdata, int len );
void JsonDataString( bool bTo, std::string strName, json_t *root, std::string *strText );
void init_rand( unsigned int seed );
unsigned short srand(void);
float frand(void);
float frand_mm( float fmin, float max );
bool  brand( void );
bool  frand_perc( float perc );

////////////////////
// module widgets
////////////////////

/*

extern Model *modelSeq_3x16x16;

extern Model *modelXFade;


*/

extern Model *modelCompressor;
extern Model *modelSynthDrums;
extern Model *modelSEQ_6x32x16;
extern Model *modelMasterClockx8;
extern Model *modelMasterClockx4;
extern Model *modelSEQ_Envelope_8;
extern Model *modelSeq_Triad2;
extern Model *modelARP700;
extern Model *modelMix_24_4_4;
extern Model *modelMix_16_4_4;
extern Model *modelMix_9_3_4;
extern Model *modelMix_4_0_4;
extern Model *modelASAF8;
extern Model *modelPingPong;
extern Model *modelStepDelay;
extern Model *modelOsc_3Ch;
extern Model *modelDronez;
extern Model *modelMorze;
extern Model *modelWindz;
extern Model *modelLorenz;
extern Model *modelAlienz;
extern Model *modelOSC_WaveMorph_3;
extern Model *modelMaude_221;
