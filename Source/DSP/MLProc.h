
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_PROC_H
#define _ML_PROC_H

#include <assert.h>
#include <math.h>
#include <map>
#include <vector>
#include <iostream>

#include "MLDSP.h"
#include "MLDSPContext.h"
#include "MLSymbol.h"
#include "MLSymbolMap.h"

#define CHECK_IO    0

#ifdef	__GNUC__
	#define	ML_UNUSED	__attribute__ (( __unused__ )) 
#else
	#define	ML_UNUSED 
#endif

// ----------------------------------------------------------------
#pragma mark important constants

 // keep this small.
const int kMLProcLocalParams = 16; // TODO band-aid!  this should be 4 or something.  crashing evil threading(?) bug.
const std::string kMLProcAliasUndefinedStr = "undefined";

// ----------------------------------------------------------------
#pragma types

typedef std::vector <std::string> MLParamValueAliasVec;
typedef std::map<MLSymbol, MLParamValueAliasVec > MLParamValueAliasMap;

// ----------------------------------------------------------------
#pragma mark templates


// virtual base class allows templated functions to be called from an 
// object of unknown derived type.
class MLProcInfoBase
{    

public:
	MLProcInfoBase() {};	
	virtual ~MLProcInfoBase() {};	
	virtual void setParam(const MLSymbol paramName, MLParamValue value) = 0;
	virtual MLParamValue getParam(const MLSymbol paramName) = 0;
	virtual MLSymbolMap& getParamMap() const = 0;
	virtual MLSymbolMap& getInputMap() const = 0;
	virtual MLSymbolMap& getOutputMap() const = 0;
	virtual bool hasVariableParams() const = 0;
	virtual bool hasVariableInputs() const = 0;
	virtual bool hasVariableOutputs() const = 0;
	virtual MLSymbol& getClassName() = 0;
	
    static const MLParamValueAliasVec kMLProcNullAliasVec;

private:
	// make uncopyable
    MLProcInfoBase (const MLProcInfoBase&); // unimplemented
    const MLProcInfoBase& operator= (const MLProcInfoBase&); // unimplemented

};

// Static member functions can't be virtual, so to provide access to 
// the class data from derived classes we create virtual wrapper functions.
// each must own an MLProcInfo<MLProcSubclass>.
//
// static part (per class): the name maps. 
// dynamic part (per object): the parameter data.
//
template <class MLProcSubclass>
class MLProcInfo : public MLProcInfoBase
{
public:
//friend class MLProcContainer;
friend class MLProcFactory;

	MLProcInfo()
	{ 
		// set up SymbolMappedArray for parameters
		mParams.setMap(getClassParamMap()); 
	}
	~MLProcInfo() {};
	
	void setParam(const MLSymbol paramName, MLParamValue value)
	{
		if (hasVariableParams())
		{
			// if entry is not present, make it
			MLSymbolMap& map = getParamMap();
			if (!map.getIndex(paramName))
			{
				map.addEntry(paramName);
			}
		}
#if CHECK_IO
        MLParamValue* paramToSet = mParams[paramName];
        if (paramToSet != mParams.getNullElement())
        {
            *paramToSet = value;
        }
        else
        {
            debug() << "setParam: " << getClassName() << " has no parameter " << paramName << "!\n";
        }
         
#else
		*(mParams[paramName]) = value;
#endif
	}
	
	MLParamValue getParam(const MLSymbol paramName)
	{
		if (hasVariableParams())
		{
			// if entry is not present, make it and initialize value to 0
			MLSymbolMap& map = getParamMap();
			if (!map.getIndex(paramName))
			{
				map.addEntry(paramName);
				setParam(paramName, 0.f); 
			}		
		}
		return *(mParams[paramName]);
	}
	
	MLSymbolMap& getParamMap() const { return getClassParamMap(); } 
	MLSymbolMap& getInputMap() const { return getClassInputMap(); } 
	MLSymbolMap& getOutputMap() const { return getClassOutputMap(); } 
	inline bool hasVariableParams() const { return getVariableParamsFlag(); }
	inline bool hasVariableInputs() const { return getVariableInputsFlag(); }
	inline bool hasVariableOutputs() const { return getVariableOutputsFlag(); }
	
	MLSymbol& getClassName() { return getClassClassName(); } 
	static void setClassName(const MLSymbol n) { getClassClassName() = n; }	

	// consider these private-- they are public so the syntax of access from templates isn't ridiculous
	static MLSymbolMap &getClassParamMap()  { static MLSymbolMap pMap; return pMap; } 
	static MLSymbolMap &getClassInputMap()  { static MLSymbolMap inMap; return inMap; } 
	static MLSymbolMap &getClassOutputMap()  { static MLSymbolMap outMap; return outMap; } 
	static MLSymbol& getClassClassName() { static MLSymbol cName; return cName; } 
	
