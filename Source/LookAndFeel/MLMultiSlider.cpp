
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMultiSlider.h"
#include "MLLookAndFeel.h"

MLMultiSlider::MLMultiSlider () :
	mpListener(0),
	mVertical(true)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

	setNumSliders(1);
	setRange(0., 1., 0.01);
	mSliderUnderMouse = -1;
	mCurrDragSlider = -1;
	mCurrDragValue = 0.f;
	
	mDoRollover = 0;
}

MLMultiSlider::~MLMultiSlider()
{
	deleteAllChildren();
}

/*
void MLMultiSlider::setAttribute(MLSymbol attr, float val)
{
	static const MLSymbol valueSym("value");
	
	// debug() << "MLMultiSlider " << getWidgetName() << ":" << attr << " = " << val << "\n";
	
	// TODO other dial attributes?
	
	if (attr.withoutFinalNumber() == valueSym)
	{
		int dialIdx = attr.getFinalNumber();
		if(within(dialIdx, 0, (int)mSliderValues.size()))
		{
			mSliderValues[dialIdx] = val;			
            triggerChangeMessage (false);
			repaint();
		}
	}
}
*/

void MLMultiSlider::setNumSliders(int n)
{
	if (n > 0)
	{
		mSliderValues.resize(n);
		mValueNeedsUpdate.resize(n);
		for(int i=0; i<n; ++i)
		{
			mSliderValues[i] = 0.;
			mValueNeedsUpdate[i] = false;
		}
		resized();
	}
}

unsigned MLMultiSlider::getNumSliders()
{
	return mSliderValues.size();
}


void MLMultiSlider::setRange(float a, float b, float c)
{
	mRange.set(a, b);
	mInterval = c;
	mZeroThreshold = a;
}

// the colors for different MLDial parts are generated algorithmically.
void MLMultiSlider::setFillColor (const Colour& c)
{
	float g = c.getFloatGreen();
	float b = (1.f - g);	

	// thumb fill
	setColour(fillColor, c);
	// bright line
	setColour(indicatorColor, Colour(c.getHue(), jmax(c.getSaturation() - (b*0.05), 0.), jmin((c.getBrightness() + b*2.f), 1.f), 1.f));
	// dial fill selected 
	setColour(trackFullLightColor, findColour(fillColor).overlaidWith(findColour(indicatorColor).withAlpha(0.15f)));
	// track fill 
	setColour(trackFullDarkColor, c.overlaidWith(Colours::black.withAlpha(0.15f)).withMultipliedSaturation(1.2f));
	// track background plain
	setColour(trackEmptyDarkColor, findColour(MLLookAndFeel::darkerFillColor));

    lookAndFeelChanged();
}

#pragma mark -

const MLRect MLMultiSlider::getActiveRect() const
{
	int w = getWidth() - kMLShadowThickness*2;
	int h = getHeight() - kMLShadowThickness*2;
	const unsigned dials = mSliderValues.size();
	w = getSliderWidth() * dials;
	int x = kMLShadowThickness;
	int y = kMLShadowThickness;
	return MLRect(x, y, w, h);
}
	
unsigned MLMultiSlider::getSliderWidth() const
{
	int w = getWidth() - 16;
	const unsigned dials = mSliderValues.size();
	const unsigned sw = dials ? (w / dials) : 1;
	return sw;
}
	
#pragma mark -

