#include "mscHack.hpp"
#include "float.h"

#define nDISPLAY_WIDTH 230
#define nDISPLAY_HEIGHT 230

#define nPAR 3

//------------------------------------------------------
// averaging mouse
#define AVG_ARRAY_LEN   64
#define AVG_AND         0x3F

typedef struct
{
    int    count;
    float  avg[ AVG_ARRAY_LEN ];
    float  tot;
}MOVING_AVG;

typedef struct
{
    float q, f;

    float lp1, bp1;

}FILTER_STRUCT;


struct Lorenz_Knob : MSCH_Widget_Knob1 
{
    Lorenz_Knob()
    {
        set( DWRGB( 222, 170, 135 ), DWRGB( 0, 0, 0 ), 10.0f );
    }
};

struct Lorenz_AMT_Knob : MSCH_Widget_Knob1 
{
    Lorenz_AMT_Knob()
    {
        set( DWRGB( 255, 255, 255 ), DWRGB( 0, 0, 0 ), 7.5f );
    }
};

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Lorenz : Module
{
	enum ParamIds 
    {
        PARAM_LEVELX,
        PARAM_LEVELY,
        PARAM_LEVELZ,
        PARAM_FCUT,
        PARAM_FCUT_CV_AMT,
        PARAM_REZ,
        PARAM_REZ_CV_AMT,
        PARAM_P,
        PARAM_P_AMT     = PARAM_P + nPAR,
        PARAM_T         = PARAM_P_AMT + nPAR,
        PARAM_T_AMT     = PARAM_T + nPAR,
        nPARAMS         = PARAM_T_AMT + nPAR
    };

	enum InputIds 
    {
        IN_P_CV,
        IN_T_CV      = IN_P_CV + nPAR,
        IN_FCUT_CV   = IN_T_CV + nPAR,
        IN_REZ_CV,
        nINPUTS 	
	};

	enum OutputIds 
    {
        OUT_AUDIO,
        nOUTPUTS,
	};

    enum FILTER_TYPES
    {
        FILTER_OFF,
        FILTER_LP,
        FILTER_BP,
        FILTER_HP,
    };

    bool            m_bInitialized = false;

    FILTER_STRUCT   m_Filter = {};
    int             m_FilterState = 0;
    MyLEDButton     *m_pButtonFilterType[ FILTER_HP ] = {};

    Widget_LineImage *m_pImageWidget = NULL;

    float           m_x = 0.0f, m_y = 1.0f, m_z = 0.0f;

    MOVING_AVG      m_DCAvg = {};

    //Label           *m_pTextLabel = NULL;

    // Contructor
    Lorenz()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS);

        configParam( PARAM_LEVELX, 0.0f, 10.0f, 0.5f, "Level X-Axis (x10)" );
        configParam( PARAM_LEVELY, 0.0f, 10.0f, 0.5f, "Level Y-Axis (x10)" );
        configParam( PARAM_LEVELZ, 0.0f, 10.0f, 0.5f, "Level Z-Axix (x10)" );

        configParam( PARAM_FCUT, 0.0f, 1.0f, 0.0f, "Filter Cutoff" );
        configParam( PARAM_FCUT_CV_AMT, 0.0f, 1.0f, 0.0f, "Cutoff LFO Amount" );
        configParam( PARAM_REZ, 0.0f, 1.0f, 0.0f, "Filter Resonance" );
        configParam( PARAM_REZ_CV_AMT,0.0f, 1.0f, 0.0f, "Resonance LFO Amount" );

        configParam( PARAM_P, 0.0f, 1.0f, 0.25f, "Param 1" );
        configParam( PARAM_P_AMT, 0.0f, 1.0f, 0.0f, "Param 1 LFO Amount" );
        configParam( PARAM_P + 1, 0.0f, 1.0f, 0.4f, "Param 2" );
        configParam( PARAM_P_AMT + 1, 0.0f, 1.0f, 0.0f, "Param 2 LFO Amount" );
        configParam( PARAM_P + 2, 0.0f, 1.0f, 0.07f, "Param 3" );
        configParam( PARAM_P_AMT + 2, 0.0f, 1.0f, 0.0f, "Param 3 LFO Amount" );

        configParam( PARAM_T, 0.0f, 1.0f, 0.02f, "T1" );
        configParam( PARAM_T_AMT, 0.0f, 1.0f, 0.0f, "T1 LFO Amount" );
        configParam( PARAM_T + 1, 0.0f, 1.0f, 0.02f, "T2" );
        configParam( PARAM_T_AMT + 1, 0.0f, 1.0f, 0.0f, "T2 LFO Amount" );
        configParam( PARAM_T + 2, 0.0f, 1.0f, 0.02f, "T3" );
        configParam( PARAM_T_AMT + 2, 0.0f, 1.0f, 0.0f, "T3 LFO Amount" );
    }

    // Overrides 
    void    JsonParams( bool bTo, json_t *root);
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;

    void    onReset() override;

    void    ChangeFilterCutoff( float amt );
    void    Filter( float *In );
    float   ProcessCV( int param, int lfocv, int amtparam, bool bLinearParam );
};

