
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginController.h"

MLPluginController::MLPluginController(MLPluginProcessor* const pProcessor) :
	MLResponder(),
	MLReporter(pProcessor),
	MLSignalReporter(pProcessor),
	mpView(nullptr),
    mpProcessor(pProcessor)
{
	// get plugin parameters and initial values from our processor and create corresponding model properties.
	MLPluginProcessor* const pProc = getProcessor();
	int params = pProc->getNumParameters();
	for(int i=0; i<params; ++i)
	{
		MLPublishedParamPtr p = pProc->getParameterPtr(i);
		MLPublishedParam* param = &(*p);
		if(param)
		{
			mpProcessor->setProperty(param->getAlias(), param->getValue());
		}
	}
	
	mMIDIProgramFiles.resize(kMLPluginMIDIPrograms);
	
	// initialize reference
	WeakReference<MLPluginController> initWeakReference = this;
}

MLPluginController::~MLPluginController()
{
	masterReference.clear();
}

MLAppView* MLPluginController::getView() 
{ 
	return mpView; 
}

void MLPluginController::setView(MLAppView* v) 
{ 
	if(!v)
	{
		// debug() << "MLPluginController::setView 0\n";
	}
	mpView = v;
}

// setup info pertaining to the plugin format we are controlling
//
void MLPluginController::initialize()
{
    AudioProcessor::WrapperType w = getProcessor()->wrapperType;
    
	std::string regStr, bitsStr, pluginType;
	switch(w)
	{
		case AudioProcessor::wrapperType_VST:
			pluginType = "VST";
		break;
		case AudioProcessor::wrapperType_AudioUnit:
			pluginType = "AU";
		break;
		case AudioProcessor::wrapperType_Standalone:
			pluginType = "App";
		break;
		default:
			pluginType = "?";
		break;
	}
		
	#if (__LP64__) || (_WIN64)
		bitsStr = ".64";
	#else
		bitsStr = ".32";
	#endif

	mVersionString = std::string("version ");
	mVersionString += (std::string(MLProjectInfo::versionString));
	mVersionString += " (" + pluginType + bitsStr + ")";
	
	regStr = mVersionString;
	#if DEMO
		regStr += " DEMO\n";
	#else
		regStr += ", licensed to:\n";
	#endif
	
	MLAppView* myView = getView();
    if(myView)
    {
        MLLabel* regLabel = static_cast<MLLabel*>(myView->getWidget("reg"));
        if(regLabel)
        {
            regLabel->setStringAttribute(MLSymbol("text"), regStr);
        }
    }
}

void MLPluginController::handleWidgetAction(MLWidget* pw, MLSymbol action, MLSymbol targetProperty, const MLProperty& val)
{
	debug() << "widget ACTION " << action << " , " << targetProperty << " to " << val << "\n";

	if(action == "start_gesture")
	{
		MLPluginProcessor* const filter = getProcessor();
		if (filter)
		{
			int idx = filter->getParameterIndex(targetProperty);
			if (idx > 0)
			{
				filter->beginParameterChangeGesture (idx);
			}
		}
	}
	else if(action == "property")
	{
		mpProcessor->setPropertyImmediate(targetProperty, val);
	}
	else if (action == "end_gesture")
	{
		MLPluginProcessor* const filter = getProcessor();
		if (filter)
		{
			int idx = filter->getParameterIndex(targetProperty);
			if (idx > 0)
			{
				filter->endParameterChangeGesture (idx);
			}
		}
	}
}


