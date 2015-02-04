
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginController.h"

MLPluginController::MLPluginController(MLPluginProcessor* pProcessor) :
	MLWidget::Listener(),
	MLReporter(),
	MLSignalReporter(pProcessor),
	mpView(nullptr),
#if ML_MAC
	mpConvertPresetsThread(nullptr),
#endif
	mpProcessor(pProcessor),
	mClockDivider(0),
	mConvertPresetsThreadMarkedForDeath(false),
	mConvertProgress(0),
	mFilesConverted(0)
{
	// initialize reference
	WeakReference<MLPluginController> initWeakReference = this;
	
	createMenu("key_scale");
	createMenu("preset");

	listenTo(pProcessor);
	listenTo(pProcessor->getEnvironment());
}

MLPluginController::~MLPluginController()
{
	stopTimer();
#if defined(__APPLE__)
	if(mpConvertPresetsThread)
	{
		mpConvertPresetsThread->stopThread(100);
		delete mpConvertPresetsThread;
	}
#endif
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
            regLabel->setPropertyImmediate(MLSymbol("text"), regStr);
        }
    }

	startTimer(42);
}


void MLPluginController::timerCallback()
{
    const int lessFrequentThingsDivision = 8;
    mClockDivider++;
	fetchChangedProperties();
    
	if(mClockDivider > lessFrequentThingsDivision)
    {
        // do less frequent things
        mClockDivider = 0;
    }
	
    if(getView() != nullptr)
    {
        viewSignals();
    }
	
#if ML_MAC && SHOW_CONVERT_PRESETS
	if(mConvertPresetsThreadMarkedForDeath)
	{
		if(mpConvertPresetsThread)
		{
			if(mpConvertPresetsThread->getThreadId())
			{
				mpConvertPresetsThread->stopThread(100);
				delete mpConvertPresetsThread;
				mpConvertPresetsThread = 0;
			}
		}
		mConvertPresetsThreadMarkedForDeath = false;
	}
#endif
}


void MLPluginController::handleWidgetAction(MLWidget* pw, MLSymbol action, MLSymbol targetProperty, const MLProperty& val)
{
	if(action == "click")
	{
		
	}
	else if(action == "begin_gesture")
	{
		int idx = mpProcessor->getParameterIndex(targetProperty);
		if (idx > 0)
		{
			mpProcessor->beginParameterChangeGesture (idx);
		}
	}
	else if(action == "change_property")
	{
		mpProcessor->setPropertyImmediateExcludingListener(targetProperty, val, pw);
	}
	else if (action == "end_gesture")
	{
		int idx = mpProcessor->getParameterIndex(targetProperty);
		if (idx > 0)
		{
			mpProcessor->endParameterChangeGesture (idx);
		}
	}
	else if(action == "show_menu")
	{
		// give subclasses a chance to rebuild menus
		updateMenu(targetProperty);
		
		showMenu(targetProperty, pw->getWidgetName());
	}
}

/*
#pragma mark presets

void MLPluginController::prevPreset()
{
	mCurrentPresetInMenu--;
	if(mCurrentPresetInMenu < mFirstPresetInMenu)
	{
		mCurrentPresetInMenu = mLastPresetInMenu;
	}
	loadPresetByMenuIndex(mCurrentPresetInMenu);
}

void MLPluginController::nextPreset()
{
	mCurrentPresetInMenu++;
	if(mCurrentPresetInMenu > mLastPresetInMenu)
	{
		mCurrentPresetInMenu = mFirstPresetInMenu;
	}
	loadPresetByMenuIndex(mCurrentPresetInMenu);
}
*/

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
	
	if(menuName == "key_scale")
	{
		populateScaleMenu(getProcessor()->getScaleCollection());
	}
	else if(menuName == "preset")
	{
		populatePresetMenu(getProcessor()->getPresetCollection());
		flagMIDIProgramsInPresetMenu();
	}
	
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
		break;
            
		case (2):	// save over previous
			if(getProcessor()->saveStateOverPrevious() != MLProc::OK)
			{
				AlertWindow::showMessageBox (AlertWindow::NoIcon,
					String::empty,
					"",
					"OK");
			}
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
					std::string fullSavePath(saveFile.getFullPathName().toUTF8());
                    getProcessor()->saveStateToLongFileName(fullSavePath);
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
			getProcessor()->setPatchStateFromText (SystemClipboard::getTextFromClipboard());
		break;

#if SHOW_CONVERT_PRESETS
#if ML_MAC
		case (7):	// show convert alert box
			convertPresets();
			getProcessor()->scanAllFilesImmediate();
		break;
