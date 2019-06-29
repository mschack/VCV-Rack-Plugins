#include "mscHack.hpp"

#define nCHANNELS 6
#define nSTEPS 32
#define nPROG 16

typedef struct
{
    bool    bPending;
    int     prog;
}PHRASE_CHANGE_STRUCT;

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct SEQ_6x32x16 : Module 
{
#define NO_COPY -1

	enum ParamIds 
    {
        PARAM_LVL1_KNOB,
        PARAM_LVL2_KNOB     = PARAM_LVL1_KNOB + nCHANNELS,
        PARAM_LVL3_KNOB     = PARAM_LVL2_KNOB + nCHANNELS,
        PARAM_LVL4_KNOB     = PARAM_LVL3_KNOB + nCHANNELS,
        PARAM_LVL5_KNOB     = PARAM_LVL4_KNOB + nCHANNELS,
        nPARAMS             = PARAM_LVL5_KNOB + nCHANNELS
    };

	enum InputIds 
    {
        IN_GLOBAL_CLK_RESET,
        IN_GLOBAL_PAT_CHANGE,
        IN_CLK,
        IN_PAT_TRIG     	= IN_CLK + nCHANNELS,
		IN_GLOBAL_TRIG_MUTE = IN_PAT_TRIG + nCHANNELS,
        nINPUTS
	};

	enum OutputIds 
    {
        OUT_TRIG,
        OUT_LEVEL    = OUT_TRIG + nCHANNELS,
        OUT_BEAT1    = OUT_LEVEL + nCHANNELS,
        nOUTPUTS     = OUT_BEAT1 + nCHANNELS
	};

    bool            m_bInitialized = false;

    bool            m_bPauseState[ nCHANNELS ] = {};
    bool            m_bBiLevelState[ nCHANNELS ] = {};

    SinglePatternClocked32  *m_pPatternDisplay[ nCHANNELS ] = {};
    int                     m_Pattern[ nCHANNELS ][ nPROG ][ nSTEPS ];
    int                     m_MaxPat[ nCHANNELS ][ nPROG ] = {};

    PatternSelectStrip      *m_pProgramDisplay[ nCHANNELS ] = {};
    int                     m_CurrentProg[ nCHANNELS ] = {};
    int                     m_MaxProg[ nCHANNELS ] = {};
    PHRASE_CHANGE_STRUCT    m_ProgPending[ nCHANNELS ] = {};

    dsp::SchmittTrigger          m_SchTrigClock[ nCHANNELS ];
    dsp::SchmittTrigger          m_SchTrigProg[ nCHANNELS ];
    dsp::SchmittTrigger          m_SchTrigGlobalClkReset;
    dsp::SchmittTrigger          m_SchTrigGlobalProg;

    bool                    m_bTrig[ nCHANNELS ] = {};
    dsp::PulseGenerator          m_gatePulse[ nCHANNELS ];
    dsp::PulseGenerator          m_gateBeatPulse[ nCHANNELS ];

    // swing
    //int                     m_SwingLen[ nCHANNELS ] = {0};
    //int                     m_SwingCount[ nCHANNELS ] = {0};
    int                     m_ClockTick[ nCHANNELS ] = {0};

    int                     m_CopySrc = NO_COPY;

    // trig mute
    bool            		m_bTrigMute = false;
    MyLEDButton     		*m_pButtonTrigMute = NULL;

    // buttons    
    MyLEDButton             *m_pButtonPause[ nCHANNELS ] = {};
    MyLEDButton             *m_pButtonCopy[ nCHANNELS ] = {};
    MyLEDButton             *m_pButtonBiLevel[ nCHANNELS ] = {};
    MyLEDButton             *m_pButtonRand[ nCHANNELS ] = {};
    MyLEDButton             *m_pButtonAutoPat[ nCHANNELS ] = {};
    MyLEDButton             *m_pButtonHoldCV[ nCHANNELS ] = {};
    MyLEDButton             *m_pButtonClear[ nCHANNELS ] = {};

    bool                    m_bAutoPatChange[ nCHANNELS ] = {};
    bool                    m_bHoldCVState[ nCHANNELS ] = {};

    float                   m_fCVRanges[ 3 ] = { 15.0f, 10.0f, 5.0f};
    int                     m_RangeSelect = 0;
    char                    m_strRange[ 10 ] = {0};

    // Contructor
	SEQ_6x32x16()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS, 0);

        for( int i = 0; i < nCHANNELS; i++ )
        {
            configParam( PARAM_LVL1_KNOB + i, 0.0, 1.0, 0.25, "Level Lo" );
            configParam( PARAM_LVL2_KNOB + i, 0.0, 1.0, 0.33, "Level Med Lo" );
            configParam( PARAM_LVL3_KNOB + i, 0.0, 1.0, 0.50, "Level Med" );
            configParam( PARAM_LVL4_KNOB + i, 0.0, 1.0, 0.75, "Level Med Hi" );
            configParam( PARAM_LVL5_KNOB + i, 0.0, 1.0, 0.9 , "Level Hi" );
        }
    }

    // Overrides 
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onRandomize() override;
    void    onReset() override;

    void    JsonParams( bool bTo, json_t *root);
    void    CpyNext( int ch );
    void    Rand( int ch );
    void    ChangeProg( int ch, int prog, bool force );
    void    SetPendingProg( int ch, int prog );
    void    Copy( int kb, bool bOn );
};

