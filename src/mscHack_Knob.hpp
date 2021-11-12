#pragma once
//-----------------------------------------------------
// MySimpleKnob
//-----------------------------------------------------

#define MAX_ANGLE 140.0f
#define MIN_ANGLE ( 360.0f - MAX_ANGLE )
#define RANGE_ANGLE (( MAX_ANGLE * 2.0f ) + 1.0f )

#define a1 DEG2RAD( 135.0f )
#define a2 DEG2RAD( 45.0f )

struct MSCH_Widget_Knob1 : Knob
{
    widget::FramebufferWidget *fb;
    bool            m_bInitialized = false;
    float           m_radius  = 1.0f;
    RGB_STRUCT      m_Colour = {}, m_BGColour = {};
    float           m_fVal    = 0.0f;
    float           m_linewidth, m_fRat;
    unsigned char   m_BgOpacity = 80;

    MSCH_Widget_Knob1()
    {
    }

    void set( int fgcol, int bgcol, float radius )
    {
        m_radius = radius;
        m_Colour.dwCol = fgcol;
        m_BGColour.dwCol = bgcol;

        fb = new widget::FramebufferWidget;

        box.size = Vec( m_radius * 2, m_radius * 2 );

        addChild(fb);

        fb->box.size = box.size;

        m_linewidth = m_radius * 0.15f;

        m_bInitialized  = true;
    }

    void draw(const DrawArgs &args) override
    {
        if( !m_bInitialized )
            return;

        nvgBeginPath( args.vg );
        nvgCircle( args.vg, m_radius, m_radius, m_radius );
        nvgFillColor( args.vg, nvgRGBA( m_BGColour.Col[ 2 ], m_BGColour.Col[ 1 ], m_BGColour.Col[ 0 ], m_BgOpacity ) );
        nvgFill( args.vg );

        nvgLineCap( args.vg, NVG_ROUND );

        nvgStrokeWidth( args.vg, m_linewidth + 0.02f );
        nvgBeginPath( args.vg );
        nvgStrokeColor( args.vg, nvgRGBA( m_Colour.Col[ 2 ], m_Colour.Col[ 1 ], m_Colour.Col[ 0 ], 250 ) );
        nvgArc( args.vg, m_radius, m_radius, m_radius, a1, DEG2RAD( ( m_fVal * 270.0f ) + 135.0f ), NVG_CW );
        nvgLineTo( args.vg, m_radius, m_radius );
        nvgStroke( args.vg );
    }

    void onChange(const event::Change &e) override
    {
        if( !m_bInitialized )
            return;

        // Re-transform the widget::TransformWidget
        float val;

        if (auto paramQuantity = getParamQuantity()) 
        {
            val = paramQuantity->getValue();

            if( Knob::snap )
                val = (float)( (int)val );

            m_fRat = 1.0f / ( paramQuantity->maxValue - paramQuantity->minValue );
            m_fVal = m_fRat * ( val - paramQuantity->minValue );

            fb->dirty = true;
        }

        Knob::onChange(e);
    }

};