void MLMultiSlider::paint (Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	if (isOpaque()) myLookAndFeel->drawBackground(g, this);	
	float outlineThickness = myLookAndFeel->getGridUnitSize() / 64.f;
	MLRect r = mPos.getLocalOutline();
	
	int dials = mSliderValues.size();

	const Colour outlineColor (findColour(MLLookAndFeel::outlineColor).withAlpha (isEnabled() ? 1.f : 0.5f));		
	// const Colour shadowColor (findColour(MLLookAndFeel::shadowColor).withAlpha (isEnabled() ? 1.f : 0.5f));						
	
	/*
 	// draw outline shadow
	for (int i=1; i<kMLShadowThickness; i++)
	{
		Path sp;
		MLRect outRect = r;
		outRect.expand(i);
		sp.addRectangle(MLToJuceRect(outRect));
		float d = (float)(kMLShadowThickness - i) / kMLShadowThickness; // 0. - 1.
		float opacity = d * d * kMLShadowOpacity;
		g.setColour (Colours::red);//(shadowColor.withAlpha(opacity));
		g.strokePath (sp, PathStrokeType (1.f));	
	}
	*/
	
	// draw fills
	// vertical only
	Path full, empty;
	Colour fullColor, emptyColor;
	MLRect fullRect, emptyRect;
	float dialY;
	
	MLRange drawRange(mRange);
	drawRange.convertTo(MLRange(r.height(), 0.));
	
	for (int i=0; i<dials; ++i)
	{
		MLRect sr = (mPos.getElementBounds(i));

		dialY = drawRange(mSliderValues[i]);
		fullRect = sr;
		emptyRect = sr;		
		fullRect.setTop(dialY);
	
		fullColor = findColour(trackFullDarkColor);
		emptyColor = findColour(trackEmptyDarkColor);
		
		// groups of 4 
		if (!(i&4))
		{
			emptyColor = emptyColor.brighter(0.1f);
			fullColor = fullColor.brighter(0.15f);
		}
		
		empty.clear();
		empty.addRectangle(MLToJuceRect(emptyRect));
		g.setColour(emptyColor);
		g.fillPath(empty);	
		
		full.clear();
		full.addRectangle(MLToJuceRect(fullRect));
		g.setColour(fullColor);
		g.fillPath(full);	
				
		g.setColour(outlineColor);
		g.strokePath(empty, PathStrokeType (outlineThickness));
		
	}
	
}

#pragma mark -

void MLMultiSlider::mouseDown (const MouseEvent& e)
{
    if (!isEnabled()) return;
	mMousePos = Vec2(e.x, e.y);
	mCurrDragSlider = getSliderUnderPoint(mMousePos);
	mouseDrag(e);
}

void MLMultiSlider::mouseUp (const MouseEvent&)
{
	if (!isEnabled()) return;
	mCurrDragSlider = -1;
}

void MLMultiSlider::modifierKeysChanged (const ModifierKeys& )
{
}

void MLMultiSlider::mouseMove (const MouseEvent& e)
{
 	if (!mDoRollover) return;
	if (!isEnabled()) return;
	mMousePos = Vec2(e.x, e.y);
	int s = getSliderUnderPoint(mMousePos);
	if (s != mSliderUnderMouse)
	{
		mSliderUnderMouse = s;
	}
	repaint();
}

void MLMultiSlider::mouseExit (const MouseEvent& )
{
	mSliderUnderMouse = -1;
	repaint();
}

void MLMultiSlider::mouseDrag(const MouseEvent& e)
{
	MLRect r = mPos.getLocalOutline();	
	float w = r.width();
	float h = r.height();
	
	int mx = clamp(e.x, (int)r.left() + 1, (int)(r.left() + w));
	int my = clamp(e.y, (int)r.top() + 1, (int)(r.top() + h));
	int dials = mSliderValues.size();
	int s = getSliderUnderPoint(Vec2(mx, my));
	
    if (isEnabled())
    {	
		if (within(s, 0, dials))
		{
			const int mousePos = mVertical ? my : mx;
			float val;
			MLRange posRange;
			if (mVertical)
			{
				posRange.set(h, 1);
			}
			else
			{
				posRange.set(1, w);
			}
			posRange.convertTo(mRange);
			val = posRange(mousePos);
			
			// if dial changed in drag, interpolate, setting dials in between
			if((mCurrDragSlider >= 0) && (mCurrDragSlider != s)) 			
			{
				int span = (s - mCurrDragSlider);
				int dir = sign(span);
				float mix, mixedval;
				int startDrag = mCurrDragSlider + dir;
				int endDrag = s + dir;
				for(int i=startDrag; i != endDrag; i += dir)
				{
					mix = fabs(float(i - startDrag)/float(span));
					mixedval = lerp(mCurrDragValue, val, mix);
	
					// finish old drag and switch to dragging new dial
					if (i != mCurrDragSlider)
					{
						mCurrDragSlider = i;
					}
					setSelectedValue(snapValue (mixedval, false), i, true, true);
				}
			}
			else if (mCurrDragSlider == s) // set current drag dial
			{
				setSelectedValue(snapValue (val, false), s, true, true);
			}
			
			if (s != mSliderUnderMouse)
			{
				mSliderUnderMouse = s;
				repaint();
			}
			mCurrDragSlider = s;
			mCurrDragValue = val;
		}	
    }
}