SEQ_6x32x16 SEQ_6x32x16Browser;

//-----------------------------------------------------
// MyLEDButton_TrigMute
//-----------------------------------------------------
void MyLEDButton_TrigMute( void *pClass, int id, bool bOn )
{
	SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->m_bTrigMute = bOn;
}

//-----------------------------------------------------
// MyLEDButton_AutoPat
//-----------------------------------------------------
void MyLEDButton_AutoPat( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->m_bAutoPatChange[ id ] = bOn;
}

//-----------------------------------------------------
// MyLEDButton_Pause
//-----------------------------------------------------
void MyLEDButton_Pause( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->m_bPauseState[ id ] = bOn;
}

//-----------------------------------------------------
// MyLEDButton_HoldCV
//-----------------------------------------------------
void MyLEDButton_HoldCV( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->m_bHoldCVState[ id ] = bOn;
}

//-----------------------------------------------------
// MyLEDButton_BiLevel
//-----------------------------------------------------
void MyLEDButton_BiLevel( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->m_bBiLevelState[ id ] = bOn;
}

//-----------------------------------------------------
// MyLEDButton_CpyNxt
//-----------------------------------------------------
void MyLEDButton_CpyNxt( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->Copy( id, bOn );
}

//-----------------------------------------------------
// MyLEDButton_Rand
//-----------------------------------------------------
void MyLEDButton_Rand( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;
    mymodule->Rand( id );
}

//-----------------------------------------------------
// MyLEDButton_Rand
//-----------------------------------------------------
void MyLEDButton_Clear( void *pClass, int id, bool bOn ) 
{
    SEQ_6x32x16 *mymodule;
    mymodule = (SEQ_6x32x16*)pClass;

    for( int i = 0; i < nSTEPS; i++ )
    {
        mymodule->m_Pattern[ id ][ mymodule->m_CurrentProg[ id ] ][ i ] = 0;
    }

    mymodule->m_pPatternDisplay[ id ]->SetPatAll( mymodule->m_Pattern[ id ][ mymodule->m_CurrentProg[ id ] ] );
}

//-----------------------------------------------------
// Procedure:   PatternChangeCallback
//
//-----------------------------------------------------
void SEQ_6x32x16_PatternChangeCallback ( void *pClass, int ch, int pat, int level, int maxpat )
{
    SEQ_6x32x16 *mymodule = (SEQ_6x32x16 *)pClass;

    if( !mymodule || !mymodule->m_bInitialized )
        return;

    mymodule->m_MaxPat[ ch ][ mymodule->m_CurrentProg[ ch ] ] = maxpat;
    mymodule->m_Pattern[ ch ][ mymodule->m_CurrentProg[ ch ] ] [ pat ] = level;
}

