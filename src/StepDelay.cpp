#include "mscHack.hpp"

#define nDELAYS 8
#define nSTEPS 4
#define nBUFFLEN 0x80000
#define MAX_nWAVES 7

typedef struct
{
    // track in clk bpm
    //SLIDING_AVG_STRUCT Avg;
    int     tickcount;
    float   ftickspersec;
    float   fbpm;

    // track sync tick
    float fsynclen;
    float fsynccount;

    bool  bClockReset;

}MAIN_SYNC_CLOCK;

typedef struct
{
    float lp1[ 2 ] = {}, bp1[ 2 ] = {};
    
}FILTER_PARAM_STRUCT;

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct StepDelay : Module 
{
	enum ParamIds 
    {
        PARAM_FILTER_MODE,
        PARAM_FILTERF,
        PARAM_FILTERQ,
        PARAM_LEVEL,
        PARAM_PAN       = (PARAM_LEVEL + nSTEPS ),
        PARAM_FB        = (PARAM_PAN + nSTEPS ),
        PARAM_DELAY     = (PARAM_FB + nSTEPS ),
        PARAM_MIX       = (PARAM_DELAY + nSTEPS ),
        nPARAMS         
    };

	enum InputIds 
    {
        IN_CLOCK,
        IN_CLK_RESET,
        IN_AUDIOL,
        IN_AUDIOR,
        IN_FILTER_MOD,
        nINPUTS 
	};

	enum OutputIds 
    {
        OUT_AUDIOL,
        OUT_AUDIOR,
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

    // Contructor
	StepDelay()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS);

        configParam( PARAM_FILTER_MODE, 0.0, 4.0, 0.0, "Filter Type" );
        configParam( PARAM_FILTERF, 0.0, 1.0, 0.0, "Filter Cutoff" );
        configParam( PARAM_FILTERQ, 0.0, 1.0, 0.0, "Filter Resonance" );

        for( int i = 0; i < nSTEPS; i++ )
        {
            configParam( PARAM_LEVEL + i, 0.0, 1.0, 0.5, "Step Level" );
            configParam( PARAM_PAN + i, -1.0, 1.0, 0.0 , "Step Pan" );
            configParam( PARAM_FB + i, 0.0, 1.0, 0.0, "Step Feedback" );
            configParam( PARAM_DELAY + i, 0.0, (float)(nDELAYS - 1), 0.0, "Step Delay" );
        }

        configParam( PARAM_MIX, 0.0, 1.0, 0.5, "Wet/Dry Mix" );
    }

    // clock     
    dsp::SchmittTrigger      m_SchTrigClk;
    MAIN_SYNC_CLOCK     m_Clock;
    
    // global triggers
    dsp::SchmittTrigger      m_SchTrigGlobalClkReset;
    bool                m_GlobalClkResetPending = false;

    Label               *m_pTextLabel[ nSTEPS ] = {};

    MyLEDButtonStrip    *m_pButtonLenMod[ nSTEPS ] = {};
    int                 m_lenmod[ nSTEPS ] = {0};

    FILTER_PARAM_STRUCT m_Filter = {};
    float               m_fCutoff;

#define L 0
#define R 1

    // buffers
    float               m_fBuffer[ 2 ][ nBUFFLEN ] = {};
    int                 m_BufferInOffset = 0;
    int                 m_BufferOutOffset[ nSTEPS ] = {0};

//-----------------------------------------------------
// MyDelayKnob
//-----------------------------------------------------
struct MyDelayKnob : Knob_Yellow3_20_Snap
{
    StepDelay *mymodule;

    void onChange( const event::Change &e ) override 
    {
        auto paramQuantity = getParamQuantity();
        mymodule = (StepDelay*)paramQuantity->module;

        if( mymodule )
        {
            mymodule->CalcDelays(); 
        }

		RoundKnob::onChange( e );
	}
};

    // Overrides 
    void   process(const ProcessArgs &args) override;
    void    JsonParams( bool bTo, json_t *root);
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;

    void    ChangeFilterCutoff( float cutfreq );
    void    Filter( float *InL, float *InR );
    void    CalcDelays( void ); 
};

