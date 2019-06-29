#define nCHANNELS 32
#define nINCHANNELS 24
#define nGROUPS   4

#define Mixer_ Mixer_24_4_4
#define Mixer_Widget_ Mixer_24_4_4_Widget
#define Mixer_Svg "res/Mixer_24_4_4.svg"

#define _Button_ChSolo      _Button_ChSolo_24_4_4
#define _Button_ChMute      _Button_ChMute_24_4_4
#define _Button_ChPreFader  _Button_ChPreFader_24_4_4
#define _RouteCallback      _RouteCallback_24_4_4
#define _BrowserClass       _BrowserClass_24_4_4

#define _GroupPreMute       _GroupPreMute_24_4_4
#define _Gainx2             _Gainx2_24_4_4
#define _AuxIgnoreSolo      _AuxIgnoreSolo_24_4_4

#include "Mixer_x_x_x.hpp"

Model *modelMix_24_4_4 = createModel<Mixer_24_4_4, Mixer_24_4_4_Widget>( "Mix_24_4_4" );