//-----------------------------------------------------
// Procedure:   ProgramChangeCallback
//
//-----------------------------------------------------
void SEQ_6x32x16_ProgramChangeCallback ( void *pClass, int ch, int pat, int max )
{
    SEQ_6x32x16 *mymodule = (SEQ_6x32x16 *)pClass;

    if( !mymodule || !mymodule->m_bInitialized )
        return;

    if( mymodule->m_MaxProg[ ch ] != max )
    {
        mymodule->m_MaxProg[ ch ] = max;
    }
    else if( mymodule->m_CurrentProg[ ch ] == pat && mymodule->m_bPauseState[ ch ] && mymodule->m_CopySrc != NO_COPY )
    {
        mymodule->ChangeProg( ch, pat, true );
    }
    else if( mymodule->m_CurrentProg[ ch ] != pat )
    {
        if( !mymodule->m_bPauseState[ ch ] && mymodule->inputs[ SEQ_6x32x16::IN_CLK + ch ].active )
            mymodule->SetPendingProg( ch, pat );
        else
            mymodule->ChangeProg( ch, pat, false );
    }
}

//-----------------------------------------------------
// Procedure:   MenuItem
//
//-----------------------------------------------------
struct SEQ_6x32x16_CVRange : MenuItem 
{
    SEQ_6x32x16 *menumod;

    void onAction(const event::Action &e) override 
    {
        menumod->m_RangeSelect++;

        if( menumod->m_RangeSelect > 2 )
            menumod->m_RangeSelect = 0;

        sprintf( menumod->m_strRange, "%.1fV", menumod->m_fCVRanges[ menumod->m_RangeSelect ] );
    }

