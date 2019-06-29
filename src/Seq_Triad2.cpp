#include "mscHack.hpp"
#include "window.hpp"

#define nKEYBOARDS  3
#define nKEYS       37
#define nOCTAVESEL  5
#define nSTEPS      16
#define nPATTERNS   8
#define SEMI ( 1.0f / 12.0f )

typedef struct
{
    float        fsemi;

}KEYBOARD_KEY_STRUCT;

typedef struct
{
    int     note;
    bool    bTrigOff;
    int     pad[ 6 ];

}PATTERN_STRUCT;

typedef struct
{
    bool    bPending;
    int     phrase;
}PHRASE_CHANGE_STRUCT;

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Seq_Triad2 : Module 
{
#define NO_COPY -1

	enum ParamIds 
    {
        PARAM_PAUSE,          
        PARAM_OCTAVES           = PARAM_PAUSE + ( nKEYBOARDS ),
        PARAM_GLIDE             = PARAM_OCTAVES + (nOCTAVESEL * nKEYBOARDS),
        PARAM_TRIGOFF           = PARAM_GLIDE + ( nKEYBOARDS ),
        nPARAMS                 = PARAM_TRIGOFF + ( nKEYBOARDS )
    };

	enum InputIds 
    {
        IN_STEP_CLK,
        IN_VOCT_OFF             = IN_STEP_CLK + ( nKEYBOARDS ),
        IN_PAT_CHANGE           = IN_VOCT_OFF + ( nKEYBOARDS ),
        IN_CLOCK_RESET          = IN_PAT_CHANGE + ( nKEYBOARDS ),
        IN_GLOBAL_PAT_CHANGE,
		IN_GLOBAL_TRIG_MUTE,
		IN_CHANNEL_TRIG_MUTE,
        nINPUTS					= IN_CHANNEL_TRIG_MUTE + ( nKEYBOARDS )
	};

	enum OutputIds 
    {
        OUT_TRIG,               
        OUT_VOCTS               = OUT_TRIG + nKEYBOARDS,
        nOUTPUTS                = OUT_VOCTS + nKEYBOARDS
	};

    bool            m_bInitialized = false;

    // octaves
    int             m_Octave[ nKEYBOARDS ] = {};
    float           m_fCvStartOut[ nKEYBOARDS ] = {};
    float           m_fCvEndOut[ nKEYBOARDS ] = {};

    // steps
    int             m_CurrentStep[ nKEYBOARDS ] = {};
    PATTERN_STRUCT  m_Notes[ nKEYBOARDS ][ nPATTERNS ][ nSTEPS ] = {};
    dsp::SchmittTrigger  m_SchTrigStepClk[ nKEYBOARDS ];
    PatternSelectStrip *m_pStepSelectStrip[ nKEYBOARDS ] = {};
    int             m_nMaxSteps[ nKEYBOARDS ][ nPATTERNS ] = {};
    int             m_CopySrc = NO_COPY;

    // pattern save
    int             m_CurrentPattern[ nKEYBOARDS ] = {};
    PHRASE_CHANGE_STRUCT m_PatternPending[ nKEYBOARDS ] = {};
    dsp::SchmittTrigger  m_SchTrigPatternClk[ nKEYBOARDS ] = {};
    int             m_MaxPat[ nKEYBOARDS ] = {0};
    PatternSelectStrip *m_pPatternSelectStrip[ nKEYBOARDS ] = {};

    bool            m_bResetToPattern1[ nKEYBOARDS ] = {};

    // pause button
    bool            m_bPause[ nKEYBOARDS ] = {};

    // trig mute
    bool            m_bTrigMute = false;
    MyLEDButton     *m_pButtonTrigMute = NULL;

    // channel trig mute
    bool            m_bChTrigMute[ nKEYBOARDS ] = {};
    MyLEDButton     *m_pButtonChTrigMute[ nKEYBOARDS ] = {};

    // triggers     
    bool            m_bTrig[ nKEYBOARDS ] = {};
    dsp::PulseGenerator  m_gatePulse[ nKEYBOARDS ];

    // global triggers
    dsp::SchmittTrigger  m_SchTrigGlobalClkReset;
    dsp::SchmittTrigger  m_SchTrigGlobalPatChange;

    // glide
    float           m_fglideInc[ nKEYBOARDS ] = {};
    int             m_glideCount[ nKEYBOARDS ] = {};
    float           m_fglide[ nKEYBOARDS ] = {};
    float           m_fLastNotePlayed[ nKEYBOARDS ];
    bool            m_bWasLastNotePlayed[ nKEYBOARDS ] = {};

    Keyboard_3Oct_Widget *pKeyboardWidget[ nKEYBOARDS ];
    float           m_fKeyNotes[ 37 ];

    float           m_VoctOffsetIn[ nKEYBOARDS ] = {};

    // buttons
    MyLEDButtonStrip *m_pButtonOctaveSelect[ nKEYBOARDS ] = {};
    MyLEDButton      *m_pButtonPause[ nKEYBOARDS ] = {};
    MyLEDButton      *m_pButtonTrig[ nKEYBOARDS ] = {};
    MyLEDButton      *m_pButtonCopy[ nKEYBOARDS ] = {};

    // Contructor
	Seq_Triad2()
    {
        int i;

        config( nPARAMS, nINPUTS, nOUTPUTS, 0 );

        for( i = 0; i < nKEYBOARDS; i++ )
            configParam( PARAM_GLIDE + i, 0.0f, 1.0f, 0.0f, "Note Glide" );

        for( i = 0; i < 37; i++ )
            m_fKeyNotes[ i ] = (float)i * SEMI;
    
    }

    // Overrides 
    void    JsonParams( bool bTo, json_t *root);
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onRandomize() override;
    void    onReset() override;

    void    SetPatSteps( int kb, int nSteps );
    void    SetSteps( int kb, int steps );
    void    SetKey( int kb );
    void    SetOut( int kb );
    void    ChangeStep( int kb, int index, bool bForce, bool bPlay );
    void    ChangePattern( int kb, int index, bool bForce );
    void    SetPendingPattern( int kb, int phrase );
    void    Copy( int kb, bool bOn );
};

