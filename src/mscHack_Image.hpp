#pragma once

//-----------------------------------------------------
// Widget_Image
//-----------------------------------------------------
struct Widget_Image : TransparentWidget
{
    bool            m_bInitialized = false;

    unsigned char   *m_pImage = NULL;
    int             m_hImage = 0;
    bool            m_bUpdate = false;
    NVGpaint        m_NvgPaint;
    RGB_STRUCT      m_BGCol = {};

    //-----------------------------------------------------
    // Procedure:   constructor
    //-----------------------------------------------------
    Widget_Image( float x, float y, float w, float h, unsigned char *pimage )
    {
        if( !pimage )
            return;

        m_pImage = pimage;

        box.pos = Vec( x, y );
        box.size = Vec( w, h );

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
    // Procedure:   draw
    //-----------------------------------------------------
    void draw(const DrawArgs &args) override
    {
        if( !m_bInitialized )
            return;

        if( !m_hImage )
        {
            m_hImage = nvgCreateImageRGBA( args.vg, box.size.x, box.size.y, 0, m_pImage );

            m_NvgPaint = nvgImagePattern( args.vg, 0, 0, box.size.x, box.size.y, 0, m_hImage, 4.0f );
        }

        nvgScissor( args.vg, 0, 0, box.size.x, box.size.y );

        // fill with background
        nvgFillColor( args.vg, nvgRGB( m_BGCol.Col[ 2 ], m_BGCol.Col[ 1 ], m_BGCol.Col[ 0 ] ) );
        nvgRect( args.vg, 0, 0, box.size.x, box.size.y );
        nvgFill( args.vg );

        nvgBeginPath( args.vg );

        if( m_bUpdate )
        {
            m_bUpdate = false;
            nvgUpdateImage( args.vg, m_hImage, m_pImage );
        }

        nvgFillPaint( args.vg, m_NvgPaint );
        nvgRect( args.vg, 0, 0, box.size.x, box.size.y );
        nvgFill( args.vg );

        //nvgDeleteImage( vg, m_hImage );
    }
};