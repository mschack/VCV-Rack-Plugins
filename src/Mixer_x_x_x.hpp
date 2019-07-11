#include "mscHack.hpp"

#define nAUX      4
#define nEQ       3
#define MAX_GROUPS 4

#define FADE_MULT (0.0005f)
#define CUTOFF (0.025f)

#define L 0
#define R 1

#define MUTE_FADE_STATE_IDLE 0
#define MUTE_FADE_STATE_INC  1
#define MUTE_FADE_STATE_DEC  2

#define STARTUP_FADE_INC (0.00001f)
#define START_DELAY_COUNT 40000
//#define CLAMPOUT (12.0f)

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Mixer_ : Module
{
	enum ParamIds 
    {
        PARAM_LEVEL_OUT,
        PARAM_CHLVL,
        PARAM_CHPAN             = PARAM_CHLVL + nCHANNELS,
        PARAM_CHEQHI            = PARAM_CHPAN + nCHANNELS,
        PARAM_CHEQMD            = PARAM_CHEQHI + nCHANNELS,
        PARAM_CHEQLO            = PARAM_CHEQMD + nCHANNELS,
        PARAM_CHAUX             = PARAM_CHEQLO + nCHANNELS,
        PARAM_AUXLVL            = PARAM_CHAUX + ( (nCHANNELS - 4) * nAUX ),
        nPARAMS                 = PARAM_AUXLVL + nAUX,
    };

	enum InputIds 
    {
        IN_LEFT,
        IN_RIGHT                = IN_LEFT + nCHANNELS,
        IN_LEVELCV              = IN_RIGHT + nCHANNELS,
        IN_PANCV                = IN_LEVELCV + nINCHANNELS,

#if nGROUPS > 0
        IN_FADEX                = IN_PANCV + nINCHANNELS,
        IN_FADEY,
        nINPUTS
#else
        nINPUTS                 = IN_PANCV + nINCHANNELS
#endif
	};

	enum OutputIds 
    {
		OUT_MAINL,
        OUT_MAINR,

#if nGROUPS > 0
        OUT_GRPL,
        OUT_GRPR                = OUT_GRPL + nGROUPS,
        OUT_AUXL                = OUT_GRPR + nGROUPS,
#else
        OUT_AUXL,
#endif
        OUT_AUXR                = OUT_AUXL + nAUX,

        nOUTPUTS                = OUT_AUXR + nAUX
	};

	enum LightIds 
    {
        nLIGHTS
	};

    bool            m_bInitialized = false;
    float           m_StartupFade  = 0.0f;
    int             m_StartCount = START_DELAY_COUNT;

    // Contructor
    Mixer_()
    {
        int pos;
        char strVal[ 30 ] = {};
        char strType[ 30 ] = {};

        config(nPARAMS, nINPUTS, nOUTPUTS, nLIGHTS);


        configParam( PARAM_LEVEL_OUT, 0.0, 1.0, 0.5, "Main Level" );


        for( int i = 0; i < nCHANNELS; i++ )
        {
            if( i < nINCHANNELS )
            {
                sprintf( strType, "Ch" );
                pos = i + 1;
            }
            else if( nGROUPS && i < (nINCHANNELS + nGROUPS) )
            {
                sprintf( strType, "Gr" );
                pos = i - nINCHANNELS + 1;
            }
            else
            {
                sprintf( strType, "AUX" );
                pos = i - ( nINCHANNELS + nGROUPS ) + 1;
            }

            sprintf( strVal, "%s%d. Level", strType, pos );
            configParam( PARAM_CHLVL + i, 0.0, 1.0, 0.0, strVal );

            sprintf( strVal, "%s%d. Pan", strType, pos );
            configParam( PARAM_CHPAN + i, -1.0, 1.0, 0.0, strVal );

            sprintf( strVal, "%s%d. EQ High", strType, pos );
            configParam( PARAM_CHEQHI + i, 0.0, 1.0, 0.5, strVal );

            sprintf( strVal, "%s%d. EQ Mid", strType, pos );
            configParam( PARAM_CHEQMD + i, 0.0, 1.0, 0.5, strVal );

            sprintf( strVal, "%s%d. EQ Low", strType, pos );
            configParam( PARAM_CHEQLO + i, 0.0, 1.0, 0.5, strVal );

            if( i < ( nINCHANNELS + nGROUPS ) )
            {
                sprintf( strVal, "%s%d. AUX 1 Level", strType, pos );
                configParam( PARAM_CHAUX + (i * 4) + 0, 0.0, 1.0, 0.0, strVal );

                sprintf( strVal, "%s%d. AUX 2 Level", strType, pos );
                configParam( PARAM_CHAUX + (i * 4) + 1, 0.0, 1.0, 0.0, strVal );

                sprintf( strVal, "%s%d. AUX 3 Level", strType, pos );
                configParam( PARAM_CHAUX + (i * 4) + 2, 0.0, 1.0, 0.0, strVal );

                sprintf( strVal, "%s%d. AUX 4 Level", strType, pos );
                configParam( PARAM_CHAUX + (i * 4) + 3, 0.0, 1.0, 0.0, strVal );
            }
        }

        configParam( PARAM_AUXLVL + 0, 0.0, 1.0, 0.0, "AUX1 Send Level" );
        configParam( PARAM_AUXLVL + 1, 0.0, 1.0, 0.0, "AUX2 Send Level" );
        configParam( PARAM_AUXLVL + 2, 0.0, 1.0, 0.0, "AUX3 Send Level" );
        configParam( PARAM_AUXLVL + 3, 0.0, 1.0, 0.0, "AUX4 Send Level" );
    }

    // mute/solo
    bool            m_bMuteStates[ nCHANNELS ] = {};
    float           m_fMuteFade[ nCHANNELS ] = {};
    int             m_FadeState[ nCHANNELS ] = {MUTE_FADE_STATE_IDLE};
    bool            m_bSoloStates[ nCHANNELS ] = {};
    bool            m_bPreFader[ nINCHANNELS + nGROUPS ] = {};

    // processing
    bool            m_bMono[ nCHANNELS ];

    // LED Meters
    LEDMeterWidget  *m_pLEDMeterChannel[ nCHANNELS ][ 2 ] ={};
    LEDMeterWidget  *m_pLEDMeterMain[ 2 ] ={};

    // EQ Rez
    float           lp1[ nCHANNELS ][ 2 ] = {}, bp1[ nCHANNELS ][ 2 ] = {}; 
    float           m_hpIn[ nCHANNELS ];
    float           m_lpIn[ nCHANNELS ];
    float           m_mpIn[ nCHANNELS ];
    float           m_Freq;

    // buttons
    MyLEDButton    *m_pButtonChannelMute[ nCHANNELS ] = {};
    MyLEDButton    *m_pButtonChannelSolo[ nCHANNELS - nAUX ] = {};
    MyLEDButton    *m_pButtonPreFader[ nCHANNELS ] = {};

    // routing

#if nGROUPS > 0
    int                     m_iRouteGroup[ nINCHANNELS ] = {nGROUPS};
    MyLEDButtonStrip       *m_pMultiButtonRoute[ nINCHANNELS ] = {0};
#endif

    // menu
    bool                    m_bGroupPreMute = true;
    bool                    m_bGainLevelx2 = false;
    bool                    m_bAuxIgnoreSolo = false;

    // Overrides 
	void   process(const ProcessArgs &args) override;
    void    JsonParams( bool bTo, json_t *root);
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onReset() override;
    
    void ProcessMuteSolo( int channel, bool bMute, bool bOn );
    void ProcessEQ( int ch, float *pL, float *pR );
    void SetControls( int ch );

    //-----------------------------------------------------
    // MyEQHi_Knob
    //-----------------------------------------------------
    struct MyEQHi_Knob : Knob_Green1_15
    {
        Mixer_ *mymodule;
        int param;

        void onChange( const event::Change &e ) override 
        {
            mymodule = (Mixer_*)paramQuantity->module;

            if( mymodule )
            {
                param = paramQuantity->paramId - Mixer_::PARAM_CHEQHI;

                mymodule->m_hpIn[ param ] = paramQuantity->getValue(); 
            }

		    RoundKnob::onChange( e );
	    }
    };

    //-----------------------------------------------------
    // MyEQHi_Knob
    //-----------------------------------------------------
    struct MyEQMid_Knob : Knob_Green1_15
    {
        Mixer_ *mymodule;
        int param;

        void onChange( const event::Change &e ) override 
        {
            mymodule = (Mixer_*)paramQuantity->module;

            if( mymodule )
            {
                param = paramQuantity->paramId - Mixer_::PARAM_CHEQMD;
                mymodule->m_mpIn[ param ] = paramQuantity->getValue(); 
            }

		    RoundKnob::onChange( e );
	    }
    };

    //-----------------------------------------------------
    // MyEQHi_Knob
    //-----------------------------------------------------
    struct MyEQLo_Knob : Knob_Green1_15
    {
        Mixer_ *mymodule;
        int param;

        void onChange( const event::Change &e ) override 
        {
            mymodule = (Mixer_*)paramQuantity->module;

            if( mymodule )
            {
                param = paramQuantity->paramId - Mixer_::PARAM_CHEQLO;
                mymodule->m_lpIn[ param ] = paramQuantity->getValue(); 
            }

		    RoundKnob::onChange( e );
	    }
    };
};

