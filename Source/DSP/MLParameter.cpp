
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLParameter.h"

// ----------------------------------------------------------------
#pragma mark published parameters

MLPublishedParam::MLPublishedParam(const MLPath & procPath, const MLSymbol name, const MLSymbol alias, int idx) :
	mAlias(alias), mIndex(idx)
{
	setRange(0.f, 1.f, 0.01f, false, 0.f);
	mUnit = kJucePluginParam_Generic;
	mWarpMode = kJucePluginParam_Linear; // TODO use instead
	mValue = 0.f;
	mDefault = 0.f;
	mZeroThreshold = 0.f - (MLParamValue)(2 << 16);
	mGroupIndex = -1;
	addAddress(procPath, name);
}

MLPublishedParam::~MLPublishedParam() 
{

}

void MLPublishedParam::setRange(MLParamValue low, MLParamValue high, MLParamValue v, bool log, MLParamValue zt)
{ 
	mRangeLo = low; 
	mRangeHi = high; 
	mInterval = v; 
	mZeroThreshold = zt;
	
	// TODO take warp mode as arg
	if (log)
	{
		mWarpMode = kJucePluginParam_Exp;
	}
	else
	{
		mWarpMode = kJucePluginParam_Linear;
	}
	
	// setup threshold for nonlinear modes
	if (mWarpMode != kJucePluginParam_Linear)
	{
		if (mRangeLo == 0.)
		{
			// set threshold under which params get set to 0.
			mRangeLo = mInterval;
			if (zt == 0.)
			{
				mZeroThreshold = mInterval;
			}
		}
	}
}

void MLPublishedParam::addAddress(const MLPath & procPath, const MLSymbol name)
{
	mAddresses.push_back(ParamAddress(procPath, name));
}

MLParamValue MLPublishedParam::getDefault(void)
{
	return mDefault;
}
	
void MLPublishedParam::setDefault(MLParamValue val)
{
	mDefault = val;
}

MLParamValue MLPublishedParam::getValue(void)
{
	return mValue;
}

MLParamValue MLPublishedParam::setValue(MLParamValue val)
{
	mValue = clamp(val, mRangeLo, mRangeHi);
	if (fabs(mValue) <= mZeroThreshold)
	{
		mValue = 0.f;
	}	
	return mValue;
}

MLParamValue MLPublishedParam::getValueAsLinearProportion() const
{
	MLParamValue lo = getRangeLo();
	MLParamValue hi = getRangeHi();
	MLParamValue p;
	MLParamValue val = mValue;
	
	switch (mWarpMode)
	{
		case kJucePluginParam_Linear:
			p = (val - lo) / (hi - lo);
			break;
		case kJucePluginParam_Exp:
			val = clamp(val, lo, hi);
			val = max(mZeroThreshold, val);
			p = logf(val/lo) / logf(hi/lo);
			break;
		case kJucePluginParam_ExpBipolar:
			bool positiveHalf = mValue > 0.;
			if (positiveHalf)
			{
				val = clamp(val, lo, hi);
				val = max(mZeroThreshold, val);
				p = logf(val/lo) / logf(hi/lo);
				p = p*0.5f + 0.5f;
			}
			else
			{
				val = -clamp(val, -hi, -lo);
				val = max(mZeroThreshold, val);
				p = logf(val/lo) / logf(hi/lo);
				p = -p*0.5f + 0.5f;
			}
			break;
	}
	
	//debug() << "val = " << val << " -> prop = " << p << "\n";
	return p;
}

MLParamValue MLPublishedParam::setValueAsLinearProportion (MLParamValue p)
{
	MLParamValue lo = getRangeLo();
	MLParamValue hi = getRangeHi();
	MLParamValue val = 0.f;
	MLParamValue pBipolar, valExp;

	switch (mWarpMode)
	{
		case kJucePluginParam_Linear:
			val = lo + p*(hi - lo);
			break;
		case kJucePluginParam_Exp:
			valExp = p*(logf(hi)/logf(lo) - 1) + 1;
			val = powf(lo, valExp);
			if (val < mZeroThreshold)
			{
				val = 0.f;
			}
			break;
		case kJucePluginParam_ExpBipolar:
			bool positiveHalf = p > 0.5f;
			pBipolar = positiveHalf ? (p - 0.5f)*2.f : (0.5f - p)*2.f;
			valExp = pBipolar*(logf(hi)/logf(lo) - 1) + 1;
			val = positiveHalf ? powf(lo, valExp) : -powf(lo, valExp);
			if (fabs(val) < mZeroThreshold)
			{
				val = 0.f;
			}
			break;
	}
	
	//debug() << " prop = " << p <<  " -> val = " << val << "\n";
	mValue = val;
	return val;
}

// ----------------------------------------------------------------
#pragma mark named parameter groups

MLParamGroupMap::MLParamGroupMap()
{
	mCurrentGroup = -1;
}

MLParamGroupMap::~MLParamGroupMap()
{
}

void MLParamGroupMap::clear()
{
	mCurrentGroup = -1;
	mGroupVec.clear();
}

void MLParamGroupMap::setGroup(const MLSymbol groupSym)
{
	unsigned i;
	bool found = false;
	std::string groupStr = groupSym.getString();
	for (i=0; i<mGroupVec.size(); i++)
	{
		if (!groupStr.compare(mGroupVec[i]))
		{
			found = true;
			break;
		}
	}
	
	if(!found) 
	{
		mGroupVec.push_back(groupStr);
	}
	
	mCurrentGroup = i;
}

void MLParamGroupMap::addParamToCurrentGroup(MLPublishedParamPtr p)
{
	p->setGroupIndex(mCurrentGroup);
}

const std::string& MLParamGroupMap::getGroupName(unsigned index)
{
	return mGroupVec[index];
}

