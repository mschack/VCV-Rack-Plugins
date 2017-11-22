#include "mscHack.hpp"
#include "mscHack_Controls.hpp"
#include "dsp/digital.hpp"
#include "CLog.h"

typedef struct
{
    int             state;
    float           finc;
    int             count;
    float           fade;
}COMP_STATE;

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct Compressor : Module 
{
	enum ParamIds 
    {
        PARAM_INGAIN, 
        PARAM_OUTGAIN,
        PARAM_THRESHOLD,
        PARAM_RATIO,
        PARAM_ATTACK,
        PARAM_RELEASE,
        PARAM_BYPASS,
        nPARAMS
    };

	enum InputIds 
    {
        IN_AUDIOL,
        IN_AUDIOR,
        nINPUTS 
	};

	enum OutputIds 
    {
        OUT_AUDIOL,
        OUT_AUDIOR,
        nOUTPUTS
	};

	enum LightIds 
    {
		LIGHT_BYPASS,
        nLIGHTS
	};

    enum CompState
    {
        COMP_IDLE,
        COMP_ATTACK,
        COMP_RELEASE,
        COMP_FINISHED,
        COMP_ON
    };

    bool            m_bInitialized = false;
    CLog            lg;

    LEDMeterWidget  *m_pLEDMeterIn[ 2 ] = {0};
    CompressorLEDMeterWidget  *m_pLEDMeterThreshold = NULL;
    CompressorLEDMeterWidget  *m_pLEDMeterComp[ 2 ] = {0};
    LEDMeterWidget  *m_pLEDMeterOut[ 2 ] = {0};

    bool            m_bBypass = false;

    COMP_STATE      m_CompL = {};
    COMP_STATE      m_CompR = {};

    //-----------------------------------------------------
    // MySquareButton_Bypass
    //-----------------------------------------------------
    struct MySquareButton_Bypass : MySquareButton
    {
        Compressor *mymodule;

        void onChange( EventChange &e ) override 
        {
            mymodule = (Compressor*)module;

            if( mymodule && value == 1.0 )
            {
                mymodule->m_bBypass = !mymodule->m_bBypass;
                mymodule->lights[ LIGHT_BYPASS ].value = mymodule->m_bBypass ? 1.0 : 0.0;
            }

		    MomentarySwitch::onChange( e );
	    }
    };


    // Contructor
	Compressor() : Module(nPARAMS, nINPUTS, nOUTPUTS, nLIGHTS ){}

    // Overrides 
	void    step() override;
    json_t* toJson() override;
    void    fromJson(json_t *rootJ) override;
    void    reset() override;
    void    randomize() override;

    bool    ProcessCompState( COMP_STATE *pComp, bool bAboveThreshold );
    void    Compress( float *inL, float *inR, float *outDiffL, float *outDiffR );
};

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------
Compressor_Widget::Compressor_Widget() 
{
    int x, y, y2;
	Compressor *module = new Compressor();
	setModule(module);
	box.size = Vec( 15*8, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Compressor.svg")));
		addChild(panel);
	}

    //module->lg.Open("Compressor.txt");
    x = 10;
    y = 34;

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365))); 
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    // bypass switch
    addParam(createParam<Compressor::MySquareButton_Bypass>( Vec( x, y ), module, Compressor::PARAM_BYPASS, 0.0, 1.0, 0.0 ) );
    addChild(createLight<SmallLight<RedLight>>( Vec( x + 2, y + 2 ), module, Compressor::LIGHT_BYPASS ) );

    // audio inputs
    addInput(createInput<MyPortInSmall>( Vec( x, y + 32 ), module, Compressor::IN_AUDIOL ) );
    addInput(createInput<MyPortInSmall>( Vec( x, y + 78 ), module, Compressor::IN_AUDIOR ) );

    // LED meters
    module->m_pLEDMeterIn[ 0 ] = new LEDMeterWidget( x + 22, y + 25, 5, 3, 2, true );
    addChild( module->m_pLEDMeterIn[ 0 ] );
    module->m_pLEDMeterIn[ 1 ] = new LEDMeterWidget( x + 28, y + 25, 5, 3, 2, true );
    addChild( module->m_pLEDMeterIn[ 1 ] );

    module->m_pLEDMeterThreshold = new CompressorLEDMeterWidget( true, x + 39, y + 25, 5, 3, DWRGB( 245, 10, 174 ), DWRGB( 96, 4, 68 ) );
    addChild( module->m_pLEDMeterThreshold );

    module->m_pLEDMeterComp[ 0 ] = new CompressorLEDMeterWidget( true, x + 48, y + 25, 5, 3, DWRGB( 0, 128, 255 ), DWRGB( 0, 64, 128 ) );
    addChild( module->m_pLEDMeterComp[ 0 ] );
    module->m_pLEDMeterComp[ 1 ] = new CompressorLEDMeterWidget( true, x + 55, y + 25, 5, 3, DWRGB( 0, 128, 255 ), DWRGB( 0, 64, 128 ) );
    addChild( module->m_pLEDMeterComp[ 1 ] );

    module->m_pLEDMeterOut[ 0 ] = new LEDMeterWidget( x + 65, y + 25, 5, 3, 2, true );
    addChild( module->m_pLEDMeterOut[ 0 ] );
    module->m_pLEDMeterOut[ 1 ] = new LEDMeterWidget( x + 72, y + 25, 5, 3, 2, true );
    addChild( module->m_pLEDMeterOut[ 1 ] );

    // audio  outputs
    addOutput(createOutput<MyPortOutSmall>( Vec( x + 83, y + 32 ), module, Compressor::OUT_AUDIOL ) );
    addOutput(createOutput<MyPortOutSmall>( Vec( x + 83, y + 78 ), module, Compressor::OUT_AUDIOR ) );

    // add param knobs
    y2 = y + 149;
    addParam(createParam<Yellow1_Small>( Vec( x + 11, y + 113 ), module, Compressor::PARAM_INGAIN, 0.0, 4.0, 0.0 ) );
    addParam(createParam<Yellow1_Small>( Vec( x + 62, y + 113 ), module, Compressor::PARAM_OUTGAIN, 0.0, 8.0, 0.0 ) );
    addParam(createParam<Yellow1_Small>( Vec( x + 34, y2 ), module, Compressor::PARAM_THRESHOLD, 0.4, 0.95, 0.0 ) ); y2 += 40;
    addParam(createParam<Yellow1_Small>( Vec( x + 34, y2 ), module, Compressor::PARAM_RATIO, 0.0, 1.0, 0.0 ) ); y2 += 40;
    addParam(createParam<Yellow1_Small>( Vec( x + 34, y2 ), module, Compressor::PARAM_ATTACK, 0.0, 1.0, 0.0 ) ); y2 += 40;
    addParam(createParam<Yellow1_Small>( Vec( x + 34, y2 ), module, Compressor::PARAM_RELEASE, 0.0, 1.0, 0.0 ) );

    //for( int i = 0; i < 15; i++ )
        //module->lg.f("level %d = %.3f\n", i, module->m_pLEDMeterThreshold->flevels[ i ] );

    module->m_bInitialized = true;
}