//Mixer_ _BrowserClass;

//-----------------------------------------------------
// MyLEDButton_ChSolo
//-----------------------------------------------------
void _Button_ChSolo( void *pClass, int id, bool bOn )
{
    Mixer_ *mymodule;

    mymodule = (Mixer_*)pClass;
    mymodule->ProcessMuteSolo( id, false, bOn );
}

//-----------------------------------------------------
// MyLEDButton_ChMute
//-----------------------------------------------------
void _Button_ChMute( void *pClass, int id, bool bOn )
{
    Mixer_ *mymodule;

    mymodule = (Mixer_*)pClass;
    mymodule->ProcessMuteSolo( id, true, bOn );
}

//-----------------------------------------------------
// Button_ChPreFader
//-----------------------------------------------------
void _Button_ChPreFader( void *pClass, int id, bool bOn )
{
    Mixer_ *mymodule;

    mymodule = (Mixer_*)pClass;
    mymodule->m_bPreFader[ id ] = bOn;
}

//-----------------------------------------------------
// Procedure:   RouteCallback
//-----------------------------------------------------
void _RouteCallback( void *pClass, int id, int nbutton, bool bOn )
{
#if nGROUPS > 0

    Mixer_ *mymodule;

    mymodule = (Mixer_*)pClass;

    if( mymodule )
    {
        mymodule->m_iRouteGroup[ id ] = nbutton;
    }

#endif
}

