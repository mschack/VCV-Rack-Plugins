#include "rack.hpp"
#include "CLog.h"

//#define RESOURCE_LOCATION "res/"
#define RESOURCE_LOCATION "plugins/mschack/res/"

struct CyanValueLight : ColorValueLight 
{
	CyanValueLight() 
    {
		baseColor = COLOR_CYAN;
    }
};

struct OrangeValueLight : ColorValueLight 
{
	OrangeValueLight() 
    {
		baseColor = nvgRGB(242, 79, 0);
	}
};

struct DarkRedValueLight : ColorValueLight 
{
	DarkRedValueLight() 
    {
		baseColor = nvgRGB(0x70, 0, 0x30);
	}
};

struct DarkGreenValueLight : ColorValueLight 
{
	DarkGreenValueLight() 
    {
		baseColor = nvgRGB(0, 0x90, 0x40);
	}
};

struct DarkGreen2ValueLight : ColorValueLight 
{
	DarkGreen2ValueLight() 
    {
		baseColor = nvgRGB(0, 0x40, 0);
	}
};

struct DarkYellow2ValueLight : ColorValueLight 
{
	DarkYellow2ValueLight() 
    {
		baseColor = nvgRGB(0x40, 0x40, 0);
	}
};

#define lvl_to_db( x ) ( 20.0 * log10( x ) )
#define db_to_lvl( x ) ( 1.0 / pow( 10, x / 20.0 ) )



typedef struct
{
    union
    {
        unsigned int dwCol;
        unsigned char Col[ 4 ];
    };
}RGB_STRUCT;

#define DWRGB( r, g, b ) (b | g<<8 | r<<16)

typedef struct
{
    int x, y, x2, y2;
}RECT_STRUCT;

typedef struct
{
    int x, y;
}POINT_STRUCT;

//-----------------------------------------------------
// SinglePatternClocked32
//-----------------------------------------------------
#define MAX_CLK_PAT 32
struct SinglePatternClocked32 : OpaqueWidget, FramebufferWidget 
{
    typedef void SINGLEPAT16CALLBACK ( void *pClass, int id, int pat, int level, int maxpat );

    bool            m_bInitialized = false;
    int             m_Id;
    int             m_nLEDs;
    int             m_MaxPat = 0;
    int             m_PatClk = 0;
    int             m_PatSelLevel[ MAX_CLK_PAT ] = {0};
    int             m_StepCount;

    SINGLEPAT16CALLBACK *m_pCallback;
    void              *m_pClass;

    RECT_STRUCT     m_RectsPatSel[ MAX_CLK_PAT ];
    RGB_STRUCT      m_PatCol[ 2 ];
    RECT_STRUCT     m_RectsMaxPat[ MAX_CLK_PAT ];
    RGB_STRUCT      m_MaxCol[ 2 ];

