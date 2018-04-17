#include "mscHack.hpp"

#define nCHANNELS 32
#define nINCHANNELS 24
#define nGROUPS   4
#define nAUX      4
#define nEQ       3

#define FADE_MULT (0.0005f)
#define CUTOFF (0.025f)

#define L 0
#define R 1

#define MUTE_FADE_STATE_IDLE 0
#define MUTE_FADE_STATE_INC  1
#define MUTE_FADE_STATE_DEC  2

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Mixer_24_4_4 : Module 
{
	enum ParamIds 
    {
        nPARAMS
    };

	enum InputIds 
    {
        IN_LEFT,
        IN_RIGHT                = IN_LEFT + nCHANNELS,
        nINPUTS                 = IN_RIGHT + nCHANNELS
	};

	enum OutputIds 
    {
		OUT_MAINL,
        OUT_MAINR,

        OUT_AUXL,
        OUT_AUXR              = OUT_AUXL + nAUX,

        nOUTPUTS              = OUT_AUXR + nAUX
	};

	enum LightIds 
    {
        nLIGHTS
	};

    bool            m_bInitialized = false;
    CLog            lg;

    // Contructor
	Mixer_24_4_4() : Module(nPARAMS, nINPUTS, nOUTPUTS, nLIGHTS){}

    // knobs
    int             m_iKnobLvl[ nCHANNELS ] = {};
    int             m_iKnobPan[ nCHANNELS ] = {};
    int             m_iKnobEq[ nCHANNELS ][ nEQ ] = {};
    int             m_iKnobAux[ nCHANNELS ][ nAUX ] = {};
    int             m_iKnobOut = 0;

    // mute/solo
    bool            m_bMuteStates[ nCHANNELS ] = {};
    float           m_fMuteFade[ nCHANNELS ] = {};
    int             m_FadeState[ nCHANNELS ] = {MUTE_FADE_STATE_IDLE};
    bool            m_bSoloStates[ nCHANNELS ] = {};

    // processing
    bool            m_bMono[ nCHANNELS ];
    float           m_fSubMix[ nGROUPS ][ 3 ] = {};

    // routing
    int             m_iRouteGroup[ nCHANNELS ] = {nGROUPS};

    // LED Meters
    LEDMeterWidget  *m_pLEDMeterChannel[ nCHANNELS ][ 2 ] ={};
    LEDMeterWidget  *m_pLEDMeterMain[ 2 ] ={};

    // EQ Rez
    float           lp1[ nCHANNELS ][ 2 ] = {}, bp1[ nCHANNELS ][ 2 ] = {}; 
    float           m_hpIn[ nCHANNELS ];
    float           m_lpIn[ nCHANNELS ];
    float           m_mpIn[ nCHANNELS ];
    float           m_rezIn[ nCHANNELS ] = {0};
    float           m_Freq;

    // buttons
    MyLEDButton            *m_pButtonChannelMute[ nCHANNELS ] = {};
    MyLEDButton            *m_pButtonChannelSolo[ nCHANNELS ] = {};

    MyLEDButtonStrip       *m_pMultiButtonRoute[ nINCHANNELS ] = {0};

    // knobs
    MySimpleKnob           *m_pKnobLvl[ nCHANNELS ] = {};
    MySimpleKnob           *m_pKnobPan[ nCHANNELS ] = {};
    MySimpleKnob           *m_pKnobEq[ nCHANNELS ][ nEQ ] = {};
    MySimpleKnob           *m_pKnobAux[ nCHANNELS ][ nAUX ] = {};
    MySimpleKnob           *m_pKnobOut = NULL;

    // Overrides 
	void    step() override;
    void    JsonParams( bool bTo, json_t *root);
    json_t* toJson() override;
    void    fromJson(json_t *rootJ) override;
    void    onRandomize() override;
    void    onReset() override;
    void    onCreate() override;
    void    onDelete() override;

    void ProcessMuteSolo( int channel, bool bMute, bool bGroup );
    void ProcessEQ( int ch, float *pL, float *pR );
    void SetControls( int ch );
};