#endif
#endif
        default:    // load preset
            loadPresetByMenuIndex(result);
            break;
	}
}

void MLPluginController::loadPresetByMenuIndex(int result)
{
	MLMenu* menu = findMenuByName("preset");
	if(menu)
	{
		const std::string& fullName = menu->getItemFullName(result);
		getProcessor()->loadStateFromPath(fullName);
	}
	MLReporter::fetchChangedProperties();
}

void MLPluginController::doScaleMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):
            mpProcessor->setPropertyImmediate("key_scale", "12-equal");
            break;
        default:
            MLMenu* menu = findMenuByName("key_scale");
            if (menu)
            {
                // set model param to the full name of the file in the menu
                const std::string& fullName = menu->getItemFullName(result);
                mpProcessor->setPropertyImmediate("key_scale", fullName);
            }
            break;
    }
}

void MLPluginController::doMoreMenu(int result)
{
    switch(result)
    {
        case (0):	// dismiss
            break;
        case (1):
		{
			// first item: OSC enable checkbox
			bool enabled = mpProcessor->getEnvironment()->getFloatProperty("osc_enabled");
			mpProcessor->getEnvironment()->setPropertyImmediate("osc_enabled", !enabled);
			break;
		}
        default:
			// other items set osc port offset.
			mpProcessor->getEnvironment()->setPropertyImmediate("osc_port_offset", result - 2);
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

	//debug() << "    MLPluginController:" << std::hex << (void *)pC << std::dec << "\n";

	// get menu by name from Controller’s menu map		
	const MLMenu* pMenu = pC->findMenuByName(menuName);
	if (pMenu)
	{
		MLWidgetContainer* pView = pC->getView();
		
		//debug() << "    pView:" << std::hex << (void *)pView << std::dec << "\n";
		if(pView != nullptr)
		{	
			//debug() << "        pView widget name:" << pView->getWidgetName() << "\n";
			
			MLWidget* pInstigator = pView->getWidget(pMenu->getInstigator());
			if(pInstigator != nullptr)
			{
				// turn instigator Widget off (typically, release button)
				pInstigator->setPropertyImmediate("value", 0);
			}
		}
		
		pC->menuItemChosen(menuName, result);
	}

}

void MLPluginController::updateMenu(MLSymbol menuName)
{
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
			else if(menuName == "key_more")
			{
				doMoreMenu(result);
			}
		}
	}
}

void MLPluginController::populatePresetMenu(const MLFileCollection& presetFiles)
{
	MLMenu* menu = findMenuByName("preset");
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
	menu->addItem("Convert presets...");
#endif
#endif
	menu->addSeparator();
    
    // add factory presets, those starting with the plugin name
    MLMenuPtr factoryMenu(new MLMenu(presetFiles.getName()));
    presetFiles.buildMenuIncludingPrefix(factoryMenu, MLProjectInfo::projectName);
    menu->appendMenu(factoryMenu);
	
	menu->addSeparator();
    
    // add user presets, all the others
    MLMenuPtr userMenu(new MLMenu(presetFiles.getName()));
    presetFiles.buildMenuExcludingPrefix(userMenu, MLProjectInfo::projectName);
    menu->appendMenu(userMenu);
	
	menu->buildIndex();
}

// create a menu of the factory scales.
//
void MLPluginController::populateScaleMenu(const MLFileCollection& fileCollection)
{
    MLMenu* pMenu = findMenuByName("key_scale");
	pMenu->clear();
 	pMenu->addItem("12-equal");
    MLMenuPtr p = fileCollection.buildMenu();
    pMenu->appendMenu(p);
}

void MLPluginController::flagMIDIProgramsInPresetMenu()
{
	MLMenu* pMenu = findMenuByName("preset");
	if(pMenu != nullptr)
	{
		MLMenu::NodePtr node = pMenu->getItem("MIDI Programs");
		if(node.get() != nullptr)
		{
			if(node->getNodeSize(0) > 0)
			{
				std::list<std::string>::const_iterator it;
				const std::list<std::string>& nodeIndex = node->getIndex();
				
				int p = 1;
				for(it = nodeIndex.begin(); it != nodeIndex.end(); it++)
				{
					const std::string& name = *it;
					MLMenu::NodePtr subNode = node->getSubnodeByName(name);
					{
						std::ostringstream s;
						s << p++;
						const std::string pStr(s.str());
						subNode->setDisplayPrefix(std::string("[") + pStr + std::string("] "));
					}
				}
			}
		}
	}
}

