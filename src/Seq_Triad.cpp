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
#define nPHRASE_SAVES 16

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
    int     note;
    int     oct;
    bool    bTrigOff;
    int     pad[ 5 ];

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
        PARAM_GLIDE             = PARAM_BKEYS + (nWHITEKEYS * nKEYBOARDS),
        PARAM_TRIGOFF           = PARAM_GLIDE + ( nKEYBOARDS ),
        PARAM_PHRASE_SAVES      = PARAM_TRIGOFF + ( nKEYBOARDS ),
        nPARAMS                 = PARAM_PHRASE_SAVES + ( nKEYBOARDS * nPHRASE_SAVES )
    };

	enum InputIds 
    {
        IN_PATTERN_TRIG,
        IN_VOCT_OFF,
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

    bool            m_bChangeAllPatternOctaves = false;
    bool            m_bChangeAllPatternNotes = false;

    KEYBOARD_KEY_STRUCT m_Keys[ nKEYBOARDS ][ nKEYS ];

    float           m_fCvStartOut[ nKEYBOARDS ] = {};
    float           m_fCvEndOut[ nKEYBOARDS ] = {};
    float           m_fLightOctaves[ nKEYBOARDS ][ nOCTAVESEL ] = {};

    // patterns
    int             m_CurrentPattern = 0;
    PATTERN_STRUCT  m_PatternNotes[ nKEYBOARDS ][ nPHRASE_SAVES ][ nPATTERNS ] = {};
    float           m_fLightPatterns[ nPATTERNS ] = {};
    SchmittTrigger  m_SchTrigPatternSelect[ nPATTERNS ] = {};
    SchmittTrigger  m_SchTrigPatternSelectInput;

    // phrase save
    int             m_CurrentPhrase[ nKEYBOARDS ] = {};
    float           m_fLightPhraseSaves[ nKEYBOARDS ][ nPHRASE_SAVES ] = {};

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

    // trig off
    float           m_fLightTrig[ nKEYBOARDS ];

    // glide
    float           m_fglideInc[ nKEYBOARDS ] = {};
    int             m_glideCount[ nKEYBOARDS ] = {};
    float           m_fglide[ nKEYBOARDS ] = {};
    float           m_fLastNotePlayed[ nKEYBOARDS ];
    bool            m_bWasLastNotePlayed[ nKEYBOARDS ] = {};

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
    void    randomize() override;
    //void    reset() override;
    void    SetSteps( int steps );
    void    SetKey( int kb );
    void    SetOut( int kb );
    void    ChangePattern( int index, bool bForce );
    void    ChangePhrase( int kb, int index, bool bForce );
    void    CopyNext( void );

    //-----------------------------------------------------
    // MySquareButton_Phrase
    //-----------------------------------------------------
    struct MySquareButton_Phrase : MySquareButtonSmall 
    {
        Seq_Triad *mymodule;
        int i, kb, param;

        void onChange() override 
        {
            mymodule = (Seq_Triad*)module;

            if( mymodule && value == 1.0 )
            {
                param = paramId - Seq_Triad::PARAM_PHRASE_SAVES;
                kb = param / nPHRASE_SAVES;
                i = param - (kb * nPHRASE_SAVES);

                mymodule->ChangePhrase( kb, i, false );
            }

		    MomentarySwitch::onChange();
	    }
    };

    //-----------------------------------------------------
    // MySquareButton_Trig
    //-----------------------------------------------------
    struct MySquareButton_Trig : MySquareButton
    {
        int kb;

        Seq_Triad *mymodule;

        void onChange() override 
        {
            mymodule = (Seq_Triad*)module;

            if( mymodule && value == 1.0 )
            {
                kb = paramId - Seq_Triad::PARAM_TRIGOFF;

                mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ mymodule->m_CurrentPattern ].bTrigOff = !mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ mymodule->m_CurrentPattern ].bTrigOff;
                mymodule->m_fLightTrig[ kb ] = mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ mymodule->m_CurrentPattern ].bTrigOff ? 1.0 : 0.0;
            }

		    MomentarySwitch::onChange();
	    }
    };

    //-----------------------------------------------------
    // MySquareButton_CopyNext
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
    // MySquareButton_Step
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
    // MySquareButton_Step
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
    // MyPianoWhiteKey
    //-----------------------------------------------------
    struct MyPianoWhiteKey : PianoWhiteKey
    {
        Seq_Triad *mymodule;
        int param, kb, note, key, i;

        void onChange() override 
        {
            mymodule = (Seq_Triad*)module;

            if( mymodule && value == 1.0 )
            {
                param = paramId - Seq_Triad::PARAM_WKEYS;
                kb = param / nWHITEKEYS;
                note = param - (kb * nWHITEKEYS);

                if( mymodule->m_bChangeAllPatternNotes )
                {
                    for( i = 0; i < nPATTERNS; i++ )
                    {
                        mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ i ].note = whiteKeyoff[ note ];
                    }
                }
                else
                {
                    mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ mymodule->m_CurrentPattern ].note = whiteKeyoff[ note ];
                }

                mymodule->SetKey( kb );
                mymodule->SetOut( kb );
            }

            SVGSwitch::onChange();
	    }
    };

    //-----------------------------------------------------
    // MyPianoBlackKey
    //-----------------------------------------------------
    struct MyPianoBlackKey : PianoBlackKey
    {
        Seq_Triad *mymodule;
        int param, kb, note, key, i;

        void onChange() override 
        {
            mymodule = (Seq_Triad*)module;

            if( mymodule && value == 1.0 )
            {
                param = paramId - Seq_Triad::PARAM_BKEYS;
                kb = param / nBLACKKEYS;
                note = param - (kb * nBLACKKEYS);

                if( mymodule->m_bChangeAllPatternNotes )
                {
                    for( i = 0; i < nPATTERNS; i++ )
                    {
                        mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ i ].note = blackKeyoff[ note ];
                    }
                }
                else
                {
                    mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ mymodule->m_CurrentPattern ].note = blackKeyoff[ note ];
                }

                mymodule->SetKey( kb );
                mymodule->SetOut( kb );
            }

            SVGSwitch::onChange();
	    }
    };

    //-----------------------------------------------------
    // MySquareButton_Pat
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
    // MyOCTButton
    //-----------------------------------------------------
    struct MyOCTButton : MySquareButton
    {
        int kb, oct, i;
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

                if( mymodule->m_bChangeAllPatternOctaves )
                {
                    for( i = 0; i < nPATTERNS; i++ )
                        mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ i ].oct = oct;
                }
                else
                {
                    mymodule->m_PatternNotes[ kb ][ mymodule->m_CurrentPhrase[ kb ] ][ mymodule->m_CurrentPattern ].oct = oct;
                }

                mymodule->SetOut( kb );
            }

		    MomentarySwitch::onChange();
	    }
    };
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

    //module->lg.Open("StickyKeysLog.txt");

    //----------------------------------------------------
    // Step Select buttons 

    y = 10;
    x = 45;

	for ( stp = 0; stp < nPATTERNS; stp++ ) 
    {
        // step button
		addParam(createParam<Seq_Triad::MySquareButton_Stp>( Vec( x, y ), module, Seq_Triad::PARAM_STEP_NUM + stp, 0.0, 1.0, 0.0 ) );
		addChild(createValueLight<SmallLight<DarkRedValueLight>>( Vec( x + 1, y + 2 ), &module->m_fLightStepNumbers[ stp ] ) );

        x += 15;

        module->m_fLightStepNumbers[ stp ] = 1.0;
    }

    // pause button
	addParam(createParam<Seq_Triad::MySquareButton_Pause>(Vec( 45, 48 ), module, Seq_Triad::PARAM_PAUSE, 0.0, 1.0, 0.0 ) );
	addChild(createValueLight<SmallLight<RedValueLight>>( Vec( 45 + 1, 48 + 2 ), &module->m_fLightPause ) );

    //----------------------------------------------------
    // Pattern Select buttons 
    y = 32;
    x = 45;

    addInput(createInput<MyPortInSmall>( Vec( x - 30, y - 3 ), module, Seq_Triad::IN_PATTERN_TRIG ) );

	for ( pat = 0; pat < nPATTERNS; pat++ ) 
    {
        // pattern button
		addParam(createParam<Seq_Triad::MySquareButton_Pat>( Vec( x, y ), module, Seq_Triad::PARAM_PATTERNS + pat, 0.0, 1.0, 0.0 ) );
		addChild(createValueLight<SmallLight<OrangeValueLight>>( Vec( x + 1, y + 2 ), &module->m_fLightPatterns[ pat ] ) );

        x += 15;
    }

    // copy next button
	addParam(createParam<Seq_Triad::MySquareButton_Cpy>(Vec( 290, 32 ), module, Seq_Triad::PARAM_COPY_NEXT, 0.0, 1.0, 0.0 ) );
	addChild(createValueLight<SmallLight<CyanValueLight>>( Vec( 290 + 1, 32 + 2 ), &module->m_fLightCopyNext ) );

    //----------------------------------------------------
    // Keyboard Keys 
    y = CHANNEL_Y;

    for( kb = 0; kb < nKEYBOARDS; kb++ )
    {
        x = 45; 

        // trig button
        addParam(createParam<Seq_Triad::MySquareButton_Trig>( Vec( x, y - 15 ), module, Seq_Triad::PARAM_TRIGOFF + kb, 0.0, 1.0, 0.0 ) );
        addChild(createValueLight<SmallLight<RedValueLight>>( Vec( x + 1, y - 13 ), &module->m_fLightTrig[ kb ] ) );

        // glide knob
        addParam( createParam<Yellow1_Tiny>( Vec( 120, y - 17 ), module, Seq_Triad::PARAM_GLIDE + kb, 0.0, 1.0, 0.0 ) );

        x = 230;

        for( oct = 0; oct < nOCTAVESEL; oct++ )
        {
            addParam(createParam<Seq_Triad::MyOCTButton>( Vec( x, y - 17), module, Seq_Triad::PARAM_OCTAVES + ( kb * nOCTAVESEL) + oct, 0.0, 1.0, 0.0 ) );
            addChild(createValueLight<SmallLight<CyanValueLight>>( Vec( x + 1, y -15 ), &module->m_fLightOctaves[ kb ][ oct ] ) );
            x += 18;
        }

        x = CHANNEL_X;

        // add white keys
        for( key = 0; key < nWHITEKEYS; key++ )
        {
            pWidget = createParam<Seq_Triad::MyPianoWhiteKey>( Vec( x, y ), module, Seq_Triad::PARAM_WKEYS + ( kb * nWHITEKEYS) + key, 0.0, 1.0, 0.0 );
            addParam( pWidget );

            module->m_Keys[ kb ][ whiteKeyoff[ key ] ].pWidget = pWidget;
            module->m_Keys[ kb ][ whiteKeyoff[ key ] ].fsemi = fWhiteKeyNotes[ key ];

            x += 13;
        }

        for( key = 0; key < nBLACKKEYS; key++ )
        {
            pWidget = createParam<Seq_Triad::MyPianoBlackKey>( Vec( bkeyoff[ key ] -3, y ), module, Seq_Triad::PARAM_BKEYS + ( kb * nBLACKKEYS) + key, 0.0, 1.0, 0.0 );
            addParam( pWidget );

            module->m_Keys[ kb ][ blackKeyoff[ key ] ].pWidget = pWidget;
            module->m_Keys[ kb ][ blackKeyoff[ key ] ].fsemi = fBlackKeyNotes[ key ];
        }

        x = 65;

        for( pat = 0; pat < nPHRASE_SAVES; pat++ )
        {
            addParam(createParam<Seq_Triad::MySquareButton_Phrase>( Vec( x, y + 69), module, Seq_Triad::PARAM_PHRASE_SAVES + ( kb * nPHRASE_SAVES) + pat, 0.0, 1.0, 0.0 ) );
            addChild(createValueLight<TinyLight<CyanValueLight>>( Vec( x + 2, y + 69 + 2 ), &module->m_fLightPhraseSaves[ kb ][ pat ] ) );
            x += 12;
        }

        y += CHANNEL_OFF_Y;
    }

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365))); 
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    // VOCT offset input
    addInput(createInput<MyPortInSmall>( Vec( 40, 360 ), module, Seq_Triad::IN_VOCT_OFF ) );

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
// Procedure:   initialize
//
//-----------------------------------------------------
struct Seq_TriadItem : MenuItem 
{
	Seq_Triad *seq;
	bool bAllOctave;