//--------------------------------------------------------------------------------
#pragma mark -

// all value changes should pass through here.
// 
float MLMultiSlider::constrainedValue (float value) const throw()
{
	float rmin = mRange.getA();
	float rmax = mRange.getB();
	if (rmin > rmax)
	{
		float temp = rmax;
		rmax = rmin;
		rmin = temp;
	}

	// quantize to chunks of interval
    if (mInterval > 0)
        value = rmin + mInterval * floor((value - rmin)/mInterval + 0.5f);
	
	value = clamp(value, rmin, rmax);
	if (value <= mZeroThreshold)
	{
		value = 0.f;
	}

    return value;
}

float MLMultiSlider::snapValue (float attemptedValue, const bool)
{
	float r = 0.;
	if (attemptedValue != attemptedValue)
	{
		MLError() << "dial " << getName() << ": not a number!\n";
	}
	else 
	{
		float rmin = mRange.getA();
		float rmax = mRange.getB();
		r = clamp(attemptedValue, rmin, rmax);
	}

	return r;
}

float MLMultiSlider::valueToProportionOfLength(float v)
{
	mRange.convertTo(UnityRange);
	return(mRange(v));
} 

float MLMultiSlider::proportionOfLengthToValue(float l)
{
	MLRange u(UnityRange);
	u.convertTo(mRange);
	return u(l);
} 

void MLMultiSlider::mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel)
{
	float wheelSpeed = 0.15f;
    if(wheel.isReversed)
        wheelSpeed = -wheelSpeed;
	
	if(mCurrDragSlider >= 0) return;
	
	// filter out zero motions from trackpad
	if ((wheel.deltaX == 0.) && (wheel.deltaY == 0.)) return;

    if (isEnabled())
	{
		//int s = getSliderUnderMouse();		
		int s = getSliderUnderPoint(Vec2(event.x, event.y));
		if ((s >= 0) && ! isMouseButtonDownAnywhere())
		{
			float val, newValue;
			val = mSliderValues[s];
			{
				float proportionDelta = (wheel.deltaX != 0 ? -wheel.deltaX : wheel.deltaY) * wheelSpeed; 
				const float currentPos = valueToProportionOfLength (val);
				const float n = proportionOfLengthToValue (clamp (currentPos + proportionDelta, 0.f, 1.f));
				float diff = fabs(n - val);
				float delta = (n != val) ? max (diff, mInterval) : 0.f;
				if (val > n)
					delta = -delta;
 				newValue = val + delta;
//printf ("X:%f, Y:%f, %f, %f, %f \n", wheel.deltaX, wheel.deltaY, proportionDelta, currentPos, newValue);
//printf("delta: %f \n", delta);
			}
			mCurrDragSlider = s;
			setSelectedValue(snapValue (newValue, false), s, true, true);
			mCurrDragSlider = -1;
        }
    }
    else
    {
        Component::mouseWheelMove (event, wheel);
    }
}


int MLMultiSlider::getSliderUnderPoint(const Vec2& p)
{
	return mPos.getElementUnderPoint(p);
}

int MLMultiSlider::getSliderUnderMouse()
{
	int r = -1;	
	r = getSliderUnderPoint(mMousePos);
	return r;
} 


void MLMultiSlider::setSelectedValue (float newValue, int selector, 
	const bool sendUpdateMessage,
	const bool sendMessageSynchronously)
{
	newValue = constrainedValue (newValue);
    if (mSliderValues[selector] != newValue)
    {
		mSliderValues[selector] = newValue;
		mValueNeedsUpdate[selector] = true;
		repaint();
        if (sendUpdateMessage)
		{
            triggerChangeMessage (sendMessageSynchronously);
		}
    }
}

