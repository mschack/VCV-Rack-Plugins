#include "mscHack.hpp"

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct OSC_WaveMorph_3 : Module 
{
#define nCHANNELS 3

	enum ParamIds 
    {
        PARAM_BAND,
        PARAM_LEVEL,
        PARAM_CUTOFF,
        PARAM_RES,
        PARAM_FILTER_MODE,
        nPARAMS
    };

	enum InputIds 
    {
        INPUT_VOCT,
        INPUT_MORPHCV,
        IN_FILTER,
        IN_REZ,
        IN_LEVEL,
        IN_WAVE_CHANGE,
        nINPUTS
	};

	enum OutputIds 
    {
        OUTPUT_AUDIO,
        nOUTPUTS
	};

	enum LightIds 
    {
        nLIGHTS
	};

    bool                m_bInitialized = false;

    // Contructor
	OSC_WaveMorph_3()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS, nLIGHTS);

        configParam( PARAM_BAND, 0.0, 0.8, 0.333, "Draw Rubber Banding" );
        configParam( PARAM_LEVEL, 0.0, 1.0, 0.0, "Level Out" );
        configParam( PARAM_CUTOFF, 0.0, 0.1, 0.0, "Filter Cutoff" );
        configParam( PARAM_RES, 0.0, 1.0, 0.0, "Filter Resonance" );
        configParam( PARAM_FILTER_MODE, 0.0, 4.0, 0.0, "Filter Mode" );
    }

    int                 m_CurrentChannel = 0;
    int                 m_GraphData[ MAX_ENVELOPE_CHANNELS ][ ENVELOPE_HANDLES ] = {};

    int                 m_waveSet = 0;
    bool                m_bCpy = false;

    bool                m_bSolo = false;

    dsp::SchmittTrigger  m_SchmittChangeWave;

    Widget_EnvelopeEdit *m_pEnvelope = NULL;
    MyLEDButtonStrip    *m_pButtonChSelect = NULL;

    MyLEDButton         *m_pButtonWaveSetBck = NULL;
    MyLEDButton         *m_pButtonWaveSetFwd = NULL;

    MyLEDButton         *m_pButtonDraw = NULL;
    MyLEDButton         *m_pButtonCopy = NULL;
    MyLEDButton         *m_pButtonRand = NULL;
    MyLEDButton         *m_pButtonInvert = NULL;
    MyLEDButton         *m_pButtonSolo = NULL;

    Label               *m_pTextLabel = NULL;

    // filter
    enum FILTER_TYPES
    {
        FILTER_OFF,
        FILTER_LP,
        FILTER_HP,
        FILTER_BP,
        FILTER_NT
     };

    int   filtertype;
    float q, f;
    float lp1 = 0, bp1 = 0;

    //-----------------------------------------------------
    // Band_Knob
    //-----------------------------------------------------
    struct Band_Knob : Knob_Yellow2_26
    {
        OSC_WaveMorph_3 *mymodule;
        int param;

        void onChange( const event::Change &e ) override 
        {
            auto paramQuantity = getParamQuantity();
            mymodule = (OSC_WaveMorph_3*)paramQuantity->module;

            if( mymodule )
                mymodule->m_pEnvelope->m_fband = paramQuantity->getValue(); 

		    RoundKnob::onChange( e );
	    }
    };

    void    ChangeChannel( int ch );
    void    ChangeFilterCutoff( float cutfreq );
    void    Filter( float *In );

    // Overrides 
    void    JsonParams( bool bTo, json_t *root);
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onRandomize() override;
    void    onReset() override;
};

OSC_WaveMorph_3 OSC_WaveMorph_3Browser;

//-----------------------------------------------------
// Seq_Triad2_Pause
//-----------------------------------------------------
void OSC_WaveMorph_3_EnvelopeEditCALLBACK ( void *pClass, float val )
{
    char strVal[ 10 ] = {};

    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;

    sprintf( strVal, "[%.3fV]", val );

    mymodule->m_pTextLabel->text = strVal;
}

