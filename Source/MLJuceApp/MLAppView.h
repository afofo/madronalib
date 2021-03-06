
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_APP_VIEW_H__
#define __ML_APP_VIEW_H__

#include "JuceHeader.h"
#include "MLUIBinaryData.h"
#include "MLResponder.h"
#include "MLReporter.h"
#include "MLWidgetContainer.h"
//#include "MLDial.h"
#include "MLButton.h"
#include "MLPanel.h"
#include "MLDrawing.h"
#include "MLDrawableButton.h"
#include "MLTextButton.h"
#include "MLMenuButton.h"
#include "MLToggleButton.h"
#include "MLTriToggleButton.h"
#include "MLMultiButton.h"
#include "MLMultiSlider.h"
#include "MLLookAndFeel.h"
#include "MLEnvelope.h"
#include "MLWaveform.h"
#include "MLProgressBar.h"
#include "MLGraph.h"
#include "MLDebugDisplay.h"
#include "MLDefaultFileLocations.h"
#include "MLJuceFilesMac.h"
#include "MLVector.h"
#include "MLSymbol.h"

extern const Colour defaultColor;

// maintains a View Component with grid dimensions set by parent. 
// provides handy UI component creation functions for grid.
// 
class MLAppView : 
	public Component,
	public MLWidgetContainer
{
public:
	MLAppView(MLResponder* pResp, MLReporter* pRep);
    ~MLAppView();

	virtual bool isWidgetContainer(void) { return true; }

	// setup view for param p as attr of widget.
	void addParamView(MLSymbol p, MLWidget* w, MLSymbol attr);
	
	void addWidgetToView(MLWidget* pW, const MLRect& r, MLSymbol name);
	void addSignalView(MLSymbol p, MLWidget* w, MLSymbol attr, int size = kMLSignalViewBufferSize);	

	virtual MLDial* addDial(const char * displayName, const MLRect & r, const MLSymbol paramName, 
		const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);	
	virtual MLMultiSlider* addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol paramName, 
		int n, const Colour& color);
	virtual MLMultiButton* addMultiButton(const char * displayName, const MLRect & r, const MLSymbol paramName, 
		int n, const Colour& color);
	virtual MLButton* addToggleButton(const char* displayName, const MLRect & r, const MLSymbol name,
                                      const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);
	virtual MLButton* addTriToggleButton(const char* displayName, const MLRect & r, const MLSymbol name,
                                      const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);

	MLPanel* addPanel(const MLRect & r, const Colour& color = defaultColor);
	MLDebugDisplay* addDebugDisplay(const MLRect & r);
		
	MLDrawableButton* addRawImageButton(const MLRect & r, const char * name, 
		const Colour& color, const Drawable* normal);
		
	MLTextButton* addTextButton(const char * displayName, const MLRect & r, const char * name,
		const Colour& color = defaultColor);
		
	MLMenuButton* addMenuButton(const char * displayName, const MLRect & r, const char * name,
		const Colour& color = defaultColor);
		
	MLLabel* addLabel(const char* displayName, const MLRect & r, 
		const float sizeMultiplier = 1.0f, const int font = eMLCaption);
		
	MLLabel* addLabelAbove(MLWidget* c, const char* displayName, 
		const float sizeMultiplier = 1.0f, const int font = eMLCaption, Vec2 offset = Vec2());

	MLGraph* addGraph(const char * name, const Colour& color);

	MLDrawing* addDrawing(const MLRect & r);
	MLProgressBar* addProgressBar(const MLRect & r);
	MLWaveform* addWaveform(const MLRect & r, const MLSymbol paramName);

	// animations
	void setAnimationsActive(bool animState);

	void resized();
	void setPeerBounds(int x, int y, int w, int h);
	
	inline MLResponder* getResponder() { return mpResponder; }
	inline MLReporter* getReporter() { return mpReporter; }

protected:
	float mGridUnitSize;	
	bool mDoAnimations;	
	MLResponder* mpResponder;
	MLReporter* mpReporter;

private:
	


};

#endif // __ML_APP_VIEW_H__