    void step() override 
    {
        rightText = menumod->m_strRange;
    }
};

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct SEQ_6x32x16_Widget : ModuleWidget 
{

SEQ_6x32x16_Widget( SEQ_6x32x16 *module )
{
    int x, y, x2, y2;
    ParamWidget *pWidget = NULL;
    SEQ_6x32x16 *pmod;

    //box.size = Vec( 15*41, 380);

    setModule(module);

    if( !module )
        pmod = &SEQ_6x32x16Browser;
    else
        pmod = module;

    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/SEQ_6x32x16.svg")));

    //screw
    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    x = 7;
    y = 22;

    // global inputs
    addInput(createInput<MyPortInSmall>( Vec( 204, 357 ), module, SEQ_6x32x16::IN_GLOBAL_CLK_RESET ) );
    addInput(createInput<MyPortInSmall>( Vec( 90, 357 ), module, SEQ_6x32x16::IN_GLOBAL_PAT_CHANGE ) );

    // trig mute
    pmod->m_pButtonTrigMute = new MyLEDButton( 491, 3, 15, 15, 13.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, 0, module, MyLEDButton_TrigMute );
    addChild( pmod->m_pButtonTrigMute );

    addInput(createInput<MyPortInSmall>( Vec( 466, 1 ), module, SEQ_6x32x16::IN_GLOBAL_TRIG_MUTE ) );

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        // inputs
        addInput(createInput<MyPortInSmall>( Vec( x + 6, y + 7 ),   module, SEQ_6x32x16::IN_CLK + ch ) );
        addInput(createInput<MyPortInSmall>( Vec( x + 64, y + 31 ), module, SEQ_6x32x16::IN_PAT_TRIG + ch ) );

        // pattern display
        pmod->m_pPatternDisplay[ ch ] = new SinglePatternClocked32( x + 39, y + 2, 13, 13, 5, 2, 7, DWRGB( 255, 128, 64 ), DWRGB( 18, 9, 0 ), DWRGB( 180, 75, 180 ), DWRGB( 80, 45, 80 ), nSTEPS, ch, module, SEQ_6x32x16_PatternChangeCallback );
	    addChild( pmod->m_pPatternDisplay[ ch ] );

        // program display
        pmod->m_pProgramDisplay[ ch ] = new PatternSelectStrip( x + 106, y + 31, 9, 7, DWRGB( 180, 180, 0 ), DWRGB( 90, 90, 64 ), DWRGB( 0, 180, 200 ), DWRGB( 0, 90, 90 ), nPROG, ch, module, SEQ_6x32x16_ProgramChangeCallback );
	    addChild( pmod->m_pProgramDisplay[ ch ] );

        // add knobs
        y2 = y + 31;
        //pWidget = ParamWidget::create<Knob_Green1_15>( Vec( x + 374, y2 ), module, SEQ_6x32x16::PARAM_SWING_KNOB + ch, 0.0, 0.6, 0.0 );

        // removed for now
        if( pWidget )
        {
            addParam( pWidget );
            pWidget->visible = false;
        }

        x2 = x + 447;
        addParam(createParam<Knob_Green1_15>( Vec( x2, y2 ), module, SEQ_6x32x16::PARAM_LVL1_KNOB + ch ) ); x2 += 19;
        addParam(createParam<Knob_Green1_15>( Vec( x2, y2 ), module, SEQ_6x32x16::PARAM_LVL2_KNOB + ch ) ); x2 += 19;
        addParam(createParam<Knob_Green1_15>( Vec( x2, y2 ), module, SEQ_6x32x16::PARAM_LVL3_KNOB + ch ) ); x2 += 19;
        addParam(createParam<Knob_Green1_15>( Vec( x2, y2 ), module, SEQ_6x32x16::PARAM_LVL4_KNOB + ch ) ); x2 += 19;
        addParam(createParam<Knob_Green1_15>( Vec( x2, y2 ), module, SEQ_6x32x16::PARAM_LVL5_KNOB + ch ) );

        // add buttons
        pmod->m_pButtonAutoPat[ ch ] = new MyLEDButton( x + 55, y + 35, 9, 9, 6.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, MyLEDButton_AutoPat );
	    addChild( pmod->m_pButtonAutoPat[ ch ] );

        pmod->m_pButtonPause[ ch ] = new MyLEDButton( x + 26, y + 10, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, MyLEDButton_Pause );
	    addChild( pmod->m_pButtonPause[ ch ] );

        y2 = y + 33;
        pmod->m_pButtonCopy[ ch ] = new MyLEDButton( x + 290, y2, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 244, 244 ), MyLEDButton::TYPE_SWITCH, ch, module, MyLEDButton_CpyNxt );
	    addChild( pmod->m_pButtonCopy[ ch ] );

        pmod->m_pButtonRand[ ch ] = new MyLEDButton( x + 315, y2, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 244, 244 ), MyLEDButton::TYPE_MOMENTARY, ch, module, MyLEDButton_Rand );
	    addChild( pmod->m_pButtonRand[ ch ] );

        pmod->m_pButtonClear[ ch ] = new MyLEDButton( x + 340, y2, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 244, 244 ), MyLEDButton::TYPE_MOMENTARY, ch, module, MyLEDButton_Clear );
	    addChild( pmod->m_pButtonClear[ ch ] );

        pmod->m_pButtonHoldCV[ ch ] = new MyLEDButton( x + 405, y2, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 244, 244 ), MyLEDButton::TYPE_SWITCH, ch, module, MyLEDButton_HoldCV );
	    addChild( pmod->m_pButtonHoldCV[ ch ] );

        pmod->m_pButtonBiLevel[ ch ] = new MyLEDButton( x + 425, y2, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 244, 244 ), MyLEDButton::TYPE_SWITCH, ch, module, MyLEDButton_BiLevel );
	    addChild( pmod->m_pButtonBiLevel[ ch ] );

        // add outputs
        addOutput(createOutput<MyPortOutSmall>( Vec( x + 580, y + 7 ), module, SEQ_6x32x16::OUT_TRIG + ch ) );
        addOutput(createOutput<MyPortOutSmall>( Vec( x + 544, y + 31 ), module, SEQ_6x32x16::OUT_LEVEL + ch ) );
        addOutput(createOutput<MyPortOutSmall>( Vec( x + 37, y + 31 ), module, SEQ_6x32x16::OUT_BEAT1 + ch ) );

        y += 56;
    }

    if( module )
    {
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

    SEQ_6x32x16 *mod = dynamic_cast<SEQ_6x32x16*>(module);
    assert(mod);

    menu->addChild( createMenuLabel( "---- CV Output Level ----" ));

    SEQ_6x32x16_CVRange *pMergeItem1 = createMenuItem<SEQ_6x32x16_CVRange>("VRange (15, 10, 5):");
    pMergeItem1->menumod = mod;
    menu->addChild(pMergeItem1);
}

};

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void SEQ_6x32x16::JsonParams( bool bTo, json_t *root) 
{
    JsonDataBool    ( bTo, "m_bPauseState",     root, &m_bPauseState[ 0 ], nCHANNELS );
    JsonDataBool    ( bTo, "m_bBiLevelState",   root, &m_bBiLevelState[ 0 ], nCHANNELS );
    JsonDataInt     ( bTo, "m_Pattern",         root, &m_Pattern[ 0 ][ 0 ][ 0 ], nCHANNELS * nSTEPS * nPROG );
    JsonDataInt     ( bTo, "m_MaxPat",          root, &m_MaxPat[ 0 ][ 0 ], nCHANNELS * nPROG );
    JsonDataInt     ( bTo, "m_CurrentProg",     root, &m_CurrentProg[ 0 ], nCHANNELS );
    JsonDataInt     ( bTo, "m_MaxProg",         root, &m_MaxProg[ 0 ], nCHANNELS );
    JsonDataBool    ( bTo, "m_bAutoPatChange",  root, &m_bAutoPatChange[ 0 ], nCHANNELS );
    JsonDataBool    ( bTo, "m_bHoldCVState",    root, &m_bHoldCVState[ 0 ], nCHANNELS );
    JsonDataInt     ( bTo, "m_RangeSelect",     root, &m_RangeSelect, 1 );
    JsonDataBool    ( bTo, "m_bTrigMute",       root, &m_bTrigMute, 1 );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *SEQ_6x32x16::dataToJson() 
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
void SEQ_6x32x16::dataFromJson( json_t *root ) 
{
    JsonParams( FROMJSON, root );

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        m_pButtonAutoPat[ ch ]->Set( m_bAutoPatChange[ ch ] );
        m_pButtonPause[ ch ]->Set( m_bPauseState[ ch ] );
        m_pButtonBiLevel[ ch ]->Set( m_bBiLevelState[ ch ] );
        m_pButtonHoldCV[ ch ]->Set( m_bHoldCVState[ ch ] );

        m_pPatternDisplay[ ch ]->SetPatAll( m_Pattern[ ch ][ m_CurrentProg[ ch ] ] );
        m_pPatternDisplay[ ch ]->SetMax( m_MaxPat[ ch ][ m_CurrentProg[ ch ] ] );

        m_pProgramDisplay[ ch ]->SetPat( m_CurrentProg[ ch ], false );
        m_pProgramDisplay[ ch ]->SetMax( m_MaxProg[ ch ] );
    }

	if( m_bTrigMute )
		m_pButtonTrigMute->Set( m_bTrigMute );

    sprintf( m_strRange, "%.1fV", m_fCVRanges[ m_RangeSelect ] );
}

