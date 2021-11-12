#include "mscHack.hpp"

#define nCHANNELS 8
#define nENVELOPE_SEGS 5

typedef struct
{
	float m, b;
}ENV_SEG;

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

    bool            m_bInitialized = false;

    // triggers
    MyLEDButton     *m_pTrigButton[ nCHANNELS ] = {};

    int             m_State[ nCHANNELS ] = {STATE_OFF};

    float           m_fFade[ nCHANNELS ] = {};
    float           m_fPos[ nCHANNELS ] = {};

    ENV_SEG         m_EnvSeg[ nENVELOPE_SEGS ] = {};

    Label           *m_pTextLabel = NULL;

    // Contructor
	ASAF8()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS, nLIGHTS);

        for( int i = 0; i < nCHANNELS; i++ )
        {
            configParam( PARAM_SPEED_IN + i, 0.05f, 40.0f, 5.0f, "Fade In Speed" );
            configParam( PARAM_SPEED_OUT + i, 0.05f, 40.0f, 5.0f, "Fade Out Speed" );
        }
    }

    //-----------------------------------------------------
    // spd_Knob
    //-----------------------------------------------------
    struct spd_Knob : Knob_Green1_15
    {
    	ASAF8 *mymodule;
        int param;
        char strVal[ 10 ] = {};

        void onChange( const event::Change &e ) override
        {
            auto paramQuantity = getParamQuantity();
            mymodule = (ASAF8*)paramQuantity->module;

            sprintf( strVal, "[%.2fs]", paramQuantity->getValue() );

            mymodule->m_pTextLabel->text = strVal;

		    RoundKnob::onChange( e );
	    }
    };

	void    envSeg_from_points( float x1, float y1, float x2, float y2, ENV_SEG *L );

	bool    processFade( int ch, bool bfin );

    // Overrides 
    void    JsonParams( bool bTo, json_t *root);
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onRandomize() override;
    void    onReset() override;
};

// dumb
ASAF8 g_ASAF8_Browser;