Seq_Triad2 Seq_Triad2Browser;

//-----------------------------------------------------
// Seq_Triad2_OctSelect
//-----------------------------------------------------
void Seq_Triad2_OctSelect( void *pClass, int id, int nbutton, bool bOn )
{
    Seq_Triad2 *mymodule;

    if( !pClass )
        return;

    mymodule = (Seq_Triad2*)pClass;

    mymodule->m_Octave[ id ] = nbutton;
    mymodule->SetOut( id );
}

//-----------------------------------------------------
// Seq_Triad2_Pause
//-----------------------------------------------------
void Seq_Triad2_Pause( void *pClass, int id, bool bOn ) 
{
    Seq_Triad2 *mymodule;

    if( !pClass )
        return;

    mymodule = (Seq_Triad2*)pClass;
    mymodule->m_bPause[ id ] = !mymodule->m_bPause[ id ];
}

//-----------------------------------------------------
// Seq_Triad2_TrigMute
//-----------------------------------------------------
void Seq_Triad2_TrigMute( void *pClass, int id, bool bOn )
{
    Seq_Triad2 *mymodule;

    if( !pClass )
        return;

    mymodule = (Seq_Triad2*)pClass;
    mymodule->m_bTrigMute = bOn;
}

//-----------------------------------------------------
// Seq_Triad2_ChTrigMute
//-----------------------------------------------------
void Seq_Triad2_ChTrigMute( void *pClass, int id, bool bOn )
{
    Seq_Triad2 *mymodule;

    if( !pClass )
        return;

    mymodule = (Seq_Triad2*)pClass;
    mymodule->m_bChTrigMute[ id ] = bOn;
}