//-----------------------------------------------------
// Procedure: Knob callbacks  
//
//-----------------------------------------------------
void Knob_Level_Callback( void *pClass, int ch, int id, float fval )
{
    Mixer_24_4_4 *module = (Mixer_24_4_4*)pClass;

    if( !module )
        return;

    module->m_iKnobLvl[ ch ] = (int)( fval * 100.0f );
}

void Knob_Pan_Callback( void *pClass, int ch, int id, float fval )
{
    Mixer_24_4_4 *module = (Mixer_24_4_4*)pClass;

    if( !module )
        return;

    module->m_iKnobPan[ ch ] = (int)( fval * 100.0f );
}

void Knob_Eq_Callback( void *pClass, int ch, int id, float fval )
{
    Mixer_24_4_4 *module = (Mixer_24_4_4*)pClass;

    if( !module )
        return;

    module->m_iKnobEq[ ch ][ id ] = (int)( fval * 100.0f );
}

void Knob_Aux_Callback( void *pClass, int ch, int id, float fval )
{
    Mixer_24_4_4 *module = (Mixer_24_4_4*)pClass;

    if( !module )
        return;

    module->m_iKnobAux[ ch ][ id ] = (int)( fval * 100.0f );
}

void Knob_MainLvl_Callback( void *pClass, int ch, int id, float fval )
{
    Mixer_24_4_4 *module = (Mixer_24_4_4*)pClass;

    if( !module )
        return;

    module->m_iKnobOut = (int)( fval * 100.0f );
}

//-----------------------------------------------------
// MyLEDButton_ChSolo
//-----------------------------------------------------
void Button_ChSolo( void *pClass, int id, bool bOn ) 
{
    Mixer_24_4_4 *mymodule;
    mymodule = (Mixer_24_4_4*)pClass;
    mymodule->ProcessMuteSolo( id, false, false );
}

//-----------------------------------------------------
// MyLEDButton_ChMute
//-----------------------------------------------------
void Button_ChMute( void *pClass, int id, bool bOn ) 
{
    Mixer_24_4_4 *mymodule;
    mymodule = (Mixer_24_4_4*)pClass;
    mymodule->ProcessMuteSolo( id, true, false );
}

