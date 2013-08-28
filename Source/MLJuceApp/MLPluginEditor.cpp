
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginEditor.h"

MLPluginEditor::MLPluginEditor (juce::AudioProcessor* ownerProcessor) : 
	AudioProcessorEditor(ownerProcessor)
{
}

MLPluginEditor::~MLPluginEditor()
{
}

void MLPluginEditor::paint(Graphics& )
{
}

MLRect MLPluginEditor::getWindowBounds()
{
	ComponentPeer *peer = getPeer();
	if(peer)
	{
		MLRect r(juceToMLRect(peer->getBounds()));
		return r;
	}
	return MLRect();
}