//-----------------------------------------------------
// Seq_Triad2_Trig
//-----------------------------------------------------
void Seq_Triad2_Trig( void *pClass, int id, bool bOn ) 
{
    Seq_Triad2 *mymodule;

    if( !pClass )
        return;

    mymodule = (Seq_Triad2*)pClass;
    mymodule->m_Notes[ id ][ mymodule->m_CurrentPattern[ id ] ][ mymodule->m_CurrentStep[ id ] ].bTrigOff = bOn;//!mymodule->m_Notes[ id ][ mymodule->m_CurrentPattern[ id ] ][ mymodule->m_CurrentStep[ id ] ].bTrigOff;
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
void Seq_Triad2_Widget_NoteChangeCallback ( void *pClass, int kb, int notepressed, int *pnotes, bool bOn, int button, int mod )
{
	bool bCtrl = false;
    Seq_Triad2 *mymodule = (Seq_Triad2 *)pClass;

    if( !pClass )
        return;

    // don't allow program unless paused
    if( button == 1 && !mymodule->m_bPause[ kb ] )
        return;

    // right click advance step
    if( button == 1 )
    {
    	if( mod & GLFW_MOD_CONTROL )
    		bCtrl = true;

        mymodule->ChangeStep( kb, mymodule->m_CurrentStep[ kb ] + 1, true, false );

        if( mymodule->m_CurrentStep[ kb ] == 0 )
        	mymodule->ChangePattern( kb, mymodule->m_CurrentPattern[ kb ] + 1, true );

        // hit control to set trig off
        if( bCtrl )
        	mymodule->m_Notes[ kb ][ mymodule->m_CurrentPattern[ kb ] ][ mymodule->m_CurrentStep[ kb ] ].bTrigOff = true;
        else
        	mymodule->m_Notes[ kb ][ mymodule->m_CurrentPattern[ kb ] ][ mymodule->m_CurrentStep[ kb ] ].bTrigOff = false;

        mymodule->m_pButtonTrig[ kb ]->Set( bCtrl );

        mymodule->m_Notes[ kb ][ mymodule->m_CurrentPattern[ kb ] ][ mymodule->m_CurrentStep[ kb ] ].note = notepressed;
        mymodule->SetKey( kb );
    }
    else
        mymodule->m_Notes[ kb ][ mymodule->m_CurrentPattern[ kb ] ][ mymodule->m_CurrentStep[ kb ] ].note = notepressed;

    mymodule->SetOut( kb );
}

//-----------------------------------------------------
// Procedure:   StepChangeCallback
//
//-----------------------------------------------------
void Seq_Triad2_Widget_StepChangeCallback ( void *pClass, int kb, int pat, int max )
{
    Seq_Triad2 *mymodule = (Seq_Triad2 *)pClass;

    if( !pClass )
        return;

    if( !mymodule->m_bInitialized )
        return;

    if( mymodule->m_CurrentStep[ kb ] != pat )
        mymodule->ChangeStep( kb, pat, false, true );
    else if( mymodule->m_nMaxSteps[ kb ][ mymodule->m_CurrentPattern[ kb ] ] != max )
        mymodule->SetSteps( kb, max );
}

//-----------------------------------------------------
// Procedure:   PatternChangeCallback
//
//-----------------------------------------------------
void Seq_Triad2_Widget_PatternChangeCallback ( void *pClass, int kb, int pat, int max )
{
    Seq_Triad2 *mymodule = (Seq_Triad2 *)pClass;

    if( !pClass )
        return;

    if( !mymodule->m_bInitialized )
        return;

    if( mymodule->m_MaxPat[ kb ] != max )
    {
        mymodule->SetPatSteps( kb, max ); 
    }
    else if( mymodule->m_CurrentPattern[ kb ] == pat && mymodule->m_bPause[ kb ] && mymodule->m_CopySrc != NO_COPY )
    {
        mymodule->ChangePattern( kb, pat, true );
    }
    else if( mymodule->m_CurrentPattern[ kb ] != pat )
    {
        if( !mymodule->m_bPause[ kb ] )
            mymodule->SetPendingPattern( kb, pat );
        else
            mymodule->ChangePattern( kb, pat, false );
            
    }
}

//-----------------------------------------------------
// MyLEDButton_Copy
//-----------------------------------------------------
void MyLEDButton_Copy( void *pClass, int id, bool bOn ) 
{
    Seq_Triad2 *mymodule;

    if( !pClass )
        return;

    mymodule = (Seq_Triad2*)pClass;
    mymodule->Copy( id, bOn );
}

//-----------------------------------------------------
// Procedure:   MenuItem
//
//-----------------------------------------------------
struct Seq_Triad2_Ch1Reset : MenuItem 
{
    Seq_Triad2 *menumod;

    void onAction(const event::Action &e) override 
    {
        menumod->m_bResetToPattern1[ 0 ] = !menumod->m_bResetToPattern1[ 0 ];
    }

    void step() override 
    {
        rightText = (menumod->m_bResetToPattern1[ 0 ]) ? "✔" : "";
    }
};

struct Seq_Triad2_Ch2Reset : MenuItem 
{
    Seq_Triad2 *menumod;

    void onAction(const event::Action &e) override 
    {
        menumod->m_bResetToPattern1[ 1 ] = !menumod->m_bResetToPattern1[ 1 ];
    }

    void step() override 
    {
        rightText = (menumod->m_bResetToPattern1[ 1 ]) ? "✔" : "";
    }
};

struct Seq_Triad2_Ch3Reset : MenuItem 
{
    Seq_Triad2 *menumod;

    void onAction(const event::Action &e) override 
    {
        menumod->m_bResetToPattern1[ 2 ] = !menumod->m_bResetToPattern1[ 2 ];
    }

    void step() override 
    {
        rightText = (menumod->m_bResetToPattern1[ 2 ]) ? "✔" : "";
    }
};

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
struct Seq_Triad2_Widget : ModuleWidget 
{

Seq_Triad2_Widget( Seq_Triad2 *module )
{
    Seq_Triad2 *pmod;
    int kb, x, x2, y, y2;

    setModule(module);

    if( !module )
        pmod = &Seq_Triad2Browser;
    else
        pmod = module;

    //box.size = Vec( 15*25, 380);
    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/TriadSequencer2.svg")));

    //----------------------------------------------------
    // Keyboard Keys 
    y = 21;
    x = 11;

    for( kb = 0; kb < nKEYBOARDS; kb++ )
    {
        // channel trig mute
        pmod->m_pButtonChTrigMute[ kb ] = new MyLEDButton( x + 310, y + 3, 15, 15, 13.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, kb, module, Seq_Triad2_ChTrigMute );
        addChild( pmod->m_pButtonChTrigMute[ kb ] );

        addInput(createInput<MyPortInSmall>( Vec( x + 285, y + 1 ), module, Seq_Triad2::IN_CHANNEL_TRIG_MUTE + kb ) );

        // pause button
        pmod->m_pButtonPause[ kb ] = new MyLEDButton( x + 60, y + 4, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, kb, module, Seq_Triad2_Pause );
	    addChild( pmod->m_pButtonPause[ kb ] );

        // trig button
        pmod->m_pButtonTrig[ kb ] = new MyLEDButton( x + 260, y + 5, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, kb, module, Seq_Triad2_Trig );
	    addChild( pmod->m_pButtonTrig[ kb ] );

        // glide knob
        addParam( createParam<Knob_Yellow1_15>( Vec( x + 235, y + 86 ), module, Seq_Triad2::PARAM_GLIDE + kb ) );

        pmod->m_pButtonCopy[ kb ] = new MyLEDButton( x + 194, y + 89, 11, 11, 8.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 244, 244 ), MyLEDButton::TYPE_SWITCH, kb, module, MyLEDButton_Copy );
	    addChild( pmod->m_pButtonCopy[ kb ] );

        x2 = x + 274;

        // octave select
        pmod->m_pButtonOctaveSelect[ kb ] = new MyLEDButtonStrip( x2, y + 90, 11, 11, 3, 8.0, nOCTAVESEL, false, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 255 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, kb, module, Seq_Triad2_OctSelect );
	    addChild( pmod->m_pButtonOctaveSelect[ kb ] );

        // keyboard widget
        pmod->pKeyboardWidget[ kb ] = new Keyboard_3Oct_Widget( x + 39, y + 19, 1, kb, DWRGB( 255, 128, 64 ), module, Seq_Triad2_Widget_NoteChangeCallback );
	    addChild( pmod->pKeyboardWidget[ kb ] );

        // step selects
        pmod->m_pStepSelectStrip[ kb ] = new PatternSelectStrip( x + 79, y + 1, 9, 7, DWRGB( 255, 128, 64 ), DWRGB( 128, 64, 0 ), DWRGB( 255, 0, 128 ), DWRGB( 128, 0, 64 ), nSTEPS, kb, module, Seq_Triad2_Widget_StepChangeCallback );
	    addChild( pmod->m_pStepSelectStrip[ kb ] );

        // phrase selects
        pmod->m_pPatternSelectStrip[ kb ] = new PatternSelectStrip( x + 79, y + 86, 9, 7, DWRGB( 255, 255, 0 ), DWRGB( 128, 128, 64 ), DWRGB( 0, 255, 255 ), DWRGB( 0, 128, 128 ), nPATTERNS, kb, module, Seq_Triad2_Widget_PatternChangeCallback );
	    addChild( pmod->m_pPatternSelectStrip[ kb ] );

        x2 = x + 9;
        y2 = y + 4;
        // prog change trigger
        addInput(createInput<MyPortInSmall>( Vec( x2, y2 ), module, Seq_Triad2::IN_STEP_CLK + kb ) ); y2 += 40;

        // VOCT offset input
        addInput(createInput<MyPortInSmall>( Vec( x2, y2 ), module, Seq_Triad2::IN_VOCT_OFF + kb ) ); y2 += 40;

        // prog change trigger
        addInput(createInput<MyPortInSmall>( Vec( x2, y2 ), module, Seq_Triad2::IN_PAT_CHANGE + kb ) );

        // outputs
        x2 = x + 330;
        addOutput(createOutput<MyPortOutSmall>( Vec( x2, y + 27 ), module, Seq_Triad2::OUT_VOCTS + kb ) );
        addOutput(createOutput<MyPortOutSmall>( Vec( x2, y + 68 ), module, Seq_Triad2::OUT_TRIG + kb ) );

        y += 111;
    }

    // trig mute
    pmod->m_pButtonTrigMute = new MyLEDButton( x + 310, 3, 15, 15, 13.0, DWRGB( 180, 180, 180 ), DWRGB( 255, 0, 0 ), MyLEDButton::TYPE_SWITCH, 0, module, Seq_Triad2_TrigMute );
    addChild( pmod->m_pButtonTrigMute );

    addInput(createInput<MyPortInSmall>( Vec( x + 285, 1 ), module, Seq_Triad2::IN_GLOBAL_TRIG_MUTE ) );

    // reset inputs
    y2 = 357; 
    addInput(createInput<MyPortInSmall>( Vec( x + 89, y2 ),  module, Seq_Triad2::IN_CLOCK_RESET ) );
    addInput(createInput<MyPortInSmall>( Vec( x + 166, y2 ), module, Seq_Triad2::IN_GLOBAL_PAT_CHANGE ) );

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

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

    Seq_Triad2 *mod = dynamic_cast<Seq_Triad2*>(module);
    assert(mod);

    menu->addChild( createMenuLabel( "---- on CLK Reset ----" ));

    Seq_Triad2_Ch1Reset *pMergeItem1 = createMenuItem<Seq_Triad2_Ch1Reset>("Ch 1: Reset Pattern to 1");
    pMergeItem1->menumod = mod;
    menu->addChild(pMergeItem1);

    Seq_Triad2_Ch2Reset *pMergeItem2 = createMenuItem<Seq_Triad2_Ch2Reset>("Ch 2: Reset Pattern to 1");
    pMergeItem2->menumod = mod;
    menu->addChild(pMergeItem2);

    Seq_Triad2_Ch3Reset *pMergeItem3 = createMenuItem<Seq_Triad2_Ch3Reset>("Ch 3: Reset Pattern to 1");
    pMergeItem3->menumod = mod;
    menu->addChild(pMergeItem3);
}

};