/*

// --------------------------------------------------------------------------------
#pragma mark MLButton::Listener

void MLPluginController::buttonClicked (MLButton* button)
{
 	const int tri = button->getAttribute("tri_button");
    float val;
    if(tri)
    {        
        val = button->getAttribute("value");
    }
    else
    {
        // TODO simplify/merge on / off states and 3-way button concepts
        const bool state = button->getToggleState();
        val = state ? button->getOnValue() : button->getOffValue();
    }
	requestPropertyChange(button->getTargetPropertyName(), val);
}

// --------------------------------------------------------------------------------
#pragma mark MLDial::Listener	
	
void MLPluginController::dialDragStarted (MLDial* pSlider)
{
	const MLSymbol paramName = pSlider->getTargetPropertyName();
	MLPluginProcessor* const filter = getProcessor();
	if (filter)
	{
		int idx = filter->getParameterIndex(paramName);
		if (idx > 0)
			filter->beginParameterChangeGesture (idx);
	}
}

void MLPluginController::dialDragEnded (MLDial* pSlider)
{
	const MLSymbol paramName = pSlider->getTargetPropertyName();
	MLPluginProcessor* const filter = getProcessor();
	if (filter)
	{
		int idx = filter->getParameterIndex(paramName);
		if (idx > 0)
			filter->endParameterChangeGesture (idx);
	}
}

// send Dial changes to Model.
void MLPluginController::dialValueChanged (MLDial* pD)
{
	const MLSymbol paramName = pD->getTargetPropertyName();

	if (pD->isMultiValued())
	{
//			minVal = pSlider->getMinValue();
	}
	else
	{
		if (!pD->isTwoValued())
		{
			requestPropertyChange(paramName, pD->getValue());
		}
		
		// NOT TESTED
		if (pD->isTwoOrThreeValued())
		{
			const std::string paramStr = paramName.getString();
			requestPropertyChange(MLSymbol(paramStr + "_min"), pD->getMinValue());
			requestPropertyChange(MLSymbol(paramStr + "_max"), pD->getMaxValue());
		}		
	}

//		debug() << "dial: " << static_cast<void *>(pSlider) << ", index " << paramIdx << 
//			" [" << minVal << " " << val << " " << maxVal << "]\n";
}

// --------------------------------------------------------------------------------
#pragma mark MLMultiSlider::Listener	

void MLPluginController::multiSliderValueChanged (MLMultiSlider* pSlider, int idx)
{
	if (pSlider)
	{
		MLSymbol paramName = pSlider->getTargetPropertyName();
		const MLSymbol nameWithNumber = paramName.withFinalNumber(idx);
		requestPropertyChange(nameWithNumber, pSlider->getValue(idx));
	}
}

// --------------------------------------------------------------------------------
#pragma mark MLMultiButton::Listener

void MLPluginController::multiButtonValueChanged (MLMultiButton* pButton, int idx)
{
	MLSymbol paramName = pButton->getTargetPropertyName();
	const MLSymbol nameWithNumber = paramName.withFinalNumber(idx);
	requestPropertyChange(nameWithNumber, pButton->getValue(idx));
}

// --------------------------------------------------------------------------------
#pragma mark MLPatcher::Listener


*/

// --------------------------------------------------------------------------------
#pragma mark file collections

// called to build the scale menu when the Processor's collection of sample files has changed.
void MLPluginController::scaleFilesChanged(const MLFileCollectionPtr fileCollection)
{
    populateScaleMenu(fileCollection);
}

void MLPluginController::presetFilesChanged(const MLFileCollectionPtr fileCollection)
{
    // when to do this now with async file search
    populatePresetMenu(fileCollection);
}

// --------------------------------------------------------------------------------
#pragma mark presets

void MLPluginController::prevPreset()
{
    mpProcessor->prevPreset();
    updateChangedProperties();
}

void MLPluginController::nextPreset()
{
    mpProcessor->nextPreset();
    updateChangedProperties();
}

// --------------------------------------------------------------------------------
#pragma mark menus

static void menuItemChosenCallback (int result, WeakReference<MLPluginController> pC, MLSymbol menuName);

MLMenu* MLPluginController::findMenuByName(MLSymbol menuName)	
{
	MLMenu* r = nullptr;
	MLMenuMapT::iterator menuIter(mMenuMap.find(menuName));		
	if (menuIter != mMenuMap.end())
	{
		MLMenuPtr menuPtr = menuIter->second;
		r = menuPtr.get();
	}	
	return r;
}

// set the menu map entry for the given name to a new, empty menu.
MLMenu* MLPluginController::createMenu(MLSymbol menuName)
{
	mMenuMap[menuName] = MLMenuPtr(new MLMenu(menuName));
	return findMenuByName(menuName);
}