//-----------------------------------------------------
// Procedure:   reset
//
//-----------------------------------------------------
void SEQ_6x32x16::onReset()
{
    if( !m_bInitialized )
        return;

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        m_pButtonPause[ ch ]->Set( false );
        m_pButtonBiLevel[ ch ]->Set( false );
    }

    memset( m_bPauseState, 0, sizeof(m_bPauseState) );
    memset( m_bBiLevelState, 0, sizeof(m_bBiLevelState) );
    memset( m_Pattern, 0, sizeof(m_Pattern) );
    memset( m_CurrentProg, 0, sizeof(m_CurrentProg) );

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        for( int prog = 0; prog < nCHANNELS; prog++ )
            m_MaxPat[ ch ][ prog ] = nSTEPS - 1;

        m_MaxProg[ ch ] = nPROG - 1;

        m_pPatternDisplay[ ch ]->SetPatAll( m_Pattern[ ch ][ 0 ] );
        m_pPatternDisplay[ ch ]->SetMax( m_MaxPat[ ch ][ 0 ] );

        m_pProgramDisplay[ ch ]->SetPat( 0, false );
        m_pProgramDisplay[ ch ]->SetMax( m_MaxProg[ ch ] );

        m_pPatternDisplay[ ch ]->SetPatAll( m_Pattern[ ch ][ m_CurrentProg[ ch ] ] );

        ChangeProg( ch, 0, true );
    }
}

