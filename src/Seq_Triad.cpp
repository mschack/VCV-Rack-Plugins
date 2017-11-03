#include "mscHack.hpp"
#include "mscHack_Controls.hpp"
#include "dsp/digital.hpp"
#include "CLog.h"

#define nKEYBOARDS  3
#define nBLACKKEYS  15
#define nWHITEKEYS  22
#define nKEYS       ( nBLACKKEYS + nWHITEKEYS )
#define nOCTAVESEL  4
#define nPATTERNS   16

#define CHANNEL_X       15
#define CHANNEL_Y       78
#define CHANNEL_OFF_Y   100

#define SEMI ( 1.0f / 12.0f )

float fWhiteKeyNotes[ nWHITEKEYS ] = 
{
    ( SEMI * 0.0 ), ( SEMI * 2.0 ), ( SEMI * 4.0 ), ( SEMI * 5.0 ), ( SEMI * 7.0 ), ( SEMI * 9.0 ), ( SEMI * 11.0 ),
    ( SEMI * 12.0 ), ( SEMI * 14.0 ), ( SEMI * 16.0 ), ( SEMI * 17.0 ), ( SEMI * 19.0 ), ( SEMI * 21.0 ), ( SEMI * 23.0 ),
    ( SEMI * 24.0 ), ( SEMI * 26.0 ), ( SEMI * 28.0 ), ( SEMI * 29.0 ), ( SEMI * 31.0 ), ( SEMI * 33.0 ), ( SEMI * 35.0 ),
    ( SEMI * 36.0 )
};

float fBlackKeyNotes[ nBLACKKEYS ] = 
{
    ( SEMI * 1.0 ), ( SEMI * 3.0 ), ( SEMI * 6.0 ), ( SEMI * 8.0 ), ( SEMI * 10.0 ),
    ( SEMI * 13.0 ), ( SEMI * 15.0 ),( SEMI * 18.0 ), ( SEMI * 20.0 ), ( SEMI * 22.0 ), 
    ( SEMI * 25.0 ), ( SEMI * 27.0 ), ( SEMI * 30.0 ), ( SEMI * 32.0 ), ( SEMI * 34.0 )
};

int blackKeyoff[ nBLACKKEYS ] = { 1, 3, 6, 8, 10,   13, 15, 18, 20, 22,   25, 27, 30, 32, 34 };
int whiteKeyoff[ nWHITEKEYS ] = { 0, 2, 4, 5, 7, 9, 11,   12, 14, 16, 17, 19, 21, 23,   24, 26, 28, 29, 31, 33, 35, 36 };

typedef struct
{
    float        fsemi;
    ParamWidget  *pWidget;

}KEYBOARD_KEY_STRUCT;

typedef struct
{
    int note;
    int oct;

}PATTERN_STRUCT;

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Seq_Triad : Module 
{
	enum ParamIds 
    {
        PARAM_COPY_NEXT,
        PARAM_PAUSE,            
        PARAM_STEP_NUM,
        PARAM_OCTAVES           = PARAM_STEP_NUM + (nPATTERNS),
        PARAM_PATTERNS          = PARAM_OCTAVES + (nOCTAVESEL * nKEYBOARDS),
        PARAM_WKEYS             = PARAM_PATTERNS + (nPATTERNS ),
        PARAM_BKEYS             = PARAM_WKEYS + (nWHITEKEYS * nKEYBOARDS),
        nPARAMS                 = PARAM_BKEYS + (nBLACKKEYS * nKEYBOARDS)
    };

	enum InputIds 
    {
        IN_PATTERN_TRIG,
        nINPUTS
	};

	enum OutputIds 
    {
        OUT_TRIG,               
        OUT_VOCTS               = OUT_TRIG + nKEYBOARDS,
        nOUTPUTS                = OUT_VOCTS + nKEYBOARDS
	};

    CLog            lg;
    bool            m_bJsonInit = false;

    KEYBOARD_KEY_STRUCT m_Keys[ nKEYBOARDS ][ nKEYS ];

    float           m_fCvOut[ nKEYBOARDS ] = {};
    float           m_fLightOctaves[ nKEYBOARDS ][ nOCTAVESEL ] = {};

    // patterns
    int             m_CurrentPattern = 0;
    PATTERN_STRUCT  m_PatternNotes[ nPATTERNS ][ nKEYBOARDS ] = {};
    float           m_fLightPatterns[ nPATTERNS ] = {};
    SchmittTrigger  m_SchTrigPatternSelect[ nPATTERNS ] = {};
    SchmittTrigger  m_SchTrigPatternSelectInput;

    // number of steps
    int             m_nSteps = nPATTERNS;    
    SchmittTrigger  m_SchTrigStepNumbers[ nPATTERNS ];
    float           m_fLightStepNumbers[ nPATTERNS ] = {};

    // pause button
    float           m_fLightPause = 0.0;
    bool            m_bPause = false;

    // triggers     
    bool            m_bTrig[ nKEYBOARDS ] = {};
    PulseGenerator  m_gatePulse[ nKEYBOARDS ];

    // copy next button
    float           m_fLightCopyNext = {};
    bool            m_bCopy = false;

    // Contructor
	Seq_Triad() : Module(nPARAMS, nINPUTS, nOUTPUTS){}

    // Overrides 
	void    step() override;
    json_t* toJson() override;
    void    fromJson(json_t *rootJ) override;
    void    initialize() override;
    void    randomize() override{}
    //void    reset() override;
    void    SetSteps( int steps );
    void    SetKey( int kb );
    void    SetOut( int kb );
    void    ChangePattern( int index, bool bForce );
    void    CopyNext( void );
};