void MLPluginController::showMenu (MLSymbol menuName, MLSymbol instigatorName)
{	
	if(!mpView) return;
	
	MLMenu* menu = findMenuByName(menuName);
	if (menu != nullptr)
	{
		menu->setInstigator(instigatorName);

		// find instigator widget and show menu beside it
		MLWidget* pInstigator = mpView->getWidget(instigatorName);
		if(pInstigator != nullptr)
		{
			Component* pInstComp = pInstigator->getComponent();
			if(pInstComp)
			{
                const int u = pInstigator->getWidgetGridUnitSize();
                int height = ((float)u)*0.35f;
                height = clamp(height, 12, 128);
				JuceMenuPtr juceMenu = menu->getJuceMenu();
				juceMenu->showMenuAsync (PopupMenu::Options().withTargetComponent(pInstComp).withStandardItemHeight(height),
					ModalCallbackFunction::withParam(menuItemChosenCallback, 
                    WeakReference<MLPluginController>(this),menuName)
                );
			}
		}
	}
}

void MLPluginController::doPresetMenu(int result)
{
    switch(result)
	{
		// do another menu command
		case (0):	// dismiss
		break;
		case (1):	// save as version in current dir
			if(getProcessor()->saveStateAsVersion() != MLProc::OK) 
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					"",
					"OK");
			}
            // update menu (overkill)
			getProcessor()->scanPresets();
		break;
            
		case (2):	// save over previous
			if(getProcessor()->saveStateOverPrevious() != MLProc::OK)
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					"",
					"OK");
			}
			getProcessor()->scanPresets();
		break;
            
		case (3):	// save as ...
		{
            int err = 0;
            String errStr;
            File userPresetsFolder = getDefaultFileLocation(kPresetFiles);
            if (userPresetsFolder != File::nonexistent)
            {
                bool nativeChooserUI = true;
                FileChooser fc ("Save preset as...", userPresetsFolder, String::empty, nativeChooserUI);
                if (fc.browseForFileToSave (true))
                {
                    File saveFile = fc.getResult();	
                    err = getProcessor()->saveStateToFullPath(std::string(saveFile.getFullPathName().toUTF8()));
                    if(err)
                    {
                        errStr = "Please choose a location in the ";
                        errStr += MLProjectInfo::projectName;
                        errStr += " folder.";
                        AlertWindow::showMessageBox (AlertWindow::NoIcon, String::empty, errStr, "OK");
                    }
                    else
                    {
                        getProcessor()->scanPresets();
                    }
                }
            }
            else
            {
                errStr = ("Error: user presets folder did not exist and could not be created.");
                AlertWindow::showMessageBox (AlertWindow::NoIcon, String::empty, errStr, "OK");
            }
		}
		break;
		case (4):	// revert
			getProcessor()->returnToLatestStateLoaded();
		break;

		case (5):	// copy
			SystemClipboard::copyTextToClipboard (getProcessor()->getStateAsText());
		break;
		case (6):	// paste
			getProcessor()->setStateFromText (SystemClipboard::getTextFromClipboard());
		break;

#if SHOW_CONVERT_PRESETS
#if ML_MAC
		case (7):	// show convert alert box
			updatePresets();
			getProcessor()->scanPresets();
		break;
#endif
#endif
        default:    // load preset
            MLMenu* menu = findMenuByName("preset");
            if (menu)
            {
                const std::string& fullName = menu->getItemFullName(result);                
                getProcessor()->loadStateFromPath(fullName);
            }
            break;
	}
}

void MLPluginController::doScaleMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):	
            mpProcessor->setProperty("key_scale", "12-equal");
            break;
        default:
            MLMenu* menu = findMenuByName("key_scale");
            if (menu)
            {
                // set model param to the full name of the file in the menu
                const std::string& fullName = menu->getItemFullName(result);
                mpProcessor->setProperty("key_scale", fullName);
            }
            break;
    }
}
	
