#include "rack.hpp"
#include "CLog.h"

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
// LEDMeterWidget
//-----------------------------------------------------
struct LEDMeterWidget : TransparentWidget 
{
    bool            m_bOn[ 7 ] = {};
    int             m_nLEDs;
    int             m_StepCount;
    bool            m_bVert;
    float           m_fLevel = 0;
    RECT_STRUCT     m_Rects[ 7 ];
    RGB_STRUCT      m_ColoursOn[ 7 ];
    RGB_STRUCT      m_ColoursOff[ 7 ];

    const float flevelsmax[ 5 ][ 7 ] =
    {
        {1.0, 0.708, 0.00001, 0, 0, 0, 0 }, 
        {1.0, 0.708, 0.5, 0.00001, 0, 0, 0 }, 
        {1.0, 0.708, 0.5, 0.316, 0.00001, 0, 0 }, 
        {1.0, 0.708, 0.5, 0.316, 0.1, 0.00001, 0 }, 
        {1.0, 0.708, 0.5, 0.316, 0.1, 0.032, 0.00001 } 
    };

    const int ledcolourratio[ 5 ][ 7 ] = 
    { 
        {0, 1, 2, 0, 0, 0, 0 }, 
        {0, 1, 2, 2, 0, 0, 0 }, 
        {0, 1, 2, 2, 2, 0, 0 }, 
        {0, 1, 1, 2, 2, 2, 0 }, 
        {0, 1, 1, 2, 2, 2, 2 } 
    };