//-----------------------------------------------------
// Procedure:   
//
//-----------------------------------------------------
json_t *Compressor::toJson() 
{
	json_t *rootJ = json_object();

    // reverse state
    json_object_set_new(rootJ, "m_bBypass", json_boolean (m_bBypass));

	return rootJ;
}

//-----------------------------------------------------
// Procedure:   fromJson
//
//-----------------------------------------------------
void Compressor::fromJson(json_t *rootJ) 
{
	// reverse state
	json_t *revJ = json_object_get(rootJ, "m_bBypass");

	if (revJ)
		m_bBypass = json_is_true( revJ );

    lights[ LIGHT_BYPASS ].value = m_bBypass ? 1.0 : 0.0;
}

//-----------------------------------------------------
// Procedure:   reset
//
//-----------------------------------------------------
void Compressor::reset()
{
}

//-----------------------------------------------------
// Procedure:   randomize
//
//-----------------------------------------------------
void Compressor::randomize()
{
}

//-----------------------------------------------------
// Procedure:   ProcessCompStatus
//
//-----------------------------------------------------
#define MAX_ATTREL_TIME (0.02f) // .5ms
bool Compressor::ProcessCompState( COMP_STATE *pComp, bool bAboveThreshold )
{
    bool bCompressing = true;

    switch( pComp->state )
    {
    case COMP_IDLE:
        pComp->count = (int)( MAX_ATTREL_TIME * engineGetSampleRate() * params[ PARAM_ATTACK ].value );

        if( pComp->count )
        {
            pComp->fade = 0.0;
            pComp->finc = 1.0 / (float)pComp->count;
            pComp->state = COMP_ATTACK;
        }
        else
        {
            pComp->fade = 1.0;
            pComp->state = COMP_ATTACK;
        }
            
        break;

    case COMP_ATTACK:
        if( --pComp->count > 0 )
        {
            pComp->fade += pComp->finc;

            if( pComp->fade > 1.0 )
                pComp->fade = 1.0;
        }
        else
        {
            pComp->count = (int)( MAX_ATTREL_TIME * engineGetSampleRate() * params[ PARAM_RELEASE ].value );

            if( pComp->count )
            {
                pComp->fade = 1.0;
                pComp->finc = 1.0 / (float)pComp->count;
                pComp->state = COMP_RELEASE;
            }
            else
            {
                pComp->fade = 1.0;
                pComp->state = COMP_ON;
            }
        }

        break;

    case COMP_RELEASE:
        if( --pComp->count > 0 )
        {
            pComp->fade -= pComp->finc;

            if( pComp->fade < 0.0 )
                pComp->fade = 0.0;
        }
        else
        {
            pComp->fade = 0.0;
            pComp->state = COMP_FINISHED;
            bCompressing = false;
        }
        break;

    case COMP_ON:
        pComp->fade = 1.0;
        break;

    case COMP_FINISHED:
        pComp->fade = 0.0;
        bCompressing = false;
        break;
    }
    //lg.f("%.3f\n", pComp->fade );
    //lg.f("%d\n", pComp->count );
    return bCompressing;
}