float MLMultiSlider::getValue(unsigned index)
{
	float r = 0.;
	if (index < mSliderValues.size())
	{
		r = mSliderValues[index];
	}
	return r;
}

void MLMultiSlider::setWave(unsigned w)
{
	unsigned dials = mSliderValues.size();
	float val = 0.;
	
	MLRange vRange;
	vRange.convertTo(mRange);
	for (unsigned i=0; i<dials; ++i)
	{
		switch(w)
		{
			case 1: // square
				val = i <dials/2 ? 0 : 1;
			break;
			case 2: // sine
				val = sin(i * kMLTwoPi / dials)/-2. + 0.5;
			break;
			case 3: // saw
				val = (float)i / (float)(dials - 1);
			break;
			case 4: // random
				val = MLRand();
			break;
			default:
				val = 0.5;
			break;
		}	
		val = vRange(val);
		setSelectedValue(val, i, true, true);
	}
}

void MLMultiSlider::setListener (MLMultiSlider::Listener* const listener) throw()
{
    jassert (listener != 0);
    if (listener != 0)
        mpListener = listener;
}

//--------------------------------------------------------------------------------
// AsyncUpdater related methods

void MLMultiSlider::triggerChangeMessage (const bool synchronous)
{
    if (synchronous)
        handleAsyncUpdate();
    else
        triggerAsyncUpdate();
}
 
void MLMultiSlider::handleAsyncUpdate()
{
	if(!mpListener) return;
    cancelPendingUpdate();
	int dials = mSliderValues.size();

	for(int j=0; j<dials; ++j)
	{
		if (mValueNeedsUpdate[j])
		{
			mpListener->multiSliderValueChanged(this, j);
			mValueNeedsUpdate[j] = false;
		}
    }
}

//--------------------------------------------------------------------------------

#pragma mark -

void MLMultiSlider::resizeWidget(const MLRect& b, const int )
{
//	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();

	Component* pC = getComponent();
	mPos.setBounds(b);
	pC->setBounds(MLToJuceRectInt(mPos.getBounds()));

	int s = mSliderValues.size();	
	mPos.setElements(s);
	mPos.setGeometry(MLPositioner::kHorizontal); 
	mPos.setSizeFlags(0);//(MLPositioner::kOnePixelOverlap); 
	mPos.setMargin(0.);	
	Vec2 panelSize = mPos.getElementSize();
	
	/*
	// setup ImageBank
	mImageBank.setImages(kMLStepDisplayImages);
	mImageBank.setDims(panelSize[0], panelSize[1]);
	mImageBank.clearPanels();
	for(int i = 0; i < panels; ++i)
	{
		mImageBank.addPanel(myPos.getElementPosition(i));
	}
	*/
	
	/*
	// colors
	Colour stepOnColor (findColour (MLStepDisplay::stepOnColourId));	
	Colour stepOffColor (findColour (MLStepDisplay::stepOffColourId));	
	Colour outlineOnColor = findColour(MLLookAndFeel::outlineColor).overlaidWith(stepOnColor.withAlpha(0.5f));
	Colour outlineOffColor = findColour(MLLookAndFeel::outlineColor);
	Colour stepColor, outlineColor;
	
	// draw images to ImageBank
	for (int i=0; i<kMLStepDisplayImages; ++i)
	{
		Image& img = mImageBank.getImage(i);
		Graphics g(img);	
		
		float val = (float)i / (float)(kMLStepDisplayImages-1);
		stepColor = stepOffColor.overlaidWith(stepOnColor.withAlpha(val));
		outlineColor = outlineOffColor.overlaidWith(outlineOnColor.withAlpha(val));

		const Colour onAlphaColor = stepOnColor.withMultipliedAlpha(val);
		const Colour blinkerColor = stepOffColor.overlaidWith(onAlphaColor);
		const Colour myOutlineColor = outlineOffColor.overlaidWith(outlineOnColor.withMultipliedAlpha(val));

		const float outlineThickness = 0.75f;
		myLookAndFeel->drawMLButtonShape (g, 0, 0, panelSize[0], panelSize[1],
			r, blinkerColor, myOutlineColor, outlineThickness, eMLAdornNone, 0., 0.);	
			
	}
	*/
	
	// Component::resized();
}

