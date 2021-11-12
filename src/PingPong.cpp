﻿#include "mscHack.hpp"

typedef struct
{
    float lp1, bp1;
    float hpIn;
    float lpIn;
    float mpIn;
    
}FILTER_PARAM_STRUCT;

#define L 0
#define R 1

#define DELAY_BUFF_LEN 0x80000

#define MAC_DELAY_SECONDS 4.0f

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct PingPong : Module 
{
	enum ParamIds 
    {
        PARAM_DELAYL,
        PARAM_DELAYR,
        PARAM_LEVEL_FB_LR,
        PARAM_LEVEL_FB_LL,
        PARAM_LEVEL_FB_RL,
        PARAM_LEVEL_FB_RR,
        PARAM_CUTOFF,
        PARAM_Q,
        PARAM_MIX,
        PARAM_FILTER_MODE,
        PARAM_REVERSE,
        nPARAMS
    };

	enum InputIds 
    {
        INPUT_L,
        INPUT_R,
        INPUT_SYNC,
		INPUT_GNIP_TOGGLE,
        nINPUTS
	};

	enum OutputIds 
    {
        OUT_L,
        OUT_R,
        nOUTPUTS
	};

    enum FILTER_TYPES
    {
        FILTER_OFF,
        FILTER_LP,
        FILTER_HP,
        FILTER_BP,
        FILTER_NT
     };

    bool            m_bInitialized = false;

    FILTER_PARAM_STRUCT m_Filter[ 2 ];

    float           m_fCutoff = 0.0;
    float           m_LastOut[ 2 ] = {};
    float           m_DelayBuffer[ 2 ][ DELAY_BUFF_LEN ];

    int             m_DelayIn = 0;
    int             m_DelayOut[ 2 ] = {0};

    dsp::SchmittTrigger  m_SchmittReverse;
    bool            m_bReverseState = false;

    // sync clock
    dsp::SchmittTrigger  m_SchmittSync;
    int             m_LastSyncCount = 0;
    int             m_SyncCount = 0;
    int             m_SyncTime = 0;

    // LAST
    int             m_LastDelayKnob[ 2 ] = {};
    bool            m_bWasSynced = false;

    MyLEDButton     *m_pButtonReverse = NULL;

    // Contructor
	PingPong()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS, 0);

        configParam( PARAM_DELAYL, 0.0f, 1.0f, 0.0f, "Left Delay Time" );
        configParam( PARAM_DELAYR, 0.0f, 1.0f, 0.0f, "Right Delay Time" );
        configParam( PARAM_LEVEL_FB_LR, 0.0f, 1.0f, 0.0f, "Feedback Left to Right" );
        configParam( PARAM_LEVEL_FB_LL, 0.0f, 1.0f, 0.0f, "Feedback Left to Left" );
        configParam( PARAM_LEVEL_FB_RL, 0.0f, 1.0f, 0.0f, "Feedback Right to Left" );
        configParam( PARAM_LEVEL_FB_RR, 0.0f, 1.0f, 0.0f, "Feedback Right to Right" );
        configParam( PARAM_CUTOFF, 0.0f, 1.0f, 0.0f, "Filter Cutoff" );
        configParam( PARAM_Q, 0.0f, 1.0f, 0.0f, "Filter Resonance" );
        configParam( PARAM_MIX, 0.0f, 1.0f, 0.0f, "Wet/Dry Mix" );
        configParam( PARAM_FILTER_MODE, 0.0, 4.0, 0.0, "Filter Type" );
    }

    // Overrides 
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onReset() override;

    void    ChangeFilterCutoff( float cutfreq );
    float   Filter( int ch, float in );
};

PingPong PingPongBrowser;

//-----------------------------------------------------
// PingPong_Reverse
//-----------------------------------------------------
void PingPong_Reverse( void *pClass, int id, bool bOn ) 
{
    float delay;
    PingPong *mymodule;
    mymodule = (PingPong*)pClass;

    mymodule->m_bReverseState = bOn;

    // recalc delay offsets when going back to forward mode
    if( !mymodule->m_bReverseState )
    {
        delay = mymodule->params[ PingPong::PARAM_DELAYL ].getValue() * MAC_DELAY_SECONDS * APP->engine->getSampleRate();
        mymodule->m_DelayOut[ L ] = ( mymodule->m_DelayIn - (int)delay ) & 0x7FFFF;

        delay = mymodule->params[ PingPong::PARAM_DELAYR ].getValue() * MAC_DELAY_SECONDS * APP->engine->getSampleRate();
        mymodule->m_DelayOut[ R ] = ( mymodule->m_DelayIn - (int)delay ) & 0x7FFFF;
    }
}

