#define nCHANNELS 24
#define nINCHANNELS 16
#define nGROUPS   4

#define Mixer_ Mixer_16_4_4
#define Mixer_Widget_ Mixer_16_4_4_Widget
#define Mixer_Svg "res/Mixer_16_4_4.svg"

#define _Button_ChSolo      _Button_ChSolo_16_4_4
#define _Button_ChMute      _Button_ChMute_16_4_4
#define _Button_ChPreFader  _Button_ChPreFader_16_4_4
#define _RouteCallback      _RouteCallback_16_4_4
#define _BrowserClass       _BrowserClass_16_4_4

#define _GroupPreMute       _GroupPreMute_16_4_4
#define _Gainx2             _Gainx2_16_4_4
#define _AuxIgnoreSolo     _AuxIgnoreSolo_16_4_4

#include "Mixer_x_x_x.hpp"

Model *modelMix_16_4_4 = createModel<Mixer_16_4_4, Mixer_16_4_4_Widget>( "Mix_16_4_4" );