//-----------------------------------------------------
// Procedure:   reset
//
//-----------------------------------------------------
void Seq_Triad2::onReset()
{
    if( !m_bInitialized )
        return;

    //lg.f("Reset\n");

    for( int kb = 0; kb < nKEYBOARDS; kb++ )
        m_pButtonOctaveSelect[ kb ]->Set( 0, true );

    memset( m_fCvStartOut, 0, sizeof(m_fCvStartOut) );
    memset( m_fCvEndOut, 0, sizeof(m_fCvEndOut) );
    memset( m_Notes, 0, sizeof(m_Notes) );
    
    for( int kb = 0; kb < nKEYBOARDS; kb++ )
    {
        for( int pat = 0; pat < nPATTERNS; pat++ )
            m_nMaxSteps[ kb ][ pat ] = 3;

        SetPatSteps( kb, 3 );
        ChangeStep( kb, 0, true, true );
        ChangePattern( kb, 0, true );
    }
}

//-----------------------------------------------------
// Procedure:   randomize
//
//-----------------------------------------------------
const int keyscalenotes[ 7 ] = { 0, 2, 4, 5, 7, 9, 11};
const int keyscalenotes_minor[ 7 ] = { 0, 2, 3, 5, 7, 9, 11};
void Seq_Triad2::onRandomize()
{
    int kb, step, pat, basekey, note;

    for( int kb = 0; kb < nKEYBOARDS; kb++ )
        m_pButtonOctaveSelect[ kb ]->Set( 0, true );

    memset( m_fCvStartOut, 0, sizeof(m_fCvStartOut) );
    memset( m_fCvEndOut, 0, sizeof(m_fCvEndOut) );
    memset( m_Notes, 0, sizeof(m_Notes) );

    basekey = (int)(random::uniform() * 24.0);

    for( kb = 0; kb < nKEYBOARDS; kb++ )
    {
        m_Octave[ kb ] = (int)( random::uniform() * 4.0 );

        for( step = 0; step < nSTEPS; step++ )
        {
            for( pat = 0; pat < nPATTERNS; pat++ )
            {
                if( random::uniform() > 0.7 )
                    note = keyscalenotes_minor[ (int)(random::uniform() * 7.0 ) ];
                else
                    note = keyscalenotes[ (int)(random::uniform() * 7.0 ) ];

                m_Notes[ kb ][ pat ][ step ].bTrigOff = ( random::uniform() < 0.10 );
                m_Notes[ kb ][ pat ][ step ].note = basekey + note; 
            }
        }

        ChangeStep( kb, 0, true, true );
    }
}

