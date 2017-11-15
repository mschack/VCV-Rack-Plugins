#include "mscHack.hpp"
#include "mscHack_Controls.hpp"
#include "dsp/digital.hpp"
#include "CLog.h"

#define nCHANNELS 3
#define CHANNEL_H 90
#define CHANNEL_Y 65
#define CHANNEL_X 10

#define freqMAX 300.0f
#define ADS_MAX_TIME_SECONDS 0.5f

#define WAVE_BUFFER_LEN ( 192000 / 20 ) // (9600) based on quality for 20Hz at max sample rate 192000

typedef struct
{
    int   state;
    int   a, s, r;
    int   acount, scount, rcount;
    float fainc, fsinc, frinc;
    float out;
    bool  bTrig;
}ASR_STRUCT;

typedef struct
{
    int   wavetype;
    

    // wave
    float phase;

    //filter
    float q, f;
    bool  bHighpass;

    float lp1, bp1;
    float hpIn;
    float lpIn;
    float mpIn;

    // ads
    ASR_STRUCT adsr_wave;
    ASR_STRUCT adsr_freq;
    
}OSC_PARAM_STRUCT;

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct SynthDrums : Module 
{
    enum WaveTypes
    {
        WAVE_SIN,
        WAVE_TRI,
        WAVE_SQR,
        WAVE_SAW,
        WAVE_NOISE,
        nWAVEFORMS
    };

	enum ParamIds 
    {
        PARAM_FREQ,
        PARAM_ATT       = PARAM_FREQ + nCHANNELS,
        PARAM_REL       = PARAM_ATT + nCHANNELS,
        PARAM_REZ       = PARAM_REL + nCHANNELS,
        PARAM_WAVES     = PARAM_REZ + nCHANNELS,
        nPARAMS         = PARAM_WAVES + (nWAVEFORMS * nCHANNELS)
    };

	enum InputIds 
    {
        IN_LEVEL,
        IN_TRIG         = IN_LEVEL + nCHANNELS,
        nINPUTS         = IN_TRIG + nCHANNELS,
	};

	enum OutputIds 
    {
        OUTPUT_AUDIO,
        nOUTPUTS        = OUTPUT_AUDIO + nCHANNELS
	};

    enum ADSRSTATES
    {
        ADSR_OFF,
        ADSR_ON,
        ADSR_ATTACK,
        ADSR_SUSTAIN,
        ADSR_RELEASE
    };

    CLog            lg;

    SchmittTrigger   m_SchTrig[ nCHANNELS ];

    OSC_PARAM_STRUCT m_Wave[ nCHANNELS ] = {};

    // waveforms
    float           m_BufferWave[ nWAVEFORMS ][ WAVE_BUFFER_LEN ] = {};
    float           m_fLightWaveSelect[ nCHANNELS ][ nWAVEFORMS ] = {};

    // Contructor
	SynthDrums() : Module(nPARAMS, nINPUTS, nOUTPUTS){}

    //-----------------------------------------------------
    // MyParamKnob
    //-----------------------------------------------------
    struct MyParamFreq : Yellow2_Small
    {
        SynthDrums *mymodule;
        int param;

        void onChange() override 
        {
            mymodule = (SynthDrums*)module;

            if( mymodule )
            {
                param = paramId - SynthDrums::PARAM_FREQ;

                if( mymodule->m_Wave[ param ].wavetype == WAVE_NOISE )
                    mymodule->ChangeFilterCutoff( param, value );
                else
                    mymodule->ChangeFilterCutoff( param, 0.6 );
            }

		    RoundKnob::onChange();
	    }
    };

    //-----------------------------------------------------
    // MyWaveButton
    //-----------------------------------------------------
    struct MyWaveButton : MySquareButton
    {
        int ch, but;
        SynthDrums *mymodule;
        int param;

        void onChange() override 
        {
            mymodule = (SynthDrums*)module;

            if( mymodule && value == 1.0 )
            {
                param = paramId - SynthDrums::PARAM_WAVES;
                ch = param / nWAVEFORMS;
                but = param - (ch * nWAVEFORMS);

                mymodule->m_Wave[ ch ].wavetype = but;
                mymodule->SetWaveLights();
            }

		    MomentarySwitch::onChange();
	    }
    };

    // Overrides 
	void    step() override;
    json_t* toJson() override;
    void    fromJson(json_t *rootJ) override;
    void    initialize() override;
    void    randomize() override;
    //void    reset() override;

    void    SetWaveLights( void );
    void    BuildWaves( void );
    void    ChangeFilterCutoff( int ch, float cutfreq );
    float   Filter( int ch, float in, bool bHighPass );
    float   GetWave( int type, float phase );
    float   ProcessADS( int ch, bool bWave );
    float   GetAudio( int ch );
};