Lorenz LorenzBrowser;

//-----------------------------------------------------
// Lorenz_FilterSelect
//-----------------------------------------------------
void Lorenz_FilterSelect( void *pClass, int id, bool bOn )
{
    Lorenz *m;
    m = (Lorenz*)pClass;

    if( !bOn )
    {
        m->m_FilterState = Lorenz::FILTER_OFF;
    }
    else
    {
        for( int i = 0; i < Lorenz::FILTER_HP; i++ )
        {
            if( i != id )
                m->m_pButtonFilterType[ i ]->Set( false );
        }

        m->m_FilterState = id + 1;
    }
}


//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct Lorenz_Widget : ModuleWidget 
{

Lorenz_Widget( Lorenz *module )
{
    int i, x, y;
    Lorenz *pmod;
	//box.size = Vec( 15*16, 380); 240

    setModule(module);

    if( !module )
        pmod = &LorenzBrowser;
    else
        pmod = module;

    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/Lorenz.svg")));

    pmod->m_pImageWidget = new Widget_LineImage( 5, 15, nDISPLAY_WIDTH, nDISPLAY_HEIGHT );
    addChild( pmod->m_pImageWidget );

    //pmod->m_pTextLabel = new Label();
    //pmod->m_pTextLabel->box.pos = Vec( 88, 249 );
    //pmod->m_pTextLabel->text = "----";
    //addChild( pmod->m_pTextLabel );

    y = 248;
    x = 6;

    // parameters
    for( i = 0; i < nPAR; i++ )
    {
        
        addParam( createParam<Lorenz_AMT_Knob>( Vec( x + 21, y ), module, Lorenz::PARAM_P_AMT + i ) );
        addInput( createInput<MyPortInSmall>( Vec( x, y + 1 ), module, Lorenz::IN_P_CV + i ) );
        addParam( createParam<Lorenz_Knob>( Vec( x + 44, y ), module, Lorenz::PARAM_P + i ) );

        x += 13;
        y += 21;
    }

    y = 314;
    x = 6;

    for( i = 0; i < nPAR; i++ )
    {
        addParam( createParam<Lorenz_AMT_Knob>( Vec( x + 21, y ), module, Lorenz::PARAM_T_AMT + i ) );
        addInput( createInput<MyPortInSmall>( Vec( x, y + 1 ), module, Lorenz::IN_T_CV + i ) );
        addParam( createParam<Lorenz_Knob>( Vec( x + 44, y ), module, Lorenz::PARAM_T + i ) );

        x += 13;
        y += 21;
    }

    x = 8;

    // filter
    addParam( createParam<Lorenz_Knob>( Vec( 122 + x, 282 ), module, Lorenz::PARAM_FCUT ) );
    addInput( createInput<MyPortInSmall>( Vec( 124 + x, 306 ), module, Lorenz::IN_FCUT_CV ) );
    addParam( createParam<Lorenz_AMT_Knob>( Vec( 125 + x, 326 ), module, Lorenz::PARAM_FCUT_CV_AMT ) );

    addParam( createParam<Lorenz_Knob>( Vec( 160 + x, 282 ), module, Lorenz::PARAM_REZ ) );
    addInput( createInput<MyPortInSmall>( Vec( 162 + x, 306 ), module, Lorenz::IN_REZ_CV ) );
    addParam( createParam<Lorenz_AMT_Knob>( Vec( 163 + x, 326 ), module, Lorenz::PARAM_REZ_CV_AMT ) );

    y = 280;

    // filter type buttons
    for( i = 0; i < 3; i++ )
    {
        pmod->m_pButtonFilterType[ i ] = new MyLEDButton( 146 + x, y, 10, 10, 8.0f, DWRGB( 180, 180, 180 ), DWRGB( 255, 255, 255 ), MyLEDButton::TYPE_SWITCH, i, module, Lorenz_FilterSelect );
        addChild( pmod->m_pButtonFilterType[ i ] );
        y += 23;
    }

    // levels xyz
    addParam( createParam<Lorenz_Knob>( Vec( 214, 264 ), module, Lorenz::PARAM_LEVELX ) );
    addParam( createParam<Lorenz_Knob>( Vec( 214, 290 ), module, Lorenz::PARAM_LEVELY ) );
    addParam( createParam<Lorenz_Knob>( Vec( 214, 316 ), module, Lorenz::PARAM_LEVELZ ) );

    // out
    addOutput(createOutput<MyPortOutSmall>( Vec( 214, 348 ), module, Lorenz::OUT_AUDIO ) );

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    //addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    //addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    if( module )
    {
	    module->m_bInitialized = true;
        module->onReset();
    }
}
};