    //-----------------------------------------------------
    // Procedure:   Constructor
    //-----------------------------------------------------
    SinglePatternClocked32( int x, int y, int w, int h, int mh, int space, int beatspace, int colourPaton, int colourPatoff, int colourMaxon, int colourMaxoff, int nleds, int id, void *pClass, SINGLEPAT16CALLBACK *pCallback )
    {
        int i;

        if ( nleds < 2 || nleds > MAX_CLK_PAT )
            return;

        m_Id = id;
        m_pCallback = pCallback;
        m_pClass = pClass;
        m_nLEDs = nleds;

        m_PatCol[ 0 ].dwCol = colourPatoff;
        m_PatCol[ 1 ].dwCol = colourPaton;

        m_MaxCol[ 0 ].dwCol = colourMaxoff;
        m_MaxCol[ 1 ].dwCol = colourMaxon;

		box.pos = Vec(x, y);

        x = 0;

        for( i = 0; i < m_nLEDs; i++ )
        {
            m_RectsMaxPat[ i ].x = x;
            m_RectsMaxPat[ i ].y = 0;
            m_RectsMaxPat[ i ].x2 = x + w;
            m_RectsMaxPat[ i ].y2 = mh;

            m_RectsPatSel[ i ].x = x;
            m_RectsPatSel[ i ].y = mh + 2;
            m_RectsPatSel[ i ].x2 = x + w;
            m_RectsPatSel[ i ].y2 = ( h + mh ) + 2;

            if( ( i & 0x3 ) == 3 )
                x += ( w + beatspace );
            else
                x += ( w + space );
        }

        box.size = Vec( x, ( h + mh ) + 2 );

        m_bInitialized = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetPatAll
    //-----------------------------------------------------
    void SetPatAll( int *pPat )
    {
        if ( !pPat )
            return;

        for( int i = 0; i < m_nLEDs; i++ )
            m_PatSelLevel[ i ] = pPat[ i ] & 0x3;

        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   GetPatAll
    //-----------------------------------------------------
    void GetPatAll( int *pPat )
    {
        if ( !pPat )
            return;

        for( int i = 0; i < m_nLEDs; i++ )
            pPat[ i ] = m_PatSelLevel[ i ];

        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetPat
    //-----------------------------------------------------
    void SetPat( int pat )
    {
        if ( pat < 0 || pat >= m_nLEDs )
            return;

        m_PatSelLevel[ pat ] = ( m_PatSelLevel[ pat ] + 1 ) & 0x3;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetPat
    //-----------------------------------------------------
    void ClrPat( int pat )
    {
        if ( pat < 0 || pat >= m_nLEDs )
            return;

        m_PatSelLevel[ pat ] = 0;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   ClockInc
    //-----------------------------------------------------
    bool ClockInc( void )
    {
        m_PatClk ++;

        if ( m_PatClk < 0 || m_PatClk > m_MaxPat || m_PatClk >= m_nLEDs )
            m_PatClk = 0;

        dirty = true;

        if( m_PatClk == 0 )
            return true;

        return false;
    }

    //-----------------------------------------------------
    // Procedure:   ClockReset
    //-----------------------------------------------------
    void ClockReset( void )
    {
        m_PatClk = 0;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetMax
    //-----------------------------------------------------
    void SetMax( int max )
    {
        m_MaxPat = max;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(NVGcontext *vg) override
    {
        RGB_STRUCT rgb = {0};
        int i;

        if( !m_bInitialized )
            return;

		nvgFillColor(vg, nvgRGBA(0, 0, 0, 0xc0));
		nvgBeginPath(vg);
		nvgMoveTo(vg, -1, -1 );
		nvgLineTo(vg, box.size.x + 1, -1 );
		nvgLineTo(vg, box.size.x + 1, box.size.y + 1 );
		nvgLineTo(vg, -1, box.size.y + 1 );
		nvgClosePath(vg);
		nvgFill(vg);

        for( i = 0; i < m_nLEDs; i++ )
        {
            // max pattern display
            if( i <= m_MaxPat )
                nvgFillColor( vg, nvgRGB( m_MaxCol[ 1 ].Col[ 2 ], m_MaxCol[ 1 ].Col[ 1 ], m_MaxCol[ 1 ].Col[ 0 ] ) );
            else
                nvgFillColor( vg, nvgRGB( m_MaxCol[ 0 ].Col[ 2 ], m_MaxCol[ 0 ].Col[ 1 ], m_MaxCol[ 0 ].Col[ 0 ] ) );

			nvgBeginPath(vg);
			nvgMoveTo(vg, m_RectsMaxPat[ i ].x, m_RectsMaxPat[ i ].y );
			nvgLineTo(vg, m_RectsMaxPat[ i ].x2, m_RectsMaxPat[ i ].y );
			nvgLineTo(vg, m_RectsMaxPat[ i ].x2, m_RectsMaxPat[ i ].y2 );
			nvgLineTo(vg, m_RectsMaxPat[ i ].x, m_RectsMaxPat[ i ].y2 );
			nvgClosePath(vg);
		    nvgFill(vg);

            // pattern select
            rgb.Col[ 0 ] = ( ( m_PatCol[ 1 ].Col[ 0 ] * m_PatSelLevel[ i ] ) + ( m_PatCol[ 0 ].Col[ 0 ] * ( 3 - m_PatSelLevel[ i ] ) ) ) / 3;
            rgb.Col[ 1 ] = ( ( m_PatCol[ 1 ].Col[ 1 ] * m_PatSelLevel[ i ] ) + ( m_PatCol[ 0 ].Col[ 1 ] * ( 3 - m_PatSelLevel[ i ] ) ) ) / 3;
            rgb.Col[ 2 ] = ( ( m_PatCol[ 1 ].Col[ 2 ] * m_PatSelLevel[ i ] ) + ( m_PatCol[ 0 ].Col[ 2 ] * ( 3 - m_PatSelLevel[ i ] ) ) ) / 3;

            nvgFillColor( vg, nvgRGB( rgb.Col[ 2 ], rgb.Col[ 1 ], rgb.Col[ 0 ] ) );

			nvgBeginPath(vg);
			nvgMoveTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y );
			nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y );
			nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y2 );
			nvgLineTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y2 );
			nvgClosePath(vg);
		    nvgFill(vg);

            if( i == m_PatClk )
            {
                nvgFillColor( vg, nvgRGBA( 0xFF, 0xFF, 0xFF, 0xc0 ) );
			    nvgBeginPath(vg);
			    nvgMoveTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y2 - 4 );
			    nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y2 - 4 );
			    nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y2 );
			    nvgLineTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y2 );
			    nvgClosePath(vg);
		        nvgFill(vg);
            }
        }
	}

    //-----------------------------------------------------
    // Procedure:   isPoint
    //-----------------------------------------------------
    bool isPoint( RECT_STRUCT *prect, int x, int y )
    {
        if( x < prect->x || x > prect->x2 || y < prect->y || y > prect->y2 )
            return false;

        return true;
    }

    //-----------------------------------------------------
    // Procedure:   onMouseDown
    //-----------------------------------------------------
    Widget *onMouseDown( Vec pos, int button ) override
    {
        int i;

        if( !m_bInitialized )
            return NULL;

        for( i = 0; i < m_nLEDs; i++)
        {
            if( isPoint( &m_RectsPatSel[ i ], (int)pos.x, (int)pos.y ) )
            {
                if( button == 0 )
                    SetPat( i );
                else
                    ClrPat( i );

                if( m_pCallback )
                    m_pCallback( m_pClass, m_Id, i, m_PatSelLevel[ i ], m_MaxPat );

                dirty = true;
                return this;
            }

            else if( isPoint( &m_RectsMaxPat[ i ], (int)pos.x, (int)pos.y ) )
            {
                m_MaxPat = i;

                if( m_pCallback )
                    m_pCallback( m_pClass, m_Id, i, m_PatSelLevel[ i ], m_MaxPat );

                dirty = true;
                return this;
            }

        }

        return NULL;
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void step() 
    {
	    FramebufferWidget::step();
    }
};

//-----------------------------------------------------
// PatternSelectStrip
//-----------------------------------------------------
#define MAX_PAT 32
struct PatternSelectStrip : OpaqueWidget, FramebufferWidget 
{
    typedef void PATCHANGECALLBACK ( void *pClass, int id, int pat, int maxpat );

    bool            m_bInitialized = false;
    int             m_Id;
    int             m_nLEDs;
    int             m_MaxPat = 0;
    int             m_PatSel = 0;
    int             m_PatPending = -1;
    int             m_StepCount;

