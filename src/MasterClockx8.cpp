#include "mscHack.hpp"

#define nCHANNELS 8

//------------------------------------------------------
// moving average
#define AVG_ARRAY_LEN   128
#define AVG_AND         0x7F

typedef struct
{
    int count;
    int avg[ AVG_ARRAY_LEN ];
    int tot;
}BPM_AVG;

const float multdisplayval[] = { 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 24, 32, 48, 64, 96 };

#define CLOCK_DIVS ( sizeof( multdisplayval ) / sizeof( float ) )
#define MID_INDEX  (CLOCK_DIVS - 1)

//-----------------------------------------------------
// Module Definition
//
//-----------------------------------------------------
struct MasterClockx8 : Module
{
	enum ParamIds 
    {
        PARAM_BPM,
        PARAM_MULT,
        nPARAMS  = PARAM_MULT + nCHANNELS
    };

	enum InputIds 
    {
        EXT_CLK,
        nINPUTS         
	};

	enum OutputIds 
    {
        OUTPUT_CLK,
		nOUTPUTS     = OUTPUT_CLK + nCHANNELS,
	};

    bool            	m_bInitialized = false;

    float                m_fBPM = 120;
    BPM_AVG              m_BpmAvg = {};

    MyLED7DigitDisplay  *m_pDigitDisplayMult[ nCHANNELS ] = {};
    MyLED7DigitDisplay  *m_pDigitDisplayBPM  = NULL;

    float               m_ChannelDivBeatCount[ nCHANNELS ] = {};
    float               m_fChannelBeatsPers[ nCHANNELS ] = {};
    float               m_fChannelClockCount[ nCHANNELS ] = {};
    int                 m_ChannelMultSelect[ nCHANNELS ] = {};

    float               m_bWasChained = false;

    float               m_fBeatsPers;
    float               m_fMainClockCount;

    dsp::PulseGenerator      m_PulseClock[ nCHANNELS ];
    dsp::PulseGenerator      m_PulseSync[ nCHANNELS ];

    dsp::SchmittTrigger      m_SchmittClkIn;

    ParamWidget        *m_pBpmKnob = NULL;

    Label               *m_pTextLabel = NULL;

    // Contructor
    MasterClockx8()
    {
        config(nPARAMS, nINPUTS, nOUTPUTS);

        configParam( PARAM_BPM, 60.0, 220.0, 120.0, "Beats Per Minute" );

        for( int i = 0; i < nCHANNELS; i++ )
        {
            configParam( PARAM_MULT + i, 0.0f, (float)( (CLOCK_DIVS * 2) - 2 ) , (float)(MID_INDEX), "Clock Mult/Div" );
        }
    }

    //-----------------------------------------------------
    // MyBPM_Knob
    //-----------------------------------------------------
    struct MyBPM_Knob : MSCH_Widget_Knob1 
    {
        MyBPM_Knob()
        {
            set( DWRGB( 188, 175, 167 ), DWRGB( 64, 0, 0 ), 13.0f );
        }

        void onChange( const event::Change &e ) override
        {
            float fbpm;
            int ibpm;
            MasterClockx8 *mymodule;

            MSCH_Widget_Knob1::onChange( e );

            mymodule = (MasterClockx8*)paramQuantity->module;

            if( !mymodule )
                return;

            if( !mymodule->inputs[ EXT_CLK ].isConnected()  )
            {
                ibpm = (int)( paramQuantity->getValue() * 100.0f );
                fbpm = (float)ibpm / 100.0f;
                mymodule->BPMChange( fbpm, false );
            }
        }
    };

    //-----------------------------------------------------
    // MyMult_Knob
    //-----------------------------------------------------
    struct MyMult_Knob : MSCH_Widget_Knob1
    {
        MyMult_Knob()
        {
            snap = true;
            set( DWRGB( 188, 175, 167 ), DWRGB( 64, 0, 0 ), 13.0f );
        }

        void onChange( const event::Change &e ) override
        {
            MasterClockx8 *mymodule;
            int ch;

            MSCH_Widget_Knob1::onChange( e );

            mymodule = (MasterClockx8*)paramQuantity->module;

            if( !mymodule )
                return;

            ch = paramQuantity->paramId - MasterClockx8::PARAM_MULT;
            mymodule->SetDisplayLED( ch, (int)paramQuantity->getValue() );
        }
    };

    // Overrides 
    void    JsonParams( bool bTo, json_t *root);
    void    process(const ProcessArgs &args) override;
    json_t* dataToJson() override;
    void    dataFromJson(json_t *rootJ) override;
    void    onReset() override;

