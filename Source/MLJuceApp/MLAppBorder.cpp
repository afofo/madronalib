
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppBorder.h"

MLAppBorder::MLAppBorder() : 
	pMainView(0),
    mpResizer(0),
	mZoomable(false)
{
	Component::setBounds(0, 0, 0, 0);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	LookAndFeel::setDefaultLookAndFeel (myLookAndFeel);	
	Component::setName("border");
    Component::setOpaque(true);
}

MLAppBorder::~MLAppBorder()
{
	deleteAllChildren();
}

void MLAppBorder::addMainView(MLAppView* pView)
{
	pMainView = pView;
	addAndMakeVisible(pView);
}

// build the resizer for target components that need them.
// Native Mac windows don't need resizers.
//
void MLAppBorder::makeResizer(Component* targetComp)
{
	// add the triangular resizer component for the bottom-right of the UI
    addAndMakeVisible (mpResizer = new ResizableCornerComponent (targetComp, &myConstrainer));
	mpResizer->setAlwaysOnTop(true);
}

void MLAppBorder::paint (Graphics& g)
{    
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	
    // TODO optimize, because this has to be redrawn behind every non-opaque component repaint.
	// This is where most of the plugin's background is actually painted.
	myLookAndFeel->drawBackground(g, this);
    
	/*
	// TEST outline border areas
	Path p, p2;
	int w = getWidth();
	int h = getHeight();
	p.addRectangle(0, 0, w, h);
	
	if (pMainView)
	{
		p2.addRectangle(pMainView->getBounds());
	}
	else
	{
		// draw X
		p2.startNewSubPath(0, 0);
		p2.lineTo(w, h);
		p2.startNewSubPath(w, 0);
		p2.lineTo(0, h);
	}
	g.setColour(Colours::blue);
	g.strokePath(p, PathStrokeType(1.f));
	g.setColour(Colours::red);
	g.strokePath(p2, PathStrokeType(1.f));
	*/
}

void MLAppBorder::centerMainViewInWindow()
{
	Rectangle<int> br = getBounds();
	int windowWidth = br.getWidth();
	int windowHeight = br.getHeight();	
	double windowRatio = (double)(windowWidth + 1)/(double)windowHeight;
	double viewRatio = (double)mGridUnitsX/(double)mGridUnitsY;
	int u = (int)(windowHeight / mGridUnitsY);
	int viewWidth, viewHeight;
	if ((!windowWidth) || (!windowHeight) || !u) return;
	
	// TODO different modes: fit fixed scale, quantize only. 
	// This is fit fixed scale, good for static layouts.
	if(windowRatio > viewRatio)
	{
		// too wide
		viewHeight = windowHeight;
		viewWidth = floor((double)windowHeight*viewRatio);
	}
	else
	{
		// too tall
		viewWidth = windowWidth;
		viewHeight = floor((double)windowWidth/viewRatio);
		u = viewWidth / mGridUnitsX;
	}
    
    // debug() << "MLAppBorder:: RESIZED to " << viewWidth << ", " << viewHeight << "\n";
	
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGridUnitSize(u);	

	if ((!viewWidth) || (!viewHeight) || !u) return;
	int vwq = viewWidth / u * u;
	int vhq = viewHeight / u * u;
	int borderX = (windowWidth - vwq)/2;
	int borderY = 0;//(windowHeight - vhq)/2;
	
	if (pMainView) pMainView->resizeWidget(MLRect(borderX, borderY, vwq, vhq), u);
}

void MLAppBorder::resized()
{
	centerMainViewInWindow();
	
	// move resizer widget
	if(mpResizer)
	{
		int w = getWidth();
		int h = getHeight();
		mpResizer->setBounds (w - 16, h - 16, 16, 16);
	}
}

void MLAppBorder::setGridUnits(double gx, double gy)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGridUnits(gx, gy);	
	myConstrainer.setFixedAspectRatio(gx/gy);	
	mGridUnitsX = gx;
	mGridUnitsY = gy;
}

void MLAppBorder::setContent(MLAppView* contentView)
{
	// set content of border to view
	addMainView(contentView);
}

void MLAppBorder::setZoomable(bool z)
{ 
	mZoomable = z;
	myConstrainer.setZoomable(z); 
}