//-----------------------------------------------------
// Procedure:   MySquareButton_CopyNext
//
//-----------------------------------------------------
struct MySquareButton_Cpy : MySquareButton
{
    Seq_Triad *mymodule;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && value == 1.0 )
        {
            mymodule->CopyNext();
        }

		MomentarySwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   MySquareButton_Step
//
//-----------------------------------------------------
struct MySquareButton_Pause : MySquareButton 
{
    Seq_Triad *mymodule;
    int stp, i;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && value == 1.0 )
        {
            mymodule->m_bPause = !mymodule->m_bPause;

            mymodule->m_fLightPause = mymodule->m_bPause ? 1.0 : 0.0;
        }

		MomentarySwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   MySquareButton_Step
//
//-----------------------------------------------------
struct MySquareButton_Stp : MySquareButton 
{
    Seq_Triad *mymodule;
    int stp, i;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && value == 1.0 )
        {
            stp = paramId - Seq_Triad::PARAM_STEP_NUM;
            mymodule->SetSteps( stp + 1 );
        }

		MomentarySwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   MyPianoWhiteKey
//
//-----------------------------------------------------
struct MyPianoWhiteKey : PianoWhiteKey
{
    Seq_Triad *mymodule;
    int param, kb, note, key;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && mymodule->m_bJsonInit && value == 1.0 )
        {
            param = paramId - Seq_Triad::PARAM_WKEYS;
            kb = param / nWHITEKEYS;
            note = param - (kb * nWHITEKEYS);

            mymodule->m_PatternNotes[ mymodule->m_CurrentPattern ][ kb ].note = whiteKeyoff[ note ];
            mymodule->SetKey( kb );
            mymodule->SetOut( kb );
        }

        SVGSwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   MyPianoBlackKey
//
//-----------------------------------------------------
struct MyPianoBlackKey : PianoBlackKey
{
    Seq_Triad *mymodule;
    int param, kb, note, key;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && mymodule->m_bJsonInit && value == 1.0 )
        {
            param = paramId - Seq_Triad::PARAM_BKEYS;
            kb = param / nBLACKKEYS;
            note = param - (kb * nBLACKKEYS);

            mymodule->m_PatternNotes[ mymodule->m_CurrentPattern ][ kb ].note = blackKeyoff[ note ];

            mymodule->SetKey( kb );
            mymodule->SetOut( kb );
        }

        SVGSwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   MySquareButton_Pat
//
//-----------------------------------------------------
struct MySquareButton_Pat : MySquareButton 
{
    Seq_Triad *mymodule;
    int stp, i;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && value == 1.0 )
        {
            stp = paramId - Seq_Triad::PARAM_PATTERNS;
            mymodule->ChangePattern( stp, false );
        }

		MomentarySwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   MyOCTButton
//
//-----------------------------------------------------
struct MyOCTButton : MySquareButton
{
    int kb, oct;
    Seq_Triad *mymodule;
    int param;