#pragma mark MLFileCollection::Listener

// looks like we are doing nothing here now. So do we need to be a MLFileCollection::Listener? MLTEST
void MLPluginController::processFileFromCollection (MLSymbol action, const MLFile& fileToProcess, const MLFileCollection& collection, int idx, int size)
{
	// MLSymbol collectionName(collection.getName());
	if(action == "begin")
	{
	}
	else if(action == "update")
	{
		// unimplemented
		// in the future we can modify the existing menus incrementally here when new files are found.
	}
	else if(action == "end")
	{
		/* MLTEST
		// for now, we populate menus only when the file search ends.
		if(collectionName == "scales")
		{
			populateScaleMenu(collection);
		}
		else if(collectionName == "presets")
		{
			populatePresetMenu(collection);
			flagMIDIProgramsInPresetMenu();
		}
		 */
	}
}

#if ML_MAC

#pragma mark ConvertProgressDisplayThread

// ConvertProgressDisplayThread: progress display for preset converter.
// TODO write on our own base class to replace ThreadWithProgressWindow.

void MLPluginController::ConvertProgressDisplayThread::run()
{
	std::string rootStr("Converting .aupreset and .mlpreset files from /Library and ~/Library...");
	setProgress(-1.0);
	setStatusMessage (rootStr);

	while(myProgress < 1.0)
	{
		if (threadShouldExit()) return;
		wait(10);
		myProgress = pController->getConvertProgress();
		setProgress(myProgress);
		
		mFilesConverted = pController->getFilesConverted();
	}
}

// This method gets called from the message thread to end our thread.
void MLPluginController::ConvertProgressDisplayThread::threadComplete (bool userPressedCancel)
{
	File destDir = getDefaultFileLocation(kPresetFiles);
	String destDirName = destDir.getFullPathName();
	if (userPressedCancel)
	{
		pController->endConvertPresets();
		AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets was cancelled.",
			"Some presets may not have been converted.");
	}
	else
	{
		if(mFilesConverted)
		{
			AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Convert presets successful.",
				String(mFilesConverted) + String(" presets were added to ") + destDirName + ".");
		}
		else
		{
			AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "No presets found to convert.", "");
		}
	}
	delete this;
}




#pragma mark ConvertPresetsThread

MLPluginController::ConvertPresetsThread::ConvertPresetsThread(MLPluginController* pC) :
	Thread("convert_presets_thread"),
	mpController(pC),
	mpPresetsToConvertAU1(0),
	mpPresetsToConvertAU2(0),
	mpPresetsToConvertVST1(0),
	mpPresetsToConvertVST2(0)
{
	mpPresetsToConvertAU1 = (new MLFileCollection("convert_presets_au1", getDefaultFileLocation(kOldPresetFiles), ".aupreset"));
	mpPresetsToConvertAU1->addListener(this);
	mpPresetsToConvertAU2 = (new MLFileCollection("convert_presets_au2", getDefaultFileLocation(kOldPresetFiles2), ".aupreset"));
	mpPresetsToConvertAU2->addListener(this);
	mpPresetsToConvertVST1 = (new MLFileCollection("convert_presets_vst1", getDefaultFileLocation(kOldPresetFiles), ".mlpreset"));
	mpPresetsToConvertVST1->addListener(this);
	mpPresetsToConvertVST2 = (new MLFileCollection("convert_presets_vst2", getDefaultFileLocation(kOldPresetFiles2), ".mlpreset"));
	mpPresetsToConvertVST2->addListener(this);
}

MLPluginController::ConvertPresetsThread::~ConvertPresetsThread()
{
	if(mpPresetsToConvertAU1)
	{
		delete mpPresetsToConvertAU1;
		mpPresetsToConvertAU1 = 0;
	}
	if(mpPresetsToConvertAU2)
	{
		delete mpPresetsToConvertAU2;
		mpPresetsToConvertAU2 = 0;
	}
	if(mpPresetsToConvertVST1)
	{
		delete mpPresetsToConvertVST1;
		mpPresetsToConvertVST1 = 0;
	}
	if(mpPresetsToConvertVST2)
	{
		delete mpPresetsToConvertVST2;
		mpPresetsToConvertVST2 = 0;
	}
}