//-----------------------------------------------------
// MyEQHi_Knob
//-----------------------------------------------------
struct MyCutoffKnob : Knob_Green1_40
{
    PingPong *mymodule;

    void onChange( const event::Change &e ) override 
    {
        auto paramQuantity = getParamQuantity();
        mymodule = (PingPong*)paramQuantity->module;

        if( mymodule )
        {
            mymodule->ChangeFilterCutoff( paramQuantity->getValue() ); 
        }

		RoundKnob::onChange( e );
	}
};

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
#define Y_OFF_H 40
#define X_OFF_W 40

struct PingPong_Widget : ModuleWidget 
{

PingPong_Widget( PingPong *module )
{
    PingPong *pmod;

    //box.size = Vec( 15*8, 380);

    setModule(module);

    if( !module )
        pmod = &PingPongBrowser;
    else
        pmod = module;

    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/PingPong.svg")));

    //screw
    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // sync clock
    addInput(createInput<MyPortInSmall>( Vec( 10, 110 ), module, PingPong::INPUT_SYNC ) );

    // Filter/Res knobs
    addParam(createParam<FilterSelectToggle>( Vec( 66, 55 ), module, PingPong::PARAM_FILTER_MODE ) );
    addParam(createParam<MyCutoffKnob>( Vec( 23, 60 ), module, PingPong::PARAM_CUTOFF ) );
    addParam(createParam<Knob_Purp1_20>( Vec( 73, 79 ), module, PingPong::PARAM_Q ) );
 
    // L Feedback
    addParam(createParam<Knob_Red1_20>( Vec( 49, 110 ), module, PingPong::PARAM_LEVEL_FB_LL ) );

    // Left
    addInput(createInput<MyPortInSmall>( Vec( 10, 154 ), module, PingPong::INPUT_L ) );
    addParam(createParam<Knob_Yellow2_40>( Vec( 38, 143 ), module, PingPong::PARAM_DELAYL ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( 90, 154 ), module, PingPong::OUT_L ) );

    // R to L level and L to R levels
    addParam(createParam<Knob_Red1_20>( Vec( 9, 191 ), module, PingPong::PARAM_LEVEL_FB_RL ) );
    addParam(createParam<Knob_Red1_20>( Vec( 9, 226 ), module, PingPong::PARAM_LEVEL_FB_LR ) );

    // mix knob
    addParam(createParam<Knob_Blue2_40>( Vec( 77, 199 ), module, PingPong::PARAM_MIX ) );

    // Left
    addInput(createInput<MyPortInSmall>( Vec( 10, 266 ), module, PingPong::INPUT_R ) );
    addParam(createParam<Knob_Yellow2_40>( Vec( 38, 255 ), module, PingPong::PARAM_DELAYR ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( 90, 266 ), module, PingPong::OUT_R ) );

    // R Feedback
    addParam(createParam<Knob_Red1_20>( Vec( 49, 308 ), module, PingPong::PARAM_LEVEL_FB_RR ) );

    // reverse button
    addInput(createInput<MyPortInSmall>( Vec( 3, 340 ), module, PingPong::INPUT_GNIP_TOGGLE ) );

    pmod->m_pButtonReverse = new MyLEDButton( 24, 343, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 255, 0 ), MyLEDButton::TYPE_SWITCH, 0, module, PingPong_Reverse );
	addChild( pmod->m_pButtonReverse );

    if( module )
        module->m_bInitialized = true;
}
};

//-----------------------------------------------------
// Procedure:   reset
//
//-----------------------------------------------------
void PingPong::onReset()
{
    if( !m_bInitialized )
        return;

    m_pButtonReverse->Set( false );
    m_bReverseState = false;
}

//-----------------------------------------------------
// Procedure:   
//
//-----------------------------------------------------
json_t *PingPong::dataToJson() 
{
	json_t *rootJ = json_object();

    // reverse state
    json_object_set_new(rootJ, "ReverseState", json_boolean (m_bReverseState));

	return rootJ;
}

