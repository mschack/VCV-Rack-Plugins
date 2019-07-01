#include "mscHack.hpp"

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Maude_221 : Module 
{
	enum ParamIds 
    {
        PARAM_LIMIT_INA,
        PARAM_LIMIT_INB,
        PARAM_AMP_OUTC,
        PARAM_MODE,
        PARAM_DCOFF,
        PARAM_CVAMTA,
        PARAM_CVAMTB,
        nPARAMS
    };

	enum InputIds 
    {
        INPUTA,
        INPUTB,
        INPUTCVA,
        INPUTCVB,
        nINPUTS 
	};

	enum OutputIds 
    {
        OUTPUTC,
        nOUTPUTS
	};

    bool            m_bInitialized = false;

    // Contructor
	Maude_221()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS);

        configParam( PARAM_LIMIT_INA, 1.0, 10.0, 10.0, "Limit Input A" );
        configParam( PARAM_LIMIT_INB, 1.0, 10.0, 10.0, "Limit Input B" );
        configParam( PARAM_AMP_OUTC, 0.0, 2.0, 1.0, "Amplitude Out" );
        configParam( PARAM_MODE, 0.0, 4.0, 0.0, "Mult Mode" );
        configParam( PARAM_DCOFF, -5.0, 5.0, 0.0, "DC Offset" );
        configParam( PARAM_CVAMTA, 0.0, 1.0, 0.0, "CVA Amount" );
        configParam( PARAM_CVAMTB, 0.0, 1.0, 0.0, "CVB Amount" );
    }

    MyLEDButtonStrip    *m_pInputA_RectMode = NULL;
    MyLEDButtonStrip    *m_pInputB_RectMode = NULL;
    MyLEDButtonStrip    *m_pOutputC_RectMode = NULL;

    int                 m_RectMode[ 3 ] = {0};

    // Overrides 
    void   process(const ProcessArgs &args) override;
    void    JsonParams( bool bTo, json_t *root);
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
};

Maude_221 Maude_221Browser;