void MLPluginController::ConvertPresetsThread::run()
{
	int interFileDelay = 5;
	float p;
	
	// convert files in immediate mode and wait for finish.
	mpPresetsToConvertAU1->searchForFilesImmediate(interFileDelay);
	while((p = mpPresetsToConvertAU1->getFloatProperty("progress")) < 1.)
	{
		mpController->setConvertProgress(p*0.25f);
		if (threadShouldExit()) return;
		wait(10);
	}
	// convert files in immediate mode and wait for finish.
	mpPresetsToConvertAU2->searchForFilesImmediate(interFileDelay);
	while((p = mpPresetsToConvertAU2->getFloatProperty("progress")) < 1.)
	{
		mpController->setConvertProgress(p*0.25f + 0.25f);
		if (threadShouldExit()) return;
		wait(10);
	}
	// convert files in immediate mode and wait for finish.
	mpPresetsToConvertVST1->searchForFilesImmediate(interFileDelay);
	while((p = mpPresetsToConvertVST1->getFloatProperty("progress")) < 1.)
	{
		mpController->setConvertProgress(p*0.25f + 0.5f);
		if (threadShouldExit()) return;
		wait(10);
	}
	// convert files in immediate mode and wait for finish.
	mpPresetsToConvertVST2->searchForFilesImmediate(interFileDelay);
	while((p = mpPresetsToConvertVST2->getFloatProperty("progress")) < 1.)
	{
		mpController->setConvertProgress(p*0.25f + 0.75f);
		if (threadShouldExit()) return;
		wait(10);
	}
	
	// notify controller we are all done
	mpController->endConvertPresets();
}

void MLPluginController::ConvertPresetsThread::processFileFromCollection
	(MLSymbol action, const MLFile& fileToProcess, const MLFileCollection& collection, int idx, int size)
{
	MLSymbol collectionName(collection.getName());
	
	if(action == "begin")
	{
	}
	else if(action == "process")
	{
		if(collectionName.beginsWith(MLSymbol("convert_presets")))
		{
			File newPresetsFolder = getDefaultFileLocation(kPresetFiles);
			File destRoot(newPresetsFolder);
			
			// get name relative to collection root.
			const std::string& relativeName = fileToProcess.getLongName();
			
			// If file at destination does not exist, or is older than the source, convert
			// source and overwrite destination.
			File destFile = destRoot.getChildFile(String(relativeName)).withFileExtension("mlpreset");
			bool destinationExists = destFile.exists();
			bool destinationIsOlder =  destFile.getLastModificationTime() < fileToProcess.getJuceFile().getLastModificationTime();
			if((!destinationExists) || (destinationIsOlder))
			{
				mpController->mpProcessor->loadPatchStateFromFile(fileToProcess);
				mpController->mpProcessor->saveStateToRelativePath(relativeName);
				mpController->fileConverted();
			}
		}
	}
	else if(action == "update")
	{
		// unimplemented
	}
}

// only convert .aupreset (AU) to .mlpreset (VST) now. After Aalto 1.6 there will be no need to convert presets.
void MLPluginController::convertPresets()
{
    if(!mpProcessor) return;
    
	mConvertProgress = 0.;
	mFilesConverted = 0;
	
    File presetsFolder = getDefaultFileLocation(kOldPresetFiles);
    if (presetsFolder != File::nonexistent)
    {
		// turn off audio -- will be turned back on by finish or cancel
        mpProcessor->suspendProcessing(true);
		
		// clear presets collection
		mpProcessor->clearPresetCollection();

		// clear menu
		findMenuByName("preset")->clear();

		if(mpConvertPresetsThread)
		{
			mpConvertPresetsThread->stopThread(100);
			delete mpConvertPresetsThread;
			mpConvertPresetsThread = 0;
		}
		
		(new ConvertProgressDisplayThread(this))->launchThread();

        mpConvertPresetsThread = new ConvertPresetsThread(this);
        mpConvertPresetsThread->startThread();
    }
    else
    {
        debug() << "convertPresets: couldn't find preset folder " << presetsFolder.getFullPathName() << ".\n";
    }
}

void MLPluginController::setConvertProgress(float f)
{
	mConvertProgress = f;
}

float MLPluginController::getConvertProgress() const
{
	return mConvertProgress;
}

int MLPluginController::getFilesConverted() const
{
	return mFilesConverted;
}

void MLPluginController::endConvertPresets()
{
	setConvertProgress(1.f);
	
	// we can't delete the convert thread here, because it calls this method directly!
	// instead a flag is set and our timer callback deletes the thread.
	mConvertPresetsThreadMarkedForDeath = true;

	// rebuild presets menu
	mpProcessor->searchForPresets();

	// MLTEST std::string defaultPreset =
	//mpProcessor->loadStateFromPath(defaultPreset);
	
	// resume processing
	mpProcessor->suspendProcessing(false);
}


#endif // ML_MAC