	// is there a variable number of inputs / outputs for this class?
	// if so, they can be accessed with names "1", "2"... instead of the map.	
	static bool& getVariableParamsFlag() { static bool mHasVariableParams; return mHasVariableParams; }
	static bool& getVariableInputsFlag() { static bool mHasVariableInputs; return mHasVariableInputs; }
	static bool& getVariableOutputsFlag() { static bool mHasVariableInputs; return mHasVariableInputs; }
    
    // new parameter alias maps: map (symbol -> map(string -> int));
    MLParamValueAliasMap& getParamValueAliasMap() const { return getClassParamValueAliasMap(); }
	static MLParamValueAliasMap &getClassParamValueAliasMap()  { static MLParamValueAliasMap aliasMap; return aliasMap; }
    
    static const MLParamValueAliasVec& getParamValueAliasVec(MLSymbol param)
    {
        MLParamValueAliasMap & aliasMap = MLProcInfo<MLProcSubclass>::getClassParamValueAliasMap();
        MLParamValueAliasMap::iterator it = aliasMap.find(param);
        if(it != aliasMap.end())
        {
            return it->second;
        }
        return MLProcInfoBase::kMLProcNullAliasVec;
    }
    
    static const std::string& getParamAliasByValue(MLSymbol param, int value)
    {
        MLParamValueAliasMap & aliasMap = MLProcInfo<MLProcSubclass>::getClassParamValueAliasMap();
        MLParamValueAliasMap::iterator it = aliasMap.find(param);
        if(it != aliasMap.end())
        {
            const std::vector<std::string> & aliasVec = it->second;
            if(within(value, 1, (int)aliasVec.size() + 1))
            {
                return aliasVec[value - 1]; // one-indexed
            }
            else
            {
                return kMLProcAliasUndefinedStr;
            }
        }
        return kMLProcAliasUndefinedStr;
    }
	
    // return the parameter value for param with the given alias, or 0 if not found. 
    static int getParamValueByAlias(MLSymbol param, const std::string& alias)
    {
        int r = 0;
        MLParamValueAliasMap & aliasMap = MLProcInfo<MLProcSubclass>::getClassParamValueAliasMap();
        MLParamValueAliasMap::iterator it = aliasMap.find(param);
        if(it != aliasMap.end())
        {
            const std::vector<std::string> & aliasVec = it->second;
            MLParamValueAliasVec::const_iterator vecIter;
            int idx = 1; // one-indexed
            for(vecIter = aliasVec.begin(); vecIter != aliasVec.end(); ++vecIter)
            {
                const std::string& aStr = *vecIter;
                if(aStr == alias)
                {
                    r = idx;
                    break;
                }
                idx++;
            }
        }
        return r;
    }
	
private:

	// parameter storage per MLProc subclass instance, each must own an MLProcInfo<MLProcSubclass>.
	// 
	SymbolMappedArray<MLParamValue, kMLProcLocalParams> mParams;
};

// an MLProcParam creates a single indexed parameter that is shared by all
// instances of an MLProc subclass.  This is written as a class only so that
// class parameters can be set up at static initialization time.  It's really
// a function that happens to be called by a constructor.
template <class MLProcSubclass>
class MLProcParam
{
public:
	MLProcParam(const char * name)
    {
		if (!std::string(name).compare("*"))
		{
			MLProcInfo<MLProcSubclass>::getVariableParamsFlag() = true;
		}
		else
		{
			MLProcInfo<MLProcSubclass>::getVariableParamsFlag() = false;
			MLSymbolMap & pMap = MLProcInfo<MLProcSubclass>::getClassParamMap();
			pMap.addEntry(MLSymbol(name));
		}
	}
};

// an MLProcParamValueAlias creates a string alias to an integer value of the parameter 'param'.
// Useful for creating menus and such. aliases will be made for integers in the order they are created.
//
template <class MLProcSubclass, const MLSymbol& param>
class MLProcParamValueAlias
{
public:
	MLProcParamValueAlias(const char * name)
    {
        MLParamValueAliasMap & aliasMap = MLProcInfo<MLProcSubclass>::getClassParamValueAliasMap();
        aliasMap[param].push_back(std::string(name));
	}
};

