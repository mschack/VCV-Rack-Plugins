#include "mscHack.hpp"
#include "mscHack_Controls.hpp"

Plugin *plugin;

void init(rack::Plugin *p)
{
	plugin = p;
	plugin->slug = "mscHack";
	plugin->website = "https://github.com/mschack/VCV-Rack-Plugins";

	p->addModel(modelMasterClockx4_Widget);
	p->addModel(modelSeq_3x16x16_Widget);
	p->addModel(modelSEQ_6x32x16_Widget);
	p->addModel(modelSeq_Triad_Widget);
	p->addModel(modelSeq_Triad2_Widget);
	p->addModel(modelARP700_Widget);
	p->addModel(modelSynthDrums_Widget);
	p->addModel(modelXFade_Widget);
	p->addModel(modelMix_1x4_Stereo_Widget);
	p->addModel(modelMix_2x4_Stereo_Widget);
	p->addModel(modelMix_4x4_Stereo_Widget);
	p->addModel(modelMix_4x4_Stereo_Widget_old);
	p->addModel(modelPingPong_Widget);
	p->addModel(modelOsc_3Ch_Widget);
	p->addModel(modelCompressor_Widget);
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