    void onChange() override 
    {
        mymodule = (Seq_Triad*)module;

        if( mymodule && value == 1.0 )
        {
            param = paramId - Seq_Triad::PARAM_OCTAVES;
            kb = param / nOCTAVESEL;
            oct = param - (kb * nOCTAVESEL);

            mymodule->m_fLightOctaves[ kb ][ 0 ] = 0.0;
            mymodule->m_fLightOctaves[ kb ][ 1 ] = 0.0;
            mymodule->m_fLightOctaves[ kb ][ 2 ] = 0.0;
            mymodule->m_fLightOctaves[ kb ][ 3 ] = 0.0;
            mymodule->m_fLightOctaves[ kb ][ oct ] = 1.0;

            mymodule->m_PatternNotes[ mymodule->m_CurrentPattern ][ kb ].oct = oct;

            mymodule->SetOut( kb );
        }

		MomentarySwitch::onChange();
	}
};

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
int bkeyoff[ nBLACKKEYS ] = { 26, 41, 66, 79, 92, 117, 132, 157, 170, 183, 209, 222, 248, 261, 274 };
Seq_Triad_Widget::Seq_Triad_Widget() 
{
    ParamWidget *pWidget;
    int kb, key, oct, x, y, pat, stp;
	Seq_Triad *module = new Seq_Triad();
	setModule(module);
	box.size = Vec( 15*22, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/TriadSequencer.svg")));
		addChild(panel);
	}

    module->lg.Open("StickyKeysLog.txt");

    //----------------------------------------------------
    // Step Select buttons 

    y = 10;
    x = 45;

	for ( stp = 0; stp < nPATTERNS; stp++ ) 
    {
        // step button
		addParam(createParam<MySquareButton_Stp>( Vec( x, y ), module, Seq_Triad::PARAM_STEP_NUM + stp, 0.0, 1.0, 0.0 ) );
		addChild(createValueLight<SmallLight<DarkRedValueLight>>( Vec( x + 1, y + 2 ), &module->m_fLightStepNumbers[ stp ] ) );

        x += 15;

        module->m_fLightStepNumbers[ stp ] = 1.0;
    }

    // pause button
	addParam(createParam<MySquareButton_Pause>(Vec( 45, 48 ), module, Seq_Triad::PARAM_PAUSE, 0.0, 1.0, 0.0 ) );
	addChild(createValueLight<SmallLight<RedValueLight>>( Vec( 45 + 1, 48 + 2 ), &module->m_fLightPause ) );

    //----------------------------------------------------
    // Pattern Select buttons 
    y = 32;
    x = 45;

    addInput(createInput<MyPortInSmall>( Vec( x - 30, y - 3 ), module, Seq_Triad::IN_PATTERN_TRIG ) );

	for ( pat = 0; pat < nPATTERNS; pat++ ) 
    {
        // step button
		addParam(createParam<MySquareButton_Pat>( Vec( x, y ), module, Seq_Triad::PARAM_PATTERNS + pat, 0.0, 1.0, 0.0 ) );
		addChild(createValueLight<SmallLight<OrangeValueLight>>( Vec( x + 1, y + 2 ), &module->m_fLightPatterns[ pat ] ) );

        x += 15;
    }

    // copy next button
	addParam(createParam<MySquareButton_Cpy>(Vec( 290, 32 ), module, Seq_Triad::PARAM_COPY_NEXT, 0.0, 1.0, 0.0 ) );
	addChild(createValueLight<SmallLight<CyanValueLight>>( Vec( 290 + 1, 32 + 2 ), &module->m_fLightCopyNext ) );

    //----------------------------------------------------
    // Keyboard Keys 
    y = CHANNEL_Y;

    for( kb = 0; kb < nKEYBOARDS; kb++ )
    {
        x = 230;

        for( oct = 0; oct < nOCTAVESEL; oct++ )
        {
            addParam(createParam<MyOCTButton>( Vec( x, y - 17), module, Seq_Triad::PARAM_OCTAVES + ( kb * nOCTAVESEL) + oct, 0.0, 1.0, 0.0 ) );
            addChild(createValueLight<SmallLight<CyanValueLight>>( Vec( x + 1, y -15 ), &module->m_fLightOctaves[ kb ][ oct ] ) );
            x += 18;
        }

        x = CHANNEL_X;

        // add white keys
        for( key = 0; key < nWHITEKEYS; key++ )
        {
            pWidget = createParam<MyPianoWhiteKey>( Vec( x, y ), module, Seq_Triad::PARAM_WKEYS + ( kb * nWHITEKEYS) + key, 0.0, 1.0, 0.0 );
            addParam( pWidget );

            module->m_Keys[ kb ][ whiteKeyoff[ key ] ].pWidget = pWidget;
            module->m_Keys[ kb ][ whiteKeyoff[ key ] ].fsemi = fWhiteKeyNotes[ key ];

            x += 13;
        }

        for( key = 0; key < nBLACKKEYS; key++ )
        {
            pWidget = createParam<MyPianoBlackKey>( Vec( bkeyoff[ key ] -3, y ), module, Seq_Triad::PARAM_BKEYS + ( kb * nBLACKKEYS) + key, 0.0, 1.0, 0.0 );
            addParam( pWidget );

            module->m_Keys[ kb ][ blackKeyoff[ key ] ].pWidget = pWidget;
            module->m_Keys[ kb ][ blackKeyoff[ key ] ].fsemi = fBlackKeyNotes[ key ];
        }

        y += CHANNEL_OFF_Y;
    }

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365))); 
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    // outputs
    x = 307;
    y = CHANNEL_Y;
    addOutput(createOutput<MyPortOutSmall>( Vec( x, y ), module, Seq_Triad::OUT_VOCTS ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( x, y + 50 ), module, Seq_Triad::OUT_TRIG ) );
    y += CHANNEL_OFF_Y;
    addOutput(createOutput<MyPortOutSmall>( Vec( x, y ), module, Seq_Triad::OUT_VOCTS + 1 ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( x, y + 50 ), module, Seq_Triad::OUT_TRIG + 1 ) );
    y += CHANNEL_OFF_Y;
    addOutput(createOutput<MyPortOutSmall>( Vec( x, y ), module, Seq_Triad::OUT_VOCTS + 2 ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( x, y + 50 ), module, Seq_Triad::OUT_TRIG + 2 ) );
}