    //-----------------------------------------------------
    // Procedure:   Constructor
    //-----------------------------------------------------
    LEDMeterWidget( int x, int y, int w, int h, int nLEDs, bool bVert )
    {
        int i, xoff = 0, yoff = 0, xpos = 0, ypos = 0;

        if( nLEDs < 3 || nLEDs > 7 )
            return;

		box.pos = Vec(x, y);
        m_nLEDs = nLEDs;

        if( bVert )
        {
            box.size = Vec( w, h * nLEDs + ( nLEDs * 2 ) );
            yoff = h+2; 
        }
        else
        {
            box.size = Vec( w * nLEDs + ( nLEDs * 2 ), h );
            xoff = w+2;
        }

        for( i = 0; i < m_nLEDs; i++ )
        {
            m_Rects[ i ].x = xpos;
            m_Rects[ i ].y = ypos;
            m_Rects[ i ].x2 = xpos + w;
            m_Rects[ i ].y2 = ypos + h;

            switch( ledcolourratio[ m_nLEDs - 3 ][ i ] )
            {
            case 0: // red
                m_ColoursOn[ i ].dwCol = DWRGB( 0xFF, 0, 0 );
                m_ColoursOff[ i ].dwCol= DWRGB( 0x80, 0, 0 );
                break;
            case 1: // yellow
                m_ColoursOn[ i ].dwCol = DWRGB( 0xFF, 0xFF, 0 );
                m_ColoursOff[ i ].dwCol= DWRGB( 0x80, 0x80, 0 );
                break;
            case 2: // green
                m_ColoursOn[ i ].dwCol = DWRGB( 0, 0xFF, 0 );
                m_ColoursOff[ i ].dwCol= DWRGB( 0, 0x80, 0 );
                break;
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

        for( i = 0; i < m_nLEDs; i++ )
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

        // only process every 1/10th of a second
        if( ++m_StepCount >= steptime )
        {
            if( m_fLevel < abs( level ) )
                m_fLevel += fmin( 0.1, abs( level ) - m_fLevel );
            else
                m_fLevel -= fmin( 0.1, m_fLevel - abs( level ) );

            m_StepCount = 0;

            for( i = 0; i < m_nLEDs; i++ )
            {
                if( abs( m_fLevel ) >= flevelsmax[ m_nLEDs - 3 ][ i ] )
                    m_bOn[ i ] = true;
                else
                    m_bOn[ i ] = false;
            }
        }
    }
};

//-----------------------------------------------------
// LEDMeterWidget
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
    { 6, { {1, 1}, {1, 67}, {12, 67}, {12, 44}, {7, 44}, {7, 1}, {0, 0}, {0, 0} } },
    { 4, { {8, 1}, {8, 43}, {16, 43}, {16, 1},  {0, 0},  {0, 0}, {0, 0}, {0, 0} } },
    { 8, { {17, 1}, {17, 44}, {14, 44}, {14, 67}, {25, 67}, {25, 44}, {22, 44}, {22, 1} } },
    { 4, { {23, 1}, {23, 43}, {31, 43}, {31, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 6, { {32, 1}, {32, 44}, {27, 44}, {27, 67}, {38, 67}, {38, 1}, {0, 0}, {0, 0} } },
    { 6, { {40, 1}, {40, 67}, {51, 67}, {51, 44}, {46, 44}, {46, 1}, {0, 0}, {0, 0} } },
    { 4, { {47, 1}, {47, 43}, {55, 43}, {55, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 8, { {56, 1}, {56, 44}, {53, 44}, {53, 67}, {64, 67}, {64, 44}, {60, 44}, {60, 1} } },
    { 4, { {61, 1}, {61, 43}, {69, 43}, {69, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 8, { {70, 1}, {70, 44}, {66, 44}, {66, 67}, {77, 67}, {77, 44}, {74, 44}, {74, 1} } },
    { 4, { {75, 1}, {75, 43}, {83, 43}, {83, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
    { 6, { {84, 1}, {84, 44}, {79, 44}, {79, 67}, {90, 67}, {90, 1}, {0, 0}, {0, 0} } },
};

int keysquare_x[ 37 ] = 
{
    1, 8, 14, 23, 27, 40, 47, 53, 61, 66, 75, 79
};

KEY_VECT_STRUCT OctaveKeyHighC [ 1 ]= 
{
    { 5, { {1, 1}, {1, 67}, {12, 67}, {12, 44}, {12, 1}, {0, 0}, {0, 0}, {0, 0} } }
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
                goto done;
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
                break;
            }
        }

    done:;
        return NULL;
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void setkey( int key ) 
    {
	    m_KeyOn = key;
        dirty = true;
    }

    //-----------------------------------------------------
    // Procedure:   draw
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
		sw->svg = SVG::load(assetGlobal("plugins/mschack/res/mschack_screw.svg"));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_square_button.svg")));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_square_button.svg")));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_Square_Button2.svg")));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_WhiteKeyOff.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_WhiteKeyOn.svg")));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_BlackKeyOff.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_BlackKeyOn.svg")));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_3p_vert_simple_01.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_3p_vert_simple_02.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_3p_vert_simple_03.svg")));
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
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_5p_filtersel_01.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_5p_filtersel_02.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_5p_filtersel_03.svg")));
		addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_5p_filtersel_04.svg")));
        //addFrame(SVG::load(assetGlobal("plugins/mschack/res/mschack_5p_filtersel_05.svg")));

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
		Vec margin = Vec(0, 0);
		maxHandlePos = Vec(0, 0).plus(margin);
		minHandlePos = Vec(0, 33).plus(margin);
		background->svg = SVG::load(assetGlobal("plugins/mschack/res/mschack_sliderBG_01.svg"));
		background->wrap();
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
		handle->svg = SVG::load(assetGlobal("plugins/mschack/res/mschack_sliderKNOB_01.svg"));
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
		background->svg = SVG::load(assetGlobal("plugins/mschack/res/mschack_PortIn_small.svg"));
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
		background->svg = SVG::load(assetGlobal("plugins/mschack/res/mschack_PortOut_small.svg"));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_KnobRed1.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_KnobBlue3.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_KnobYellow3.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_KnobPurp1.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Green1_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Green1_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Blue1_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Blue2_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Blue2_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Blue2_big.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Blue2_big.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Yellow1_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Yellow1_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Yellow2_small.svg")));
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
		setSVG(SVG::load(assetGlobal("plugins/mschack/res/mschack_Yellow2_small.svg")));
		box.size = Vec(40, 40);
	}
};