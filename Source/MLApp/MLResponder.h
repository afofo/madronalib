
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_RESPONDER_H
#define __ML_RESPONDER_H

//#include "MLModel.h"
#include "MLWidget.h"
#include "MLProperty.h"
// include all widgets that report values ( make allwidgets header file? )
// No, TODO finish rewrite so that widgets know about Responder, send property changes.
//#include "MLButton.h"
//#include "MLMenuButton.h"
//#include "MLMultiButton.h"
// #include "MLDial.h"
//#include "MLMultiSlider.h"
//#include "MLMenu.h"

// --------------------------------------------------------------------------------
#pragma mark MLResponder

// Responders listen to Widgets and perform actions. Typically they ask Models to do things.

class MLResponder :
	public MLWidget::Listener
//	public MLButton::Listener,
//	public MLMenuButton::Listener,
//	public MLMultiButton::Listener
//	public MLDial::Listener,
//	public MLMultiSlider::Listener
{
public:
	MLResponder() {}
    virtual ~MLResponder() {}
	
	

/*	// from MLButton::Listener
    virtual void buttonClicked (MLButton*) = 0;
	
	// from MLMenuButton::Listener
 	virtual void showMenu(MLSymbol menuName, MLSymbol instigatorName) = 0;
	virtual void menuItemChosen(MLSymbol menuName, int result) = 0;
*/
	/*
 	// from MLDial::Listener
	 virtual void dialValueChanged (MLDial*) = 0;
	 */	
//	virtual void dialDragStarted (MLDial*) = 0;
//	virtual void dialDragEnded (MLDial*) = 0;
	 
	// from MLMultiSlider::Listener
//    virtual void multiSliderValueChanged (MLMultiSlider* , int ) = 0;

	// from MLMultiButton::Listener
//    virtual void multiButtonValueChanged (MLMultiButton* , int ) = 0;
};

#endif // __ML_RESPONDER_H