//-----------------------------------------------------
// Procedure: JsonParams  
//
//-----------------------------------------------------
void Lorenz::JsonParams( bool bTo, json_t *root)
{
    JsonDataInt( bTo, "m_FilterState", root, &m_FilterState, 1 );
}

//-----------------------------------------------------
// Procedure: toJson  
//
//-----------------------------------------------------
json_t *Lorenz::dataToJson()
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
void Lorenz::dataFromJson( json_t *root )
{
    JsonParams( FROMJSON, root );

    if( m_FilterState == FILTER_LP )
        m_pButtonFilterType[ 0 ]->Set( true );
    else if( m_FilterState == FILTER_BP )
        m_pButtonFilterType[ 1 ]->Set( true );
    else if( m_FilterState == FILTER_HP )
        m_pButtonFilterType[ 2 ]->Set( true );
}

//-----------------------------------------------------
// Procedure:   onReset
//
//-----------------------------------------------------
void Lorenz::onReset()
{
    if( !m_bInitialized )
        return;

    m_FilterState = FILTER_OFF;

    m_pButtonFilterType[ 0 ]->Set( false );
    m_pButtonFilterType[ 1 ]->Set( false );
    m_pButtonFilterType[ 2 ]->Set( false );

    m_x = 0.0f, m_y = 1.0f, m_z = 0.0f;
}

//-----------------------------------------------------
// Procedure:   ChangeFilterCutoff
//
//-----------------------------------------------------
void Lorenz::ChangeFilterCutoff( float amt )
{
    float fx, fx2, fx3, fx5, fx7, cutfreq, cutin;

    cutin = ProcessCV( PARAM_FCUT, IN_FCUT_CV, PARAM_FCUT_CV_AMT, false );

    cutfreq = clamp( ( cutin * cutin * 0.5f ) + cutin + amt, 0.0f, 1.0f );

    // clamp at 1.0 and 20/samplerate
    cutfreq = fmax( cutfreq, 20.0f / APP->engine->getSampleRate() ); 
    cutfreq = fmin( cutfreq, 1.0f );

    // calculate eq rez freq
    fx = 3.141592f * (cutfreq * 0.026315789473684210526315789473684f ) * 2.0f * 3.141592f; 
    fx2 = fx*fx;
    fx3 = fx2*fx; 
    fx5 = fx3*fx2; 
    fx7 = fx5*fx2;

    m_Filter.f = 2.0f * (fx 
        - (fx3 * 0.16666666666666666666666666666667f ) 
        + (fx5 * 0.0083333333333333333333333333333333f ) 
        - (fx7 * 0.0001984126984126984126984126984127f ));
}

//-----------------------------------------------------
// Procedure:   Filter
//
//-----------------------------------------------------
#define MULTI (0.33333333333f)
void Lorenz::Filter( float *In )
{
    FILTER_STRUCT *p;
    float rez, hp1; 
    float lowpass, highpass, bandpass;

    if( m_FilterState == FILTER_OFF )
        return;

    p = &m_Filter;

    rez = 1.0 - clamp( ProcessCV( PARAM_REZ, IN_REZ_CV, PARAM_REZ_CV_AMT, false ), 0.0f, 0.9f );

    *In += 0.000000001;

    p->lp1      = p->lp1 + p->f * p->bp1; 
    hp1         = *In - p->lp1 - rez * p->bp1; 
    p->bp1      = p->f * hp1 + p->bp1; 
    lowpass     = p->lp1; 
    highpass    = hp1; 
    bandpass    = p->bp1; 

    p->lp1      = p->lp1 + p->f * p->bp1; 
    hp1         = *In - p->lp1 - rez * p->bp1; 
    p->bp1      = p->f * hp1 + p->bp1; 
    lowpass     = lowpass  + p->lp1; 
    highpass    = highpass + hp1; 
    bandpass    = bandpass + p->bp1; 

    *In -= 0.000000001;

    p->lp1      = p->lp1 + p->f * p->bp1; 
    hp1         = *In - p->lp1 - rez * p->bp1; 
    p->bp1      = p->f * hp1 + p->bp1; 

    switch( m_FilterState )
    {
    case FILTER_LP:
        *In = (lowpass  + p->lp1) * MULTI;
        break;
    case FILTER_HP:
        *In  = (highpass + hp1) * MULTI;
        break;
    case FILTER_BP:
        *In  = (bandpass + p->bp1) * MULTI;
        break;
    default:
        break;
    }
}