	void onAction() override 
    {
		seq->m_bChangeAllPatternOctaves = bAllOctave;
	}
	void step() override 
    {
		rightText = (seq->m_bChangeAllPatternOctaves == bAllOctave) ? "✔" : "";
	}
};

//-----------------------------------------------------
// Procedure:   initialize
//
//-----------------------------------------------------
struct Seq_TriadItem2 : MenuItem 
{
	Seq_Triad *seq;
	bool bAllNotes;

	void onAction() override 
    {
		seq->m_bChangeAllPatternNotes = bAllNotes;
	}
	void step() override 
    {
		rightText = (seq->m_bChangeAllPatternNotes == bAllNotes) ? "✔" : "";
	}
};

//-----------------------------------------------------
// Procedure:   initialize
//
//-----------------------------------------------------
Menu *Seq_Triad_Widget::createContextMenu() 
{
	Menu *menu = ModuleWidget::createContextMenu();

	MenuLabel *spacerLabel = new MenuLabel();
	menu->pushChild(spacerLabel);

	Seq_Triad *seq = dynamic_cast<Seq_Triad*>(module);
	assert(seq);

	MenuLabel *modeLabel = new MenuLabel();
	modeLabel->text = "Octave Change";
	menu->pushChild(modeLabel);

	Seq_TriadItem *ChangeAllOffItem = new Seq_TriadItem();
	ChangeAllOffItem->text = "Change All Patterns Off";
	ChangeAllOffItem->seq = seq;
	ChangeAllOffItem->bAllOctave = false;
	menu->pushChild(ChangeAllOffItem);

	Seq_TriadItem *ChangeAllOnItem = new Seq_TriadItem();
	ChangeAllOnItem->text = "Change All Patterns";
	ChangeAllOnItem->seq = seq;
	ChangeAllOnItem->bAllOctave = true;
	menu->pushChild(ChangeAllOnItem);


	modeLabel = new MenuLabel();
	modeLabel->text = "Note Change";
	menu->pushChild(modeLabel);

	Seq_TriadItem2 *ChangeAllOffItem2 = new Seq_TriadItem2();
	ChangeAllOffItem2->text = "Change All Notes Off";
	ChangeAllOffItem2->seq = seq;
	ChangeAllOffItem2->bAllNotes = false;
	menu->pushChild(ChangeAllOffItem2);

	Seq_TriadItem2 *ChangeAllOnItem2 = new Seq_TriadItem2();
	ChangeAllOnItem2->text = "Change All Notes";
	ChangeAllOnItem2->seq = seq;
	ChangeAllOnItem2->bAllNotes = true;
	menu->pushChild(ChangeAllOnItem2);

	return menu;
}