//-----------------------------------------------------
// Procedure:   
//
//-----------------------------------------------------
json_t *SynthDrums::toJson() 
{
    json_t *gatesJ;
	json_t *rootJ = json_object();

    // wavetypes
	gatesJ = json_array();

	for (int i = 0; i < nCHANNELS; i++)
    {
		json_t *gateJ = json_integer( m_Wave[ i ].wavetype );
		json_array_append_new( gatesJ, gateJ );
	}

	json_object_set_new( rootJ, "wavetypes", gatesJ );

	return rootJ;
}

//-----------------------------------------------------
// Procedure:   fromJson
//
//-----------------------------------------------------
void SynthDrums::fromJson(json_t *rootJ) 
{
    int i;
    json_t *StepsJ;

    // wave select
	StepsJ = json_object_get( rootJ, "wavetypes" );

	if (StepsJ) 
    {
		for ( i = 0; i < nCHANNELS; i++)
        {
			json_t *gateJ = json_array_get(StepsJ, i);

			if (gateJ)
				m_Wave[ i ].wavetype = json_integer_value( gateJ );
		}
	}

    SetWaveLights();
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
SynthDrums_Widget::SynthDrums_Widget() 
{
    int ch, x, y, i;
	SynthDrums *module = new SynthDrums();
	setModule(module);
	box.size = Vec( 15*11, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/SynthDrums.svg")));
		addChild(panel);
	}

    //module->lg.Open("SynthDrums.txt");

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365))); 
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    y = CHANNEL_Y;
    x = CHANNEL_X;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        x = CHANNEL_X;

        // inputs
        addInput(createInput<MyPortInSmall>( Vec( x, y ), module, SynthDrums::IN_LEVEL + ch ) );

        y += 43;

        addInput(createInput<MyPortInSmall>( Vec( x, y ), module, SynthDrums::IN_TRIG + ch ) );

        x += 32;
        y += 7;

        for( i = 0; i < SynthDrums::nWAVEFORMS; i++ )
        {
            addParam(createParam<SynthDrums::MyWaveButton>( Vec( x, y ), module, SynthDrums::PARAM_WAVES + ( ch * SynthDrums::nWAVEFORMS ) + i, 0.0, 1.0, 0.0 ) );
            addChild(createValueLight<SmallLight<YellowValueLight>>( Vec( x + 1, y + 2 ), &module->m_fLightWaveSelect[ ch ][ i ] ) );
            x += 16;
        }

        x = CHANNEL_X + 25;
        y -= 34;

        // params
        addParam(createParam<SynthDrums::MyParamFreq>( Vec( x, y ), module, SynthDrums::PARAM_FREQ + ch, 0.0, 1.0, 0.0 ) );

        x += 34;

        addParam(createParam<Yellow2_Small>( Vec( x, y ), module, SynthDrums::PARAM_ATT + ch, 0.0, 0.1, 0.0 ) );

        x += 34;

        addParam(createParam<Yellow2_Small>( Vec( x, y ), module, SynthDrums::PARAM_REL + ch, 0.0, 1.0, 0.0 ) );

        y -= 15;
        x += 34;

        // outputs
        addOutput(createOutput<MyPortOutSmall>( Vec( x, y ), module, SynthDrums::OUTPUT_AUDIO + ch ) );
        
        y += CHANNEL_H;
    }

    module->BuildWaves();

    module->ChangeFilterCutoff( 0, 0.6 );
    module->ChangeFilterCutoff( 1, 0.6 );
    module->ChangeFilterCutoff( 2, 0.6 );

    module->SetWaveLights();
}

//-----------------------------------------------------
// Procedure:   initialize
//
//-----------------------------------------------------
void SynthDrums::initialize()
{
}

//-----------------------------------------------------
// Procedure:   randomize
//
//-----------------------------------------------------
void SynthDrums::randomize()
{
    int ch;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        m_Wave[ ch ].wavetype = (int)( randomf() * (nWAVEFORMS-1) );
    }

    SetWaveLights();
}

//-----------------------------------------------------
// Procedure:   SetWaveLights
//
//-----------------------------------------------------
void SynthDrums::SetWaveLights( void )
{
    int ch;

    memset( m_fLightWaveSelect, 0, sizeof(m_fLightWaveSelect) );

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        m_fLightWaveSelect[ ch ][ m_Wave[ ch ].wavetype ] = 1.0;
    }
}