//-----------------------------------------------------
// Procedure:   Compress
//
//-----------------------------------------------------
void Compressor::Compress( float *inL, float *inR, float *outDiffL, float *outDiffR )
{
    float diffR = 0, diffL = 0, th, rat;

    th = ( 1.0 - params[ PARAM_THRESHOLD ].value );
    rat= params[ PARAM_RATIO ].value;

    *outDiffL = fabs(*inL);
    *outDiffR = fabs(*inR);

    if( fabs( *inL ) > th )
    {
        if( *inL < 0.0 )
            diffL = *inL + th;
        else
            diffL = *inL - th;

        if( ProcessCompState( &m_CompL, true ) )
            *inL -= (diffL * rat * m_CompL.fade );
    }
    else
    {
        m_CompL.state = COMP_IDLE;
    }

    if( fabs( *inR ) > th )
    {
        if( *inR < 0.0 )
            diffR = *inR + th;
        else
            diffR = *inR - th;

        if( ProcessCompState( &m_CompR, true ) )
            *inR -= (diffR * rat * m_CompR.fade);
    }
    else
    {
        m_CompR.state = COMP_IDLE;
    }

    *outDiffL -= fabs(*inL);
    *outDiffR -= fabs(*inR);
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void Compressor::step() 
{
    float outL, outR, diffL, diffR, orgL, orgR;

    if( !m_bInitialized )
        return;

    outL = clampf( inputs[ IN_AUDIOL ].normalize( 0.0 ) / AUDIO_MAX, -1.0, 1.0 );
    outR = clampf( inputs[ IN_AUDIOR ].normalize( 0.0 ) / AUDIO_MAX, -1.0, 1.0 );

    outL = outL * params[ PARAM_INGAIN ].value;
    outR = outR * params[ PARAM_INGAIN ].value;

    orgL = outL;
    orgR = outR;

    if( m_pLEDMeterIn[ 0 ] )
        m_pLEDMeterIn[ 0 ]->Process( outL );

    if( m_pLEDMeterIn[ 1 ] )
       m_pLEDMeterIn[ 1 ]->Process( outR );

    if( !m_bBypass )
    {
        Compress( &outL, &outR, &diffL, &diffR );

        if( m_pLEDMeterComp[ 0 ] )
            m_pLEDMeterComp[ 0 ]->Process( diffL );

        if( m_pLEDMeterComp[ 1 ] )
            m_pLEDMeterComp[ 1 ]->Process( diffR );

        if( m_pLEDMeterThreshold )
            m_pLEDMeterThreshold->Process( params[ PARAM_THRESHOLD ].value - 0.4 );
    }
    else
    {
        if( m_pLEDMeterComp[ 0 ] )
            m_pLEDMeterComp[ 0 ]->Process( 0 );

        if( m_pLEDMeterComp[ 1 ] )
            m_pLEDMeterComp[ 1 ]->Process( 0 );

        if( m_pLEDMeterThreshold )
           m_pLEDMeterThreshold->Process( 0 );

        outL = orgL;
        outR = orgR;
    }

    outL = outL * params[ PARAM_OUTGAIN ].value;
    outR = outR * params[ PARAM_OUTGAIN ].value;

    if( m_pLEDMeterOut[ 0 ] )
        m_pLEDMeterOut[ 0 ]->Process( outL );

    if( m_pLEDMeterOut[ 1 ] )
        m_pLEDMeterOut[ 1 ]->Process( outR );

    outputs[ OUT_AUDIOL ].value = clampf( outL * AUDIO_MAX, -AUDIO_MAX, AUDIO_MAX );
    outputs[ OUT_AUDIOR ].value = clampf( outR * AUDIO_MAX, -AUDIO_MAX, AUDIO_MAX );
}