//-----------------------------------------------------
// Procedure:   initialize
//
//-----------------------------------------------------
void Seq_Triad::initialize()
{
    memset( m_fLightOctaves, 0, sizeof(m_fLightOctaves) );
    memset( m_fCvStartOut, 0, sizeof(m_fCvStartOut) );
    memset( m_fCvEndOut, 0, sizeof(m_fCvEndOut) );
    memset( m_PatternNotes, 0, sizeof(m_PatternNotes) );
    
    ChangePattern( 0, true );
}

//-----------------------------------------------------
// Procedure:   randomize
//
//-----------------------------------------------------
int keyscalenotes[ 7 ] = { 0, 2, 4, 5, 7, 9, 11};
int keyscalenotes_minor[ 7 ] = { 0, 2, 3, 5, 7, 9, 11};
void Seq_Triad::randomize()
{
    int kb, pat, phrase, basekey, note, oct;

    memset( m_fLightOctaves, 0, sizeof(m_fLightOctaves) );
    memset( m_fCvStartOut, 0, sizeof(m_fCvStartOut) );
    memset( m_fCvEndOut, 0, sizeof(m_fCvEndOut) );
    memset( m_PatternNotes, 0, sizeof(m_PatternNotes) );

    basekey = (int)(randomf() * 24.4);
    oct = (int)( randomf() * 3.4 );

    for( kb = 0; kb < nKEYBOARDS; kb++ )
    {
        for( pat = 0; pat < nPATTERNS; pat++ )
        {
            for( phrase = 0; phrase < nPHRASE_SAVES; phrase++ )
            {
                if( randomf() > 0.7 )
                    note = keyscalenotes_minor[ (int)(randomf() * 7.4 ) ];
                else
                    note = keyscalenotes[ (int)(randomf() * 7.4 ) ];

                m_PatternNotes[ kb ][ phrase ][ pat ].bTrigOff = ( randomf() < 0.10 );
                m_PatternNotes[ kb ][ phrase ][ pat ].note = basekey + note; 
                m_PatternNotes[ kb ][ phrase ][ pat ].oct = oct;
            }
        }
    }

    ChangePattern( 0, true );
}

