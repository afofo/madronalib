
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLScale.h"

MLScale::MLScale()
{
	setDefaultScale();
	setDefaultMapping();
	recalcRatios();
}

MLScale::~MLScale()
{

}

void MLScale::operator= (const MLScale& b)
{
	mName = b.mName;
	mDescription = b.mDescription;
	mRatioList = b.mRatioList;
	for(int p=0; p<kMLNumRatios; ++p)
	{
		mRatios[p] = b.mRatios[p];
		mPitches[p] = b.mPitches[p];
	}
	for(int n=0; n<kMLNumScaleNotes; ++n)
	{
		mNotes[n] = b.mNotes[n];
	}
}

void MLScale::setDefaultScale()
{
	clear();
	setName("12-equal");
	setDescription("The chromatic equal-tempered scale.");
	// make 12-ET scale	
	for(int i=1; i<=12; ++i)
	{
		addRatio(100.0 * i);
	}
}

void MLScale::clear()
{
	mRatioList.clear();
	addRatio(0.0);
}

void MLScale::addRatio(int n, int d)
{
	double ratio = ((double)n / (double)d);
	mRatioList.push_back(ratio);
}

void MLScale::addRatio(double cents)
{
	double ratio = pow(2., cents / 1200.);
	mRatioList.push_back(ratio);
}

void MLScale::recalcRatios()
{
	int notesInOctave = mRatioList.size() - 1;
	double octaveRatio = mRatioList[notesInOctave];
	int octave, noteInOctave;
	for (int i=0; i < kMLNumRatios; ++i)
	{
		int middleRelativeNote = i - 69;
		if (middleRelativeNote >= 0)
		{
			octave = middleRelativeNote / notesInOctave;
			noteInOctave = middleRelativeNote % notesInOctave;
		}
		else
		{
			octave = ((middleRelativeNote + 1) / notesInOctave) - 1;
			noteInOctave = notesInOctave - 1 + ((middleRelativeNote + 1) % notesInOctave);
		}
		double octaveStartRatio = pow(octaveRatio, (double)octave);		
		mRatios[i] = (float)octaveStartRatio*mRatioList[noteInOctave];
		mPitches[i] = log2(mRatios[i]);
	}
}

// set up a default scale mapping.  we choose to make octaves on the keyboard wrap to octaves
// of pitch when possible.  for scales with 12 notes this is obvious.  with fewer notes in an octave,
// we repeat keys for the same note.  with more notes, we use a multiple of 12 > the number of notes
// and then repeat keys if necessary.
// 
//
// TODO load .kbm files!

void MLScale::setDefaultMapping()
{
	mTonicNote = 69;

	// default
	for(int n=0; n<kMLNumScaleNotes; ++n)
	{
		mNotes[n] = mTonicNote;
	}
	
	int scaleSize = mRatioList.size() - 1;
	if (scaleSize < 1) return;
	
	int keyCycle = scaleSize;
	
	int octave, noteInOctave;

	for (int i=0; i < kMLNumScaleNotes; ++i)
	{
		int tonicRelativeNote = i - mTonicNote;
		if (tonicRelativeNote >= 0)
		{
			octave = tonicRelativeNote / keyCycle;
			noteInOctave = tonicRelativeNote % keyCycle;
		}
		else
		{
			octave = ((tonicRelativeNote + 1) / keyCycle) - 1;
			noteInOctave = keyCycle - 1 + ((tonicRelativeNote + 1) % keyCycle);
		}
		
		mNotes[i] = mTonicNote + octave*scaleSize + noteInOctave*(scaleSize + 1)/(keyCycle);
	}	
}

float MLScale::noteToPitch(float note)
{
	float fn = clamp(note, 0.f, (float)(kMLNumScaleNotes - 1));
	int i = fn;
	float intPart = i;
	float fracPart = fn - intPart;
	float a = mRatios[mNotes[i]];
	float b = mRatios[mNotes[i + 1]];
	return lerp(a, b, fracPart);
}

float MLScale::noteToPitch(int note)
{
	int n = clamp(note, 0, kMLNumScaleNotes - 1);
	return mRatios[mNotes[n]];
}

// quantize an incoming pitch in linear octave space. 
//
float MLScale::quantizePitch(float a)
{
	float r = 1.0f;
	for (int i = kMLNumRatios - 1; i > 0; i--)
	{
		float p = mPitches[i];
		if(p <= a) 
		{
			r = p;
			break;
		}
	}
	return r;
}

void MLScale::setName(const char* nameStr)
{
	mName = nameStr;
}

void MLScale::setDescription(const char* descStr)
{
	mDescription = descStr;
}

void MLScale::dump()
{
	debug() << "scale " << mName << ":\n";
	int n = mRatioList.size();
	for(int i = 0; i<n; ++i)
	{
		debug() << "    " << i << " : " << mRatioList[i] << "\n";
	}
}