//-----------------------------------------------------
// Procedure:   CopyNext
//
//-----------------------------------------------------
void Seq_Triad::CopyNext( void )
{
    if( m_CurrentPattern < ( nPATTERNS - 1 ) )
    {
        memcpy( &m_PatternNotes[ m_CurrentPattern + 1 ][ 0 ], &m_PatternNotes[ m_CurrentPattern ][ 0 ], sizeof(PATTERN_STRUCT ) );
        memcpy( &m_PatternNotes[ m_CurrentPattern + 1 ][ 1 ], &m_PatternNotes[ m_CurrentPattern ][ 1 ], sizeof(PATTERN_STRUCT ) );
        memcpy( &m_PatternNotes[ m_CurrentPattern + 1 ][ 2 ], &m_PatternNotes[ m_CurrentPattern ][ 2 ], sizeof(PATTERN_STRUCT ) );

        ChangePattern( m_CurrentPattern + 1, true );

        m_fLightCopyNext = 1.0;
        m_bCopy = true;
    }
}

//-----------------------------------------------------
// Procedure:   SetSteps
//
//-----------------------------------------------------
void Seq_Triad::SetSteps( int nSteps )
{
    int i;

    if( nSteps < 1 || nSteps > nPATTERNS )
        nSteps = nPATTERNS;

    m_nSteps = nSteps;

	for ( i = 0; i < nPATTERNS; i++ ) 
    {
        // level button
		if( i < nSteps )
            m_fLightStepNumbers[ i ] = 1.0f;
        else
            m_fLightStepNumbers[ i ] = 0.0f;
    }
}

//-----------------------------------------------------
// Procedure:   SetOut
//
//-----------------------------------------------------
void Seq_Triad::SetOut( int kb )
{
    int note;
    float foct;

    foct = (float)m_PatternNotes[ m_CurrentPattern ][ kb ].oct;
    note = m_PatternNotes[ m_CurrentPattern ][ kb ].note;

    m_fCvOut[ kb ] = foct + m_Keys[ kb ][ note ].fsemi;

    m_bTrig[ kb ] = true;
}

//-----------------------------------------------------
// Procedure:   SetKey
//
//-----------------------------------------------------
void Seq_Triad::SetKey( int kb )
{
    int i;

    for( i = 0; i < nKEYS; i ++ )
        m_Keys[ kb ][ i ].pWidget->setValue( 0.0 );

    m_Keys[ kb ][ m_PatternNotes[ m_CurrentPattern ][ kb ].note ].pWidget->setValue( 0.9 );
}