//-----------------------------------------------------
// Procedure:   CopyNext
//
//-----------------------------------------------------
void Seq_Triad::CopyNext( void )
{
    if( m_CurrentPattern < ( nPATTERNS - 1 ) )
    {
        memcpy( &m_PatternNotes[ 0 ][ m_CurrentPattern + 1 ], &m_PatternNotes[ 0 ][ m_CurrentPattern ], sizeof(PATTERN_STRUCT ) );
        memcpy( &m_PatternNotes[ 1 ][ m_CurrentPattern + 1 ], &m_PatternNotes[ 1 ][ m_CurrentPattern ], sizeof(PATTERN_STRUCT ) );
        memcpy( &m_PatternNotes[ 2 ][ m_CurrentPattern + 1 ], &m_PatternNotes[ 2 ][ m_CurrentPattern ], sizeof(PATTERN_STRUCT ) );

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
    float foct, offset;

    offset = inputs[ IN_VOCT_OFF ].normalize( 0.0 );

    // end glide note (current pattern note)
    foct = (float)m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].oct;
    note = m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].note;
    m_fCvEndOut[ kb ] = foct + m_Keys[ kb ][ note ].fsemi + offset;

    // start glide note (last pattern note)
    if( m_bWasLastNotePlayed[ kb ] )
    {
        m_fCvStartOut[ kb ] = m_fLastNotePlayed[ kb ] + offset;
    }
    else
    {
        m_bWasLastNotePlayed[ kb ] = true;
        m_fCvStartOut[ kb ] = m_fCvEndOut[ kb ] + offset;
    }

    m_fLastNotePlayed[ kb ] = m_fCvEndOut[ kb ] + offset;

    // glide time ( max glide = 0.5 seconds )
    m_glideCount[ kb ] = 1 + (int)( ( params[ PARAM_GLIDE + kb ].value * 0.5 ) * gSampleRate);
    m_fglideInc[ kb ] = 1.0 / (float)m_glideCount[ kb ];
    m_fglide[ kb ] = 1.0;

    if( !m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].bTrigOff )
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

    m_Keys[ kb ][ m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].note ].pWidget->setValue( 0.9 );
}

