#define nCHANNELS 8
#define nINCHANNELS 4
#define nGROUPS   0

#define Mixer_ Mixer_4_0_4
#define Mixer_Widget_ Mixer_4_0_4_Widget
#define Mixer_Svg "res/Mixer_4_0_4.svg"

#define _Button_ChSolo      _Button_ChSolo_4_0_4
#define _Button_ChMute      _Button_ChMute_4_0_4
#define _Button_ChPreFader  _Button_ChPreFader_4_0_4
#define _RouteCallback      _RouteCallback_4_0_4
#define _BrowserClass       _BrowserClass_4_0_4


#define _GroupPreMute       _GroupPreMute_4_0_4
#define _Gainx2             _Gainx2_4_0_4
#define _AuxIgnoreSolo      _AuxIgnoreSolo_4_0_4

#include "Mixer_x_x_x.hpp"

Model *modelMix_4_0_4 = createModel<Mixer_4_0_4, Mixer_4_0_4_Widget>( "Mix_4_0_4" );