//-----------------------------------------------------
// Procedure:   fromJson
//
//-----------------------------------------------------
void PingPong::dataFromJson(json_t *rootJ) 
{
	// reverse state
	json_t *revJ = json_object_get(rootJ, "ReverseState");

	if (revJ)
		m_bReverseState = json_is_true( revJ );

    m_pButtonReverse->Set( m_bReverseState );
}

//-----------------------------------------------------
// Procedure:   ChangeFilterCutoff
//
//-----------------------------------------------------
void PingPong::ChangeFilterCutoff( float cutfreq )
{
    float fx, fx2, fx3, fx5, fx7;

    // clamp at 1.0 and 20/samplerate
    cutfreq = fmax(cutfreq, 20 / APP->engine->getSampleRate()); 
    cutfreq = fmin(cutfreq, 1.0);

    // calculate eq rez freq
    fx = 3.141592 * (cutfreq * 0.026315789473684210526315789473684) * 2 * 3.141592; 
    fx2 = fx*fx;
    fx3 = fx2*fx; 
    fx5 = fx3*fx2; 
    fx7 = fx5*fx2;

    m_fCutoff = 2.0 * (fx 
	    - (fx3 * 0.16666666666666666666666666666667) 
	    + (fx5 * 0.0083333333333333333333333333333333) 
	    - (fx7 * 0.0001984126984126984126984126984127));
}