//-----------------------------------------------------
// Procedure:   randomize
//
//-----------------------------------------------------
void SEQ_6x32x16::onRandomize()
{
    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        for( int p = 0; p < nPROG; p++ )
        {
            for( int i = 0; i < nSTEPS; i++ )
            {
                m_Pattern[ ch ][ p ][ i ] = (int)(random::uniform() * 5.0 );
            }
        }

        m_pPatternDisplay[ ch ]->SetPatAll( m_Pattern[ ch ][ m_CurrentProg[ ch ] ] );
    }
}

//-----------------------------------------------------
// Procedure:   Rand
//
//-----------------------------------------------------
void SEQ_6x32x16::Rand( int ch )
{
    for( int i = 0; i < nSTEPS; i++ )
    {
        if( i <= m_MaxPat[ ch ][ m_CurrentProg[ ch ] ] && random::uniform() > 0.5f )
            m_Pattern[ ch ][ m_CurrentProg[ ch ] ][ i ] = (int)(random::uniform() * 5.0 );
        else
            m_Pattern[ ch ][ m_CurrentProg[ ch ] ][ i ] = 0;
    }

    m_pPatternDisplay[ ch ]->SetPatAll( m_Pattern[ ch ][ m_CurrentProg[ ch ] ] );
}

//-----------------------------------------------------
// Procedure:   CpyNext
//
//-----------------------------------------------------
void SEQ_6x32x16::CpyNext( int ch )
{
    int next;

    next = m_CurrentProg[ ch ] + 1;

    if( next >= nPROG )
        return;

    memcpy( m_Pattern[ ch ][ next ], m_Pattern[ ch ][ m_CurrentProg[ ch ] ], sizeof(int) * nSTEPS );

    if(m_bPauseState[ ch ] || !inputs[ SEQ_6x32x16::IN_CLK + ch ].getVoltage() )
        ChangeProg( ch, next, false );
}

//-----------------------------------------------------
// Procedure:   Copy
//
//-----------------------------------------------------
void SEQ_6x32x16::Copy( int ch, bool bOn )
{
    if( !m_bPauseState[ ch ] || !bOn || m_CopySrc != NO_COPY )
    {
        if( m_CopySrc != NO_COPY )
            m_pButtonCopy[ m_CopySrc ]->Set( false );

        m_CopySrc = NO_COPY;
        m_pButtonCopy[ ch ]->Set( false );
    }
    else if( bOn )
    {
        m_CopySrc = ch;
    }
}