    PATCHANGECALLBACK *m_pCallback;
    void              *m_pClass;

    RECT_STRUCT     m_RectsMaxPat[ MAX_PAT ];
    RECT_STRUCT     m_RectsPatSel[ MAX_PAT ];
    RGB_STRUCT      m_PatCol[ 2 ];
    RGB_STRUCT      m_MaxCol[ 2 ];

    //-----------------------------------------------------
    // Procedure:   Constructor
    //-----------------------------------------------------
    PatternSelectStrip( int x, int y, int w, int h, int colourPaton, int colourPatoff, int colourMaxon, int colourMaxoff, int nleds, int id, void *pClass, PATCHANGECALLBACK *pCallback )
    {
        int i;

        if ( nleds < 0 || nleds > MAX_PAT )
            return;

        m_Id = id;
        m_pCallback = pCallback;
        m_pClass = pClass;
        m_nLEDs = nleds;

        m_PatCol[ 0 ].dwCol = colourPatoff;
        m_PatCol[ 1 ].dwCol = colourPaton;

        m_MaxCol[ 0 ].dwCol = colourMaxoff;
        m_MaxCol[ 1 ].dwCol = colourMaxon;

		box.pos = Vec(x, y);
        box.size = Vec( w * m_nLEDs + ( m_nLEDs * 2 ) - 1, ( h * 2 ) + 2 );

        x = 0;

        for( i = 0; i < m_nLEDs; i++ )
        {
            m_RectsMaxPat[ i ].x = x;
            m_RectsMaxPat[ i ].y = 0;
            m_RectsMaxPat[ i ].x2 = x + w;
            m_RectsMaxPat[ i ].y2 = h;

            m_RectsPatSel[ i ].x = x;
            m_RectsPatSel[ i ].y = h + 2;
            m_RectsPatSel[ i ].x2 = x + w;
            m_RectsPatSel[ i ].y2 = ( h * 2 ) + 2;

            x += ( w + 2 );
        }

        m_bInitialized = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetPat
    //-----------------------------------------------------
    void SetPat( int pat, bool bPending )
    {
        if( bPending )
            m_PatPending = pat;
        else
        {
            m_PatPending = -1;
            m_PatSel = pat;
        }

        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetMax
    //-----------------------------------------------------
    void SetMax( int max )
    {
        m_MaxPat = max;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(NVGcontext *vg) override
    {
        int i;

        if( !m_bInitialized )
            return;

		nvgFillColor(vg, nvgRGBA(0, 0, 0, 0xc0));
		nvgBeginPath(vg);
		nvgMoveTo(vg, -1, -1 );
		nvgLineTo(vg, box.size.x + 1, -1 );
		nvgLineTo(vg, box.size.x + 1, box.size.y + 1 );
		nvgLineTo(vg, -1, box.size.y + 1 );
		nvgClosePath(vg);
		nvgFill(vg);

        for( i = 0; i < m_nLEDs; i++ )
        {
            if( i <= m_MaxPat )
                nvgFillColor( vg, nvgRGB( m_MaxCol[ 1 ].Col[ 2 ], m_MaxCol[ 1 ].Col[ 1 ], m_MaxCol[ 1 ].Col[ 0 ] ) );
            else
                nvgFillColor( vg, nvgRGB( m_MaxCol[ 0 ].Col[ 2 ], m_MaxCol[ 0 ].Col[ 1 ], m_MaxCol[ 0 ].Col[ 0 ] ) );

			nvgBeginPath(vg);
			nvgMoveTo(vg, m_RectsMaxPat[ i ].x, m_RectsMaxPat[ i ].y );
			nvgLineTo(vg, m_RectsMaxPat[ i ].x2, m_RectsMaxPat[ i ].y );
			nvgLineTo(vg, m_RectsMaxPat[ i ].x2, m_RectsMaxPat[ i ].y2 );
			nvgLineTo(vg, m_RectsMaxPat[ i ].x, m_RectsMaxPat[ i ].y2 );
			nvgClosePath(vg);
		    nvgFill(vg);

            if( m_PatSel == i )
                nvgFillColor( vg, nvgRGB( m_PatCol[ 1 ].Col[ 2 ], m_PatCol[ 1 ].Col[ 1 ], m_PatCol[ 1 ].Col[ 0 ] ) );
            else
                nvgFillColor( vg, nvgRGB( m_PatCol[ 0 ].Col[ 2 ], m_PatCol[ 0 ].Col[ 1 ], m_PatCol[ 0 ].Col[ 0 ] ) );

			nvgBeginPath(vg);
			nvgMoveTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y );
			nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y );
			nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y2 );
			nvgLineTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y2 );
			nvgClosePath(vg);
		    nvgFill(vg);

            if( m_PatPending == i )
            {
                nvgFillColor( vg, nvgRGBA( m_PatCol[ 1 ].Col[ 2 ], m_PatCol[ 1 ].Col[ 1 ], m_PatCol[ 1 ].Col[ 0 ], 0x50 ) );

			    nvgBeginPath(vg);
			    nvgMoveTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y );
			    nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y );
			    nvgLineTo(vg, m_RectsPatSel[ i ].x2, m_RectsPatSel[ i ].y2 );
			    nvgLineTo(vg, m_RectsPatSel[ i ].x, m_RectsPatSel[ i ].y2 );
			    nvgClosePath(vg);
		        nvgFill(vg);
            }
        }
	}

    //-----------------------------------------------------
    // Procedure:   isPoint
    //-----------------------------------------------------
    bool isPoint( RECT_STRUCT *prect, int x, int y )
    {
        if( x < prect->x || x > prect->x2 || y < prect->y || y > prect->y2 )
            return false;

        return true;
    }

    //-----------------------------------------------------
    // Procedure:   onMouseDown
    //-----------------------------------------------------
    Widget *onMouseDown( Vec pos, int button ) override
    {
        int i;

        if( !m_bInitialized )
            return NULL;

        if( button != 0 )
            return NULL;

        for( i = 0; i < m_nLEDs; i++)
        {
            if( isPoint( &m_RectsMaxPat[ i ], (int)pos.x, (int)pos.y ) )
            {
                m_MaxPat = i;

                if( m_pCallback )
                    m_pCallback( m_pClass, m_Id, m_PatSel, m_MaxPat );

                dirty = true;
                return this;
            }

            else if( isPoint( &m_RectsPatSel[ i ], (int)pos.x, (int)pos.y ) )
            {
                m_PatSel = i;

                if( m_pCallback )
                    m_pCallback( m_pClass, m_Id, m_PatSel, m_MaxPat );

                dirty = true;
                return this;
            }
        }

        return NULL;
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void step() 
    {
	    FramebufferWidget::step();
    }
};