//-----------------------------------------------------
// Procedure:   SetPatSteps
//
//-----------------------------------------------------
void Seq_Triad2::SetPatSteps( int kb, int nSteps )
{
    if( nSteps < 0 || nSteps >= nPATTERNS )
        nSteps = 0;

    m_MaxPat[ kb ] = nSteps;
}

//-----------------------------------------------------
// Procedure:   SetSteps
//
//-----------------------------------------------------
void Seq_Triad2::SetSteps( int kb, int nSteps )
{
    if( nSteps < 0 || nSteps >= nSTEPS )
        nSteps = 0;

    m_nMaxSteps[ kb ][ m_CurrentPattern[ kb ] ] = nSteps;
}

//-----------------------------------------------------
// Procedure:   Copy
//
//-----------------------------------------------------
void Seq_Triad2::Copy( int kb, bool bOn )
{
    if( kb < 0 || kb >= nKEYBOARDS )
        return;

    if( !m_bPause[ kb ] || !bOn || m_CopySrc != NO_COPY )
    {
        if( m_CopySrc != NO_COPY )
            m_pButtonCopy[ m_CopySrc ]->Set( false );

        m_CopySrc = NO_COPY;
        m_pButtonCopy[ kb ]->Set( false );
    }
    else if( bOn )
    {
        m_CopySrc = kb;
    }
}