//-----------------------------------------------------
// Procedure:   ChangProg
//
//-----------------------------------------------------
void SEQ_6x32x16::ChangeProg( int ch, int prog, bool bforce )
{
    if( ch < 0 || ch >= nCHANNELS )
        return;

    if( !bforce && prog == m_CurrentProg[ ch ] )
        return;

    if( prog < 0 )
        prog = nPROG - 1;
    else if( prog >= nPROG )
        prog = 0;

    if( m_CopySrc != NO_COPY )
    {
        if( m_bPauseState[ ch ] )
        {
            memcpy( m_Pattern[ ch ][ prog ], m_Pattern[ m_CopySrc ][ m_CurrentProg[ m_CopySrc ] ], sizeof(int) * nSTEPS );
            m_pButtonCopy[ m_CopySrc ]->Set( false );
            m_MaxPat[ ch ][ prog ] = m_MaxPat[ m_CopySrc ][ m_CurrentProg[ m_CopySrc ] ];
            m_CopySrc = NO_COPY;
        }
    }

    m_CurrentProg[ ch ] = prog;

    m_pPatternDisplay[ ch ]->SetPatAll( m_Pattern[ ch ][ prog ] );
    m_pPatternDisplay[ ch ]->SetMax( m_MaxPat[ ch ][ prog ] );
    m_pProgramDisplay[ ch ]->SetPat( prog, false );
}