//-----------------------------------------------------
// Procedure:   ProcessCV
//
//-----------------------------------------------------
float Lorenz::ProcessCV( int param, int lfocv, int amtparam, bool bLinearParam )
{
    float lfo = 0.0f;

    if( inputs[ lfocv ].isConnected() )
        lfo = clamp( inputs[ lfocv ].getVoltage() / CV_MAXn5, -1.0f, 1.0f ) * ( params[ amtparam ].getValue() * params[ amtparam ].getValue() );

    if( bLinearParam )
        return clamp( params[ param ].getValue() + lfo, 0.0f, 1.0f );
    else
        return clamp( ( params[ param ].getValue() * params[ param ].getValue() ) + lfo, 0.0f, 1.0f );
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void Lorenz::process(const ProcessArgs &args)
{
    int x, y;
    float p[ nPAR ], t[ nPAR ], xi, yi, zi, outx, outy, outz, out, dc;
    char strVal[ 20 ] = {};
    static int bla = 0;

	if( !m_bInitialized )
		return;

    // get parameters
    for( int i = 0; i < nPAR; i++ )
    {
        p[ i ] = 0.1f + ( 40.0f * ProcessCV( PARAM_P + i, IN_P_CV + i, PARAM_P_AMT + i, true ) );
        t[ i ] = ProcessCV( PARAM_T + i, IN_T_CV + i, PARAM_T_AMT + i, true ) * 500.0f;
    }

    // lorenz
    xi = p[ 0 ] * (m_y - m_x);
    yi = -( m_z * m_x ) + ( p[ 1 ] * m_x ) - m_y;
    zi = ( m_x * m_y ) - ( p[ 2 ] * m_z );

    m_x += ( xi / ( 50.0f + t[ 0 ] ) );
    m_y += ( yi / ( 50.0f + t[ 1 ] ) );
    m_z += ( zi / ( 50.0f + t[ 2 ] ) );

    if( std::isnan( m_x ) || std::isnan( m_y ) )
    {
        return;
    }

    x = (int)(m_x * 5.0f) + (nDISPLAY_WIDTH / 2);
    y = (int)(m_y * 5.0f) + (nDISPLAY_HEIGHT / 2);

    /*if( x < 0 || x >= nDISPLAY_WIDTH || y < 0 || y >= nDISPLAY_HEIGHT )
    {
        return;
    }*/

    m_pImageWidget->addQ( x, y );

    /*if( ++bla >= 4000 )
    {
        bla = 0;
        sprintf( strVal, "[%d, %d]", x, y );
        m_pTextLabel->text = strVal;
    }*/

    outx = AUDIO_MAX * clamp( m_x / nDISPLAY_WIDTH, -1.0, 1.0 ) * params[ PARAM_LEVELX ].getValue();
    outy = AUDIO_MAX * clamp( m_y / nDISPLAY_WIDTH, -1.0, 1.0 ) * params[ PARAM_LEVELY ].getValue();
    outz = AUDIO_MAX * clamp( m_z / nDISPLAY_WIDTH, -1.0, 1.0 ) * params[ PARAM_LEVELZ ].getValue();

    out = outx + outy + outz;

    // DC compensate
    m_DCAvg.tot += out;
    m_DCAvg.avg[ m_DCAvg.count++ & AVG_AND ] = out;

    if( m_DCAvg.count >= AVG_ARRAY_LEN )
    {
        dc = m_DCAvg.tot / AVG_ARRAY_LEN;
    }
    else
    {
        dc = m_DCAvg.tot / m_DCAvg.count;
    }

    out -= dc;

    m_DCAvg.tot -= m_DCAvg.avg[ m_DCAvg.count & AVG_AND ];

    ChangeFilterCutoff( 0.0f );
    Filter( &out );

    outputs[ OUT_AUDIO ].setVoltage( clamp( out, -10.0f, 10.0f ) );
}

Model *modelLorenz = createModel<Lorenz, Lorenz_Widget>( "Lorenz" );