//-----------------------------------------------------
// Procedure:   MenuItem
//
//-----------------------------------------------------

struct _GroupPreMute : MenuItem
{
    Mixer_ *menumod;

    void onAction(const event::Action &e) override 
    {
        menumod->m_bGroupPreMute = !menumod->m_bGroupPreMute;
    }

    void step() override 
    {
        rightText = (menumod->m_bGroupPreMute) ? "✔" : "";
    }
};

struct _Gainx2 : MenuItem
{
    Mixer_ *menumod;

    void onAction(const event::Action &e) override 
    {
        menumod->m_bGainLevelx2 = !menumod->m_bGainLevelx2;
    }

    void step() override 
    {
        rightText = (menumod->m_bGainLevelx2) ? "✔" : "";
    }
};

struct _AuxIgnoreSolo : MenuItem
{
    Mixer_ *menumod;

    void onAction(const event::Action &e) override
    {
        menumod->m_bAuxIgnoreSolo = !menumod->m_bAuxIgnoreSolo;

        // cause an update with a passive call
        menumod->ProcessMuteSolo( nINCHANNELS - nAUX, false, false );
    }

    void step() override
    {
        rightText = (menumod->m_bAuxIgnoreSolo) ? "✔" : "";
    }
};

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
struct Mixer_Widget_ : ModuleWidget
{

Mixer_Widget_( Mixer_ *module )
{
    //Mixer_ *pmod;
    PortWidget *pPort;
    float fx, fx2, fx3, fx5, fx7;
    int ch, x, y, x2, y2;
    bool bGroup, bAux, bNormal;

    setModule(module);

    /*if( !module )
        pmod = &_BrowserClass;
    else
        pmod = module;*/

    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, Mixer_Svg )));

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
        pPort = createInput<MyPortInSmall>( Vec( x2, y2 ), module, Mixer_::IN_LEFT + ch );
        addInput( pPort ); y2 += 23;

        if( bGroup )
            pPort->visible = false;

        pPort = createInput<MyPortInSmall>( Vec( x2, y2 ), module, Mixer_::IN_RIGHT + ch );
        addInput( pPort );

        if( bGroup )
            pPort->visible = false;

        x2 = x + 4;
        y2 += 20;

        // aux sends
        if( !bAux )
        {
            addParam(createParam<Knob_Red1_15>( Vec( x2, y2 ), module, Mixer_::PARAM_CHAUX + (ch * 4) + 0 ) );
            y2 += 17;
            addParam(createParam<Knob_Yellow3_15>( Vec( x2, y2 ), module, Mixer_::PARAM_CHAUX + (ch * 4) + 1 ) );
            y2 += 17;
            addParam(createParam<Knob_Blue3_15>( Vec( x2, y2 ), module, Mixer_::PARAM_CHAUX + (ch * 4) + 2 ) );
            y2 += 17;
            addParam(createParam<Knob_Purp1_15>( Vec( x2, y2 ), module, Mixer_::PARAM_CHAUX + (ch * 4) + 3 ) );

            if( module )
            {
                module->m_pButtonPreFader[ ch ] = new MyLEDButton( x2 - 3, y2 + 15, 7, 7, 5.0f, DWRGB( 180, 180, 180 ), DWRGB( 255, 255, 255 ), MyLEDButton::TYPE_SWITCH, ch, module, _Button_ChPreFader );
	        addChild( module->m_pButtonPreFader[ ch ] );
            }

            y2 += 24;
        }
        else
        {
            switch( ch - ( nINCHANNELS + nGROUPS ) )
            {
            case 0:
                addParam(createParam<Knob_Red1_15>( Vec( x2, y2 ), module, Mixer_::PARAM_AUXLVL + 0 ) );
                break;
            case 1:
                addParam(createParam<Knob_Yellow3_15>( Vec( x2, y2 ), module, Mixer_::PARAM_AUXLVL + 1 ) );
                break;
            case 2:
                addParam(createParam<Knob_Blue3_15>( Vec( x2, y2 ), module, Mixer_::PARAM_AUXLVL + 2 ) );
                break;
            case 3:
                addParam(createParam<Knob_Purp1_15>( Vec( x2, y2 ), module, Mixer_::PARAM_AUXLVL + 3 ) );
                break;
            }

            addOutput(createOutput<MyPortOutSmall>( Vec( x2 - 1, y2 + 22), module, Mixer_::OUT_AUXL + (ch - (nCHANNELS - nAUX) ) ) );
            addOutput(createOutput<MyPortOutSmall>( Vec( x2 - 1, y2 + 47), module, Mixer_::OUT_AUXR + (ch - (nCHANNELS - nAUX) ) ) );

            y2 += 75;
        }

        // EQ
        addParam(createParam<Mixer_::MyEQHi_Knob>( Vec( x2, y2 ), module, Mixer_::PARAM_CHEQHI + ch ) );
        y2 += 18;
        addParam(createParam<Mixer_::MyEQMid_Knob>( Vec( x2, y2 ), module, Mixer_::PARAM_CHEQMD + ch ) );
        y2 += 18;
        addParam(createParam<Mixer_::MyEQLo_Knob>( Vec( x2, y2 ), module, Mixer_::PARAM_CHEQLO + ch ) );
        y2 += 20;

        // CVs
        if( bNormal )
        {
            addInput(createInput<MyPortInSmall>( Vec( x2 - 1, y2 ), module, Mixer_::IN_LEVELCV + ch ) );
            y2 += 25;
            addInput(createInput<MyPortInSmall>( Vec( x2 - 1, y2 ), module, Mixer_::IN_PANCV + ch ) );
        }
        // group outs
        else if( bGroup )
        {
#if nGROUPS > 0
            addOutput(createOutput<MyPortOutSmall>( Vec( x2 - 1, y2 ), module, Mixer_::OUT_GRPL + (ch - nINCHANNELS) ) );
            y2 += 25;
            addOutput(createOutput<MyPortOutSmall>( Vec( x2 - 1, y2 ), module, Mixer_::OUT_GRPR + (ch - nINCHANNELS) ) );
#endif
        }
        else
            y2 += 25;

        y2 += 23;

        addParam(createParam<Knob_Yellow1_15>( Vec( x2, y2 ), module, Mixer_::PARAM_CHPAN + ch ) );

        y2 += 19;

        if( module )
        {
        // mute/solo
        if( bNormal || bGroup )
        {
            module->m_pButtonChannelMute[ ch ] = new MyLEDButton( x + 3, y2, 8, 8, 6.0f, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, _Button_ChMute );
	        addChild( module->m_pButtonChannelMute[ ch ] );

            module->m_pButtonChannelSolo[ ch ] = new MyLEDButton( x + 12, y2, 8, 8, 6.0f, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, _Button_ChSolo );
	        addChild( module->m_pButtonChannelSolo[ ch ] );
        }
        // mute only
        else
        {
            module->m_pButtonChannelMute[ ch ] = new MyLEDButton( x + 9, y2, 8, 8, 6.0f, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, _Button_ChMute );
	        addChild( module->m_pButtonChannelMute[ ch ] );
        }
        }

        if( module )
        {
        // VUMeter
            module->m_pLEDMeterChannel[ ch ][ 0 ] = new LEDMeterWidget( x + 7, y + 260, 4, 1, 1, true );
        addChild( module->m_pLEDMeterChannel[ ch ][ 0 ] );
        module->m_pLEDMeterChannel[ ch ][ 1 ] = new LEDMeterWidget( x + 12, y + 260, 4, 1, 1, true );
        addChild( module->m_pLEDMeterChannel[ ch ][ 1 ] );
        }

        x2 = x + 2;
        y2 = y + 290;