//-----------------------------------------------------
// CompressorLEDMeterWidget
//-----------------------------------------------------
#define nDISPLAY_LEDS 10
const float fleveldb[ nDISPLAY_LEDS ] = { 0, 3, 6, 10, 20, 30, 40, 50, 60, 80 };
struct CompressorLEDMeterWidget : TransparentWidget 
{
    bool            m_bOn[ nDISPLAY_LEDS ] = {};
    int             m_StepCount;
    float           m_fLargest = 0;
    RECT_STRUCT     m_Rects[ nDISPLAY_LEDS ];
    RGB_STRUCT      m_ColourOn;
    RGB_STRUCT      m_ColourOff;
    bool            m_bInvert;

    float flevels[ nDISPLAY_LEDS ];

    //-----------------------------------------------------
    // Procedure:   Constructor
    //-----------------------------------------------------
    CompressorLEDMeterWidget( bool bInvert, int x, int y, int w, int h, int colouron, int colouroff )
    {
        int i;

        m_bInvert = bInvert;
        m_ColourOn.dwCol = colouron;
        m_ColourOff.dwCol = colouroff;

		box.pos = Vec(x, y);
        box.size = Vec( w, h * nDISPLAY_LEDS + ( nDISPLAY_LEDS * 2 ) );

        y = 1;

        for( i = 0; i < nDISPLAY_LEDS; i++ )
        {
            flevels[ i ] = db_to_lvl( fleveldb[ i ] );

            m_Rects[ i ].x = 0;
            m_Rects[ i ].y = y;
            m_Rects[ i ].x2 = w;
            m_Rects[ i ].y2 = y + h;

            y += ( h + 2 );
        }
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(NVGcontext *vg) override
    {
        int i;

		nvgFillColor(vg, nvgRGBA(0, 0, 0, 0xc0));
		nvgBeginPath(vg);
		nvgMoveTo(vg, -1, -1 );
		nvgLineTo(vg, box.size.x + 1, -1 );
		nvgLineTo(vg, box.size.x + 1, box.size.y + 1 );
		nvgLineTo(vg, -1, box.size.y + 1 );
		nvgClosePath(vg);
		nvgFill(vg);

        for( i = 0; i < nDISPLAY_LEDS; i++ )
        {
            if( m_bOn[ i ] )
                nvgFillColor( vg, nvgRGB( m_ColourOn.Col[ 2 ], m_ColourOn.Col[ 1 ], m_ColourOn.Col[ 0 ] ) );
            else
                nvgFillColor( vg, nvgRGB( m_ColourOff.Col[ 2 ], m_ColourOff.Col[ 1 ], m_ColourOff.Col[ 0 ] ) );

			nvgBeginPath(vg);
			nvgMoveTo(vg, m_Rects[ i ].x, m_Rects[ i ].y );
			nvgLineTo(vg, m_Rects[ i ].x2, m_Rects[ i ].y );
			nvgLineTo(vg, m_Rects[ i ].x2, m_Rects[ i ].y2 );
			nvgLineTo(vg, m_Rects[ i ].x, m_Rects[ i ].y2 );
			nvgClosePath(vg);
		    nvgFill(vg);
        }
	}

    //-----------------------------------------------------
    // Procedure:   Process
    //-----------------------------------------------------
    void Process( float level )
    {
        int steptime = (int)( gSampleRate * 0.05 );
        int i;

        if( abs( level ) > m_fLargest )
            m_fLargest = abs( level );

        // only process every 1/10th of a second
        if( ++m_StepCount >= steptime )
        {
            m_StepCount = 0;

            if( m_bInvert )
            {
                for( i = 0; i < nDISPLAY_LEDS; i++ )
                {
                    if( m_fLargest >= flevels[ i ] )
                        m_bOn[ ( nDISPLAY_LEDS - 1 ) - i ] = true;
                    else
                        m_bOn[ ( nDISPLAY_LEDS - 1 ) - i ] = false;
                }
            }
            else
            {
                for( i = 0; i < nDISPLAY_LEDS; i++ )
                {
                    if( m_fLargest >= flevels[ i ] )
                        m_bOn[ i ] = true;
                    else
                        m_bOn[ i ] = false;
                }
            }

            m_fLargest = 0.0f;
        }
    }
};

//-----------------------------------------------------
// LEDMeterWidget
//-----------------------------------------------------
struct LEDMeterWidget : TransparentWidget 
{
    bool            m_bOn[ nDISPLAY_LEDS ] = {};
    int             m_space;
    int             m_StepCount;
    bool            m_bVert;
    float           m_fLargest = 0.0;
    RECT_STRUCT     m_Rects[ nDISPLAY_LEDS ];
    RGB_STRUCT      m_ColoursOn[ nDISPLAY_LEDS ];
    RGB_STRUCT      m_ColoursOff[ nDISPLAY_LEDS ];
    float           flevels[ nDISPLAY_LEDS ] = {};

