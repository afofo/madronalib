
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_TEXT_BUTTON_HEADER__
#define __ML_TEXT_BUTTON_HEADER__


#include "MLUI.h"
#include "MLButton.h"

class MLTextButton : 
	public MLButton
{
public:
     MLTextButton (const String& buttonName = String::empty,
                const String& toolTip = String::empty);

    ~MLTextButton();

    enum ColourIds
    {
        buttonColourId                  = 0x1000100,
		textColourId					= 0x1000102, 
	};

protected:
	void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown);
    void colourChanged();

private:

};


#endif   // __ML_TEXT_BUTTON_HEADER__