static void menuItemChosenCallback (int result, WeakReference<MLPluginController> wpC, MLSymbol menuName)
{
	MLPluginController* pC = wpC;
	
	// get Controller ptr from weak reference
	if(pC == nullptr)
	{
		debug() << "    null MLPluginController ref!\n";
		return;
	}
	
	if(pC != nullptr)
	{	
		//debug() << "    MLPluginController:" << std::hex << (void *)pC << std::dec << "\n";

		// get menu by name from Controller’s menu map		
		const MLMenu* pMenu = pC->findMenuByName(menuName);
		if (pMenu == nullptr)
		{
			debug() << "    MLPluginController::populatePresetMenu(): menu not found!\n";
		}	
		else
		{		
			MLWidgetContainer* pView = pC->getView();
			
			//debug() << "    pView:" << std::hex << (void *)pView << std::dec << "\n";
			if(pView != nullptr)
			{	
				//debug() << "        pView widget name:" << pView->getWidgetName() << "\n";
				
				MLWidget* pInstigator = pView->getWidget(pMenu->getInstigator());
				if(pInstigator != nullptr)
				{
					// turn instigator Widget off
					pInstigator->setPropertyImmediate("value", 0);
				}
			}
			
			pC->menuItemChosen(menuName, result);
		}
	}
}

void MLPluginController::menuItemChosen(MLSymbol menuName, int result)
{
	if (result > 0)
	{
		MLAppView* pV = getView();
		if(pV)
		{
			if (menuName == "preset")
			{
				doPresetMenu(result);
			}	
			else if(menuName == "key_scale")
			{
				doScaleMenu(result);
			}			
		}		
	}
}

void MLPluginController::populatePresetMenu(const MLFileCollectionPtr presetFiles)
{
	MLMenu* menu = createMenu("preset");
	if (menu == nullptr)
	{
		MLError() << "MLPluginController::populatePresetMenu(): menu not found!\n";
		return;
	}			
	menu->clear();
	
#if DEMO	
	menu->addItem("Save as version", false);
#else
	menu->addItem("Save as version");
#endif	
	
#if DEMO
	menu->addItem("Save", false);
#else

	menu->addItem("Save");
#endif	
	
#if DEMO
	menu->addItem("Save as...", false); 
#else
	menu->addItem("Save as...");
#endif	

	menu->addItem("Revert to saved"); 

	menu->addSeparator();		

	menu->addItem("Copy to clipboard");
	menu->addItem("Paste from clipboard");
	
#if SHOW_CONVERT_PRESETS
#if ML_MAC
	menu->addItem("Update presets...");
#endif
#endif
	menu->addSeparator();
    
    // add factory presets, those starting with the plugin name    
    MLMenuPtr factoryMenu(new MLMenu(presetFiles->getName()));
    presetFiles->getRoot()->buildMenuIncludingPrefix(factoryMenu, MLProjectInfo::projectName);
    menu->appendMenu(factoryMenu);
    
    menu->addSeparator();
    
    // add user presets, all the others
    MLMenuPtr userMenu(new MLMenu(presetFiles->getName()));
    presetFiles->getRoot()->buildMenuExcludingPrefix(userMenu, MLProjectInfo::projectName);
    menu->appendMenu(userMenu);
    
    menu->buildIndex();
     
    // send MIDI program info to processor
    MLPluginProcessor* const pProc = getProcessor();
    if(pProc)
    {
        pProc->clearMIDIProgramFiles();
        for(int i=0; i<kMLPluginMIDIPrograms; ++i)
        {
            if(mMIDIProgramFiles[i].exists())
            {
        debug() << "MIDI pgm " << i << " " << mMIDIProgramFiles[i].getFileName() << "\n";
                pProc->setMIDIProgramFile(i, mMIDIProgramFiles[i]);
            }
        }
    }
}

// create a menu of the factory scales.
//
void MLPluginController::populateScaleMenu(const MLFileCollectionPtr fileCollection)
{
    MLMenu* pMenu = createMenu("key_scale");
	pMenu->clear();
 	pMenu->addItem("12-equal");
    MLMenuPtr p = fileCollection->buildMenu();
    pMenu->appendMenu(p);
}