#if nGROUPS > 0
        // Group Route
        if( module && bNormal )
        {
            module->m_pMultiButtonRoute[ ch ] = new MyLEDButtonStrip( x2, y2, 6, 6, 3, 4.0f, nGROUPS + 1, true, DWRGB( 0, 128, 128 ), DWRGB( 0, 255, 255 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, ch, module, _RouteCallback );
	        addChild( module->m_pMultiButtonRoute[ ch ] );
        }
#endif

        // level slider
        addParam(createParam<Slider02_10x15>( Vec( x + 10, y2 - 8 ), module, Mixer_::PARAM_CHLVL + ch ) );

        x += 23;
    }

    x += 5;

    // output
    addParam(createParam<Knob_Blue2_56>( Vec( x, 256 ), module, Mixer_::PARAM_LEVEL_OUT ) );

    if( module )
    {
        module->m_pLEDMeterMain[ 0 ] = new LEDMeterWidget( x + 10, 315, 5, 3, 2, true );
    addChild( module->m_pLEDMeterMain[ 0 ] );
    module->m_pLEDMeterMain[ 1 ] = new LEDMeterWidget( x + 10 + 7, 315, 5, 3, 2, true );
    addChild( module->m_pLEDMeterMain[ 1 ] );
    }

    // main outputs
    addOutput(createOutput<MyPortOutSmall>( Vec( x + 33, 319 ), module, Mixer_::OUT_MAINL ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( x + 33, 344 ), module, Mixer_::OUT_MAINR ) );

    // xfade inputs