//-----------------------------------------------------
// Procedure:   RouteCallback
//-----------------------------------------------------
void RouteCallback( void *pClass, int id, int nbutton, bool bOn ) 
{
    Mixer_24_4_4 *mymodule;
    mymodule = (Mixer_24_4_4*)pClass;

    if( mymodule )
    {
        mymodule->m_iRouteGroup[ id ] = nbutton;
    }
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct Mixer_24_4_4_Widget : ModuleWidget {
	Mixer_24_4_4_Widget( Mixer_24_4_4 *module );
};

Mixer_24_4_4_Widget::Mixer_24_4_4_Widget( Mixer_24_4_4 *module ) : ModuleWidget(module) 
{
    float fx, fx2, fx3, fx5, fx7;
    int ch, eq, aux, x, y, x2, y2;
    bool bGroup, bAux, bNormal;

	box.size = Vec( 15*54, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Mixer_24_4_4.svg")));
		addChild(panel);
	}

    //module->lg.Open("Mixer_24_4_4.txt");

    x = 47;
    y = 13;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        bGroup = false;
        bAux   = false;
        bNormal= false;

        // groups
        if( ch < nINCHANNELS )
            bNormal = true;
        else if( ch >= nINCHANNELS && ch < ( nINCHANNELS + nGROUPS) )
            bGroup = true;
        else
            bAux = true;

        x2 = x + 3;
        y2 = y + 8;

        // inputs
        if( !bGroup )
        {
            addInput( Port::create<MyPortInSmall>( Vec( x2, y2 ), Port::INPUT, module, Mixer_24_4_4::IN_LEFT + ch ) ); y2 += 25;
            addInput( Port::create<MyPortInSmall>( Vec( x2, y2 ), Port::INPUT, module, Mixer_24_4_4::IN_RIGHT + ch ) );
        }
        else
            y2 += 25;

        x2 = x + 4;
        y2 += 23;

        // level/pan
        module->m_pKnobLvl[ ch ] = new MySimpleKnob( module, (float)x2, (float)y2, 15, DWRGB( 1, 47, 94 ), DWRGB( 255, 255, 255 ), ch, 0, 0, Knob_Level_Callback, 0.0f, 1.0f, 0.5f ); y2 += 20;
        addChild( module->m_pKnobLvl[ ch ] );

        module->m_pKnobPan[ ch ] = new MySimpleKnob( module, (float)x2, (float)y2, 15, DWRGB( 133, 104, 3 ), DWRGB( 255, 255, 255 ), ch, 0, 0, Knob_Pan_Callback, 0.0f, 1.0f, 0.0f );
        addChild( module->m_pKnobPan[ ch ] );

        y2 += 22;

        // EQ
        for( eq = 0; eq < nEQ; eq++ )
        {
            module->m_pKnobEq[ ch ][ eq ] = new MySimpleKnob( module, (float)x2, (float)y2, 15, DWRGB( 27, 30, 11 ), DWRGB( 255, 255, 255 ), ch, eq, 0, Knob_Eq_Callback, 0.0f, 1.0f, 0.0f ); y2 += 20;
            addChild( module->m_pKnobEq[ ch ][ eq ] );
        }

        y2 += 2;

        // aux sends
        if( !bAux )
        {
            for( aux = 0; aux < nAUX; aux++ )
            {
                module->m_pKnobAux[ ch ][ aux ] = new MySimpleKnob( module, (float)x2, (float)y2, 15, DWRGB( 163, 78, 77 ), DWRGB( 255, 255, 255 ), ch, aux, 0, Knob_Aux_Callback, 0.0f, 1.0f, 0.0f ); y2 += 20;
                addChild( module->m_pKnobAux[ ch ][ aux ] );
            }
        }
        else
        {
            module->m_pKnobAux[ ch ][ 0 ] = new MySimpleKnob( module, (float)x2, (float)y2, 15, DWRGB( 180, 180, 180 ), DWRGB( 0, 0, 0 ), ch, 0, 0, Knob_Aux_Callback, 0.0f, 1.0f, 0.0f );
            addChild( module->m_pKnobAux[ ch ][ 0 ] );
        }

        y2 = y + 242;

        // mute/solo
        if( !bAux )
        {
            module->m_pButtonChannelMute[ ch ] = new MyLEDButton( x + 3, y2, 8, 8, 6.0f, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, Button_ChMute );
	        addChild( module->m_pButtonChannelMute[ ch ] );

            module->m_pButtonChannelSolo[ ch ] = new MyLEDButton( x + 12, y2, 8, 8, 6.0f, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, Button_ChSolo );
	        addChild( module->m_pButtonChannelSolo[ ch ] );
        }
        // mute only
        else
        {
            module->m_pButtonChannelMute[ ch ] = new MyLEDButton( x + 7, y2, 8, 8, 6.0f, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, Button_ChMute );
	        addChild( module->m_pButtonChannelMute[ ch ] );
        }

        // VUMeter
        module->m_pLEDMeterChannel[ ch ][ 0 ] = new LEDMeterWidget( x + 7, y + 260, 4, 1, 1, true );
        addChild( module->m_pLEDMeterChannel[ ch ][ 0 ] );
        module->m_pLEDMeterChannel[ ch ][ 1 ] = new LEDMeterWidget( x + 12, y + 260, 4, 1, 1, true );
        addChild( module->m_pLEDMeterChannel[ ch ][ 1 ] );

        x2 = x + 8;
        y2 = y + 290;

        // Group Route
        if( bNormal )
        {
            module->m_pMultiButtonRoute[ ch ] = new MyLEDButtonStrip( x2, y2, 8, 8, 1, 6.0f, 5, true, DWRGB( 180, 180, 180 ), DWRGB( 128, 128, 200 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, ch, module, RouteCallback );
	        addChild( module->m_pMultiButtonRoute[ ch ] );
        }

        x += 23;
    }

    // output
    module->m_pKnobOut = new MySimpleKnob( module, (float)695, (float)300, 56, DWRGB( 1, 47, 94 ), DWRGB( 255, 255, 255 ), ch, aux, 0, Knob_Aux_Callback, 0.0f, 1.0f, 0.0f );
    addChild( module->m_pKnobOut );

    module->m_pLEDMeterMain[ 0 ] = new LEDMeterWidget( 759, 302, 5, 3, 2, true );
    addChild( module->m_pLEDMeterMain[ 0 ] );
    module->m_pLEDMeterMain[ 1 ] = new LEDMeterWidget( 759 + 7, 302, 5, 3, 2, true );
    addChild( module->m_pLEDMeterMain[ 1 ] );

    addOutput(Port::create<MyPortOutSmall>( Vec( 779, 307 ), Port::OUTPUT, module, Mixer_24_4_4::OUT_MAINL ) );
    addOutput(Port::create<MyPortOutSmall>( Vec( 779, 332 ), Port::OUTPUT, module, Mixer_24_4_4::OUT_MAINR ) );

    //screw
	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365))); 
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    // calculate eq rez freq
    fx = 3.141592 * (CUTOFF * 0.026315789473684210526315789473684) * 2 * 3.141592; 
    fx2 = fx*fx;
    fx3 = fx2*fx; 
    fx5 = fx3*fx2; 
    fx7 = fx5*fx2;

    module->m_Freq = 2.0 * (fx 
	    - (fx3 * 0.16666666666666666666666666666667) 
	    + (fx5 * 0.0083333333333333333333333333333333) 
	    - (fx7 * 0.0001984126984126984126984126984127));

    module->m_bInitialized = true;
    module->onReset();
}

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void Mixer_24_4_4::JsonParams( bool bTo, json_t *root) 
{
    JsonDataBool( bTo, "m_bMuteStates", root, m_bMuteStates, 32 );
    JsonDataBool( bTo, "m_bSoloStates", root, m_bSoloStates, 32 );
    JsonDataInt( bTo, "m_iKnobLvl", root, &m_iKnobLvl[ 0 ], nCHANNELS );
    JsonDataInt( bTo, "m_iKnobPan", root, &m_iKnobPan[ 0 ], nCHANNELS );
    JsonDataInt( bTo, "m_iKnobEq", root, &m_iKnobEq[ 0 ][ 0 ], nCHANNELS * nEQ );
    JsonDataInt( bTo, "m_iKnobAux", root, &m_iKnobAux[ 0 ][ 0 ], nCHANNELS * nAUX );
    JsonDataInt( bTo, "m_iRouteGroup", root, &m_iRouteGroup[ 0 ], nCHANNELS );
    JsonDataInt( bTo, "m_iKnobOut", root, &m_iKnobOut, 1 );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *Mixer_24_4_4::toJson() 
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
void Mixer_24_4_4::fromJson( json_t *root ) 
{
    int ch;
    bool bSolo = false;

    JsonParams( FROMJSON, root );

    // anybody soloing?
    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        if( m_bSoloStates[ ch ] )
            bSolo = true;
    }

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        if( bSolo )
        {
            // only open soloing channels
            if( m_bSoloStates[ ch ] )
                m_fMuteFade[ ch ] = 1.0;
            else
                m_fMuteFade[ ch ] = 0.0;
        }
        else
        {
            // nobody is soloing so just open the non muted channels
            m_fMuteFade[ ch ] = m_bMuteStates[ ch ] ? 0.0: 1.0;
        }

        SetControls( ch );
    }
}