/*

// TEST implementation TODO in v.2
// settings menu component. just contains number and animate buttons now.
// in the future this should contain anything that affects the plugin but
// not its sound.  Examples would be other look and feel changes or
// localization.
//
class SettingsComponent : public Component
{
public:
    SettingsComponent(const String& componentName = String::empty) : Component(componentName)
    {
		// num display toggle
		mNumbersButton = new MLButton("numbers");
		mNumbersButton->setDrawName(true);
		mNumbersButton->setBounds(10, 10, 32, 26);
		addAndMakeVisible(mNumbersButton);
		mComponents.add(mNumbersButton);		
		setSize(192, 64);
    }

    ~SettingsComponent()
    {
    }
	
	void buttonClicked (Button* button)
	{
	debug() << "YA";
	
		if (button == mNumbersButton)
		{
			debug() << "numbers!\n";
		}
	}
	
private:
	OwnedArray<Component> mComponents;
	MLButton* mNumbersButton;
};


void MLPluginEditor::doSettingsMenu()
{
	SettingsComponent settings("SETTINGS");
	MLCallOutBox callOut (settings, *mHeaderSettingsButton, this);
	callOut.runModalLoopAsync();
}
*/

// --------------------------------------------------------------------------------
#pragma mark MLFileCollection::Listener

void MLPluginController::processFile (const MLSymbol collection, const MLFile& srcFile, int idx, int size)
{
    //debug() << "got file from " << collection << " : " << srcFile.getShortName() << "\n";
    if(collection == "convert_user_presets")
    {
        File newPresetsFolder = getDefaultFileLocation(kPresetFiles);
        File destRoot(newPresetsFolder);
        const std::string& relativeName = srcFile.getLongName();
        
        // If file at destination does not exist, or is older than the source, convert
        // source and overwrite destination.
        File destFile = destRoot.getChildFile(String(relativeName)).withFileExtension("mlpreset");
        if(!destFile.exists()  )
        {
            mpProcessor->loadStateFromFile(srcFile.mFile);
            mpProcessor->saveStateToRelativePath(relativeName);
        }
        
        if(srcFile.mFile.getLastModificationTime() > destFile.getLastModificationTime())
        {
            mpProcessor->loadStateFromFile(srcFile.mFile);
            mpProcessor->saveStateToRelativePath(relativeName);
        }
        
        // finishing?
        if(idx == size)
        {
            mpProcessor->scanPresets();
            mpProcessor->suspendProcessing(false);
        }
    }
    else if(collection == "move_user_presets")
    {
        // move from old place to new if new file does not exist.
        File newPresetsFolder = getDefaultFileLocation(kPresetFiles);
        File destRoot(newPresetsFolder);
        const std::string& relativeName = srcFile.getLongName();
        File destFile = destRoot.getChildFile(String(relativeName));
        if(!destFile.exists())
        {
            String presetStr(srcFile.mFile.loadFileAsString());
            destFile.create();
            destFile.replaceWithText(presetStr);
        }
    }
    else
    {
    }
}

#if ML_MAC

// --------------------------------------------------------------------------------
#pragma mark ConvertProgressDisplayThread

// ConvertProgressDisplayThread: progress display for preset converter.
// TODO write on our own base class to replace ThreadWithProgressWindow.

class ConvertProgressDisplayThread : public ThreadWithProgressWindow, public MLPropertyListener, public DeletedAtShutdown
{
public:
    ConvertProgressDisplayThread(MLPluginController* pC, MLFileCollectionPtr pFiles) :
        pController(pC),
        MLPropertyListener(&(*pFiles)),
        mpFileCollection(pFiles),
        myProgress(0),
        mFilesConverted(0),
        ThreadWithProgressWindow ("", true, true)
    {
    }
    
    ~ConvertProgressDisplayThread() {}
    
    void run() override
    {
        std::string rootStr = mpFileCollection->getRoot()->getAbsolutePath();
        rootStr = std::string("Updating presets from ") + rootStr + std::string("...");
        setProgress(-1.0);
        setStatusMessage (rootStr);

        while(myProgress < 1.0)
        {
            if (threadShouldExit()) return;
            updateChangedProperties();
            wait(10);
        }
    }
    