//-----------------------------------------------------
// Procedure:   SetOut
//
//-----------------------------------------------------
void Seq_Triad2::SetOut( int kb )
{
    int note;
    float foct;

    if( kb < 0 || kb >= nKEYBOARDS )
        return;

    // end glide note (current pattern note)
    foct = (float)m_Octave[ kb ];

    note = m_Notes[ kb ][ m_CurrentPattern[ kb ] ][ m_CurrentStep[ kb ] ].note;

    if( note > 36 || note < 0 )
        note = 0;

    m_fCvEndOut[ kb ] = foct + m_fKeyNotes[ note ] + m_VoctOffsetIn[ kb ];

    // start glide note (last pattern note)
    if( m_bWasLastNotePlayed[ kb ] )
    {
        m_fCvStartOut[ kb ] = m_fLastNotePlayed[ kb ] + m_VoctOffsetIn[ kb ];
    }
    else
    {
        m_bWasLastNotePlayed[ kb ] = true;
        m_fCvStartOut[ kb ] = m_fCvEndOut[ kb ] + m_VoctOffsetIn[ kb ];
    }

    m_fLastNotePlayed[ kb ] = m_fCvEndOut[ kb ] + m_VoctOffsetIn[ kb ];

    // glide time ( max glide = 0.5 seconds )
    m_glideCount[ kb ] = 1 + (int)( ( params[ PARAM_GLIDE + kb ].getValue() * 0.5 ) * APP->engine->getSampleRate());

    m_fglideInc[ kb ] = 1.0 / (float)m_glideCount[ kb ];

    m_fglide[ kb ] = 1.0;

    // always trig on pause
    if( m_bPause[ kb ] )
        m_bTrig[ kb ] = true;
    else if( !m_Notes[ kb ][ m_CurrentPattern[ kb ] ][ m_CurrentStep[ kb ] ].bTrigOff )
        m_bTrig[ kb ] = true;
    else
        m_bTrig[ kb ] = false;
}

//-----------------------------------------------------
// Procedure:   SetKey
//
//-----------------------------------------------------
void Seq_Triad2::SetKey( int kb )
{
    pKeyboardWidget[ kb ]->setkey( &m_Notes[ kb ][ m_CurrentPattern[ kb ] ][ m_CurrentStep[ kb ] ].note );
}

//-----------------------------------------------------
// Procedure:   SetPendingPattern
//
//-----------------------------------------------------
void Seq_Triad2::SetPendingPattern( int kb, int phraseIn )
{
    int phrase;

    if( phraseIn < 0 || phraseIn >= nPATTERNS )
        phrase = ( m_CurrentPattern[ kb ] + 1 ) & 0x7;
    else
        phrase = phraseIn;

    if( phrase > m_MaxPat[ kb ] )
        phrase = 0;

    m_PatternPending[ kb ].bPending = true;
    m_PatternPending[ kb ].phrase = phrase;
    m_pPatternSelectStrip[ kb ]->SetPat( m_CurrentPattern[ kb ], false );
    m_pPatternSelectStrip[ kb ]->SetPat( phrase, true );
}

//-----------------------------------------------------
// Procedure:   ChangePattern
//
//-----------------------------------------------------
void Seq_Triad2::ChangePattern( int kb, int index, bool bForce )
{
    if( kb < 0 || kb >= nKEYBOARDS )
        return;

    if( !bForce && index == m_CurrentPattern[ kb ] )
        return;

    if( index < 0 )
        index = nPATTERNS - 1;
    else if( index >= nPATTERNS )
        index = 0;

    if( m_CopySrc != NO_COPY )
    {
        // do not copy if we are not paused
        if( m_bPause[ kb ] )
        {
            memcpy( m_Notes[ kb ][ index ], m_Notes[ m_CopySrc ][ m_CurrentPattern[ m_CopySrc ] ], sizeof(PATTERN_STRUCT) * nSTEPS );
            m_pButtonCopy[ m_CopySrc ]->Set( false );
            m_nMaxSteps[ kb ][ index ] = m_nMaxSteps[ m_CopySrc ][ m_CurrentPattern[ m_CopySrc ] ];
            m_CopySrc = NO_COPY;
        }
    }

    m_CurrentPattern[ kb ] = index;

    m_pPatternSelectStrip[ kb ]->SetPat( index, false );
    m_pPatternSelectStrip[ kb ]->SetMax( m_MaxPat[ kb ] );
    m_pStepSelectStrip[ kb ]->SetMax( m_nMaxSteps[ kb ][ index ] );

    // set keyboard key
    SetKey( kb );

    m_pButtonTrig[ kb ]->Set( m_Notes[ kb ][ m_CurrentPattern[ kb ] ][ m_CurrentStep[ kb ] ].bTrigOff );

    // change octave light
    m_pButtonOctaveSelect[ kb ]->Set( m_Octave[ kb ], true );

    // set output note
    SetOut( kb );
}

