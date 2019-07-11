#pragma once

#define QITEMS 256

typedef struct
{
    Vec item[ QITEMS ];

    unsigned char index;
}LINE_Q;

//-----------------------------------------------------
// Widget_Image
//-----------------------------------------------------
struct Widget_LineImage : TransparentWidget
{
    bool            m_bInitialized = false;

    LINE_Q          m_Q;

    RGB_STRUCT      m_BGCol = {};

    //-----------------------------------------------------
    // Procedure:   constructor
    //-----------------------------------------------------
    Widget_LineImage( float x, float y, float w, float h )
    {
        box.pos = Vec( x, y );
        box.size = Vec( w, h );

        memset( &m_Q, 0, sizeof( m_Q ) );

        m_bInitialized = true;
    }

    //-----------------------------------------------------
    // Procedure:   SetBGCol
    //-----------------------------------------------------
    void SetBGCol( int bg )
    {
        m_BGCol.dwCol = bg;
    }

    //-----------------------------------------------------
    // Procedure:   addQ
    //-----------------------------------------------------
    bool addQ( float x, float y )
    {
        if( !m_bInitialized )
            return false;

        m_Q.item[ m_Q.index ].x = x;
        m_Q.item[ m_Q.index++ ].y = y;

        return true;
    }

    //-----------------------------------------------------
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(const DrawArgs &args) override
    {
        Vec *pv;//, *last, *next, dl, dn;
        unsigned char index;

        //nvgBezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);

        if( !m_bInitialized )
            return;

        nvgScissor( args.vg, 0, 0, box.size.x, box.size.y );
        //nvgShapeAntiAlias( args.vg, 1);

        nvgBeginPath( args.vg );
        nvgRect( args.vg, 0, 0, box.size.x - 1, box.size.y - 1 );
        nvgFillColor( args.vg, nvgRGBA( 0, 0, 0, 0x80 ) );
        nvgFill( args.vg );


        nvgStrokeWidth( args.vg, 2.5f );
        nvgStrokeColor( args.vg, nvgRGBA( 0, 255, 255, 250 ) );

        index = m_Q.index + 1;

        pv =&m_Q.item[ index++ ];
        //last = pv;

        nvgLineJoin( args.vg, NVG_ROUND );

        nvgBeginPath( args.vg );
        nvgMoveTo( args.vg, pv->x, pv->y );

        //pv =&m_Q.item[ index++ ];

        for( int i = 0; i < 255; i++ )
        {
            pv =&m_Q.item[ index++ ];
            nvgLineTo( args.vg, pv->x, pv->y );

           /* dl.x = (pv->x - last->x) * 0.5f;
            dl.y = (pv->y - last->y) * 0.5f;
            dn.x = (pv->x - next->x) * 0.5f;
            dn.y = (pv->y - next->y) * 0.5f;

            if( last->x > pv->x || last->y > pv->y )
                nvgBezierTo( args.vg, pv->x + dn.x, pv->y + dn.y, pv->x + dl.x, pv->y + dl.y, pv->x, pv->y );
            else
                nvgBezierTo( args.vg, pv->x + dl.x, pv->y + dl.y, pv->x + dn.x, pv->y + dn.y, pv->x, pv->y );

            //nvgBezierTo( args.vg, pv->x + dn.x, pv->y + dn.y, pv->x + dl.x, pv->y + dl.y, pv->x, pv->y );
            //nvgBezierTo( args.vg, pv->y, pv->x, pv->y, pv->x, pv->y, pv->y );l
            last = pv;
            pv = next;*/
            
        }

        nvgStroke( args.vg );
    }
};