//-----------------------------------------------------
// Procedure:   SetControls
//
//-----------------------------------------------------
void Mixer_24_4_4::SetControls( int ch )
{
    int aux, eq;

    if( !m_bInitialized || ch >= nCHANNELS || ch < 0 )
        return;

    //lg.f("Here\n");

    if( m_pButtonChannelMute[ ch ] )
        m_pButtonChannelMute[ ch ]->Set( m_bMuteStates[ ch ] );

    if( m_pButtonChannelSolo[ ch ] )
        m_pButtonChannelSolo[ ch ]->Set( m_bSoloStates[ ch ] );

    if( ch < nINCHANNELS && m_pMultiButtonRoute[ ch ] )
        m_pMultiButtonRoute[ ch ]->Set( m_iRouteGroup[ ch ], true );

    if( m_pKnobLvl[ ch ] )
        m_pKnobLvl[ ch ]->setVal( (float)m_iKnobLvl[ ch ] / 100.0f );

    if( m_pKnobPan[ ch ] )
        m_pKnobPan[ ch ]->setVal( (float)m_iKnobPan[ ch ] / 100.0f );

    if( m_pKnobOut )
        m_pKnobOut->setVal( (float)m_iKnobOut / 100.0f );

    for( eq = 0; eq < nEQ; eq++ )
    {
        if( m_pKnobEq[ ch ][ eq ] )
            m_pKnobEq[ ch ][ eq ]->setVal( (float)m_iKnobEq[ ch ][ eq ] / 100.0f );
    }

    for( aux = 0; aux < nAUX; aux++ )
    {
        if( m_pKnobAux[ ch ][ aux ] )
            m_pKnobAux[ ch ][ aux ]->setVal( (float)m_iKnobAux[ ch ][ aux ] / 100.0f );
    }
}