//-----------------------------------------------------
// Procedure:   ChangePattern
//
//-----------------------------------------------------
void Seq_Triad::ChangePattern( int index, bool bForce )
{
    int kb;

    if( !bForce && index == m_CurrentPattern )
        return;

    if( index < 0 )
        index = nPATTERNS - 1;
    else if( index >= nPATTERNS )
        index = 0;

    m_CurrentPattern = index;

    // change pattern light
    memset( m_fLightPatterns, 0, sizeof(m_fLightPatterns) );
    m_fLightPatterns[ index ] = 1.0;

    memset( m_fLightOctaves, 0, sizeof(m_fLightOctaves) );

    // change key select
    for( kb = 0; kb < nKEYBOARDS; kb ++ )
    {
        // set keyboard key
        SetKey( kb );

        // change octave light
        m_fLightOctaves[ kb ][ m_PatternNotes[ m_CurrentPattern ][ kb ].oct ] = 1.0;

        // set outputted note
        SetOut( kb );
    }
}

//-----------------------------------------------------
// Procedure:   initialize
//
//-----------------------------------------------------
void Seq_Triad::initialize()
{
    memset( m_fLightOctaves, 0, sizeof(m_fLightOctaves) );
    memset( m_fCvOut, 0, sizeof(m_fCvOut) );
    memset( m_PatternNotes, 0, sizeof(m_PatternNotes) );
    
    ChangePattern( 0, true );
}

//-----------------------------------------------------
// Procedure:   
//
//-----------------------------------------------------
json_t *Seq_Triad::toJson() 
{
    int *pint;
    json_t *gatesJ;
	json_t *rootJ = json_object();

    // number of steps
	gatesJ = json_integer( m_nSteps );
	json_object_set_new(rootJ, "numberofsteps", gatesJ );

    // patterns
    pint = (int*)&m_PatternNotes[ 0 ][ 0 ];

	gatesJ = json_array();

	for (int i = 0; i < (nPATTERNS * nKEYBOARDS * 2); i++)
    {
		json_t *gateJ = json_integer( pint[ i ] );
		json_array_append_new( gatesJ, gateJ );
	}

	json_object_set_new( rootJ, "patterns", gatesJ );

	return rootJ;
}

//-----------------------------------------------------
// Procedure:   fromJson
//
//-----------------------------------------------------
void Seq_Triad::fromJson(json_t *rootJ) 
{
    int *pint;
    int i;
    json_t *StepsJ;

    // number of steps
	StepsJ = json_object_get(rootJ, "numberofsteps");
	if (StepsJ)
		m_nSteps = json_integer_value( StepsJ );

    // patterns
    pint = (int*)&m_PatternNotes[ 0 ][ 0 ];

	StepsJ = json_object_get( rootJ, "patterns" );

	if (StepsJ) 
    {
		for ( i = 0; i < (nPATTERNS * nKEYBOARDS * 2); i++)
        {
			json_t *gateJ = json_array_get(StepsJ, i);

			if (gateJ)
				pint[ i ] = json_integer_value( gateJ );
		}
	}

    SetSteps( m_nSteps );
    ChangePattern( 0, true );

    m_bJsonInit = true;
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
#define LIGHT_LAMBDA ( 0.065f )
void Seq_Triad::step() 
{
    int i;

    if( m_bCopy )
    {
        m_fLightCopyNext -= m_fLightCopyNext / LIGHT_LAMBDA / gSampleRate;

        if( m_fLightCopyNext < 0.001 )
        {
            m_bCopy = false;
            m_fLightCopyNext = 0.0;
        }
    }

	// pat change trigger - ignore if already pending
    if( inputs[ IN_PATTERN_TRIG ].active && !m_bPause )
    {
        if( m_CurrentPattern >= (m_nSteps-1 ) )
        {
            m_CurrentPattern = (nPATTERNS - 1);
        }

	    if( m_SchTrigPatternSelectInput.process( inputs[ IN_PATTERN_TRIG ].value ) ) 
        {
            ChangePattern( m_CurrentPattern + 1, true );
        }
    }

    // process triggers
    for( i = 0; i < nKEYBOARDS; i++ )
    {
        if( m_bTrig[ i ] )
        {
            m_bTrig[ i ] = false;
            m_gatePulse[ i ].trigger(1e-3);
        }

        outputs[ OUT_TRIG + i ].value = m_gatePulse[ i ].process( 1.0 / gSampleRate ) ? 10.0 : 0.0;
    }

    outputs[ OUT_VOCTS ].value = m_fCvOut[ 0 ];
    outputs[ OUT_VOCTS + 1 ].value = m_fCvOut[ 1 ];
    outputs[ OUT_VOCTS + 2 ].value = m_fCvOut[ 2 ];
}