    void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal)
    {
        if(property == "progress")
        {
            float p = newVal.getFloatValue();
            if(p < 1.)
            {
                mFilesConverted++;
            }
            myProgress = p;
            setProgress(p);
        }
    }
    
    MLPluginController* pController;
    MLFileCollectionPtr mpFileCollection;
    float myProgress;
    int mFilesConverted;
    
    // This method gets called from the message thread to end our thread.
    void threadComplete (bool userPressedCancel)
    {
        File destDir = getDefaultFileLocation(kPresetFiles);
        String destDirName = destDir.getFullPathName();
        if (userPressedCancel)
        {
            pController->cancelUpdate();
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets was cancelled.",
                "Some presets may not have been converted.");
        }
        else
        {
            if(mFilesConverted)
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets successful.",
                    String("Converted presets were added to ") + destDirName + ".");
            }
            else
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "No presets found to convert.", "");
            }
        }
    }
};

// --------------------------------------------------------------------------------
#pragma mark ConvertPresetsThread

class ConvertPresetsThread : public Thread, public DeletedAtShutdown
{
public:
    ConvertPresetsThread(MLFileCollectionPtr p, MLFileCollectionPtr q) :
        Thread("update_presets_thread"),
        mPresetsToMove(p),
        mPresetsToConvert(q)
    {
    }
    
    ~ConvertPresetsThread()
    {
        stopThread(1000);
    }
    
    void run()
    {
        int interFileDelay = 2;
        
        // move files in immediate mode
        mPresetsToMove->searchForFilesNow(interFileDelay);
        
        // wait for move to finish
        while(mPresetsToMove->getFloatProperty("progress") < 1.)
        {
            if (threadShouldExit()) return;
            wait(10);
        }
        
        // convert files in immediate mode
        mPresetsToConvert->searchForFilesNow(interFileDelay);
        
        // wait for convert to finish
        while(mPresetsToConvert->getFloatProperty("progress") < 1.)
        {
            if (threadShouldExit()) return;
            wait(10);
        }
    }
    
private:
    MLFileCollectionPtr mPresetsToConvert;
    MLFileCollectionPtr mPresetsToMove;
};

// only convert .aupreset (AU) to .mlpreset (VST) now. After Aalto 1.6 there will be no need to convert presets.
void MLPluginController::updatePresets()
{
    if(!mpProcessor) return;
    
    File presetsFolder = getDefaultFileLocation(kOldPresetFiles);
    if (presetsFolder != File::nonexistent)
    {
        mPresetsToMove = MLFileCollectionPtr(new MLFileCollection("move_user_presets", getDefaultFileLocation(kOldPresetFiles), ".mlpreset"));
        mPresetsToMove->setListener(this);
        mPresetsToConvert = MLFileCollectionPtr(new MLFileCollection("convert_user_presets", getDefaultFileLocation(kOldPresetFiles), ".aupreset"));
        mPresetsToConvert->setListener(this);

        // turn off audio -- will be turned back on by finish or cancel
        mpProcessor->suspendProcessing(true);

        mConvertProgressThread = std::tr1::shared_ptr<ThreadWithProgressWindow>(new ConvertProgressDisplayThread(this, mPresetsToConvert));
        mConvertProgressThread->launchThread();
        
        mConvertPresetsThread = std::tr1::shared_ptr<Thread>(new ConvertPresetsThread(mPresetsToMove, mPresetsToConvert));
        mConvertPresetsThread->startThread();
    }
    else
    {
        debug() << "updatePresets: couldn't find preset folder " << presetsFolder.getFullPathName() << ".\n";
    }
}

void MLPluginController::cancelUpdate()
{
    mPresetsToMove->cancelSearch();
    mPresetsToConvert->cancelSearch();
    mConvertPresetsThread->stopThread(1000);
    mpProcessor->scanPresets();
    mpProcessor->suspendProcessing(false);
}



#endif // ML_MAC