//-----------------------------------------------------
// Procedure:   ChangeStep
//
//-----------------------------------------------------
void Seq_Triad2::ChangeStep( int kb, int index, bool bForce, bool bPlay )
{
    if( kb < 0 || kb >= nKEYBOARDS )
        return;

    if( !bForce && index == m_CurrentStep[ kb ] )
        return;

    if( index < 0 )
        index = nSTEPS - 1;
    else if( index >= nSTEPS )
        index = 0;

    // update octave offset immediately when not running
    m_VoctOffsetIn[ kb ] = inputs[ IN_VOCT_OFF + kb ].getNormalVoltage( 0.0 );

    m_CurrentStep[ kb ] = index;

    m_pStepSelectStrip[ kb ]->SetPat( index, false );

    // set keyboard key
    SetKey( kb );

    m_pButtonTrig[ kb ]->Set( m_Notes[ kb ][ m_CurrentPattern[ kb ] ][ m_CurrentStep[ kb ] ].bTrigOff );

    // change octave light
    m_pButtonOctaveSelect[ kb ]->Set( m_Octave[ kb ], true );

    // set outputted note
    if( bPlay )
        SetOut( kb );
}

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void Seq_Triad2::JsonParams( bool bTo, json_t *root) 
{
    int bi[ nKEYBOARDS ] = {};

    if( bTo )
    {
        bi[ 0 ] = m_bPause[ 0 ];
        bi[ 1 ] = m_bPause[ 1 ];
        bi[ 2 ] = m_bPause[ 2 ];
    }

    JsonDataInt( bTo, "m_bPause", root, &bi[ 0 ], nKEYBOARDS );

    if( !bTo )
    {
        m_bPause[ 0 ] = bi[ 0 ];
        m_bPause[ 1 ] = bi[ 1 ];
        m_bPause[ 2 ] = bi[ 2 ];
    }

    JsonDataInt( bTo, "m_nSteps", root, &m_nMaxSteps[ 0 ][ 0 ], nKEYBOARDS * nPATTERNS );
    JsonDataInt( bTo, "m_Octave", root, &m_Octave[ 0 ], nKEYBOARDS );
    JsonDataInt( bTo, "m_CurrentPhrase", root, &m_CurrentPattern[ 0 ], nKEYBOARDS );
    JsonDataInt( bTo, "m_PatternNotes", root, (int*)&m_Notes[ 0 ][ 0 ][ 0 ], nPATTERNS * nSTEPS * nKEYBOARDS * 8 );
    JsonDataInt( bTo, "m_PhrasesUsed", root, &m_MaxPat[ 0 ], nKEYBOARDS );
    JsonDataInt( bTo, "m_CurrentPattern", root, &m_CurrentStep[ 0 ], nKEYBOARDS );
    JsonDataBool( bTo, "m_bTrigMute", root, &m_bTrigMute, 1 );

    if( bTo )
    {
        bi[ 0 ] = m_bChTrigMute[ 0 ];
        bi[ 1 ] = m_bChTrigMute[ 1 ];
        bi[ 2 ] = m_bChTrigMute[ 2 ];
    }

    JsonDataInt( bTo, "m_bChTrigMute", root, &bi[ 0 ], nKEYBOARDS );

    if( !bTo )
    {
        m_bChTrigMute[ 0 ] = bi[ 0 ];
        m_bChTrigMute[ 1 ] = bi[ 1 ];
        m_bChTrigMute[ 2 ] = bi[ 2 ];
    }

    JsonDataBool( bTo, "m_bResetToPattern1", root, &m_bResetToPattern1[ 0 ], nKEYBOARDS );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *Seq_Triad2::dataToJson() 
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
void Seq_Triad2::dataFromJson( json_t *root ) 
{
    int i;

    JsonParams( FROMJSON, root );

    for( i = 0; i < nKEYBOARDS; i++ )
    {
        m_pPatternSelectStrip[ i ]->SetMax( m_MaxPat[ i ] );
        m_pPatternSelectStrip[ i ]->SetPat( m_CurrentPattern[ i ], false );

        m_pStepSelectStrip[ i ]->SetMax( m_nMaxSteps[ i ][ m_CurrentPattern[ i ] ] );
        m_pStepSelectStrip[ i ]->SetPat( m_CurrentStep[ i ], false );

        m_pButtonPause[ i ]->Set( m_bPause[ i ] );

        SetPatSteps( i, m_MaxPat[ i ] );
        ChangePattern( i, m_CurrentPattern[ i ], true );
        ChangeStep( i, m_CurrentStep[ i ], true, false );
    }

	if( m_bTrigMute )
		m_pButtonTrigMute->Set( m_bTrigMute );

	for( i = 0; i < nKEYBOARDS; i++ )
	{
		if( m_bChTrigMute[ i ] )
			m_pButtonChTrigMute[ i ]->Set( m_bChTrigMute[ i ] );
	}
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
#define LIGHT_LAMBDA ( 0.065f )
void Seq_Triad2::process(const ProcessArgs &args)
{
	static float flast[ nKEYBOARDS ] = {};
    int kb, useclock;
    bool bGlobalPatChange = false, bGlobalClkReset = false, bStepTrig[ nKEYBOARDS ] = {};
    float trigout = 0.0f;

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

    // global clock reset
    if( inputs[ IN_CLOCK_RESET ].isConnected() )
        bGlobalClkReset = m_SchTrigGlobalClkReset.process( inputs[ IN_CLOCK_RESET ].getVoltage() );

    // global pattern change trigger
    if( inputs[ IN_GLOBAL_PAT_CHANGE ].isConnected() )
        bGlobalPatChange = m_SchTrigGlobalPatChange.process( inputs[ IN_GLOBAL_PAT_CHANGE ].getVoltage() );

    // get step trigs
    for( kb = 0; kb < nKEYBOARDS; kb++ )
        bStepTrig[ kb ] = m_SchTrigStepClk[ kb ].process( inputs[ IN_STEP_CLK + kb ].getVoltage() );

    // process triggered phrase changes
    for( kb = 0; kb < nKEYBOARDS; kb++ )
    {
        useclock = kb;

        if( inputs[ IN_CHANNEL_TRIG_MUTE + kb ].isConnected() )
        {
    		if( inputs[ IN_CHANNEL_TRIG_MUTE + kb ].getVoltage() >= 0.00001 )
    		{
    			m_bChTrigMute[ kb ] = true;
    			m_pButtonChTrigMute[ kb ]->Set( true );
    		}
    		else if( inputs[ IN_CHANNEL_TRIG_MUTE + kb ].getVoltage() < 0.00001 )
    		{
    			m_bChTrigMute[ kb ] = false;
    			m_pButtonChTrigMute[ kb ]->Set( false );
    		}
        }

        // global clock reset
        if( bGlobalClkReset && !m_bPause[ kb ] )
        {
            m_PatternPending[ kb ].bPending = false;

            ChangeStep( kb, 0, true, false );

            if( m_bResetToPattern1[ kb ] )
                ChangePattern( kb, 0, true );
            else
                ChangePattern( kb, m_CurrentPattern[ kb ], true );

            goto output;
        }

        // if no keyboard clock active then use kb 0's clock
        if( !inputs[ IN_STEP_CLK + kb ].isConnected() && inputs[ IN_STEP_CLK + 0 ].isConnected() )
            useclock = 0;

        // phrase change trigger
        if( bGlobalPatChange && !m_bPause[ kb ] && inputs[ IN_STEP_CLK + useclock ].isConnected() )
        {
            SetPendingPattern( kb, -1 );
        }
        else if( inputs[ IN_PAT_CHANGE + kb ].isConnected() && !m_bPause[ kb ] && inputs[ IN_STEP_CLK + useclock ].isConnected() )
        {
	        if( m_SchTrigPatternClk[ kb ].process( inputs[ IN_PAT_CHANGE + kb ].getVoltage() ) )
                SetPendingPattern( kb, -1 );
        }

	    // step change trigger - ignore if already pending
        if( inputs[ IN_STEP_CLK + useclock ].isConnected() && !m_bPause[ kb ] )
        {
            if( bStepTrig[ useclock ] ) 
            {
                if( m_CurrentStep[ kb ] >= ( m_nMaxSteps[ kb ][ m_CurrentPattern[ kb ] ] ) )
                {
                    m_CurrentStep[ kb ] = (nSTEPS - 1);
                }

                ChangeStep( kb, m_CurrentStep[ kb ] + 1, true, true );

                if( m_CurrentStep[ kb ] == 0 )
                {
                    if( m_PatternPending[ kb ].bPending )
                    {
                        m_PatternPending[ kb ].bPending = false;
                        ChangePattern( kb, m_PatternPending[ kb ].phrase, false );
                    }
                }
            }
        }
        else
        {
            // resolve any left over phrase triggers
            if( m_PatternPending[ kb ].bPending )
            {
                m_PatternPending[ kb ].bPending = false;
                ChangePattern( kb, m_PatternPending[ kb ].phrase, false );
            }
        }

output:;

        if( m_bTrig[ kb ] )
        {
            m_bTrig[ kb ] = false;
            m_gatePulse[ kb ].trigger( 0.050f );
        }

        trigout = m_gatePulse[ kb ].process( 1.0 / args.sampleRate ) ? CV_MAX10 : 0.0;

        if( !m_bTrigMute && !m_bChTrigMute[ kb ] )
        	outputs[ OUT_TRIG + kb ].setVoltage( trigout );
        else
        	outputs[ OUT_TRIG + kb ].setVoltage( 0.0f );

        if( --m_glideCount[ kb ] > 0 )
            m_fglide[ kb ] -= m_fglideInc[ kb ];
        else
            m_fglide[ kb ] = 0.0;

        if( m_bTrigMute || m_bChTrigMute[ kb ] )
        	outputs[ OUT_VOCTS + kb ].setVoltage( flast[ kb ] );
        else
        {
        	flast[ kb ] = ( m_fCvStartOut[ kb ] * m_fglide[ kb ] ) + ( m_fCvEndOut[ kb ] * ( 1.0 - m_fglide[ kb ] ) );
        	outputs[ OUT_VOCTS + kb ].setVoltage( flast[ kb ] );
        }
    }
}

Model *modelSeq_Triad2 = createModel<Seq_Triad2, Seq_Triad2_Widget>( "TriadSeq2" );