//-----------------------------------------------------
// Procedure:   onCreate
//
//-----------------------------------------------------
void Mixer_24_4_4::onCreate()
{
}

//-----------------------------------------------------
// Procedure:   onDelete
//
//-----------------------------------------------------
void Mixer_24_4_4::onDelete()
{
}

//-----------------------------------------------------
// Procedure:   onReset
//
//-----------------------------------------------------
void Mixer_24_4_4::onReset()
{
    int ch, aux, eq;

    if( !m_bInitialized )
        return;

    m_iKnobOut = 0;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        m_iKnobLvl[ ch ] = 0;
        m_iKnobPan[ ch ] = 0;

        m_FadeState[ ch ] = MUTE_FADE_STATE_IDLE;

        m_bMuteStates[ ch ] = false;
        m_bSoloStates[ ch ] = false;
        m_fMuteFade[ ch ] = 1.0f;
        m_iRouteGroup[ ch ] = nGROUPS;

        for( eq = 0; eq < nEQ; eq++ )
            m_iKnobEq[ ch ][ eq ] = 0;

        for( aux = 0; aux < nAUX; aux++ )
            m_iKnobAux[ ch ][ aux ] = 0;

        SetControls( ch );
    }
}

//-----------------------------------------------------
// Procedure:   onRandomize
//
//-----------------------------------------------------
void Mixer_24_4_4::onRandomize()
{
}

//-----------------------------------------------------
// Procedure:   ProcessEQ
//
//-----------------------------------------------------
#define MULTI (0.33333333333333333333333333333333f)
void Mixer_24_4_4::ProcessEQ( int ch, float *pL, float *pR )
{
    float rez, hp1; 
    float input[ 2 ], out[ 2 ], lowpass, bandpass, highpass;

    input[ L ] = *pL / AUDIO_MAX;
    input[ R ] = *pR / AUDIO_MAX;

    rez = 1.00;

    // do left and right channels
    for( int i = 0; i < 2; i++ )
    {
        input[ i ] = input[ i ] + 0.000000001;

        lp1[ ch ][ i ] = lp1[ ch ][ i ] + m_Freq * bp1[ ch ][ i ]; 
        hp1 = input[ i ] - lp1[ ch ][ i ] - rez * bp1[ ch ][ i ]; 
        bp1[ ch ][ i ] = m_Freq * hp1 + bp1[ ch ][ i ]; 
        lowpass  = lp1[ ch ][ i ]; 
        highpass = hp1; 
        bandpass = bp1[ ch ][ i ]; 

        lp1[ ch ][ i ] = lp1[ ch ][ i ] + m_Freq * bp1[ ch ][ i ]; 
        hp1 = input[ i ] - lp1[ ch ][ i ] - rez * bp1[ ch ][ i ]; 
        bp1[ ch ][ i ] = m_Freq * hp1 + bp1[ ch ][ i ]; 
        lowpass  = lowpass  + lp1[ ch ][ i ]; 
        highpass = highpass + hp1; 
        bandpass = bandpass + bp1[ ch ][ i ]; 

        input[ i ] = input[ i ] - 0.000000001;
        lp1[ ch ][ i ] = lp1[ ch ][ i ] + m_Freq * bp1[ ch ][ i ]; 
        hp1 = input[ i ] - lp1[ ch ][ i ] - rez * bp1[ ch ][ i ]; 
        bp1[ ch ][ i ] = m_Freq * hp1 + bp1[ ch ][ i ]; 

        lowpass  = (lowpass  + lp1[ ch ][ i ]) * MULTI; 
        highpass = (highpass + hp1) * MULTI; 
        bandpass = (bandpass + bp1[ ch ][ i ]) * MULTI;

        out[ i ] = ( highpass * m_hpIn[ ch ] ) + ( lowpass * m_lpIn[ ch ] ) + ( bandpass * m_mpIn[ ch ] );
    }

    *pL = clamp( out[ L ] * AUDIO_MAX, -AUDIO_MAX, AUDIO_MAX );
    *pR = clamp( out[ R ] * AUDIO_MAX, -AUDIO_MAX, AUDIO_MAX );
}

