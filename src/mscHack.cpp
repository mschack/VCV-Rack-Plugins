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
    p->addModel( modelMix_9_3_4 );
    p->addModel( modelMix_16_4_4 );
    p->addModel( modelMix_24_4_4 );
    p->addModel( modelASAF8 );
    p->addModel( modelStepDelay );
    p->addModel( modelPingPong );
    p->addModel( modelOsc_3Ch );
    p->addModel( modelOSC_WaveMorph_3 );
    p->addModel( modelDronez );
    p->addModel( modelMorze );
    p->addModel( modelWindz );
    p->addModel( modelAlienz );
    p->addModel( modelCompressor );
}

//-----------------------------------------------------
// Procedure: random
//
//-----------------------------------------------------
#define PHI 0x9e3779b9
unsigned int Q[4096], c = 362436;
unsigned int g_myrindex = 4095;

void init_rand( unsigned int seed )
{
    int i;

    Q[0] = seed;
    Q[1] = seed + PHI;
    Q[2] = seed + PHI + PHI;

    for (i = 3; i < 4096; i++)
            Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i;

    c = 362436;
    g_myrindex = 4095;
}

unsigned short srand(void)
{
	long long t, a = 18782LL;
    unsigned int x, r = 0xfffffffe;

    g_myrindex = (g_myrindex + 1) & 4095;
    t = a * Q[g_myrindex] + c;
    c = (t >> 32);
    x = t + c;

    if (x < c)
    {
        x++;
        c++;
    }

    return (unsigned short)( ( Q[g_myrindex] = r - x ) & 0xFFFF );
}

float frand(void)
{
	return (float)srand() / (float)0xFFFF;
}

float frand_mm( float fmin, float fmax )
{
	float range = fmax - fmin;

	return fmin + ( frand() * range );
}

bool brand( void )
{
	return ( srand() & 1 );
}

bool frand_perc( float perc )
{
	return ( frand() <= (perc / 100.0f) );
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

//-----------------------------------------------------
// Procedure: JsonDataString
//
//-----------------------------------------------------
void JsonDataString( bool bTo, std::string strName, json_t *root, std::string *strText )
{
    json_t *textJ;

    if( !root )
        return;

    if( bTo )
    {
    	json_object_set_new( root, strName.c_str(), json_string( strText->c_str() ) );
    }
    else
    {
    	textJ = json_object_get( root, strName.c_str() );

		if( textJ )
			*strText = json_string_value( textJ );
    }
}