//-----------------------------------------------------
// Procedure:   initialize
//
//-----------------------------------------------------
#define DEG2RAD( x ) ( ( x ) * ( 3.14159f / 180.0f ) )
void SynthDrums::BuildWaves( void )
{
    int i;
    float finc, pos, val;

    finc = 360.0 / WAVE_BUFFER_LEN;
    pos = 0;

    // create sin wave
    for( i = 0; i < WAVE_BUFFER_LEN; i++ )
    {
        m_BufferWave[ WAVE_SIN ][ i ] = sin( DEG2RAD( pos ) );
        pos += finc;
    }

    // create sqr wave
    for( i = 0; i < WAVE_BUFFER_LEN; i++ )
    {
        if( i < WAVE_BUFFER_LEN / 2 )
            m_BufferWave[ WAVE_SQR ][ i ] = 1.0;
        else
            m_BufferWave[ WAVE_SQR ][ i ] = -1.0;
    }

    finc = 2.0 / (float)WAVE_BUFFER_LEN;
    val = 1.0;

    // create saw wave
    for( i = 0; i < WAVE_BUFFER_LEN; i++ )
    {
        m_BufferWave[ WAVE_SAW ][ i ] = val;

        val -= finc;
    }

    finc = 4 / (float)WAVE_BUFFER_LEN;
    val = 0;

    // create tri wave
    for( i = 0; i < WAVE_BUFFER_LEN; i++ )
    {
        m_BufferWave[ WAVE_TRI ][ i ] = val;

        if( i < WAVE_BUFFER_LEN / 4 )
            val += finc;
        else if( i < (WAVE_BUFFER_LEN / 4) * 3 )
            val -= finc;
        else if( i < WAVE_BUFFER_LEN  )
            val += finc;
    }
}

//-----------------------------------------------------
// Procedure:   GetAudio
//
//-----------------------------------------------------
float SynthDrums::GetWave( int type, float phase )
{
    float fval = 0.0;
    float ratio = (float)(WAVE_BUFFER_LEN-1) / gSampleRate;

    switch( type )
    {
    case WAVE_SIN:
    case WAVE_TRI:
    case WAVE_SQR:
    case WAVE_SAW:
        fval = m_BufferWave[ type ][ int( ( phase * ratio ) + 0.5 ) ];
        break;

    case WAVE_NOISE:
        fval = ( randomf() > 0.5 ) ? (randomf() * -1.0) : randomf();
        break;

    default:
        break;
    }

    return fval;
}

//-----------------------------------------------------
// Procedure:   ProcessADS
//
//-----------------------------------------------------
float SynthDrums::ProcessADS( int ch, bool bWave )
{
    ASR_STRUCT *pasr;

    if( bWave )
        pasr = &m_Wave[ ch ].adsr_wave;
    else
        pasr = &m_Wave[ ch ].adsr_freq;

    // rettrig the adsr
    if( pasr->bTrig )
    {
        pasr->state = ADSR_ATTACK;

        pasr->acount = 0;
        pasr->fainc  = 0;

        pasr->scount = (int)(.1 * gSampleRate);
        pasr->fsinc  = 1.0 / pasr->scount;
        
        if( bWave )
        {
            // for the wave asr the release is the waveform release
            pasr->rcount = (int)( params[ PARAM_REL + ch ].value * ADS_MAX_TIME_SECONDS * gSampleRate );
        }
        else
        {
            // for the wave asr the release is the hit
            pasr->rcount = (int)( params[ PARAM_ATT + ch ].value * ADS_MAX_TIME_SECONDS * gSampleRate );
        }

        if( pasr->rcount )
            pasr->frinc = 1.0 / pasr->rcount;

        m_Wave[ ch ].phase = 0.0;

        pasr->bTrig = false;
    }

    // process
    switch( pasr->state )
    {
    case ADSR_OFF:
        pasr->out = 0.0;
        break;

    case ADSR_ATTACK:
        pasr->out += pasr->fainc;
        if( --pasr->acount <= 0 )
            pasr->state = ADSR_SUSTAIN;
        break;

    case ADSR_SUSTAIN:
        pasr->out = 1.0;
        //if( --pasr->scount <= 0 )
            pasr->state = ADSR_RELEASE;
        break;

    case ADSR_RELEASE:
        pasr->out -= pasr->frinc;
        if( --pasr->rcount <= 0 )
            pasr->state = ADSR_OFF;
        break;
    }

    return clampf( pasr->out, 0.0, 1.0 );
}