//-----------------------------------------------------
// Procedure:   SetPendingProg
//
//-----------------------------------------------------
void SEQ_6x32x16::SetPendingProg( int ch, int progIn )
{
    int prog;

    if( progIn < 0 || progIn >= nPROG )
        prog = ( m_CurrentProg[ ch ] + 1 ) & 0xF;
    else
        prog = progIn;

    if( prog > m_MaxProg[ ch ] )
        prog = 0;

    m_ProgPending[ ch ].bPending = true;
    m_ProgPending[ ch ].prog = prog;
    m_pProgramDisplay[ ch ]->SetPat( m_CurrentProg[ ch ], false );
    m_pProgramDisplay[ ch ]->SetPat( prog, true );
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void SEQ_6x32x16::process(const ProcessArgs &args)
{
	static float flast[ nCHANNELS ] = {};
    int ch, level, useclock;
    bool bClk, bClkTrig, bClockAtZero = false, bGlobalClk = false, bGlobalProg = false, bTrigOut, bPatTrig[ nCHANNELS ] = {};
    float fout = 0.0;

    if( !m_bInitialized )
        return;

    if( inputs[ IN_GLOBAL_TRIG_MUTE ].isConnected() )
    {
		if( inputs[ IN_GLOBAL_TRIG_MUTE ].getVoltage() >= 0.00001 )
		{
			m_bTrigMute = true;
			m_pButtonTrigMute->Set( true );
		}
		else if( inputs[ IN_GLOBAL_TRIG_MUTE ].getVoltage() < 0.00001 )
		{
			m_bTrigMute = false;
			m_pButtonTrigMute->Set( false );
		}
    }

    if( inputs[ IN_GLOBAL_CLK_RESET ].active )
        bGlobalClk = m_SchTrigGlobalClkReset.process( inputs[ IN_GLOBAL_CLK_RESET ].getVoltage() );

    if( inputs[ IN_GLOBAL_PAT_CHANGE ].active )
        bGlobalProg= m_SchTrigGlobalProg.process( inputs[ IN_GLOBAL_PAT_CHANGE ].getVoltage() );

    // get trigs
    for( ch = 0; ch < nCHANNELS; ch++ )
        bPatTrig[ ch ] = m_SchTrigClock[ ch ].process( inputs[ IN_CLK + ch ].getNormalVoltage( 0.0f ) );

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        bClkTrig = false;
        bClk = false;
        bTrigOut = false;
        bClockAtZero = false;
        useclock = ch;

        // if no keyboard clock active then use kb 0's clock
        if( !inputs[ IN_CLK + ch ].isConnected() && inputs[ IN_CLK + 0 ].isConnected() )
            useclock = 0;

        if( inputs[ IN_CLK + useclock ].isConnected() )
        {
            // time the clock tick
            if( bGlobalClk )
            {
                m_pPatternDisplay[ ch ]->ClockReset();
                bClkTrig = true;
                bClockAtZero = true;
                bClk = true;
            }

            // clock
            if( bPatTrig[ useclock ] && !bGlobalClk )
                bClkTrig = true;

            bClk = bClkTrig;

            // clock the pattern
            if( !m_bPauseState[ ch ] )
            {
                if( bGlobalProg || m_SchTrigProg[ ch ].process( inputs[ IN_PAT_TRIG + ch ].getVoltage() ) )
                    SetPendingProg( ch, -1 );

                // clock in
                if( bClk )
                {
                    if( !bGlobalClk )
                        bClockAtZero = m_pPatternDisplay[ ch ]->ClockInc();

                    bTrigOut = ( m_Pattern[ ch ][ m_CurrentProg[ ch ] ][ m_pPatternDisplay[ ch ]->m_PatClk ] );
                }
            }
        }
        // no clock input
        else
        {
            // resolve any left over phrase triggers
            if( m_ProgPending[ ch ].bPending )
            {
                m_ProgPending[ ch ].bPending = false;
                ChangeProg( ch, m_ProgPending[ ch ].prog, true );
            }

            continue;
        }

        // auto inc pattern
        if( m_bAutoPatChange[ ch ] && bClockAtZero )
        {
            SetPendingProg( ch, -1 );
            m_ProgPending[ ch ].bPending = false;
            ChangeProg( ch, m_ProgPending[ ch ].prog, true );
            bTrigOut = ( m_Pattern[ ch ][ m_CurrentProg[ ch ] ][ m_pPatternDisplay[ ch ]->m_PatClk ] );
        }
        // resolve pattern change
        else if( m_ProgPending[ ch ].bPending && bClockAtZero )
        {
            m_ProgPending[ ch ].bPending = false;
            ChangeProg( ch, m_ProgPending[ ch ].prog, false );
            bTrigOut = ( m_Pattern[ ch ][ m_CurrentProg[ ch ] ][ m_pPatternDisplay[ ch ]->m_PatClk ] );
        }

        // trigger the beat 0 output
        if( bClockAtZero )
            m_gateBeatPulse[ ch ].trigger(1e-3);

        outputs[ OUT_BEAT1 + ch ].setVoltage( m_gateBeatPulse[ ch ].process( 1.0 / APP->engine->getSampleRate() ) ? CV_MAX10 : 0.0 );

        // trigger out
        if( bTrigOut )
        {
            m_gatePulse[ ch ].trigger(1e-3);
        }

        if( !m_bTrigMute )
        	outputs[ OUT_TRIG + ch ].setVoltage( m_gatePulse[ ch ].process( 1.0 / APP->engine->getSampleRate() ) ? CV_MAX10 : 0.0 );
        else
        	outputs[ OUT_TRIG + ch ].setVoltage( 0.0f );

        level = m_Pattern[ ch ][ m_CurrentProg[ ch ] ][ m_pPatternDisplay[ ch ]->m_PatClk ];

        // get actual level from knobs
        switch( level )
        {
        case 1:
            fout = params[ PARAM_LVL1_KNOB + ch ].value;
            break;
        case 2:
            fout = params[ PARAM_LVL2_KNOB + ch ].value;
            break;
        case 3:
            fout = params[ PARAM_LVL3_KNOB + ch ].value;
            break;
        case 4:
            fout = params[ PARAM_LVL4_KNOB + ch ].value;
            break;
        case 5:
            fout = params[ PARAM_LVL5_KNOB + ch ].value;
            break;
        default:
            if( m_bHoldCVState[ ch ] )
                continue;
            else
                fout = 0.0f;
        }

        // bidirectional convert to -1.0 to 1.0
        if( m_bBiLevelState[ ch ] )
            fout = ( fout * 2 ) - 1.0;

        if( m_bTrigMute )
        	outputs[ OUT_LEVEL + ch ].setVoltage( flast[ ch ] );
        else
        {
        	flast[ ch ] = m_fCVRanges[ m_RangeSelect ] * fout;
        	outputs[ OUT_LEVEL + ch ].setVoltage( flast[ ch ] );
        }
    }
}

Model *modelSEQ_6x32x16 = createModel<SEQ_6x32x16, SEQ_6x32x16_Widget>( "Seq_6ch_32step" );