//-----------------------------------------------------
// OSC_WaveMorph_3_DrawMode
//-----------------------------------------------------
void OSC_WaveMorph_3_DrawMode( void *pClass, int id, bool bOn ) 
{
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;
    mymodule->m_pEnvelope->m_bDraw = bOn;
}

//-----------------------------------------------------
// OSC_WaveMorph_3_Solo
//-----------------------------------------------------
void OSC_WaveMorph_3_Solo( void *pClass, int id, bool bOn ) 
{
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;
    mymodule->m_bSolo = bOn;
}

//-----------------------------------------------------
// Procedure:   OSC_WaveMorph_3_ChSelect
//-----------------------------------------------------
void OSC_WaveMorph_3_ChSelect( void *pClass, int id, int nbutton, bool bOn )
{
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;

    mymodule->ChangeChannel( nbutton );
}

//-----------------------------------------------------
// Procedure:   OSC_WaveMorph_3_WaveSet
//-----------------------------------------------------
void OSC_WaveMorph_3_WaveSet( void *pClass, int id, bool bOn )
{
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;

    if( id == 0 )
    {
        if( ++mymodule->m_waveSet >= EnvelopeData::nPRESETS )
            mymodule->m_waveSet = 0;
    }
    else
    {
        if( --mymodule->m_waveSet < 0 )
            mymodule->m_waveSet = EnvelopeData::nPRESETS - 1;
    }

    mymodule->m_pEnvelope->m_EnvData[ mymodule->m_CurrentChannel ].Preset( mymodule->m_waveSet );
}

//-----------------------------------------------------
// Procedure:   OSC_WaveMorph_3_WaveInvert
//-----------------------------------------------------
void OSC_WaveMorph_3_WaveInvert( void *pClass, int id, bool bOn )
{
    int i;
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;

    for( i = 0; i < ENVELOPE_HANDLES; i++ )
        mymodule->m_pEnvelope->setVal( mymodule->m_CurrentChannel, i, 1.0f - mymodule->m_pEnvelope->m_EnvData[ mymodule->m_CurrentChannel ].m_HandleVal[ i ] );
}

//-----------------------------------------------------
// Procedure:   OSC_WaveMorph_3_WaveRand
//-----------------------------------------------------
void OSC_WaveMorph_3_WaveRand( void *pClass, int id, bool bOn )
{
    int i;
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;

    for( i = 0; i < ENVELOPE_HANDLES; i++ )
        mymodule->m_pEnvelope->setVal( mymodule->m_CurrentChannel, i, random::uniform() );
}