//-----------------------------------------------------
// ASAF8_TrigButton
//-----------------------------------------------------
void ASAF8_TrigButton( void *pClass, int id, bool bOn )
{
	ASAF8 *mymodule;
    mymodule = (ASAF8*)pClass;

    if( bOn )
        mymodule->m_pTrigButton[ id ]->Set( true );
    	//mymodule->m_State[ id ] = ASAF8::STATE_FIN;
    else
        mymodule->m_pTrigButton[ id ]->Set( false );
    	//mymodule->m_State[ id ] = ASAF8::STATE_FOUT;
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
struct ASAF8_Widget : ModuleWidget 
{

ASAF8_Widget( ASAF8 *module )
{
	int x, y;
    ASAF8 *pmod;

	//box.size = Vec( 15*12, 380);

    setModule(module);

    if( !module )
        pmod = &g_ASAF8_Browser;
    else
        pmod = module;

    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/ASAF8.svg")));

    pmod->m_pTextLabel = new Label();
    pmod->m_pTextLabel->box.pos = Vec( 90, 28 );
    pmod->m_pTextLabel->text = "----";
	addChild( pmod->m_pTextLabel );

	x = 3;
	y = 77;

	for( int ch = 0; ch < nCHANNELS; ch++ )
	{
		// inputs
		addInput(createInput<MyPortInSmall>( Vec( x + 1, y ), module, ASAF8::IN_AUDIOL + ch ) );
		addInput(createInput<MyPortInSmall>( Vec( x + 22, y ), module, ASAF8::IN_AUDIOR + ch ) );

		// trigger input
		addInput(createInput<MyPortInSmall>( Vec( x + 47, y ), module, ASAF8::IN_TRIGS + ch ) );

        // trigger button
        pmod->m_pTrigButton[ ch ] = new MyLEDButton( x + 68, y - 1, 19, 19, 15.0, DWRGB( 180, 180, 180 ), DWRGB( 0, 255, 0 ), MyLEDButton::TYPE_SWITCH, ch, module, ASAF8_TrigButton );
	    addChild( pmod->m_pTrigButton[ ch ] );

        // speed knobs
        addParam(createParam<ASAF8::spd_Knob>( Vec( x + 94, y ), module, ASAF8::PARAM_SPEED_IN + ch  ) );
        addParam(createParam<ASAF8::spd_Knob>( Vec( x + 115, y ), module, ASAF8::PARAM_SPEED_OUT + ch ) );

        // outputs
        addOutput(createOutput<MyPortOutSmall>( Vec( x + 137, y ), module, ASAF8::OUT_AUDIOL + ch ) );
        addOutput(createOutput<MyPortOutSmall>( Vec( x + 158, y ), module, ASAF8::OUT_AUDIOR + ch ) );

        y += 33;
	}

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    if( module )
    {
	    module->envSeg_from_points( 0.0f, 1.0f, 0.2f, 0.975f, &module->m_EnvSeg[ 0 ] );
	    module->envSeg_from_points( 0.2f, 0.975f, 0.3f, 0.9f, &module->m_EnvSeg[ 1 ] );
	    module->envSeg_from_points( 0.3f, 0.9f, 0.7f, 0.1f, &module->m_EnvSeg[ 2 ] );
	    module->envSeg_from_points( 0.7f, 0.1f, 0.8f, 0.075f, &module->m_EnvSeg[ 3 ] );
	    module->envSeg_from_points( 0.8f, 0.075f, 1.0f, 0.0f, &module->m_EnvSeg[ 4 ] );

        module->m_bInitialized = true;
    }
}
};

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
json_t *ASAF8::dataToJson()
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
void ASAF8::dataFromJson( json_t *root )
{
    JsonParams( FROMJSON, root );

    if( !m_bInitialized )
        return;

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
}

//-----------------------------------------------------
// Function:    envSeg_from_points
//
//-----------------------------------------------------
void ASAF8::envSeg_from_points( float x1, float y1, float x2, float y2, ENV_SEG *L )
{
    L->m = (y2 - y1) / (x2 - x1);
    L->b  = y1 - (L->m * x1);
}

//-----------------------------------------------------
// Procedure:   processFade
//
//-----------------------------------------------------
bool ASAF8::processFade( int ch, bool bfin )
{
	int i = 0;
	float inc;

	if( bfin )
		inc = 1.0f / ( APP->engine->getSampleRate() * params[ PARAM_SPEED_IN + ch ].getValue() );
	else
		inc = 1.0f / ( APP->engine->getSampleRate() * params[ PARAM_SPEED_OUT + ch ].getValue() );

	if( m_fPos[ ch ] < 0.2f )
		i = 0;
	else if( m_fPos[ ch ] < 0.3f )
		i = 1;
	else if( m_fPos[ ch ] < 0.7f )
		i = 2;
	else if( m_fPos[ ch ] < 0.8f )
		i = 3;
	else
		i = 4;

	if( bfin )
		m_fFade[ ch ] = 1.0f - ( ( m_fPos[ ch ] * m_EnvSeg[ i ].m ) + m_EnvSeg[ i ].b );
	else
		m_fFade[ ch ] = ( m_fPos[ ch ] * m_EnvSeg[ i ].m ) + m_EnvSeg[ i ].b;

	m_fPos[ ch ] += inc;

	// return true means we are finished fade
	if( m_fPos[ ch ] >= 1.0f )
		return true;

	return false;
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
#define FADE_GATE_LVL (0.01f)

void ASAF8::process(const ProcessArgs &args)
{
	if( !m_bInitialized )
		return;

	for( int ch = 0; ch < nCHANNELS; ch++ )
	{
		switch( m_State[ ch ] )
		{
		case STATE_FOUT:

			if( inputs[ IN_TRIGS + ch ].getNormalVoltage( 0.0f ) >= FADE_GATE_LVL || m_pTrigButton[ ch ]->m_bOn )
			{
				m_pTrigButton[ ch ]->Set( true );
				m_fPos[ ch ] = 1.0f - m_fPos[ ch ];
				m_State[ ch ] = STATE_FIN;
				break;
			}

			if( processFade( ch, false ) )
			{
				m_fFade[ ch ] = 0.0f;
				m_State[ ch ] = STATE_OFF;
			}
			break;

		case STATE_OFF:

			if( inputs[ IN_TRIGS + ch ].getNormalVoltage( 0.0f ) >= FADE_GATE_LVL || m_pTrigButton[ ch ]->m_bOn )
			{
				m_pTrigButton[ ch ]->Set( true );
				m_State[ ch ] = STATE_FIN;
				m_fPos[ ch ] = 0.0f;
				break;
			}

			m_fFade[ ch ] = 0.0f;
			break;

		case STATE_FIN:

			if( inputs[ IN_TRIGS + ch ].getNormalVoltage( 1.0f ) < FADE_GATE_LVL || !m_pTrigButton[ ch ]->m_bOn )
			{
				m_pTrigButton[ ch ]->Set( false );
				m_fPos[ ch ] = 1.0f - m_fPos[ ch ];
				m_State[ ch ] = STATE_FOUT;
				break;
			}

			if( processFade( ch, true )  )
			{
				m_fFade[ ch ] = 1.0f;
				m_State[ ch ] = STATE_ON;
			}
			break;

		case STATE_ON:

			if( inputs[ IN_TRIGS + ch ].getNormalVoltage( 1.0f ) < FADE_GATE_LVL || !m_pTrigButton[ ch ]->m_bOn )
			{
				m_pTrigButton[ ch ]->Set( false );
				m_State[ ch ] = STATE_FOUT;
				m_fPos[ ch ] = 0.0f;
				break;
			}

			m_fFade[ ch ] = 1.0f;
			break;
		}

		if( inputs[ IN_AUDIOL + ch ].isConnected() )
			outputs[ OUT_AUDIOL + ch ].setVoltage( inputs[ IN_AUDIOL + ch ].getVoltageSum() * m_fFade[ ch ] );
		else
			outputs[ OUT_AUDIOL + ch ].value = CV_MAX10 * m_fFade[ ch ];

		if( inputs[ IN_AUDIOR + ch ].isConnected() )
			outputs[ OUT_AUDIOR + ch ].setVoltage( inputs[ IN_AUDIOR + ch ].getVoltageSum() * m_fFade[ ch ] );
		else
			outputs[ OUT_AUDIOR + ch ].value = CV_MAX10 * m_fFade[ ch ];
	}
}

Model *modelASAF8 = createModel<ASAF8, ASAF8_Widget>( "ASAF8" );