#if nGROUPS > 0
    addInput(createInput<MyPortInSmall>( Vec( x + 50, 203 ), module, Mixer_::IN_FADEX ) );
    addInput(createInput<MyPortInSmall>( Vec( x + 50, 230 ), module, Mixer_::IN_FADEY ) );
#endif

    //screw
	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    if( module )
    {
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
}

//-----------------------------------------------------
// Procedure:   createContextMenu
//
//-----------------------------------------------------
void appendContextMenu(Menu *menu) override 
{
    menu->addChild(new MenuEntry);

    Mixer_ *mod = dynamic_cast<Mixer_*>(module);
    assert(mod);

#if nGROUPS > 0
    menu->addChild( createMenuLabel( "---- Group Outputs ----" ));

    _GroupPreMute *pMergeItem1 = createMenuItem<_GroupPreMute>("Pre-Mute");
    pMergeItem1->menumod = mod;
    menu->addChild(pMergeItem1);
#endif

    menu->addChild( createMenuLabel( "---- Level Sliders ----" ));

    _Gainx2 *pMergeItem2 = createMenuItem<_Gainx2>("Gain x1.5");
    pMergeItem2->menumod = mod;
    menu->addChild(pMergeItem2);

    menu->addChild( createMenuLabel( "---- Aux Output ----" ));

    _AuxIgnoreSolo *pMergeItem3 = createMenuItem<_AuxIgnoreSolo>("Do Not Mute when SOLOing");
    pMergeItem3->menumod = mod;
    menu->addChild(pMergeItem3);
}

};

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void Mixer_::JsonParams( bool bTo, json_t *root)
{
    JsonDataBool( bTo, "m_bMuteStates", root, m_bMuteStates, nCHANNELS );
    JsonDataBool( bTo, "m_bSoloStates", root, m_bSoloStates, nCHANNELS );

#if nGROUPS > 0
    JsonDataInt( bTo, "m_iRouteGroup", root, &m_iRouteGroup[ 0 ], nINCHANNELS );
#endif

    JsonDataBool( bTo, "m_bGroupPreMute", root, &m_bGroupPreMute, 1 );
    JsonDataBool( bTo, "m_bGainLevelx2", root, &m_bGainLevelx2, 1 );
    JsonDataBool( bTo, "m_bPreFader", root, m_bPreFader, nINCHANNELS + nGROUPS );
    JsonDataBool( bTo, "m_bAuxIgnoreSolo", root, &m_bAuxIgnoreSolo, 1 );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *Mixer_::dataToJson()
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
void Mixer_::dataFromJson( json_t *root )
{
    int ch;

    JsonParams( FROMJSON, root );


    if( !m_bInitialized )
        return;

    // anybody soloing?
    for( ch = 0; ch < nCHANNELS; ch++ )
    {
    	if( m_bMuteStates[ ch ] )
    		ProcessMuteSolo( ch, true, m_bMuteStates[ ch ] );
    	else if( m_bSoloStates[ ch ] )
    		ProcessMuteSolo( ch, false, m_bSoloStates[ ch ] );

        SetControls( ch );
    }
}

//-----------------------------------------------------
// Procedure:   SetControls
//
//-----------------------------------------------------
void Mixer_::SetControls( int ch )
{
    if( !m_bInitialized || ch >= nCHANNELS || ch < 0 )
        return;

    if( m_pButtonChannelMute[ ch ] )
        m_pButtonChannelMute[ ch ]->Set( m_bMuteStates[ ch ] );

    if( ch < ( nINCHANNELS + nGROUPS ) )
    {
        if( m_pButtonChannelSolo[ ch ] )
            m_pButtonChannelSolo[ ch ]->Set( m_bSoloStates[ ch ] );
    }

#if nGROUPS > 0
    if( ch < nINCHANNELS && m_pMultiButtonRoute[ ch ] )
        m_pMultiButtonRoute[ ch ]->Set( m_iRouteGroup[ ch ], true );
#endif

    if( ch < (nINCHANNELS + nGROUPS ) )
        m_pButtonPreFader[ ch ]->Set( m_bPreFader[ ch ] );
}

//-----------------------------------------------------
// Procedure:   onReset
//
//-----------------------------------------------------
void Mixer_::onReset()
{
    int ch;

    if( !m_bInitialized )
        return;

    m_StartupFade = 0.0f;
    m_StartCount = START_DELAY_COUNT;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        m_FadeState[ ch ] = MUTE_FADE_STATE_IDLE;

        m_bMuteStates[ ch ] = false;
        m_bSoloStates[ ch ] = false;
        m_fMuteFade[ ch ] = 1.0f;

#if nGROUPS > 0
        if( ch < nINCHANNELS )
            m_iRouteGroup[ ch ] = nGROUPS;
#endif

        SetControls( ch );
    }
}

