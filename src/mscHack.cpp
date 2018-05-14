#include "mscHack.hpp"

Plugin *plugin;

void init(rack::Plugin *p) 
{
	plugin = p;

	plugin->website = "https://github.com/mschack/VCV-Rack-Plugins";
	p->slug = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

    p->addModel( modelMasterClockx4 );
    p->addModel( modelSeq_3x16x16 );
    p->addModel( modelSEQ_6x32x16 );
    p->addModel( modelSeq_Triad2 );
    p->addModel( modelSEQ_Envelope_8 );
    p->addModel( modelMaude_221 );
    p->addModel( modelARP700 );
    p->addModel( modelSynthDrums );
    p->addModel( modelXFade );
    p->addModel( modelMix_1x4_Stereo );
    p->addModel( modelMix_2x4_Stereo );
    p->addModel( modelMix_4x4_Stereo );
    p->addModel( modelMix_24_4_4 );
    p->addModel( modelStepDelay );
    p->addModel( modelPingPong );
    p->addModel( modelOsc_3Ch );
    p->addModel( modelCompressor );
}

//-----------------------------------------------------
// Procedure: JsonDataInt  
//
//-----------------------------------------------------
void JsonDataInt( bool bTo, std::string strName, json_t *root, int *pdata, int len )
{
    int i;
    json_t *jsarray, *js;

    if( !pdata || !root || len <= 0 )
        return;

    if( bTo )
    {
        jsarray = json_array();

        for ( i = 0; i < len; i++ )
        {
	        js = json_integer( pdata[ i ] );
	        json_array_append_new( jsarray, js );
        }

        json_object_set_new( root, strName.c_str(), jsarray );
    }
    else
    {
        jsarray = json_object_get( root, strName.c_str() );

        if( jsarray )
        {
		    for ( i = 0; i < len; i++)
            {
			    js = json_array_get( jsarray, i );

			    if( js )
				    pdata[ i ] = json_integer_value( js );
		    }
        }
    }
}

//-----------------------------------------------------
// Procedure: JsonDataBool  
//
//-----------------------------------------------------
void JsonDataBool( bool bTo, std::string strName, json_t *root, bool *pdata, int len )
{
    int i;
    json_t *jsarray, *js;

    if( !pdata || !root || len <= 0 )
        return;

    if( bTo )
    {
        jsarray = json_array();

        for ( i = 0; i < len; i++ )
        {
	        js = json_boolean( pdata[ i ] );
	        json_array_append_new( jsarray, js );
        }

        json_object_set_new( root, strName.c_str(), jsarray );
    }
    else
    {
        jsarray = json_object_get( root, strName.c_str() );

        if( jsarray )
        {
		    for ( i = 0; i < len; i++)
            {
			    js = json_array_get( jsarray, i );

			    if( js )
				    pdata[ i ] = json_boolean_value( js );
		    }
        }
    }
}