//-----------------------------------------------------
// Procedure:   ChangePhrase
//
//-----------------------------------------------------
void Seq_Triad::ChangePhrase( int kb, int index, bool bForce )
{
    if( !bForce && index == m_CurrentPhrase[ kb ] )
        return;

    if( index < 0 )
        index = nPHRASE_SAVES - 1;
    else if( index >= nPHRASE_SAVES )
        index = 0;

    m_CurrentPhrase[ kb ] = index;

    // change phrase save light
    memset ( &m_fLightPhraseSaves[ kb ], 0, sizeof(float) * nPHRASE_SAVES );
    m_fLightPhraseSaves[ kb ][ index ] = 1.0;

    // set keyboard key
    SetKey( kb );

    m_fLightTrig[ kb ] = m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].bTrigOff ? 1.0 : 0.0;

    // change octave light
     memset( m_fLightOctaves, 0, sizeof(m_fLightOctaves) );
    m_fLightOctaves[ kb ][ m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].oct ] = 1.0;

    // set outputted note
    SetOut( kb );
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

        m_fLightTrig[ kb ] = m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].bTrigOff ? 1.0 : 0.0;

        // change octave light
        m_fLightOctaves[ kb ][ m_PatternNotes[ kb ][ m_CurrentPhrase[ kb ] ][ m_CurrentPattern ].oct ] = 1.0;

        // set outputted note
        SetOut( kb );
    }
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

    json_object_set_new(rootJ, "m_bChangeAllPatternOctaves", json_boolean (m_bChangeAllPatternOctaves));

    json_object_set_new(rootJ, "m_bChangeAllPatternNotes", json_boolean (m_bChangeAllPatternNotes));

    // number of steps
	gatesJ = json_integer( m_nSteps );
	json_object_set_new(rootJ, "numberofsteps", gatesJ );

    // phrase select
    pint = (int*)&m_CurrentPhrase[ 0 ];

	gatesJ = json_array();

	for (int i = 0; i < nKEYBOARDS; i++)
    {
		json_t *gateJ = json_integer( pint[ i ] );
		json_array_append_new( gatesJ, gateJ );
	}

	json_object_set_new( rootJ, "m_CurrentPhrase", gatesJ );

    // patterns
    pint = (int*)&m_PatternNotes[ 0 ][ 0 ][ 0 ];

	gatesJ = json_array();

	for (int i = 0; i < (nPHRASE_SAVES * nPATTERNS * nKEYBOARDS * 8); i++)
    {
		json_t *gateJ = json_integer( pint[ i ] );
		json_array_append_new( gatesJ, gateJ );
	}

	json_object_set_new( rootJ, "m_PatternNotes", gatesJ );

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

	json_t *revJ = json_object_get(rootJ, "m_bChangeAllPatternOctaves");

	if (revJ)
		m_bChangeAllPatternOctaves = json_is_true( revJ );

    revJ = json_object_get(rootJ, "m_bChangeAllPatternNotes");

 	if (revJ)
		m_bChangeAllPatternNotes = json_is_true( revJ );   

    // phrase select
    pint = (int*)&m_CurrentPhrase[ 0 ];
	StepsJ = json_object_get( rootJ, "m_CurrentPhrase" );

	if (StepsJ) 
    {
		for ( i = 0; i < nKEYBOARDS; i++)
        {
			json_t *gateJ = json_array_get(StepsJ, i);

			if (gateJ)
				pint[ i ] = json_integer_value( gateJ );
		}
	}

    // number of steps
	StepsJ = json_object_get(rootJ, "numberofsteps");
	if (StepsJ)
		m_nSteps = json_integer_value( StepsJ );

    // all patterns and phrases
    pint = (int*)&m_PatternNotes[ 0 ][ 0 ][ 0 ];

	StepsJ = json_object_get( rootJ, "m_PatternNotes" );

	if (StepsJ) 
    {
		for ( i = 0; i < (nPHRASE_SAVES * nPATTERNS * nKEYBOARDS * 8); i++)
        {
			json_t *gateJ = json_array_get(StepsJ, i);

			if (gateJ)
				pint[ i ] = json_integer_value( gateJ );
		}
	}

    SetSteps( m_nSteps );
    ChangePhrase( 0, 0, true );
    ChangePhrase( 1, 0, true );
    ChangePhrase( 2, 0, true );
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

        if( --m_glideCount[ i ] > 0 )
            m_fglide[ i ] -= m_fglideInc[ i ];
        else
            m_fglide[ i ] = 0.0;

        outputs[ OUT_VOCTS + i ].value = ( m_fCvStartOut[ i ] * m_fglide[ i ] ) + ( m_fCvEndOut[ i ] * ( 1.0 - m_fglide[ i ] ) );
    }
}