//-----------------------------------------------------
// Procedure:   Filter
//
//-----------------------------------------------------
#define MULTI (0.33333333333333333333333333333333f)
float PingPong::Filter( int ch, float in )
{
    FILTER_PARAM_STRUCT *p;
    float rez, hp1, out = 0.0; 
    float lowpass, highpass, bandpass;

    if( (int)params[ PARAM_FILTER_MODE ].getValue() == 0 )
        return in;

    p = &m_Filter[ ch ];

    rez = 1.0 - params[ PARAM_Q ].value;

    in = in + 0.000000001;

    p->lp1   = p->lp1 + m_fCutoff * p->bp1; 
    hp1      = in - p->lp1 - rez * p->bp1; 
    p->bp1   = m_fCutoff * hp1 + p->bp1; 
    lowpass  = p->lp1; 
    highpass = hp1; 
    bandpass = p->bp1; 

    p->lp1   = p->lp1 + m_fCutoff * p->bp1; 
    hp1      = in - p->lp1 - rez * p->bp1; 
    p->bp1   = m_fCutoff * hp1 + p->bp1; 
    lowpass  = lowpass  + p->lp1; 
    highpass = highpass + hp1; 
    bandpass = bandpass + p->bp1; 

    in = in - 0.000000001;

    p->lp1   = p->lp1 + m_fCutoff * p->bp1; 
    hp1      = in - p->lp1 - rez * p->bp1; 
    p->bp1   = m_fCutoff * hp1 + p->bp1; 

    lowpass  = (lowpass  + p->lp1) * MULTI; 
    highpass = (highpass + hp1) * MULTI; 
    bandpass = (bandpass + p->bp1) * MULTI;

    switch( (int)params[ PARAM_FILTER_MODE ].value )
    {
    case FILTER_LP:
        out = lowpass;
        break;
    case FILTER_HP:
        out = highpass;
        break;
    case FILTER_BP:
        out = bandpass;
        break;
    case FILTER_NT:
        out  = lowpass + highpass;
        break;
    default:
        break;
    }

    return out;
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
float syncQuant[ 10 ] = { 0.125, 0.25, 0.333, 0.375, 0.5, 0.625, 0.666, 0.750, 0.875, 1.0 };
void PingPong::process(const ProcessArgs &args) 
{
    float outL, outR, inL = 0.0, inR = 0.0, inOrigL = 0.0, inOrigR = 0.0, syncq = 0.0, mix;
    bool bMono = false;
    int i, dR, dL;

    if( !m_bInitialized )
        return;

    dL = params[ PARAM_DELAYL ].getValue() * MAC_DELAY_SECONDS * args.sampleRate;
    dR = params[ PARAM_DELAYR ].getValue() * MAC_DELAY_SECONDS * args.sampleRate;

	if( m_SchmittReverse.process( inputs[ INPUT_GNIP_TOGGLE ].getVoltage() ) )
	{
		if( m_pButtonReverse->m_bOn )
			m_pButtonReverse->Set( false );
		else
			m_pButtonReverse->Set( true );

		m_bReverseState = m_pButtonReverse->m_bOn;
	}

    // check right channel first for possible mono
    if( inputs[ INPUT_SYNC ].isConnected() )
    {
        m_SyncCount++;

        // sync'd delay
        if( m_SchmittSync.process( inputs[ INPUT_SYNC ].value ) )
        {
            if( !m_bWasSynced || ( (m_SyncTime / 10) != (m_SyncCount / 10) ) || ( m_LastDelayKnob[ L ] != dL ) || ( m_LastDelayKnob[ R ] != dR ) )
            {
                m_SyncTime = m_SyncCount;

                for( i = 0; i < 10; i++ )
                {
                    if( params[ PARAM_DELAYL ].getValue() <= syncQuant[ i ] )
                    {
                        syncq = syncQuant[ i ] * MAC_DELAY_SECONDS;
                        break;
                    }
                }

                m_DelayOut[ L ] = ( m_DelayIn - (int)(syncq * m_SyncTime) ) & 0x7FFFF;

                for( i = 0; i < 10; i++ )
                {
                    if( params[ PARAM_DELAYR ].getValue() <= syncQuant[ i ] )
                    {
                        syncq = syncQuant[ i ] * MAC_DELAY_SECONDS;
                        break;
                    }
                }

                m_DelayOut[ R ] = ( m_DelayIn - (int)(syncq * m_SyncTime) ) & 0x7FFFF;
            }
     
            m_SyncCount = 0;
        }

        m_bWasSynced = true;
    }
    else
    {
        // non sync'd delay
        if( m_bWasSynced || ( m_LastDelayKnob[ L ] != dL ) )
            m_DelayOut[ L ] = ( m_DelayIn - (int)dL ) & 0x7FFFF;

        if( m_bWasSynced || ( m_LastDelayKnob[ R ] != dR ) )
            m_DelayOut[ R ] = ( m_DelayIn - (int)dR ) & 0x7FFFF;

        m_bWasSynced = false;
        m_SyncCount = 0;
    }

    m_LastDelayKnob[ L ] = dL;
    m_LastDelayKnob[ R ] = dR;

    // check right channel first for possible mono
    if( inputs[ INPUT_R ].isConnected() )
    {
        inR = inputs[ INPUT_R ].getVoltage();
        inR = Filter( R, inR );
        inOrigR = inR;
        bMono = false;
    }
    else
        bMono = true;

    // left channel
    if( inputs[ INPUT_L ].isConnected() )
    {
        inL = inputs[ INPUT_L ].getVoltage();
        inL = Filter( L, inL );
        inOrigL = inL;

        if( bMono )
        {
            inOrigR = inL;
            inR = inL;
        }
    }

    m_DelayBuffer[ L ][ m_DelayIn ] = inL + ( m_LastOut[ L ] * params[ PARAM_LEVEL_FB_LL ].getValue() ) + ( m_LastOut[ R ] * params[ PARAM_LEVEL_FB_RL ].getValue() );
    m_DelayBuffer[ R ][ m_DelayIn ] = inR + ( m_LastOut[ R ] * params[ PARAM_LEVEL_FB_RR ].getValue() ) + ( m_LastOut[ L ] * params[ PARAM_LEVEL_FB_LR ].getValue() );

    m_DelayIn = ( ( m_DelayIn + 1 ) & 0x7FFFF );

    outL = m_DelayBuffer[ L ][ m_DelayOut[ L ] ];
    outR = m_DelayBuffer[ R ][ m_DelayOut[ R ] ];

    if( m_bReverseState )
    {
        m_DelayOut[ L ] = ( ( m_DelayOut[ L ] - 1 ) & 0x7FFFF );
        m_DelayOut[ R ] = ( ( m_DelayOut[ R ] - 1 ) & 0x7FFFF );
    }
    else
    {
        m_DelayOut[ L ] = ( ( m_DelayOut[ L ] + 1 ) & 0x7FFFF );
        m_DelayOut[ R ] = ( ( m_DelayOut[ R ] + 1 ) & 0x7FFFF );
    }

    m_LastOut[ L ] = outL;
    m_LastOut[ R ] = outR;

    mix = params[ PARAM_MIX ].getValue();

    // output
    outputs[ OUT_L ].setVoltage( ( inOrigL * ( 1.0f - mix ) ) + ( outL * mix ) );
    outputs[ OUT_R ].setVoltage( ( inOrigR * ( 1.0f - mix ) ) + ( outR * mix ) );
}

Model *modelPingPong = createModel<PingPong, PingPong_Widget>( "PingPong_Widget" );
