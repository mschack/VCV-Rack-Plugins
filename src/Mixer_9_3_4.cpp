#define nCHANNELS 16
#define nINCHANNELS 9
#define nGROUPS   3

#define Mixer_ Mixer_9_3_4
#define Mixer_Widget_ Mixer_9_3_4_Widget
#define Mixer_Svg "res/Mixer_9_3_4.svg"

#define _Button_ChSolo      _Button_ChSolo_9_3_4
#define _Button_ChMute      _Button_ChMute_9_3_4
#define _Button_ChPreFader  _Button_ChPreFader_9_3_4
#define _RouteCallback      _RouteCallback_9_3_4
#define _BrowserClass       _BrowserClass_9_3_4

#define _GroupPreMute       _GroupPreMute_9_3_4
#define _Gainx2             _Gainx2_9_3_4
#define _AuxIgnoreSolo      _AuxIgnoreSolo_9_3_4

#include "Mixer_x_x_x.hpp"

Model *modelMix_9_3_4 = createModel<Mixer_9_3_4, Mixer_9_3_4_Widget>( "Mix_9_3_4" );