//-----------------------------------------------------
// Procedure:   OSC_WaveMorph_3_WaveCopy
//-----------------------------------------------------
void OSC_WaveMorph_3_WaveCopy( void *pClass, int id, bool bOn )
{
    OSC_WaveMorph_3 *mymodule;
    mymodule = (OSC_WaveMorph_3*)pClass;

    mymodule->m_bCpy = bOn;
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct OSC_WaveMorph_3_Widget : ModuleWidget 
{

OSC_WaveMorph_3_Widget( OSC_WaveMorph_3 *module )
{
    OSC_WaveMorph_3 *pmod;

    setModule(module);

    if( !module )
        pmod = &OSC_WaveMorph_3Browser;
    else
        pmod = module;

    //box.size = Vec( 15*16, 380 );
    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/OSC_WaveMorph_3.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // input V/OCT
    addInput(createInput<MyPortInSmall>( Vec( 14, 20 ), module, OSC_WaveMorph_3::INPUT_VOCT ) );

    // input morph cv
    addInput(createInput<MyPortInSmall>( Vec( 14, 311 ), module, OSC_WaveMorph_3::INPUT_MORPHCV ) );

    // invert
    pmod->m_pButtonInvert = new MyLEDButton( 88, 31, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButton::TYPE_MOMENTARY, 0, module, OSC_WaveMorph_3_WaveInvert );
	addChild( pmod->m_pButtonInvert );

    // envelope editor
    pmod->m_pEnvelope = new Widget_EnvelopeEdit( 16, 47, 208, 96, 5, module, OSC_WaveMorph_3_EnvelopeEditCALLBACK, nCHANNELS );
	addChild( pmod->m_pEnvelope );

    pmod->m_pEnvelope->m_EnvData[ 0 ].m_Range = EnvelopeData::RANGE_Audio;
    pmod->m_pEnvelope->m_EnvData[ 1 ].m_Range = EnvelopeData::RANGE_Audio;
    pmod->m_pEnvelope->m_EnvData[ 2 ].m_Range = EnvelopeData::RANGE_Audio;

    // solo button
    pmod->m_pButtonSolo = new MyLEDButton( 158, 146, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 0 ), MyLEDButton::TYPE_SWITCH, 0, module, OSC_WaveMorph_3_Solo );
    addChild( pmod->m_pButtonSolo );

    // wave change (when soloing) cv input
    addInput(createInput<MyPortInSmall>( Vec( 131, 144 ), module, OSC_WaveMorph_3::IN_WAVE_CHANGE ) );

    // envelope select buttons
    pmod->m_pButtonChSelect = new MyLEDButtonStrip( 183, 146, 11, 11, 3, 8.0, nCHANNELS, false, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, 0, module, OSC_WaveMorph_3_ChSelect );
    addChild( pmod->m_pButtonChSelect );

    pmod->m_pTextLabel = new Label();
    pmod->m_pTextLabel->box.pos = Vec( 150, 4 );
    pmod->m_pTextLabel->text = "----";
	addChild( pmod->m_pTextLabel );

    // wave set buttons
    pmod->m_pButtonWaveSetBck = new MyLEDButton( 122, 31, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButton::TYPE_MOMENTARY, 0, module, OSC_WaveMorph_3_WaveSet );
	addChild( pmod->m_pButtonWaveSetBck );

    pmod->m_pButtonWaveSetFwd = new MyLEDButton( 134, 31, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButton::TYPE_MOMENTARY, 1, module, OSC_WaveMorph_3_WaveSet );
	addChild( pmod->m_pButtonWaveSetFwd );

    // random
    pmod->m_pButtonRand = new MyLEDButton( 163, 31, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButton::TYPE_MOMENTARY, 0, module, OSC_WaveMorph_3_WaveRand );
	addChild( pmod->m_pButtonRand );

    // copy
    pmod->m_pButtonCopy = new MyLEDButton( 188, 31, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButton::TYPE_SWITCH, 0, module, OSC_WaveMorph_3_WaveCopy );
	addChild( pmod->m_pButtonCopy );

    // draw mode
    pmod->m_pButtonDraw = new MyLEDButton( 17, 145, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 128, 0 ), MyLEDButton::TYPE_SWITCH, 0, module, OSC_WaveMorph_3_DrawMode );
	addChild( pmod->m_pButtonDraw );

    // band knob
    addParam(createParam<OSC_WaveMorph_3::Band_Knob>( Vec( 60 , 145 ), module, OSC_WaveMorph_3::PARAM_BAND ) );

    // filter
    addParam(createParam<Knob_Green1_40>( Vec( 30, 200 ), module, OSC_WaveMorph_3::PARAM_CUTOFF ) );
    addParam(createParam<FilterSelectToggle>( Vec( 73, 200 ), module, OSC_WaveMorph_3::PARAM_FILTER_MODE ) );
    addParam(createParam<Knob_Purp1_20>( Vec( 76, 219 ), module, OSC_WaveMorph_3::PARAM_RES ) );

    // in cvs
    addInput(createInput<MyPortInSmall>( Vec( 41, 244 ), module, OSC_WaveMorph_3::IN_FILTER ) );
    addInput(createInput<MyPortInSmall>( Vec( 77, 244 ), module, OSC_WaveMorph_3::IN_REZ ) );
    addInput(createInput<MyPortInSmall>( Vec( 162, 265 ), module, OSC_WaveMorph_3::IN_LEVEL ) );

    // level knob
    addParam(createParam<Knob_Blue2_56>( Vec( 143 , 200 ), module, OSC_WaveMorph_3::PARAM_LEVEL ) );

    // audio out
    addOutput(createOutput<MyPortOutSmall>( Vec( 203, 218 ), module, OSC_WaveMorph_3::OUTPUT_AUDIO ) );

    if( module )
        module->m_bInitialized = true;
}
};

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void OSC_WaveMorph_3::JsonParams( bool bTo, json_t *root) 
{
    JsonDataInt( bTo, "m_GraphData", root, (int*)m_GraphData, nCHANNELS * ENVELOPE_HANDLES );
    JsonDataBool( bTo, "m_bSolo", root, &m_bSolo, 1 );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *OSC_WaveMorph_3::dataToJson() 
{
	json_t *root = json_object();

    if( !root )
        return NULL;

    m_pEnvelope->getDataAll( (int*)m_GraphData );

    JsonParams( TOJSON, root );
    
	return root;
}

//-----------------------------------------------------
// Procedure:   fromJson
//
//-----------------------------------------------------
void OSC_WaveMorph_3::dataFromJson( json_t *root ) 
{
    JsonParams( FROMJSON, root );

    m_pEnvelope->setDataAll( (int*)m_GraphData );

    m_pButtonSolo->Set( m_bSolo );

    ChangeChannel( 0 );
}

//-----------------------------------------------------
// Procedure:   onReset
//
//-----------------------------------------------------
void OSC_WaveMorph_3::onReset()
{
    memset( m_GraphData, 0, sizeof( m_GraphData ) );

    m_pEnvelope->setDataAll( (int*)m_GraphData );

    ChangeChannel( 0 );
}

//-----------------------------------------------------
// Procedure:   onRandomize
//
//-----------------------------------------------------
void OSC_WaveMorph_3::onRandomize()
{
    int ch, i;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        for( i = 0; i < ENVELOPE_HANDLES; i++ )
            m_pEnvelope->setVal( ch, i, random::uniform() );
    }
}

