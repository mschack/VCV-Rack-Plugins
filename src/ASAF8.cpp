#include "mscHack.hpp"
#include "dsp/digital.hpp"

#define nCHANNELS 8

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct ASAF8 : Module
{
	enum ParamIds 
    {
		PARAM_SPEED_IN,
		PARAM_SPEED_OUT	= PARAM_SPEED_IN + nCHANNELS,
        nPARAMS			= PARAM_SPEED_OUT + nCHANNELS
    };

	enum InputIds 
    {
		IN_TRIGS,
		IN_AUDIOL	= IN_TRIGS + nCHANNELS,
		IN_AUDIOR	= IN_AUDIOL + nCHANNELS,
        nINPUTS 	= IN_AUDIOR + nCHANNELS
	};

	enum OutputIds 
    {
		OUT_AUDIOL,
		OUT_AUDIOR	= OUT_AUDIOL + nCHANNELS,
        nOUTPUTS	= OUT_AUDIOR + nCHANNELS,
	};

	enum LightIds 
    {
        nLIGHTS
	};

	enum FadeStates
    {
        STATE_OFF,
		STATE_FIN,
		STATE_ON,
		STATE_FOUT
	};

    CLog            lg;
    bool            m_bInitialized = false;

    // triggers
    MyLEDButton     *m_pTrigButton[ nCHANNELS ] = {};
    SchmittTrigger  m_SchmittTrig[ nCHANNELS ];

    int             m_State[ nCHANNELS ] = {STATE_OFF};

    float           m_fFade[ nCHANNELS ] = {};
    float           m_fInc[ nCHANNELS ] = {};

    // Contructor
	ASAF8() : Module(nPARAMS, nINPUTS, nOUTPUTS, nLIGHTS){}

    // Overrides 
	void    step() override;
    void    JsonParams( bool bTo, json_t *root);
    json_t* toJson() override;
    void    fromJson(json_t *rootJ) override;
    void    onRandomize() override;
    void    onReset() override;
    void    onCreate() override;
    void    onDelete() override;
};

//-----------------------------------------------------
// ASAF8_TrigButton
//-----------------------------------------------------
void ASAF8_TrigButton( void *pClass, int id, bool bOn )
{
	ASAF8 *mymodule;
    mymodule = (ASAF8*)pClass;

    if( bOn )
    	mymodule->m_State[ id ] = ASAF8::STATE_FIN;
    else
    	mymodule->m_State[ id ] = ASAF8::STATE_FOUT;
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct ASAF8_Widget : ModuleWidget {
	ASAF8_Widget( ASAF8 *module );
};

ASAF8_Widget::ASAF8_Widget( ASAF8 *module ) : ModuleWidget(module)
{
	int x, y;

	box.size = Vec( 15*12, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ASAF8.svg")));
		addChild(panel);
	}

    //module->lg.Open("ASAF8.txt");

	x = 3;
	y = 77;

	for( int ch = 0; ch < nCHANNELS; ch++ )
	{
		// inputs
		addInput(Port::create<MyPortInSmall>( Vec( x + 1, y ), Port::INPUT, module, ASAF8::IN_AUDIOL + ch ) );
		addInput(Port::create<MyPortInSmall>( Vec( x + 22, y ), Port::INPUT, module, ASAF8::IN_AUDIOR + ch ) );

		// trigger input
		addInput(Port::create<MyPortInSmall>( Vec( x + 47, y ), Port::INPUT, module, ASAF8::IN_TRIGS + ch ) );

        // trigger button
        module->m_pTrigButton[ ch ] = new MyLEDButton( x + 68, y - 1, 19, 19, 15.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, ASAF8_TrigButton );
	    addChild( module->m_pTrigButton[ ch ] );

        // speed knobs
        addParam(ParamWidget::create<Knob_Green1_15>( Vec( x + 94, y ), module, ASAF8::PARAM_SPEED_IN + ch, 0.0f, 1.0f, 0.5f ) );
        addParam(ParamWidget::create<Knob_Green1_15>( Vec( x + 115, y ), module, ASAF8::PARAM_SPEED_OUT + ch, 0.0f, 1.0f, 0.5f ) );

        // outputs
        addOutput(Port::create<MyPortOutSmall>( Vec( x + 137, y ), Port::OUTPUT, module, ASAF8::OUT_AUDIOL + ch ) );
        addOutput(Port::create<MyPortOutSmall>( Vec( x + 158, y ), Port::OUTPUT, module, ASAF8::OUT_AUDIOR + ch ) );

        y += 33;
	}

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365))); 
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));
}

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void ASAF8::JsonParams( bool bTo, json_t *root)
{
    JsonDataInt( bTo, "m_State", root, m_State, nCHANNELS );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *ASAF8::toJson()
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
void ASAF8::fromJson( json_t *root )
{
    JsonParams( FROMJSON, root );

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
    	if( m_State[ ch ] == STATE_OFF || m_State[ ch ] == STATE_FOUT )
    	{
    		m_pTrigButton[ ch ]->Set( false );
    		m_State[ ch ] = STATE_OFF;
    		m_fFade[ ch ] = 0.0f;
    	}
    	else
    	{
    		m_pTrigButton[ ch ]->Set( true );
    		m_State[ ch ] = STATE_ON;
    		m_fFade[ ch ] = 1.0f;
    	}
    }
}

//-----------------------------------------------------
// Procedure:   onCreate
//
//-----------------------------------------------------
void ASAF8::onCreate()
{
}

//-----------------------------------------------------
// Procedure:   onDelete
//
//-----------------------------------------------------
void ASAF8::onDelete()
{
}

//-----------------------------------------------------
// Procedure:   onReset
//
//-----------------------------------------------------
void ASAF8::onReset()
{
}

//-----------------------------------------------------
// Procedure:   onRandomize
//
//-----------------------------------------------------
void ASAF8::onRandomize()
{
    div = (float)(ENVELOPE_HANDLES - 1) / (PI * 1.0f);
    for( i = 0; i < ENVELOPE_HANDLES; i++ )
    {
        a = ( 1.0f + cosf( (float)i / div ) ) / 2.0f;
        setVal( i, a );
    }
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void ASAF8::step()
{
	for( int ch = 0; ch < nCHANNELS; ch++ )
	{
		outputs[ OUT_AUDIOL + ch ].value = inputs[ IN_AUDIOL + ch ].value * m_fFade[ ch ];
		outputs[ OUT_AUDIOR + ch ].value = inputs[ IN_AUDIOR + ch ].value * m_fFade[ ch ];
	}
}

Model *modelASAF8 = Model::create<ASAF8, ASAF8_Widget>( "mscHack", "ASAF8", "ASAF-8 Channel Auto Stereo Audio Fader", ATTENUATOR_TAG, CONTROLLER_TAG, UTILITY_TAG, MULTIPLE_TAG );
