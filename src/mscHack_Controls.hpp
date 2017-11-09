#include "rack.hpp"

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