    //-----------------------------------------------------
    // Procedure:   Constructor
    //-----------------------------------------------------
    LEDMeterWidget( int x, int y, int w, int h, int space, bool bVert )
    {
        int i, xoff = 0, yoff = 0, xpos = 0, ypos = 0;

        m_space = space;
		box.pos = Vec(x, y);

        if( bVert )
        {
            box.size = Vec( w, h * nDISPLAY_LEDS + (m_space * nDISPLAY_LEDS) );
            yoff = h + m_space; 
        }
        else
        {
            box.size = Vec( w * nDISPLAY_LEDS + (m_space * nDISPLAY_LEDS), h );
            xoff = w + m_space;
        }

        for( i = 0; i < nDISPLAY_LEDS; i++ )
        {
            flevels[ i ] = db_to_lvl( fleveldb[ i ] );

            m_Rects[ i ].x = xpos;
            m_Rects[ i ].y = ypos;
            m_Rects[ i ].x2 = xpos + w;
            m_Rects[ i ].y2 = ypos + h;

            // always red
            if( i == 0 )
            {
                m_ColoursOn[ i ].dwCol = DWRGB( 0xFF, 0, 0 );
                m_ColoursOff[ i ].dwCol= DWRGB( 0x80, 0, 0 );
            }
            // yellow
            else if( i < 3 )
            {
                m_ColoursOn[ i ].dwCol = DWRGB( 0xFF, 0xFF, 0 );
                m_ColoursOff[ i ].dwCol= DWRGB( 0x80, 0x80, 0 );
            }
            // green
            else
            {
                m_ColoursOn[ i ].dwCol = DWRGB( 0, 0xFF, 0 );
                m_ColoursOff[ i ].dwCol= DWRGB( 0, 0x80, 0 );
            }

            xpos += xoff;
            ypos += yoff;
        }
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(NVGcontext *vg) override
    {
        int i;

		nvgFillColor(vg, nvgRGBA(0, 0, 0, 0xc0));
		nvgBeginPath(vg);
		nvgMoveTo(vg, -1, -1 );
		nvgLineTo(vg, box.size.x + 1, -1 );
		nvgLineTo(vg, box.size.x + 1, box.size.y + 1 );
		nvgLineTo(vg, -1, box.size.y + 1 );
		nvgClosePath(vg);
		nvgFill(vg);

        for( i = 0; i < nDISPLAY_LEDS; i++ )
        {
            if( m_bOn[ i ] )
                nvgFillColor( vg, nvgRGB( m_ColoursOn[ i ].Col[ 2 ], m_ColoursOn[ i ].Col[ 1 ], m_ColoursOn[ i ].Col[ 0 ] ) );
            else
                nvgFillColor( vg, nvgRGB( m_ColoursOff[ i ].Col[ 2 ], m_ColoursOff[ i ].Col[ 1 ], m_ColoursOff[ i ].Col[ 0 ] ) );

			nvgBeginPath(vg);
			nvgMoveTo(vg, m_Rects[ i ].x, m_Rects[ i ].y );
			nvgLineTo(vg, m_Rects[ i ].x2, m_Rects[ i ].y );
			nvgLineTo(vg, m_Rects[ i ].x2, m_Rects[ i ].y2 );
			nvgLineTo(vg, m_Rects[ i ].x, m_Rects[ i ].y2 );
			nvgClosePath(vg);
		    nvgFill(vg);
        }
	}

    //-----------------------------------------------------
    // Procedure:   Process
    //-----------------------------------------------------
    void Process( float level )
    {
        int steptime = (int)( gSampleRate * 0.05 );
        int i;

        if( abs( level ) > m_fLargest )
            m_fLargest = abs( level );

        // only process every 1/10th of a second
        if( ++m_StepCount >= steptime )
        {
            m_StepCount = 0;

            for( i = 0; i < nDISPLAY_LEDS; i++ )
            {
                if( m_fLargest >= flevels[ i ] )
                    m_bOn[ i ] = true;
                else
                    m_bOn[ i ] = false;
            }

            m_fLargest = 0.0;
        }
    }
};

//-----------------------------------------------------
// Keyboard_3Oct_Widget
//-----------------------------------------------------

typedef struct
{
    int nUsed;
    POINT_STRUCT p[ 8 ];
}KEY_VECT_STRUCT;

struct Keyboard_3Oct_Widget : OpaqueWidget, FramebufferWidget
{
    typedef void NOTECHANGECALLBACK ( void *pClass, int kb, int note );