// MLProcInput and MLProcOutput are similar to MLProcParam, except there is no
// limit on the number of entries. 
template <class MLProcSubclass>
class MLProcInput
{
public:
	MLProcInput(const char *name)
    {
		if (!std::string(name).compare("*"))
		{
			MLProcInfo<MLProcSubclass>::getVariableInputsFlag() = true;
			// debug() << "added input " << name << ", variable size \n";
		}
		else
		{
			MLProcInfo<MLProcSubclass>::getVariableInputsFlag() = false;
			MLSymbolMap & iMap = MLProcInfo<MLProcSubclass>::getClassInputMap();
			iMap.addEntry(MLSymbol(name));
			// debug() << "added input " << name << ", size " << iMap.getSize() << "\n";
		}
	}
};

template <class MLProcSubclass>
class MLProcOutput
{
public:
	MLProcOutput(const char *name)
    {
		if (!std::string(name).compare("*"))
		{
			MLProcInfo<MLProcSubclass>::getVariableOutputsFlag() = true;
			// debug() << "added output " << name << ", variable size \n";
		}
		else
		{
			MLProcInfo<MLProcSubclass>::getVariableOutputsFlag() = false;
			MLSymbolMap & oMap = MLProcInfo<MLProcSubclass>::getClassOutputMap();
			oMap.addEntry(MLSymbol(name));
			//		debug() << "added output " << name << ", size " << oMap.getSize() << "\n";
		}
	}
};

// an MLProc processes signals.  It contains Signals to receive its output.  
// If the block size is small enough, the buffers in these Signals are 
// internal to the MLProc object.  
//
// All inputs and outputs to an MLProc must have the same sampling rate
// and buffer size.  The one exception is MLProcResample, which is only
// created by MLProcContainer.  Possibly this means container resamplers 
// should not really be MLProcs... TODO

class MLProc
{
friend class MLProcContainer;
friend class MLProcMultiple;
friend class MLMultProxy;
friend class MLMultiProc;
friend class MLMultiContainer;
friend class MLProcFactory;
public:
    enum err
	{
		OK = 0,
		memErr,
		inputBoundsErr,
		inputOccupiedErr,
		inputRateErr,
		noInputErr,
		inputMismatchErr,
		fractionalBlockSizeErr,
		connectScopeErr,
		nameInUseErr,
		headNotContainerErr,
		nameNotFoundErr,
		fileOpenErr,
		newProcErr,
		docSyntaxErr,
		needsResampleErr, // ??
		ratioErr,
		scopeErr, 
		resizeErr,
		badIndexErr,
		SSE2RequiredErr,
		SSE3RequiredErr,
		unknownErr
	};

	MLProc();
	
	// ----------------------------------------------------------------
	// wrapper for class static info
	virtual MLProcInfoBase& procInfo() = 0; 	
	//
	virtual bool isContainer() { return false; } 
	inline bool isEnabled() { return getContext()->isProcEnabled(this); }
	
	// for subclasses to make changes based on startup parameters, before prepareToProcess() is called.
	// currently being used for resamplers.
	virtual void setup() {}
	
	// for subclasses to set up buffers, etc.  Can be left at this if there is nothing to resize.
	virtual err resize() { return OK; }		
	virtual err prepareToProcess();
    
    // the process method.
    virtual void process(const int n) = 0;	

	// clearProc() is called by engine, procs override clear() to clear histories. 
	void clearProc();	 
	virtual void clear() = 0;
	virtual void clearInputs();	 
	virtual void clearInput(const int i);

	// getInput and getOutput get references to I/O signals.
	// used commonly in process(), so non-virtual and inline.
	// They do no checking in a release build. 
	// In a debug build they will complain and return a bogus Signal
	// if a signal or signal ptr has not been made at
	// the given index.  Signals and room for signal ptrs are created when 
	// connections are made in the DSP graph.   
	
	// TODO making all inputs and outputs 0-indexed would save a little time. 

	inline const MLSignal& getInput(const int idx)
	{ 	
#if CHECK_IO
		if (idx > (int)mInputs.size())
		{
			debug() << "MLProc::getInput: no input " << idx << " of proc " << getName() << "!\n";
			return (getContext()->getNullInput());
		}
#endif
		return (*mInputs[idx-1]); 
	}

	inline MLSignal& getOutput(const int idx) 
	{	
#if CHECK_IO
		if (idx > (int)mOutputs.size())
		{
			debug() << "MLProc::getOutput: no output " << idx << " of proc " << getName() << "!\n";
			return (getContext()->getNullOutput());
		}
#endif
		return (*mOutputs[idx-1]); 
	}

	inline MLSignal& getOutput() 
	{	
		return (*mOutputs[0]); 
	}