//-----------------------------------------------------
// Procedure:   onRandomize
//
//-----------------------------------------------------
void OSC_WaveMorph_3::ChangeChannel( int ch )
{
    int i;

    if( ch < 0 || ch >= nCHANNELS )
        return;

    if( m_bCpy )
    {
        m_bCpy = false;
        m_pButtonCopy->Set( false );

        for( i = 0; i < ENVELOPE_HANDLES; i++ )
        {
            m_pEnvelope->setVal( ch, i, m_pEnvelope->m_EnvData[ m_CurrentChannel ].m_HandleVal[ i ] );
        }
    }

    m_CurrentChannel = ch;
    m_pButtonChSelect->Set( ch, true );
    m_pEnvelope->setView( ch );
}

//-----------------------------------------------------
// Procedure:   ChangeFilterCutoff
//
//-----------------------------------------------------
void OSC_WaveMorph_3::ChangeFilterCutoff( float cutfreq )
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

    f = 2.0 * (fx 
	    - (fx3 * 0.16666666666666666666666666666667) 
	    + (fx5 * 0.0083333333333333333333333333333333) 
	    - (fx7 * 0.0001984126984126984126984126984127));
}

//-----------------------------------------------------
// Procedure:   Filter
//
//-----------------------------------------------------
#define MULTI (0.33333333333333333333333333333333f)
void OSC_WaveMorph_3::Filter( float *In )
{
    float rez, hp1; 
    float input, out = 0.0f, lowpass, bandpass, highpass;

    if( (int)params[ PARAM_FILTER_MODE ].getValue() == 0 )
        return;

    rez = 1.0 - clamp( params[ PARAM_RES ].getValue() * ( inputs[ IN_REZ ].getNormalVoltage( CV_MAX10 ) / CV_MAX10 ), 0.0f, 1.0f );

    input = *In + 0.000000001;

    lp1         = lp1 + f * bp1; 
    hp1         = input - lp1 - rez * bp1; 
    bp1         = f * hp1 + bp1; 
    lowpass     = lp1; 
    highpass    = hp1; 
    bandpass    = bp1; 

    lp1         = lp1 + f * bp1; 
    hp1         = input - lp1 - rez * bp1; 
    bp1         = f * hp1 + bp1; 
    lowpass     = lowpass  + lp1; 
    highpass    = highpass + hp1; 
    bandpass    = bandpass + bp1; 

    input  = input - 0.000000001;

    lp1         = lp1 + f * bp1; 
    hp1         = input - lp1 - rez * bp1; 
    bp1         = f * hp1 + bp1; 

    lowpass  = (lowpass  + lp1) * MULTI; 
    highpass = (highpass + hp1) * MULTI; 
    bandpass = (bandpass + bp1) * MULTI;

    switch( (int)params[ PARAM_FILTER_MODE ].value )
    {
    case FILTER_LP:
        out = lowpass;
        break;
    case FILTER_HP:
        out  = highpass;
        break;
    case FILTER_BP:
        out  = bandpass;
        break;
    case FILTER_NT:
        out  = lowpass + highpass;
        break;
    default:
        return;
    }

    *In = out;
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void OSC_WaveMorph_3::process(const ProcessArgs &args)
{
    int ch;
    float fout = 0.0f, fmorph[ nCHANNELS ] = {}, fcv, cutoff, flevel;
    bool bChangeWave = false;

    if( !m_bInitialized )
        return;

    fcv = clamp( inputs[ INPUT_MORPHCV ].getNormalVoltage( 0.0f ) / CV_MAXn5, -1.0f, 1.0f );

    fmorph[ 1 ] = 1.0 - fabs( fcv );

    // left wave
    if( fcv <= 0.0f )
        fmorph[ 0 ] = -fcv;
    else if( fcv > 0.0f )
        fmorph[ 2 ] = fcv;

    bChangeWave = ( m_SchmittChangeWave.process( inputs[ IN_WAVE_CHANGE ].getNormalVoltage( 0.0f ) ) );

    if( bChangeWave && m_bSolo )
    {
        ch = m_CurrentChannel + 1;

        if( ch >= nCHANNELS )
            ch = 0;

        ChangeChannel( ch );
    }

    // process each channel
    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        m_pEnvelope->m_EnvData[ ch ].m_Clock.syncInc = 32.7032f * clamp( powf( 2.0f, clamp( inputs[ INPUT_VOCT ].getNormalVoltage( 4.0f ), 0.0f, VOCT_MAX ) ), 0.0f, 4186.01f );

        if( m_bSolo && ch == m_CurrentChannel )
            fout += m_pEnvelope->procStep( ch, false, false );
        else if( !m_bSolo )
            fout += m_pEnvelope->procStep( ch, false, false ) * fmorph[ ch ];
    }

    cutoff = clamp( params[ PARAM_CUTOFF ].getValue() * ( inputs[ IN_FILTER ].getNormalVoltage( CV_MAX10 ) / CV_MAX10 ), 0.0f, 1.0f );
    ChangeFilterCutoff( cutoff );

    flevel = clamp( params[ PARAM_LEVEL ].getValue() * ( inputs[ IN_LEVEL ].getNormalVoltage( CV_MAX10 ) / CV_MAX10 ), 0.0f, 1.0f );
    fout *= flevel * AUDIO_MAX;

    Filter( &fout );

    outputs[ OUTPUT_AUDIO ].setVoltage( fout );
}

Model *modelOSC_WaveMorph_3 = createModel<OSC_WaveMorph_3, OSC_WaveMorph_3_Widget>( "OSC_WaveMorph_3" );