//-----------------------------------------------------
// Procedure:   ProcessEQ
//
//-----------------------------------------------------
#define MULTI (0.33333333333333333333333333333333f)
void Mixer_::ProcessEQ( int ch, float *pL, float *pR )
{
    float rez, hp1; 
    float input[ 2 ], out[ 2 ], lowpass, bandpass, highpass;

    input[ L ] = *pL;
    input[ R ] = *pR;

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

    *pL = out[ L ];
    *pR = out[ R ];
}

//-----------------------------------------------------
// Procedure:   ProcessMuteSolo
//
//-----------------------------------------------------
void Mixer_::ProcessMuteSolo( int index, bool bMute, bool bOn )
{
    int i;
    bool bSoloing = false;
  
#if nGROUPS > 0
    int j;
    bool bSoloGroup[ nGROUPS ] = {}, bSoloRoutedToGroup[ nGROUPS ] = {};
#endif

    if( bMute )
    {
        m_bMuteStates[ index ] = bOn;

        // turn solo off
        if( m_bSoloStates[ index ] )
        {
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
        m_bSoloStates[ index ] = bOn;

        // turn mute off
        if( m_bMuteStates[ index ] )
        {
            m_bMuteStates[ index ] = false;
            m_pButtonChannelMute[ index ]->Set( false );
        }

        // toggle solo
        if( !m_bSoloStates[ index ] )
        {
            m_pButtonChannelSolo[ index ]->Set( false );
        }
        else
        {
            m_pButtonChannelSolo[ index ]->Set( true );
        }
    }

    // is a track soloing?
    for( i = 0; i < nCHANNELS - nAUX; i++ )
    {
        if( m_bSoloStates[ i ] )
        {
        	bSoloing = true;

#if nGROUPS > 0
        	if( i < nINCHANNELS )
        	{
				if( m_iRouteGroup[ i ] != nGROUPS )
					bSoloRoutedToGroup[ m_iRouteGroup[ i ] ] = true;
        	}
        	else
        	{
        		bSoloGroup[ i - nINCHANNELS ] = true;
        	}
#endif

        }
    }

    // somebody is soloing
    if( bSoloing )
    {
        // first shut down volume of all not in solo
        for( i = 0; i < nCHANNELS; i++ )
        {
        	// no aux mute on solo
        	if( i >= ( nCHANNELS - nAUX ) )
        	{
        		if( m_bAuxIgnoreSolo && !m_bMuteStates[ i ] )
        			m_FadeState[ i ] = MUTE_FADE_STATE_INC;
        		else
        			m_FadeState[ i ] = MUTE_FADE_STATE_DEC;
        	}
        	else
        	{
				if( !m_bSoloStates[ i ] )
					m_FadeState[ i ] = MUTE_FADE_STATE_DEC;
				else
					m_FadeState[ i ] = MUTE_FADE_STATE_INC;
        	}
        }

#if nGROUPS > 0
        // second, re-enable all groups that are being soloed from an input channel
        for( i = nINCHANNELS; i < (nINCHANNELS + nGROUPS); i++ )
        {
            if( bSoloRoutedToGroup[ i - nINCHANNELS ] && !m_bMuteStates[ i ] )
                m_FadeState[ i ] = MUTE_FADE_STATE_INC;
        }

        // third solo any input channels that are routed to a soloing group
        for( i = nINCHANNELS; i < (nINCHANNELS + nGROUPS); i++ )
        {
        	// if this group is soloing
            if( bSoloGroup[ i - nINCHANNELS ] )
            {
            	// enable output for each input channel routed to this group
                for( j = 0; j < nINCHANNELS; j++ )
                {
                	if( m_iRouteGroup[ j ] == ( i - nINCHANNELS ) && !m_bMuteStates[ j ] )
                	{
                		m_FadeState[ j ] = MUTE_FADE_STATE_INC;
                	}
                }
            }
        }
#endif
    }
    // nobody soloing and just turned solo off then enable all channels that aren't muted
    else //if( bSoloOff )
    {
        // turn on everything except muted
        for( i = 0; i < nCHANNELS; i++ )
        {
            // bring back if not muted
            if( !m_bMuteStates[ i ] )
            {
                m_FadeState[ i ] = MUTE_FADE_STATE_INC;
            }
            else
            {
            	m_FadeState[ i ] = MUTE_FADE_STATE_DEC;
            }
        }
    }
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
#define SNORMAL 0
#define SGROUP 1
#define SAUX   2
void Mixer_::process(const ProcessArgs &args)
{
    int section = 0;
    int ch, aux, group = 0;
    float GroupMixL[ MAX_GROUPS ] = {0}, GroupMixR[ MAX_GROUPS ] = {0}, fMixOutL = 0.0f, fMixOutR = 0.0f, inL, inR, flevel, fx, fy, fade[ MAX_GROUPS ];
    float auxL[ nAUX ] = {}, auxR[ nAUX ] = {};
    bool bChannelActive, bGroupActive[ MAX_GROUPS ] = {false};
    float pan, levelmult = 1.0;

    if( !m_bInitialized )
        return;

    if( m_StartCount )
    {
        if( --m_StartCount )
            return;
    }

    if( m_StartupFade < 1.0f )
        m_StartupFade += STARTUP_FADE_INC;
    else
        m_StartupFade = 1.0f;


    if( m_bGainLevelx2 )
        levelmult = 1.5f;

    fade[ 0 ] = 1.0f;
    fade[ 1 ] = 1.0f;
    fade[ 2 ] = 1.0f;
    fade[ 3 ] = 1.0f;

#if nGROUPS > 0
    // calc XFADE
    if( inputs[ IN_FADEX ].isConnected() || inputs[ IN_FADEY ].isConnected() )
    {
        fx = clamp( inputs[ IN_FADEX ].getNormalVoltage( 5.0f ), 0.0f, 10.0f ) / 10.0f;
        fy = clamp( inputs[ IN_FADEY ].getNormalVoltage( 5.0f ), 0.0f, 10.0f ) / 10.0f;

        if( fx > 0.5f || fy < 0.5 )
            fade[ 0 ] = fmin( (1.0f - fx) * 2, fy * 2 );

        if( fx < 0.5f || fy < 0.5 )
            fade[ 1 ] = fmin( fx * 2, fy * 2 );

        if( fx > 0.5f || fy > 0.5 )
            fade[ 2 ] = fmin( (1.0f - fx) * 2, (1.0f - fy) * 2 );

        if( fx < 0.5f || fy > 0.5 )
            fade[ 3 ] = fmin( fx * 2, (1.0f - fy) * 2 );
    }
#endif

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        inL = 0.0f;
        inR = 0.0f;

        // normal(0), group(1) or aux channels(2) section
        if( ch == nINCHANNELS || ch == ( nINCHANNELS + nGROUPS ) )
        {
#if nGROUPS > 0
            section++;
#else
            section += 2;  // skip group on 4Ch mixer
#endif

        }

        bChannelActive = false;

        // see if channel active
        if( section != SGROUP )
        {
            if( inputs[ IN_LEFT + ch ].isConnected() || inputs[ IN_RIGHT + ch ].isConnected() ) 
                bChannelActive = true;
        }
        else if( section == SGROUP )
        {
            group = ch - nINCHANNELS;

            if( bGroupActive[ group ] )
                bChannelActive = true;
        }

        if( bChannelActive )
        {
            if( section == SNORMAL )
                flevel = clamp( ( params[ PARAM_CHLVL + ch ].getValue() * levelmult ) * ( inputs[ IN_LEVELCV + ch ].getNormalVoltage( CV_MAX10 ) / CV_MAX10 ), 0.0f, levelmult );
            else
                flevel = params[ PARAM_CHLVL + ch ].getValue() * levelmult;

            if( section == SGROUP )
            {
                // process xfade
                inL = GroupMixL[ group ];
                inR = GroupMixR[ group ];
            }
            else
            {
                // check right channel first for possible mono
                if( inputs[ IN_RIGHT + ch ].isConnected() )
                {
                    inR = inputs[ IN_RIGHT + ch ].getVoltageSum();

                    m_bMono[ ch ] = false;
                }
                else
                    m_bMono[ ch ] = true;

                // left channel
                if( inputs[ IN_LEFT + ch ].isConnected() )
                {
                    inL = inputs[ IN_LEFT + ch ].getVoltageSum();

                    if( m_bMono[ ch ] )
                        inR = inL;
                }
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

            // attenuate for EQ

            ProcessEQ( ch, &inL, &inR );
            inL *= 2.0f;
            inR *= 2.0f;

            if( section == SNORMAL )
                pan = clamp( params[ PARAM_CHPAN + ch ].getValue() + ( inputs[ IN_PANCV + ch ].getNormalVoltage( 0.0 ) / CV_MAXn5 ), -1.0f, 1.0f );
            else
                pan = params[ PARAM_CHPAN + ch ].getValue();

            if( pan <= 0.0 )
                inR *= ( 1.0 + pan );
            else
                inL *= ( 1.0 - pan );

            // put output to aux ( pre fader )
            if( section != SAUX )
            {
                if( m_bPreFader[ ch ] )
                {
                    for ( aux = 0; aux < nAUX; aux++ )
                    {
                        auxL[ aux ] += inL * params[ PARAM_CHAUX + (ch * 4) + aux ].getValue();
                        auxR[ aux ] += inR * params[ PARAM_CHAUX + (ch * 4) + aux ].getValue();
                    }
                }
            }

            inL *= flevel;
            inR *= flevel;

#if nGROUPS > 0
            if( section == SGROUP )
            {
                // process xfade
                inL *= fade[ group ];
                inR *= fade[ group ];
            }
#endif
            // mute comes before group outputs
            if( !m_bGroupPreMute )
            {
                inL *= m_fMuteFade[ ch ];
                inR *= m_fMuteFade[ ch ];
            }

#if nGROUPS > 0
            // group output (pre mute)
            if( section == SGROUP )
            {
                if( bGroupActive[ group ] )
                {
                    outputs[ OUT_GRPL + group ].setVoltage( inL );
                    outputs[ OUT_GRPR + group ].setVoltage( inR );
                }
                else
                {
                    outputs[ OUT_GRPL + group ].setVoltage( 0.0f );
                    outputs[ OUT_GRPR + group ].setVoltage( 0.0f );
                }
            }
#endif
            // mute comes after group outputs
            if( m_bGroupPreMute )
            {
                inL *= m_fMuteFade[ ch ];
                inR *= m_fMuteFade[ ch ];
            }

            // put output to aux ( post fader )
            if( section != SAUX )
            {
                if( !m_bPreFader[ ch ] )
                {
                    for ( aux = 0; aux < nAUX; aux++ )
                    {
                        auxL[ aux ] += inL * params[ PARAM_CHAUX + (ch * 4) + aux ].getValue();
                        auxR[ aux ] += inR * params[ PARAM_CHAUX + (ch * 4) + aux ].getValue();
                    }
                }
            }

            // non normal input channels go directly to output
            if( section != SNORMAL )
            {
                fMixOutL += inL;
                fMixOutR += inR;
            }
            else
            {
#if nGROUPS > 0
                // normal channel direct out
                if( m_iRouteGroup[ ch ] == nGROUPS )
                {
                    fMixOutL += inL;
                    fMixOutR += inR;
                }
                // normal channel routed to group
                else
                {
                    GroupMixL[ m_iRouteGroup[ ch ] ] += inL;
                    GroupMixR[ m_iRouteGroup[ ch ] ] += inR;

                    bGroupActive[ m_iRouteGroup[ ch ] ] = true;
                }
#else
            fMixOutL += inL;
            fMixOutR += inR;
#endif
            }
        }

        if( m_pLEDMeterChannel[ ch ][ 0 ] )
            m_pLEDMeterChannel[ ch ][ 0 ]->Process( inL / AUDIO_MAX );
        if( m_pLEDMeterChannel[ ch ][ 1 ] )
            m_pLEDMeterChannel[ ch ][ 1 ]->Process( inR / AUDIO_MAX);
    }

    fMixOutL = fMixOutL * params[ PARAM_LEVEL_OUT ].getValue();
    fMixOutR = fMixOutR * params[ PARAM_LEVEL_OUT ].getValue();

    // update main VUMeters
    if( m_pLEDMeterMain[ 0 ] )
        m_pLEDMeterMain[ 0 ]->Process( fMixOutL / AUDIO_MAX );
    if( m_pLEDMeterMain[ 1 ] )
        m_pLEDMeterMain[ 1 ]->Process( fMixOutR / AUDIO_MAX );

    // put aux output
    for ( aux = 0; aux < nAUX; aux++ )
    {
        outputs[ OUT_AUXL + aux ].setVoltage( auxL[ aux ] * params[ PARAM_AUXLVL + aux ].getValue() * m_StartupFade );
        outputs[ OUT_AUXR + aux ].setVoltage( auxR[ aux ] * params[ PARAM_AUXLVL + aux ].getValue() * m_StartupFade );
    }

    outputs[ OUT_MAINL ].setVoltage( fMixOutL * m_StartupFade );
    outputs[ OUT_MAINR ].setVoltage( fMixOutR * m_StartupFade );
}