	virtual err setInput(const int idx, const MLSignal& srcSig);	 
	void setOutput(const int idx, MLSignal& srcSig);	 
	
	bool paramExists(const MLSymbol p);
	virtual MLParamValue getParam(const MLSymbol p);	
	virtual void setParam(const MLSymbol p, MLParamValue v);	

	// MLProc returns the index to an entry in its proc map.
	// MLProcContainer returns an index to a published input or output.
	virtual int getInputIndex(const MLSymbol name);	
	virtual int getOutputIndex(const MLSymbol name);	

	// MLProcContainer overrides this to return published output names
	virtual MLSymbol getOutputName(int index);
	
	int getNumInputs();
	int getNumRequiredInputs();
	int getNumOutputs();	
	int getNumRequiredOutputs();	
	
	virtual void resizeInputs(int n);
	virtual void resizeOutputs(int n);

	bool inputIsValid(int idx);
	bool outputIsValid(int idx);
	
	MLSymbol& getClassName() { return procInfo().getClassName(); }
	const MLSymbol& getName() const { return mName; }
    int getCopyIndex() const { return mCopyIndex; }
    MLSymbol getNameWithCopyIndex();
	void dumpParams();
	virtual void dumpProc(int indent);

	// get enclosing DSP context
	MLDSPContext* getContext() const { return mpContext; }

	inline int getContextVectorSize() { return mpContext ? mpContext->getVectorSize() : 0; }	
	inline MLSampleRate getContextSampleRate() { return mpContext ? mpContext->getSampleRate() : kMLTimeless; }
	inline float getContextInvSampleRate() { return mpContext ? mpContext->getInvSampleRate() : kMLTimeless; }
	
protected:	
	virtual ~MLProc() = 0;	
	void dumpNode(int indent);
	void printErr(MLProc::err err);	

	void setName(const MLSymbol name) { mName = name; }
	void setContext(MLDSPContext* pc) { mpContext = pc; }
    void setCopyIndex(int c)  { mCopyIndex = c; }
	
	virtual void createInput(const int idx);
	
	// ----------------------------------------------------------------
	// data
	
protected:
	MLDSPContext* mpContext;		// set by our enclosing context to itself on creation
	bool mParamsChanged;			// set by setParam() // TODO  container stores list of changes

	// pointers to input signals.  A subclass of MLProc will get data from these
	// signals directly in its process() method.  A subclass of MLProcContainer
	// will pass the pointers to the inputs of its subprocs.
	std::vector<const MLSignal*> mInputs;
	std::vector<MLSignal*> mOutputs;
	
private:	
	int mCopyIndex;		// copy index if in multicontainer, 0 otherwise
	MLSymbol mName;
};

typedef std::tr1::shared_ptr<MLProc> MLProcPtr;
typedef std::list<MLProcPtr> MLProcList;
typedef MLProcList::iterator MLProcListIterator;

// ----------------------------------------------------------------
#pragma mark factory


class MLProcFactory
{
public:
	// singleton: we only want one MLProcFactory, even for multiple MLDSPEngines. 
    static MLProcFactory &theFactory()  { static MLProcFactory f; return f; }

	typedef MLProcPtr (*MLProcCreateFnT)(void);
    typedef std::map<MLSymbol, MLProcCreateFnT> FnRegistryT;
    FnRegistryT procRegistry;
 
	// register an object creation function by the name of the class.
    void registerFn(const MLSymbol className, MLProcCreateFnT fn);
	
	// create a new object of the named class.  
    MLProcPtr create(const MLSymbol className, MLDSPContext* context);
	
	// debug. 
	void printRegistry(void);
	
private:
    MLProcFactory();
    MLProcFactory(const MLProcFactory &); // Not implemented
    MLProcFactory & operator=(const MLProcFactory &); // Not implemented
	~MLProcFactory();
};


// Subclasses of MLProc make an MLProcRegistryEntry object.
// This class is passed a className and links a creation function 
// for the subclass to the className in the registry.  This way the MLProcFactory
// knows how to make them.
template <class MLProcSubclass>
class MLProcRegistryEntry
{
public:
	MLProcRegistryEntry(const char* className)
    {
		MLSymbol classSym = MLSymbol(className);
        MLProcFactory::theFactory().registerFn(classSym, createInstance);	
		MLProcInfo<MLProcSubclass>::setClassName(classSym);
//debug() << "class " << className << " entry initialized\n"; // TEMP
    }
	
	// return shared_ptr to a new MLProc instance. 
	static MLProcPtr createInstance()
    {
		MLProcPtr pNew(new MLProcSubclass);
		return pNew;
    }
};




#endif // _ML_PROC_H