StepDelay StepDelayBrowser;

//-----------------------------------------------------
// Procedure:   LenModCallback
//-----------------------------------------------------
void LenModCallback( void *pClass, int id, int nbutton, bool bOn ) 
{
    StepDelay *mymodule;
    mymodule = (StepDelay*)pClass;

    if( mymodule )
    {
        mymodule->m_lenmod[ id ] = nbutton;
        mymodule->CalcDelays(); 
    }
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct StepDelay_Widget : ModuleWidget 
{

StepDelay_Widget( StepDelay *module )
{
    int steps, x, y;
    StepDelay *pmod;

    setModule(module);

    if( !module )
        pmod = &StepDelayBrowser;
    else
        pmod = module;

    //box.size = Vec( 15*14, 380 );
    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/StepDelay.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Clock Inputs
    addInput(createInput<MyPortInSmall>( Vec( 14, 23 ), module, StepDelay::IN_CLOCK ) );
    
    // Audio Inputs
    addInput(createInput<MyPortInSmall>( Vec( 14, 70 ), module, StepDelay::IN_AUDIOL ) );
    addInput(createInput<MyPortInSmall>( Vec( 40, 70 ), module, StepDelay::IN_AUDIOR ) );

    // Audio Outputs
    addOutput(createOutput<MyPortOutSmall>( Vec( 14, 109 ), module, StepDelay::OUT_AUDIOL ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( 40, 109 ), module, StepDelay::OUT_AUDIOR ) );

    // Mix
    addParam(createParam<Knob_Blue2_26>( Vec( 11, 149 ), module, StepDelay::PARAM_MIX ) );

    // Filter knobs
    addParam(createParam<Knob_Green1_40>( Vec( 93, 58 ), module, StepDelay::PARAM_FILTERF ) );
    addParam(createParam<Knob_Purp1_20>( Vec( 144, 77 ), module, StepDelay::PARAM_FILTERQ ) );
    addParam(createParam<FilterSelectToggle>( Vec( 139, 57 ), module, StepDelay::PARAM_FILTER_MODE ) );

    addInput(createInput<MyPortInSmall>( Vec( 104, 103 ), module, StepDelay::IN_FILTER_MOD ) );

    x = 93;
    y = 173;

    for( steps = 0; steps < nSTEPS; steps++ )
    {
        y = 173;

        // step knobs
        addParam(createParam<Knob_Red1_20>( Vec( x , y ), module, StepDelay::PARAM_LEVEL + steps ) );
        y += 28;
        addParam(createParam<Knob_Blue3_20>( Vec( x , y ), module, StepDelay::PARAM_PAN + steps ) );
        y += 28;
        addParam(createParam<Knob_Yellow3_20>( Vec( x , y ), module, StepDelay::PARAM_FB + steps ) );
        y += 28;

        addParam(createParam<StepDelay::MyDelayKnob>( Vec( x , y ), module, StepDelay::PARAM_DELAY + steps ) );

        pmod->m_pTextLabel[ steps ] = new Label();
        pmod->m_pTextLabel[ steps ]->box.pos = Vec( x - 9, y + 18 );
        pmod->m_pTextLabel[ steps ]->text = "Delay";
        //module->m_pTextLabel[ steps ]->
		addChild( pmod->m_pTextLabel[ steps ] );

        y += 38;

        pmod->m_pButtonLenMod[ steps ] = new MyLEDButtonStrip( x + 4, y, 12, 12, 2, 10.0, 2, true, DWRGB( 180, 180, 180 ), DWRGB( 255, 255, 0 ), MyLEDButtonStrip::TYPE_EXCLUSIVE_WOFF, steps, module, LenModCallback );
	    addChild( pmod->m_pButtonLenMod[ steps ] );

        x += 28;
    }

	if( module )
    {
        module->m_Clock.fsynclen = 2.0 * 48.0;  // default to 120bpm
        module->CalcDelays();
        module->m_bInitialized = true;
    }
}
};

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void StepDelay::JsonParams( bool bTo, json_t *root) 
{
    JsonDataInt( bTo, "m_lenmod", root, (int*)m_lenmod, nSTEPS );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *StepDelay::dataToJson() 
{
	json_t *root = json_object();

    if( !root )
        return NULL;

    JsonParams( TOJSON, root );
    
	return root;
}

//-----------------------------------------------------
// Procedure:   fromJson
//
//-----------------------------------------------------
void StepDelay::dataFromJson( json_t *root ) 
{
    JsonParams( FROMJSON, root );

    for( int i = 0; i < nSTEPS; i++ )
    {
        m_pButtonLenMod[ i ]->Set( m_lenmod[ i ], true );
    }

    CalcDelays(); 
}

//-----------------------------------------------------
// Procedure:   ChangeFilterCutoff
//
//-----------------------------------------------------
void StepDelay::ChangeFilterCutoff( float cutfreq )
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
void StepDelay::Filter( float *InL, float *InR )
{
    FILTER_PARAM_STRUCT *p;
    float rez, hp1; 
    float input[ 2 ], out[ 2 ] = {0}, lowpass, bandpass, highpass;

    if( (int)params[ PARAM_FILTER_MODE ].getValue() == 0 )
        return;

    ChangeFilterCutoff( clamp( params[ PARAM_FILTERF ].getValue() + clamp( inputs[ IN_FILTER_MOD ].getNormalVoltage( 0.0f ) / CV_MAXn5, -1.0f, 1.0f ), -1.0f, 1.0f ) );

    p = &m_Filter;

    rez = 1.0 - params[ PARAM_FILTERQ ].value;

    input[ 0 ] = *InL; //clamp( *InL / AUDIO_MAX, -1.0f, 1.0f );
    input[ 1 ] = *InR; //clamp( *InR / AUDIO_MAX, -1.0f, 1.0f );

    // do left and right channels
    for( int i = 0; i < 2; i++ )
    {
        input[ i ]  = input[ i ] + 0.000000001;

        p->lp1[ i ] = p->lp1[ i ] + m_fCutoff * p->bp1[ i ]; 
        hp1         = input[ i ] - p->lp1[ i ] - rez * p->bp1[ i ]; 
        p->bp1[ i ] = m_fCutoff * hp1 + p->bp1[ i ]; 
        lowpass     = p->lp1[ i ]; 
        highpass    = hp1; 
        bandpass    = p->bp1[ i ]; 

        p->lp1[ i ] = p->lp1[ i ] + m_fCutoff * p->bp1[ i ]; 
        hp1         = input[ i ] - p->lp1[ i ] - rez * p->bp1[ i ]; 
        p->bp1[ i ] = m_fCutoff * hp1 + p->bp1[ i ]; 
        lowpass     = lowpass  + p->lp1[ i ]; 
        highpass    = highpass + hp1; 
        bandpass    = bandpass + p->bp1[ i ]; 

        input[ i ]  = input[ i ] - 0.000000001;

        p->lp1[ i ] = p->lp1[ i ] + m_fCutoff * p->bp1[ i ]; 
        hp1         = input[ i ] - p->lp1[ i ] - rez * p->bp1[ i ]; 
        p->bp1[ i ] = m_fCutoff * hp1 + p->bp1[ i ]; 

        lowpass  = (lowpass  + p->lp1[ i ]) * MULTI; 
        highpass = (highpass + hp1) * MULTI; 
        bandpass = (bandpass + p->bp1[ i ]) * MULTI;

        switch( (int)params[ PARAM_FILTER_MODE ].value )
        {
        case FILTER_LP:
            out[ i ] = lowpass;
            break;
        case FILTER_HP:
            out[ i ]  = highpass;
            break;
        case FILTER_BP:
            out[ i ]  = bandpass;
            break;
        case FILTER_NT:
            out[ i ]  = lowpass + highpass;
            break;
        default:
            break;
        }
    }

    *InL = out[ 0 ];// * AUDIO_MAX;
    *InR = out[ 1 ];// * AUDIO_MAX;
}

//-----------------------------------------------------
// Procedure:   CalcDelays
//
//-----------------------------------------------------
#define BASE_TICK 48 

const float fdelaylen[ nDELAYS ] = { 0.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f };
const float delaymod[ 3 ] = { 1.0f, 1.5f, 0.3333f };
const char strDelay[ nDELAYS ][ 8 ] = { {" Off"}, {"2Bar"}, {"Bar"}, {" Half"}, {" 4th"}, {"  8th"}, {" 16th"}, {" 32nd"} }; 

void StepDelay::CalcDelays( void )
{
    int i, delay, delaysum = 0;

    if( !m_bInitialized )
        return;

    for( i = 0; i < nSTEPS; i++ )
    {
        delay = (int)params[ PARAM_DELAY + i ].getValue();

        if( m_pTextLabel[ i ] )
            m_pTextLabel[ i ]->text = strDelay[ delay ];

        if( delay )
        {
            delaysum += (int)( m_Clock.fsynclen * ( fdelaylen[ delay ] * delaymod[ m_lenmod[ i ] ] ) );
            m_BufferOutOffset[ i ] = ( m_BufferInOffset - delaysum ) & 0x7FFFF;
        }
    }
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void StepDelay::process(const ProcessArgs &args)
{
    int i;
    bool bMono = true;
    float delayL, delayR, fFbL = 0.0f, fFbR = 0.0f, inLOrig = 0.0f, inROrig = 0.0f, inL = 0.0f, inR = 0.0f, outL = 0.0f, outR = 0.0f, inPan, mix;

    if( !m_bInitialized )
        return;

    // get audio inputs, Mono if L only
    if( inputs[ IN_AUDIOR ].isConnected() )
    {
        // right channel connected, this is not mono
        bMono = false;
        inR = inputs[ IN_AUDIOR ].getVoltage();
    }

    if( inputs[ IN_AUDIOL ].isConnected() )
    {
        inL = inputs[ IN_AUDIOL ].getVoltage();

        // mono, make R channel equal left
        if( bMono )
            inR = inL;
    }

    inLOrig = inL;
    inROrig = inR;

    Filter( &inL, &inR );

    m_Clock.tickcount++;

    // track clock period
    if( m_SchTrigClk.process( inputs[ IN_CLOCK ].getNormalVoltage( 0.0f ) ) )
    {
        //m_Clock.fsynclen = (float)( engineGetSampleRate() / (float)m_Clock.tickcount ) * 48.0;
        m_Clock.fsynclen = m_Clock.tickcount;
        CalcDelays();
        m_Clock.tickcount = 0;
    }

    // add delays
    for( i = 0; i < nSTEPS; i++ )
    {
        // add each activated delay
        if( params[ PARAM_DELAY + i ].getValue() != 0 )
        {
            // add delay
            delayL = m_fBuffer[ L ][ m_BufferOutOffset[ i ] ] * params[ PARAM_LEVEL + i ].getValue();
            delayR = m_fBuffer[ R ][ m_BufferOutOffset[ i ] ] * params[ PARAM_LEVEL + i ].getValue();

            m_BufferOutOffset[ i ] = ( m_BufferOutOffset[ i ] + 1 ) & 0x7FFFF;

            inPan = params[ PARAM_PAN + i ].getValue();

            if( inPan <= 0.0 )
                delayR *= ( 1.0 + inPan );
            else
                delayL *= ( 1.0 - inPan );

            outL += delayL;
            outR += delayR;

            fFbL += delayL * params[ PARAM_FB + i ].getValue();
            fFbR += delayR * params[ PARAM_FB + i ].getValue();
        }
    }

    // main delay buffer
    m_fBuffer[ L ][ m_BufferInOffset ] = inL + fFbL;
    m_fBuffer[ R ][ m_BufferInOffset ] = inR + fFbR;

    m_BufferInOffset = ( m_BufferInOffset + 1 ) & 0x7FFFF;

    mix = params[ PARAM_MIX ].getValue();

    outputs[ OUT_AUDIOL ].setVoltage( ( outL * mix ) + ( inLOrig * ( 1.0f - mix ) ) );
    outputs[ OUT_AUDIOR ].setVoltage( ( outR * mix ) + ( inROrig * ( 1.0f - mix ) ) );
}

Model *modelStepDelay = createModel<StepDelay, StepDelay_Widget>( "StepDelay" );