    int m_KeyOn = 0;
    int m_LastKeyOn = 0;
    int m_x, m_y;
    RECT_STRUCT keyrects[ 37 ] ={};
    NOTECHANGECALLBACK *pNoteChangeCallback = NULL;
    void *m_pClass = NULL;
    int m_nKb;

KEY_VECT_STRUCT OctaveKeyDrawVects[ 37 ] = 
{
    { 6, { {1, 1}, {1, 62}, {12, 62}, {12, 39}, {7, 39}, {7, 1}, {0, 0}, {0, 0} } },
    { 4, { {8, 1}, {8, 38}, {16, 38}, {16, 1},  {0, 0},  {0, 0}, {0, 0}, {0, 0} } },
    { 8, { {17, 1}, {17, 39}, {14, 39}, {14, 62}, {25, 62}, {25, 39}, {22, 39}, {22, 1} } },
    { 4, { {23, 1}, {23, 38}, {31, 38}, {31, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 6, { {32, 1}, {32, 39}, {27, 39}, {27, 62}, {38, 62}, {38, 1}, {0, 0}, {0, 0} } },
    { 6, { {40, 1}, {40, 62}, {51, 62}, {51, 39}, {46, 39}, {46, 1}, {0, 0}, {0, 0} } },
    { 4, { {47, 1}, {47, 38}, {55, 38}, {55, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 8, { {56, 1}, {56, 39}, {53, 39}, {53, 62}, {64, 62}, {64, 39}, {60, 39}, {60, 1} } },
    { 4, { {61, 1}, {61, 38}, {69, 38}, {69, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 8, { {70, 1}, {70, 39}, {66, 39}, {66, 62}, {77, 62}, {77, 39}, {74, 39}, {74, 1} } },
    { 4, { {75, 1}, {75, 38}, {83, 38}, {83, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 6, { {84, 1}, {84, 39}, {79, 39}, {79, 62}, {90, 62}, {90, 1}, {0, 0}, {0, 0} } },
};

int keysquare_x[ 37 ] = 
{
    1, 8, 14, 23, 27, 40, 47, 53, 61, 66, 75, 79
};

KEY_VECT_STRUCT OctaveKeyHighC [ 1 ]= 
{
    { 5, { {1, 1}, {1, 62}, {12, 62}, {12, 44}, {12, 1}, {0, 0}, {0, 0}, {0, 0} } }
};

#define OCT_OFFSET_X 91
    
    bool    m_bForceChange = true;
    RGB_STRUCT rgb_white, rgb_black, rgb_on;
    CLog *lg;

    //-----------------------------------------------------
    // Procedure:   Constructor
    //-----------------------------------------------------
    Keyboard_3Oct_Widget( int x, int y, int nKb, void *pClass, NOTECHANGECALLBACK *pcallback, CLog *plog )
    {
        int i, j, oct = 0;

        pNoteChangeCallback = pcallback;
        m_pClass = pClass;
        m_nKb = nKb;

        m_x = x;
        m_y = y;

        lg = plog;

        // calc key rects and calculate the remainder of the key vec list
        for( i = 0; i < 37; i++ )
        {
            if( i >= 12 )
            {
                oct = i / 12;

                if( i == 36 )
                {
                    // populate the rest of the key vect table based on the first octave
                    memcpy( &OctaveKeyDrawVects[ i ], &OctaveKeyHighC[ 0 ], sizeof(KEY_VECT_STRUCT) ); 

                    for( j = 0; j < 8; j++ )
                        OctaveKeyDrawVects[ i ].p[ j ].x = OctaveKeyHighC[ 0 ].p[ j ].x + ( OCT_OFFSET_X * oct );
                        
                    keysquare_x[ i ] = keysquare_x[ 0 ] + ( OCT_OFFSET_X * oct );
                }
                else
                {
                    // populate the rest of the key vect table based on the first octave
                    memcpy( &OctaveKeyDrawVects[ i ], &OctaveKeyDrawVects[ i - ( oct * 12 ) ], sizeof(KEY_VECT_STRUCT) ); 

                    for( j = 0; j < 8; j++ )
                        OctaveKeyDrawVects[ i ].p[ j ].x = OctaveKeyDrawVects[ i - ( oct * 12 ) ].p[ j ].x + ( OCT_OFFSET_X * oct );

                    keysquare_x[ i ] = keysquare_x[ i - ( oct * 12 ) ] + ( OCT_OFFSET_X * oct );
                }
            }

            // build the key rects for key press detection
            keyrects[ i ].x = keysquare_x[ i ] + m_x - 1;
            keyrects[ i ].y = 1 + m_y;

            if( OctaveKeyDrawVects[ i ].nUsed == 4 )
            {
                // black key
                keyrects[ i ].x2 = keyrects[ i ].x + 9;
                keyrects[ i ].y2 = keyrects[ i ].y + 43;
            }
            else
            {
                // white key
                keyrects[ i ].x2 = keyrects[ i ].x + 14;
                keyrects[ i ].y2 = keyrects[ i ].y + 67;
            }
        }

        rgb_white.dwCol = DWRGB( 215, 207, 198 );
        rgb_black.dwCol = 0;
        rgb_on.dwCol = DWRGB( 217, 97, 38 ); 
    }

    //-----------------------------------------------------
    // Procedure:   drawkey
    //-----------------------------------------------------
    void drawkey( NVGcontext *vg, int key, bool bOn )
    {
        int i;

        if( key < 0 || key >= 37 )
            return;

        if( bOn )
        {
            nvgFillColor(vg, nvgRGB( rgb_on.Col[ 2 ], rgb_on.Col[ 1 ], rgb_on.Col[ 0 ] ) );
        }
        else
        {
            if( OctaveKeyDrawVects[ key ].nUsed == 4 )
                nvgFillColor(vg, nvgRGB( rgb_black.Col[ 2 ], rgb_black.Col[ 1 ], rgb_black.Col[ 0 ] ) );
            else
                nvgFillColor(vg, nvgRGB( rgb_white.Col[ 2 ], rgb_white.Col[ 1 ], rgb_white.Col[ 0 ] ) );
        }

        // draw key
		nvgBeginPath(vg);

        for( i = 0; i < OctaveKeyDrawVects[ key ].nUsed; i++ )
        {
            if( i == 0 )
                nvgMoveTo(vg, (float)OctaveKeyDrawVects[ key ].p[ i ].x + m_x, (float)OctaveKeyDrawVects[ key ].p[ i ].y + m_y );
            else
		        nvgLineTo(vg, (float)OctaveKeyDrawVects[ key ].p[ i ].x + m_x, (float)OctaveKeyDrawVects[ key ].p[ i ].y + m_y );
        }
        
        nvgClosePath(vg);
		nvgFill(vg);
    }

    //-----------------------------------------------------
    // Procedure:   isPoint
    //-----------------------------------------------------
    bool isPoint( RECT_STRUCT *prect, int x, int y )
    {
        if( x < prect->x || x > prect->x2 || y < prect->y || y > prect->y2 )
            return false;

        return true;
    }

    //-----------------------------------------------------
    // Procedure:   onMouseDown
    //-----------------------------------------------------
    Widget *onMouseDown( Vec pos, int button ) override
    {
        int i;
        //bool dirty = false;

        if( button != 0 )
            return NULL;

        //lg->f("Down x = %.3f, y = %.3f\n", pos.x, pos.y );

        // check black keys first they are on top
        for( i = 0; i < 37; i++)
        {
            //lg->f("rect %d) x = %d, y = %d, x2 = %d, y2 = %d\n", i, keyrects[ i ].x, keyrects[ i ].y, keyrects[ i ].x2, keyrects[ i ].y2 );
            if( OctaveKeyDrawVects[ i ].nUsed == 4 && isPoint( &keyrects[ i ], (int)pos.x, (int)pos.y ) )
            {
                if( pNoteChangeCallback )
                    pNoteChangeCallback( m_pClass, m_nKb, i );
                m_KeyOn = i;
                dirty = true;
                return this;
            }
        }

        // check white keys
        for( i = 0; i < 37; i++)
        {
            //lg->f("rect %d) x = %d, y = %d, x2 = %d, y2 = %d\n", i, keyrects[ i ].x - m_x, keyrects[ i ].y, keyrects[ i ].x2 - m_x, keyrects[ i ].y2 );
            if( OctaveKeyDrawVects[ i ].nUsed != 4 && isPoint( &keyrects[ i ], (int)pos.x, (int)pos.y ) )
            {
                //lg->f("found rect %d) x = %d, y = %d, x2 = %d, y2 = %d\n", i, keyrects[ i ].x - m_x, keyrects[ i ].y, keyrects[ i ].x2, keyrects[ i ].y2 );
                //lg->f("white found!\n");
                if( pNoteChangeCallback )
                    pNoteChangeCallback( m_pClass, m_nKb, i );
                m_KeyOn = i;
                dirty = true;
                return this;
            }
        }

        return NULL;
    }

    //-----------------------------------------------------
    // Procedure:   setkey
    //-----------------------------------------------------
    void setkey( int key ) 
    {
	    m_KeyOn = key;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   step
    //-----------------------------------------------------
    void step() 
    {
	    FramebufferWidget::step();
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(NVGcontext *vg) override
    {
        for( int key = 0; key < 37; key++ )
            drawkey( vg, key, false );

        //don't draw if we don't have to
        //if( m_KeyOn == m_LastKeyOn && !m_bForceChange )
            //return;

        m_bForceChange = false;
        

        //drawkey( vg, m_LastKeyOn, false );
        drawkey( vg, m_KeyOn, true );

        m_LastKeyOn = m_KeyOn;
	}
};

//-----------------------------------------------------
// Procedure:   MyScrew
//
//-----------------------------------------------------
struct MyScrew : SVGScrew 
{
	MyScrew() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_screw.svg" );
        sw->svg = SVG::load(assetGlobal( resource ));
		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   MySquareButton
//
//-----------------------------------------------------
struct MySquareButtonSmall : SVGSwitch, MomentarySwitch 
{
	MySquareButtonSmall() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_square_button.svg" );
		addFrame(SVG::load(assetGlobal(resource)));
		sw->wrap();
		box.size = Vec(9, 9);
	}
};

//-----------------------------------------------------
// Procedure:   MySquareButton
//
//-----------------------------------------------------
struct MySquareButton : SVGSwitch, MomentarySwitch 
{
	MySquareButton() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_square_button.svg" );
		addFrame(SVG::load(assetGlobal(resource)));
		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   MySquareButton2
//
//-----------------------------------------------------
struct MySquareButton2 : SVGSwitch, MomentarySwitch 
{
	MySquareButton2() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Square_Button2.svg" );
		addFrame(SVG::load(assetGlobal(resource)));
		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   PianoWhiteKey
//
//-----------------------------------------------------
struct PianoWhiteKey : SVGSwitch, ToggleSwitch
{
	PianoWhiteKey()
    {
        std::string resource( RESOURCE_LOCATION );
		addFrame(SVG::load(assetGlobal(resource + "mschack_WhiteKeyOff.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_WhiteKeyOn.svg")));
		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   PianoBlackKey
//
//-----------------------------------------------------
struct PianoBlackKey : SVGSwitch, ToggleSwitch
{
	PianoBlackKey()
    {
        std::string resource( RESOURCE_LOCATION );
		addFrame(SVG::load(assetGlobal(resource + "mschack_BlackKeyOff.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_BlackKeyOn.svg")));
		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   MyToggle1
//
//-----------------------------------------------------
struct MyToggle1 : SVGSwitch, ToggleSwitch
{
	MyToggle1()
    {
        std::string resource( RESOURCE_LOCATION );
		addFrame(SVG::load(assetGlobal(resource + "mschack_3p_vert_simple_01.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_3p_vert_simple_02.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_3p_vert_simple_03.svg")));
		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   FilterSelectToggle
//
//-----------------------------------------------------
struct FilterSelectToggle : SVGSwitch, ToggleSwitch
{
	FilterSelectToggle()
    {
        std::string resource( RESOURCE_LOCATION );
		addFrame(SVG::load(assetGlobal(resource + "mschack_5p_filtersel_01.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_5p_filtersel_02.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_5p_filtersel_03.svg")));
		addFrame(SVG::load(assetGlobal(resource + "mschack_5p_filtersel_04.svg")));
        //addFrame(SVG::load(assetGlobal("res/mschack_5p_filtersel_05.svg")));

		sw->wrap();
		box.size = sw->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   MySlider_01
//
//-----------------------------------------------------
struct MySlider_01 : SVGSlider 
{
	MySlider_01() 
    {
        std::string resource( RESOURCE_LOCATION );
		Vec margin = Vec(0, 0);
		maxHandlePos = Vec(0, -4).plus(margin);
		minHandlePos = Vec(0, 33).plus(margin);
		background->svg = SVG::load(assetGlobal(resource + "mschack_sliderBG_01.svg"));
		background->wrap();
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
		handle->svg = SVG::load(assetGlobal(resource + "mschack_sliderKNOB_01.svg"));
		handle->wrap();
	}
};

//-----------------------------------------------------
// Procedure:   MyPortInSmall
//
//-----------------------------------------------------
struct MyPortInSmall : SVGPort 
{
	MyPortInSmall() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_PortIn_small.svg" );
		background->svg = SVG::load(assetGlobal(resource));
		background->wrap();
		box.size = background->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   MyPortOutSmall
//
//-----------------------------------------------------
struct MyPortOutSmall : SVGPort 
{
	MyPortOutSmall() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_PortOut_small.svg" );
		background->svg = SVG::load(assetGlobal(resource));
		background->wrap();
		box.size = background->box.size;
	}
};

//-----------------------------------------------------
// Procedure:   Red1_Med
//
//-----------------------------------------------------
struct Red1_Med : RoundKnob 
{
	Red1_Med() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_KnobRed1.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(20, 20);
	}
};

//-----------------------------------------------------
// Procedure:   Blue3_Med
//
//-----------------------------------------------------
struct Blue3_Med : RoundKnob 
{
	Blue3_Med() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_KnobBlue3.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(20, 20);
	}
};

//-----------------------------------------------------
// Procedure:   Yellow3_Med
//
//-----------------------------------------------------
struct Yellow3_Med : RoundKnob 
{
	Yellow3_Med() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_KnobYellow3.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(20, 20);
	}
};

//-----------------------------------------------------
// Procedure:   Purp1_Med
//
//-----------------------------------------------------
struct Purp1_Med : RoundKnob 
{
	Purp1_Med() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_KnobPurp1.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(20, 20);
	}
};

//-----------------------------------------------------
// Procedure:   Green1_Tiny
//
//-----------------------------------------------------
struct Green1_Tiny : RoundKnob 
{
	Green1_Tiny() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Green1_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(15, 15);
	}
};

//-----------------------------------------------------
// Procedure:   Green1_Big
//
//-----------------------------------------------------
struct Green1_Big : RoundKnob 
{
	Green1_Big() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Green1_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(40, 40);
	}
};

//-----------------------------------------------------
// Procedure:   Blue1_Small
//
//-----------------------------------------------------
struct Blue1_Small : RoundKnob 
{
	Blue1_Small() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Blue1_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(26, 26);
	}
};

//-----------------------------------------------------
// Procedure:   Blue2_Small
//
//-----------------------------------------------------
struct Blue2_Small : RoundKnob 
{
	Blue2_Small() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Blue2_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(26, 26);
	}
};

//-----------------------------------------------------
// Procedure:   Blue2_Tiny
//
//-----------------------------------------------------
struct Blue2_Tiny : RoundKnob 
{
	Blue2_Tiny() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Blue2_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(15, 15);
	}
};

//-----------------------------------------------------
// Procedure:   Blue2_Med
//
//-----------------------------------------------------
struct Blue2_Med : RoundKnob 
{
	Blue2_Med() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Blue2_big.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(40, 40);
	}
};

//-----------------------------------------------------
// Procedure:   Blue2_Big
//
//-----------------------------------------------------
struct Blue2_Big : RoundKnob 
{
	Blue2_Big() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Blue2_big.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(56, 56);
	}
};

//-----------------------------------------------------
// Procedure:   Yellow1_Small
//
//-----------------------------------------------------
struct Yellow1_Small : RoundKnob 
{
	Yellow1_Small() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Yellow1_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(26, 26);
	}
};

//-----------------------------------------------------
// Procedure:   Yellow1_Tiny
//
//-----------------------------------------------------
struct Yellow1_Tiny : RoundKnob 
{
	Yellow1_Tiny() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Yellow1_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(15, 15);
	}
};


//-----------------------------------------------------
// Procedure:   Yellow2_Small
//
//-----------------------------------------------------
struct Yellow2_Small : RoundKnob 
{
	Yellow2_Small() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Yellow2_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(26, 26);
	}
};

//-----------------------------------------------------
// Procedure:   Yellow2_Big
//
//-----------------------------------------------------
struct Yellow2_Big : RoundKnob 
{
	Yellow2_Big() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Yellow2_small.svg" );
		setSVG(SVG::load(assetGlobal(resource)));
		box.size = Vec(40, 40);
	}
};

//-----------------------------------------------------
// Procedure:   Yellow2_Huge
//
//-----------------------------------------------------
struct Yellow2_Huge : RoundKnob 
{
	Yellow2_Huge() 
    {
        std::string resource( RESOURCE_LOCATION );
        resource.append( "mschack_Yellow2_small.svg" );
		setSVG(SVG::load(assetGlobal( resource )));
		box.size = Vec(56, 56);
	}
};