//-----------------------------------------------------
// Procedure:   ChangeFilterCutoff
//
//-----------------------------------------------------
void SynthDrums::ChangeFilterCutoff( int ch, float cutfreq )
{
    float fx, fx2, fx3, fx5, fx7;

    // clamp at 1.0 and 20/samplerate
    cutfreq = fmax(cutfreq, 20 / gSampleRate); 
    cutfreq = fmin(cutfreq, 1.0);

    // calculate eq rez freq
    fx = 3.141592 * (cutfreq * 0.026315789473684210526315789473684) * 2 * 3.141592; 
    fx2 = fx*fx;
    fx3 = fx2*fx; 
    fx5 = fx3*fx2; 
    fx7 = fx5*fx2;

    m_Wave[ ch ].f = 2.0 * (fx 
	    - (fx3 * 0.16666666666666666666666666666667) 
	    + (fx5 * 0.0083333333333333333333333333333333) 
	    - (fx7 * 0.0001984126984126984126984126984127));
}

//-----------------------------------------------------
// Procedure:   Filter
//
//-----------------------------------------------------
#define MULTI (0.33333333333333333333333333333333f)
float SynthDrums::Filter( int ch, float in, bool bHighPass )
{
    OSC_PARAM_STRUCT *p;
    float rez, hp1; 
    float lowpass, highpass;

    p = &m_Wave[ ch ];

    rez = 1.00;//1.00 - p->q;

    in = in + 0.000000001;

    p->lp1   = p->lp1 + p->f * p->bp1; 
    hp1      = in - p->lp1 - rez * p->bp1; 
    p->bp1   = p->f * hp1 + p->bp1; 
    lowpass  = p->lp1; 
    highpass = hp1; 
    //bandpass = p->bp1; 

    p->lp1   = p->lp1 + p->f * p->bp1; 
    hp1      = in - p->lp1 - rez * p->bp1; 
    p->bp1   = p->f * hp1 + p->bp1; 
    lowpass  = lowpass  + p->lp1; 
    highpass = highpass + hp1; 
    //bandpass = bandpass + p->bp1; 

    in = in - 0.000000001;

    p->lp1   = p->lp1 + p->f * p->bp1; 
    hp1      = in - p->lp1 - rez * p->bp1; 
    p->bp1   = p->f * hp1 + p->bp1; 

    lowpass  = (lowpass  + p->lp1) * MULTI; 
    highpass = (highpass + hp1) * MULTI; 
    //bandpass = (bandpass + p->bp1) * MULTI;

    if( bHighPass )
        return highpass;

    return lowpass;
}

//-----------------------------------------------------
// Procedure:   GetAudio
//
//-----------------------------------------------------
float SynthDrums::GetAudio( int ch )
{
    float fout = 0, fenv = 0.0, freq;

    if( outputs[ OUTPUT_AUDIO + ch ].active )
    {
        // process our second envelope for hit
        fenv = ProcessADS( ch, false );

        // if noise then frequency affects the filter cutoff and not the wave frequency
        if( m_Wave[ ch ].wavetype == WAVE_NOISE )
        {
            freq = clampf( ( params[ PARAM_FREQ + ch ].value + (fenv*2) ), 0.0, 1.0 );

            ChangeFilterCutoff( ch, freq );
        }
        // other signals the second ADS affects the frequency for the hit
        else
        {
            m_Wave[ ch ].phase += 35 + ( params[ PARAM_FREQ + ch ].value * freqMAX ) + ( fenv * 400 );

            if( m_Wave[ ch ].phase >= gSampleRate )
                m_Wave[ ch ].phase = m_Wave[ ch ].phase - gSampleRate;
        }

        fout = ProcessADS( ch, true ) * GetWave( m_Wave[ ch ].wavetype, m_Wave[ ch ].phase );
        fout = Filter( ch, fout, ( m_Wave[ ch ].wavetype == WAVE_NOISE ) );
    }

    return fout;
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void SynthDrums::step() 
{
    int ch;

    // check for triggers
    for( ch = 0; ch < nCHANNELS; ch++ )
    {
	    if( inputs[ IN_TRIG + ch ].active ) 
        {
		    if( m_SchTrig[ ch ].process( inputs[ IN_TRIG + ch ].value ) )
            {
    		    m_Wave[ ch ].adsr_freq.bTrig = true;
                m_Wave[ ch ].adsr_wave.bTrig = true;
            }
	    }
    }

    // process sounds
    outputs[ OUTPUT_AUDIO + 0 ].value = GetAudio( 0 ) * AUDIO_MAX * clampf( (inputs[ IN_LEVEL + 0 ].value / CV_MAX), 0.0, 1.0 );
    outputs[ OUTPUT_AUDIO + 1 ].value = GetAudio( 1 ) * AUDIO_MAX * clampf( (inputs[ IN_LEVEL + 1 ].value / CV_MAX), 0.0, 1.0 );
    outputs[ OUTPUT_AUDIO + 2 ].value = GetAudio( 2 ) * AUDIO_MAX * clampf( (inputs[ IN_LEVEL + 2 ].value / CV_MAX), 0.0, 1.0 );
}