    void    BPMChange( float fbmp, bool bforce );
    void    CalcChannelClockRate( int ch );
    void    SetDisplayLED( int ch, int val );
};

MasterClockx8 MasterClockx8Browser;

//-----------------------------------------------------
// Procedure:   Widget
//
//-----------------------------------------------------

struct MasterClockx8_Widget : ModuleWidget 
{

MasterClockx8_Widget( MasterClockx8 *module )
{
    int ch, x, y;
    MasterClockx8 *pmod;

    //box.size = Vec( 15*8, 380);
    setModule(module);

    if( !module )
        pmod = &MasterClockx8Browser;
    else
        pmod = module;

    setPanel(APP->window->loadSvg(asset::plugin( thePlugin, "res/MasterClockx8.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365))); 
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // bpm knob
    pmod->m_pBpmKnob = createParam<MasterClockx8::MyBPM_Knob>( Vec( 50, 52 ), module, MasterClockx8::PARAM_BPM );
    addParam( pmod->m_pBpmKnob );

    // bpm display
    pmod->m_pDigitDisplayBPM = new MyLED7DigitDisplay( 30, 20, 0.08, DWRGB( 0, 0, 0 ), DWRGB( 0xFF, 0xFF, 0xFF ), MyLED7DigitDisplay::TYPE_FLOAT1, 4 );
    addChild( pmod->m_pDigitDisplayBPM );

    // chain in
    addInput(createInput<MyPortInSmall>( Vec( 15, 25 ), module, MasterClockx8::EXT_CLK ) );

    pmod->m_pTextLabel = new Label();
    pmod->m_pTextLabel->box.pos = Vec( 20, 50 );
    pmod->m_pTextLabel->text = "----";
	addChild( pmod->m_pTextLabel );

    x = 10;
    y = 73;

    for( ch = 0; ch < nCHANNELS; ch++ )
    {
        // clock mult knob
        addParam(createParam<MasterClockx8::MyMult_Knob>( Vec( x, y + 13 ), module, MasterClockx8::PARAM_MULT + ch ) );

        // mult display
        pmod->m_pDigitDisplayMult[ ch ] = new MyLED7DigitDisplay( x + 38, y + 18, 0.07, DWRGB( 0, 0, 0 ), DWRGB( 0xFF, 0xFF, 0xFF ), MyLED7DigitDisplay::TYPE_FLOAT1, 3 );
        addChild( pmod->m_pDigitDisplayMult[ ch ] );

        addOutput(createOutput<MyPortOutSmall>( Vec( x + 82, y + 17 ), module, MasterClockx8::OUTPUT_CLK + ch ) );

        y += 31;
    }

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
void MasterClockx8::JsonParams( bool bTo, json_t *root)
{
    JsonDataInt ( bTo, "m_ChannelMultSelect", root, m_ChannelMultSelect, nCHANNELS );
}

//-----------------------------------------------------
// Procedure:   
//
//-----------------------------------------------------
json_t *MasterClockx8::dataToJson()
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
void MasterClockx8::dataFromJson(json_t *root)
{
    JsonParams( FROMJSON, root );

    if( !m_bInitialized )
        return;

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        SetDisplayLED( ch, m_ChannelMultSelect[ ch ] );
    }

    m_fMainClockCount = 0;
    BPMChange( params[ PARAM_BPM ].getValue(), true );

    if( m_pDigitDisplayBPM )
        m_pDigitDisplayBPM->SetVal( (int)(m_fBPM * 10.0f) );
}

//-----------------------------------------------------
// Procedure:   reset
//
//-----------------------------------------------------
void MasterClockx8::onReset()
{
    if( !m_bInitialized )
        return;

    m_fBPM = 120;

    if( m_pDigitDisplayBPM )
        m_pDigitDisplayBPM->SetVal( (int)(m_fBPM * 10.0f) );

    for( int ch = 0; ch < nCHANNELS; ch++ )
    {
        SetDisplayLED( ch, MID_INDEX );
    }

    BPMChange( m_fBPM, true );
}

//-----------------------------------------------------
// Procedure:   SetDisplayLED
//
//-----------------------------------------------------
void MasterClockx8::SetDisplayLED( int ch, int val )
{
    int col;
    float fval;

    if( !m_bInitialized )
        return;

    if( val < (int)MID_INDEX )
    {
        fval = (float) multdisplayval[ MID_INDEX - val ];
        col = DWRGB( 0xFF, 0, 0 );
    }
    else if( val > (int)MID_INDEX )
    {
        fval = (float) multdisplayval[ val - MID_INDEX ];
        col = DWRGB( 0, 0xFF, 0xFF );
    }
    else
    {
        fval = 1.0f;
        col = DWRGB( 0xFF, 0xFF, 0xFF );
    }

    if( m_pDigitDisplayMult[ ch ] )
    {
        m_ChannelMultSelect[ ch ] = val;
        m_pDigitDisplayMult[ ch ]->SetLEDCol( col );
        m_pDigitDisplayMult[ ch ]->SetVal( (int)(fval * 10.0f) );
    }

    CalcChannelClockRate( ch );
}

//-----------------------------------------------------
// Procedure:   BMPChange
//
//-----------------------------------------------------
void MasterClockx8::BPMChange( float fbpm, bool bforce )
{
    // don't change if it is already the same
    if( !bforce && ( (int)(fbpm * 100.0f ) == (int)(m_fBPM * 100.0f ) ) )
        return;

    m_fBPM = fbpm;
    m_fBeatsPers = fbpm / 60.0;

    if( m_pDigitDisplayBPM )
       m_pDigitDisplayBPM->SetVal( (int)(m_fBPM * 10.0f) );

    for( int i = 0; i < nCHANNELS; i++ )
        CalcChannelClockRate( i );
}

//-----------------------------------------------------
// Procedure:   CalcChannelClockRate
//
//-----------------------------------------------------
void MasterClockx8::CalcChannelClockRate( int ch )
{
    // for beat division just keep a count of beats
    if( m_ChannelMultSelect[ ch ] == MID_INDEX )
        m_ChannelDivBeatCount[ ch ] = 1;
    else if( m_ChannelMultSelect[ ch ] < (int)MID_INDEX )
        m_ChannelDivBeatCount[ ch ] = multdisplayval[ (int)MID_INDEX - m_ChannelMultSelect[ ch ] ];
    else
        m_fChannelBeatsPers[ ch ] = m_fBeatsPers * (float)( multdisplayval[ m_ChannelMultSelect[ ch ] - (int)MID_INDEX ] );
}

//-----------------------------------------------------
// Procedure:   step
//
//-----------------------------------------------------
void MasterClockx8::process(const ProcessArgs &args)
{
    static bool bWaitFirstExt = true, bWasExt = false;
    static int ext_lencount = 0;
    static float lastbpm = 0.0f;
    int ch;
    bool bClk = false;
    char strVal[ 10 ] = {};

    if( !m_bInitialized )
        return;

    if( inputs[ EXT_CLK ].isConnected() )
    {
        if( !bWasExt )
        {
            m_pBpmKnob->visible = false;
            bWasExt = true;
            bWaitFirstExt = true;
        }

        if( m_SchmittClkIn.process( inputs[ EXT_CLK ].getVoltage() ) )
        {
            if( bWaitFirstExt )
            {
                ext_lencount = 0;
                memset( &m_BpmAvg, 0, sizeof(m_BpmAvg) );
                bWaitFirstExt = false;
                return;
            }
            else
            {
                ext_lencount++;
                float fBpm;

                sprintf( strVal, "[%.5d]", ext_lencount );
                m_pTextLabel->text = strVal;

                m_BpmAvg.tot += ext_lencount;
                m_BpmAvg.avg[ ( m_BpmAvg.count++ & AVG_AND ) ] = ext_lencount;

                if( m_BpmAvg.count >= AVG_ARRAY_LEN )
                    fBpm = (float)m_BpmAvg.tot / (float)AVG_ARRAY_LEN;
                else
                    fBpm = (float)m_BpmAvg.tot / (float)m_BpmAvg.count;
 
                m_BpmAvg.tot -= m_BpmAvg.avg[ ( m_BpmAvg.count & AVG_AND ) ];

                fBpm = ( args.sampleRate / fBpm ) * 60.0f;

                m_pDigitDisplayBPM->SetVal( (int)( (fBpm + 0.05f) * 10.0f) );

                ext_lencount = 0;

                if( (int)(lastbpm * 10.0f) != (int)(fBpm * 10.0f) )
                {
                    memset( &m_BpmAvg, 0, sizeof(m_BpmAvg) );
                }

                lastbpm = fBpm;
            }
        }
        else
        {
            ext_lencount++;
        }
    }
    else
    {
        if( bWasExt )
        {
            m_pBpmKnob->visible = true;
            bWasExt = false;
        }
    }

    for( ch = 0; ch < nCHANNELS; ch++ )
    {

        if( bClk )
            m_PulseClock[ ch ].trigger( 0.050f );

        outputs[ OUTPUT_CLK + ch ].setVoltage( m_PulseClock[ ch ].process( 1.0 / args.sampleRate ) ? CV_MAX10 : 0.0 );
    }
}

Model *modelMasterClockx8 = createModel<MasterClockx8, MasterClockx8_Widget>( "MasterClockx8" );