//-----------------------------------------------------
// Procedure:   ProcessMuteSolo
//
//-----------------------------------------------------
void Mixer_24_4_4::ProcessMuteSolo( int index, bool bMute, bool bGroup )
{
    int i;
    bool bSoloEnabled = false, bSoloOff = false;

    if( bMute )
    {
        m_bMuteStates[ index ] = !m_bMuteStates[ index ];

        // turn solo off
        if( m_bSoloStates[ index ] )
        {
            bSoloOff = true;
            m_bSoloStates[ index ] = false;
            m_pButtonChannelSolo[ index ]->Set( false );
        }

        // if mute is off then set volume
        if( m_bMuteStates[ index ] )
        {
            m_pButtonChannelMute[ index ]->Set( true );
            m_FadeState[ index ] = MUTE_FADE_STATE_DEC;
        }
        else
        {
            m_pButtonChannelMute[ index ]->Set( false );
            m_FadeState[ index ] = MUTE_FADE_STATE_INC;
        }
    }
    else
    {
        m_bSoloStates[ index ] = !m_bSoloStates[ index ];

        // turn mute off
        if( m_bMuteStates[ index ] )
        {
            m_bMuteStates[ index ] = false;
            m_pButtonChannelMute[ index ]->Set( false );
        }

        // toggle solo
        if( !m_bSoloStates[ index ] )
        {
            bSoloOff = true;
            m_pButtonChannelSolo[ index ]->Set( false );
        }
        else
        {
            m_pButtonChannelSolo[ index ]->Set( true );
        }
    }

    // is a track soloing?
    for( i = 0; i < nCHANNELS; i++ )
    {
        if( m_bSoloStates[ i ] )
        {
            bSoloEnabled = true;
            break;
        }
    }

    if( bSoloEnabled )
    {
        // process solo
        for( i = 0; i < nCHANNELS; i++ )
        {
            // shut down volume of all not in solo
            if( !m_bSoloStates[ i ] )
            {
                m_FadeState[ i ] = MUTE_FADE_STATE_DEC;
            }
            else
            {
                m_FadeState[ i ] = MUTE_FADE_STATE_INC;
            }
        }
    }
    // nobody soloing and just turned solo off then enable all channels that aren't muted
    else if( bSoloOff )
    {
        // process solo
        for( i = 0; i < nCHANNELS; i++ )
        {
            // bring back if not muted
            if( !m_bMuteStates[ i ] )
            {
                m_FadeState[ i ] = MUTE_FADE_STATE_INC;
            }
        }
    }
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void Mixer_24_4_4::step() 
{
    int ch, aux;
    float fMixOutL = 0.0, fMixOutR = 0.0, inL, inR;
    float auxL[ nAUX ] = {}, auxR[ nAUX ] = {};
    static bool  bChWasActive[ nCHANNELS ]= {};

    //if( !m_bInitialized )
        return;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        bChWasActive[ ch ] = true;

        inL = 0.0f;
        inR = 0.0f;

        // is this channel active?
        if( inputs[ IN_LEFT + ch ].active || inputs[ IN_RIGHT ].active )
        {
            // check right channel first for possible mono
            if( inputs[ IN_RIGHT + ch ].active )
            {
                inR = inputs[ IN_RIGHT + ch ].value * m_pKnobLvl[ ch ]->m_fVal;
                m_bMono[ ch ] = false;
            }
            else
                m_bMono[ ch ] = true;

            // left channel
            if( inputs[ IN_LEFT + ch ].active )
            {
                inL = inputs[ IN_LEFT + ch ].value * m_pKnobLvl[ ch ]->m_fVal;

                if( m_bMono[ ch ] )
                    inR = inL;
            }

            if( m_FadeState[ ch ] == MUTE_FADE_STATE_DEC )
            {
                m_fMuteFade[ ch ] -= FADE_MULT;

                if( m_fMuteFade[ ch ] <= 0.0 )
                {
                    m_fMuteFade[ ch ] = 0.0;
                    m_FadeState[ ch ] = MUTE_FADE_STATE_IDLE;
                }
            }
            else if( m_FadeState[ ch ] == MUTE_FADE_STATE_INC )
            {
                m_fMuteFade[ ch ] += FADE_MULT;

                if( m_fMuteFade[ ch ] >= 1.0 )
                {
                    m_fMuteFade[ ch ] = 1.0;
                    m_FadeState[ ch ] = MUTE_FADE_STATE_IDLE;
                }
            }

            ProcessEQ( ch, &inL, &inR );

            inL *= m_fMuteFade[ ch ];
            inR *= m_fMuteFade[ ch ];

            if( m_pKnobPan[ ch ]->m_fVal <= 0.0 )
                inR *= ( 1.0 + m_pKnobPan[ ch ]->m_fVal );
            else
                inL *= ( 1.0 - m_pKnobPan[ ch ]->m_fVal );

            // put output to aux if not pre fader
            for ( aux = 0; aux < nAUX; aux++ )
            {
                auxL[ aux ] += inL * m_pKnobAux[ ch ][ aux ]->m_fVal;
                auxR[ aux ] += inR * m_pKnobAux[ ch ][ aux ]->m_fVal;
            }

            fMixOutL += inL;
            fMixOutR += inR;

            if( m_pLEDMeterChannel[ ch ][ 0 ] )
                m_pLEDMeterChannel[ ch ][ 0 ]->Process( inL / AUDIO_MAX );
            if( m_pLEDMeterChannel[ ch ][ 1 ] )
                m_pLEDMeterChannel[ ch ][ 1 ]->Process( inR / AUDIO_MAX);
        }
        else if( bChWasActive[ ch ] )
        {
            bChWasActive[ ch ] = false;

            if( m_pLEDMeterChannel[ ch ][ 0 ] )
                m_pLEDMeterChannel[ ch ][ 0 ]->Process( 0.0f );
            if( m_pLEDMeterChannel[ ch ][ 1 ] )
                m_pLEDMeterChannel[ ch ][ 1 ]->Process( 0.0f );
        }
    }

    // apply main level
    fMixOutL = clamp( fMixOutL * m_pKnobOut->m_fVal, -AUDIO_MAX, AUDIO_MAX );
    fMixOutR = clamp( fMixOutR * m_pKnobOut->m_fVal, -AUDIO_MAX, AUDIO_MAX );

    // update main VUMeters
    if( m_pLEDMeterMain[ 0 ] )
        m_pLEDMeterMain[ 0 ]->Process( fMixOutL / AUDIO_MAX );
    if( m_pLEDMeterMain[ 1 ] )
        m_pLEDMeterMain[ 1 ]->Process( fMixOutR / AUDIO_MAX );

    // put aux output
    for ( aux = 0; aux < nAUX; aux++ )
    {
        outputs[ OUT_AUXL + aux ].value = clamp( auxL[ aux ] * m_pKnobAux[ ch ][ aux ]->m_fVal, -AUDIO_MAX, AUDIO_MAX );
        outputs[ OUT_AUXR + aux ].value = clamp( auxR[ aux ] * m_pKnobAux[ ch ][ aux ]->m_fVal, -AUDIO_MAX, AUDIO_MAX );
    }

    outputs[ OUT_MAINL ].value = fMixOutL;
    outputs[ OUT_MAINR ].value = fMixOutR;
}

Model *modelMix_24_4_4 = Model::create<Mixer_24_4_4, Mixer_24_4_4_Widget>( "mscHack", "Mix_24_4_4", "MIXER 24ch (In Development)", MIXER_TAG, EQUALIZER_TAG, PANNING_TAG, AMPLIFIER_TAG, MULTIPLE_TAG );