//-----------------------------------------------------
// Procedure:   Maude_221_RectSelect
//-----------------------------------------------------
void Maude_221_RectSelect( void *pClass, int id, int nbutton, bool bOn )
{
    Maude_221 *mymodule;
    mymodule = (Maude_221*)pClass;

    if( !mymodule || !mymodule->m_bInitialized )
        return;

    mymodule->m_RectMode[ id ] = nbutton;
}

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct Maude_221_Widget : ModuleWidget 
{

Maude_221_Widget( Maude_221 *module )
{
    Maude_221 *pmod;

    setModule(module);

    if( !module )
        pmod = &Maude_221Browser;
    else
        pmod = module;

    //box.size = Vec( 15*8, 380 );
    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/Maude221.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addInput(createInput<MyPortInSmall>( Vec( 22, 48 ), module, Maude_221::INPUTA ) );
    addInput(createInput<MyPortInSmall>( Vec( 79, 48 ), module, Maude_221::INPUTB ) );

    // rectify mode
    pmod->m_pInputA_RectMode = new MyLEDButtonStrip( 15, 81, 12, 12, 0, 11.5, 3, false, DWRGB( 0, 0, 0 ), DWRGB( 180, 180, 180 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, 0, module, Maude_221_RectSelect );
    addChild( pmod->m_pInputA_RectMode );

    pmod->m_pInputB_RectMode = new MyLEDButtonStrip( 71, 81, 12, 12, 0, 11.5, 3, false, DWRGB( 0, 0, 0 ), DWRGB( 180, 180, 180 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, 1, module, Maude_221_RectSelect );
    addChild( pmod->m_pInputB_RectMode );

    // limit knobs
    addParam(createParam<Knob_Green1_40>( Vec( 12, 106 ), module, Maude_221::PARAM_LIMIT_INA ) );
    addParam(createParam<Knob_Green1_40>( Vec( 67, 106 ), module, Maude_221::PARAM_LIMIT_INB ) );

    // CV amount knobs
    addParam(createParam<Knob_Green1_15>( Vec( 8, 152 ), module, Maude_221::PARAM_CVAMTA ) );
    addParam(createParam<Knob_Green1_15>( Vec( 94, 152 ), module, Maude_221::PARAM_CVAMTB ) );

    // limit CV inputs
    addInput(createInput<MyPortInSmall>( Vec( 6, 173 ), module, Maude_221::INPUTCVA ) );
    addInput(createInput<MyPortInSmall>( Vec( 93, 173 ), module, Maude_221::INPUTCVB ) );

    // mode select knob
    addParam(createParam<Knob_Blue2_26_Snap>( Vec( 47, 188 ), module, Maude_221::PARAM_MODE ) );

    // output rectify mode select
    pmod->m_pOutputC_RectMode = new MyLEDButtonStrip( 32, 248, 12, 12, 0, 11.5, 5, false, DWRGB( 0, 0, 0 ), DWRGB( 180, 180, 180 ), MyLEDButtonStrip::TYPE_EXCLUSIVE, 2, module, Maude_221_RectSelect );
    addChild( pmod->m_pOutputC_RectMode );

    // output amp
    addParam(createParam<Knob_Green1_40>( Vec( 40, 273 ), module, Maude_221::PARAM_AMP_OUTC ) );

    // DC Offset
    addParam(createParam<Knob_Green1_15>( Vec( 80, 315 ), module, Maude_221::PARAM_DCOFF ) );

    // output
    addOutput(createOutput<MyPortOutSmall>( Vec( 50, 344 ), module, Maude_221::OUTPUTC ) );

	if( module )
    {
        module->m_pInputA_RectMode->Set( 1, true );
        module->m_pInputB_RectMode->Set( 1, true );
        module->m_pOutputC_RectMode->Set( 1, true );
        module->m_RectMode[ 0 ] = 1;
        module->m_RectMode[ 1 ] = 1;
        module->m_RectMode[ 2 ] = 1;

        module->m_bInitialized = true;
    }
}
};

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void Maude_221::JsonParams( bool bTo, json_t *root) 
{
    JsonDataInt( bTo, "RectModes", root, m_RectMode, 3 );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *Maude_221::dataToJson() 
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
void Maude_221::dataFromJson( json_t *root ) 
{
    JsonParams( FROMJSON, root );

    m_pInputA_RectMode->Set( m_RectMode[ 0 ], true );
    m_pInputB_RectMode->Set( m_RectMode[ 1 ], true );
    m_pOutputC_RectMode->Set( m_RectMode[ 2 ], true );
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void Maude_221::process(const ProcessArgs &args)
{
    float inA, inB, out = 0.0f, lima, limb;

    if( !m_bInitialized )
        return;

    if( !inputs[ INPUTA ].active && !inputs[ INPUTB ].isConnected() )
    {
        outputs[ OUTPUTC ].setVoltage( 0.0f );
        return;
    }

    lima = clamp( params[ PARAM_LIMIT_INA ].getValue() + ( inputs[ INPUTCVA ].getNormalVoltage( 0.0f ) * params[ PARAM_CVAMTA ].getValue() ), -CV_MAXn5, CV_MAXn5 );
    limb = clamp( params[ PARAM_LIMIT_INB ].getValue() + ( inputs[ INPUTCVB ].getNormalVoltage( 0.0f ) * params[ PARAM_CVAMTB ].getValue() ), -CV_MAXn5, CV_MAXn5 );

    inA = clamp( inputs[ INPUTA ].getNormalVoltage( 0.0f ), -lima, lima );
    inB = clamp( inputs[ INPUTB ].getNormalVoltage( 0.0f ), -limb, limb );

    switch( m_RectMode[ 0 ] )
    {
    case 0: // negative only
        if( inA > 0.0f )
            inA = 0.0f;
        break;
    case 1: // bipolar
        break;
    case 2: // positive only
        if( inA < 0.0f )
            inA = 0.0f;
        break;
    }

    switch( m_RectMode[ 1 ] )
    {
    case 0: // negative only
        if( inB > 0.0f )
            inB = 0.0f;
        break;
    case 1: // bipolar
        break;
    case 2: // positive only
        if( inB < 0.0f )
            inB = 0.0f;
        break;
    }

    // only do math if both inputs are active
    if( inputs[ INPUTA ].isConnected() && inputs[ INPUTB ].isConnected() )
    {
        switch( (int)( params[ PARAM_MODE ].getValue() ) )
        {
        case 0: // add
            out = inA + inB;
            break;
        case 1: // subtract
            out = inA - inB;
            break;
        case 2: // multiply
            out = sin(inA) * cos(inB);
            break;
        case 3: // divide
            if( fabs( inA ) < fabs( inB ) )
                out = inA;
            else
                out = inB;

            break;

        case 4: // divide
            if( fabs( inA ) > fabs( inB ) )
                out = inA;
            else
                out = inB;

            break;
        }
    }
    else if( inputs[ INPUTA ].isConnected() )
    {
        out = inA;
    }
    else
    {
        out = inB;
    }

     switch( m_RectMode[ 2 ] )
    {
    case 0: // negative half wave
        if( out > 0.0f )
            out = 0.0f;
        break;
    case 1: // negative full wave
        if( out > 0.0f )
            out = -out;
        break;
    case 2: // bipolar
        break;
    case 3: // positive full wave
        if( out < 0.0f )
            out = -out;
        break;
    case 4: // positive half wave
        if( out < 0.0f )
            out = 0.0f;
        break;
    }

    out = ( out * params[ PARAM_AMP_OUTC ].getValue() ) + params[ PARAM_DCOFF ].getValue();

    outputs[ OUTPUTC ].setVoltage( out );
}

Model *modelMaude_221 = createModel<Maude_221, Maude_221_Widget>( "Maude221" );