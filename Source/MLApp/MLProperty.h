
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PROPERTY__
#define __ML_PROPERTY__

#include <string>
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLDebug.h"

// MLProperty: a modifiable property. Properties have three types: float, string, and signal.
// Properties start out with undefined type. Once a type is assigned, properties cannot change type.

class MLProperty
{
public:
	enum eType
	{
		kUndefinedProperty	= 0,
		kFloatProperty	= 1,
		kStringProperty = 2,
		kSignalProperty = 3
	};

	MLProperty();
	MLProperty(const MLProperty& other);
	MLProperty& operator= (const MLProperty & other);
	MLProperty(float v);
	MLProperty(const std::string& s);
	MLProperty(const MLSignal& s);
	~MLProperty();
    
	const float& getFloatValue() const;
	const std::string& getStringValue() const;
	const MLSignal& getSignalValue() const;
    
	void setValue(const float& v);
	void setValue(const std::string& v);
	void setValue(const MLSignal& v);
	void setValue(const MLProperty& v);
	
	bool operator== (const MLProperty& b) const;
	bool operator!= (const MLProperty& b) const;
	eType getType() const { return mType; }
	
	bool operator<< (const MLProperty& b) const;
	
private:
	eType mType;
	union
	{
		float mFloatVal;
		std::string* mpStringVal;
		MLSignal* mpSignalVal;
	}   mVal;
};

std::ostream& operator<< (std::ostream& out, const MLProperty & r);

class MLPropertyListener;
// class MLPropertyModifier;

// MLPropertySet: a Set of Properties. Property names are stored here.

class MLPropertySet
{
    friend class MLPropertyModifier;
	
public:
	MLPropertySet();
	virtual ~MLPropertySet();
    
	const MLProperty& getProperty(MLSymbol p) const;
	const float& getFloatProperty(MLSymbol p) const;
	const std::string& getStringProperty(MLSymbol p) const;
	const MLSignal& getSignalProperty(MLSymbol p) const;
    
	/*
    void setProperty(MLSymbol p, const float& v, bool immediate = false);
    void setProperty(MLSymbol p, const std::string& v, bool immediate = false);
    void setProperty(MLSymbol p, const MLSignal& v, bool immediate = false);
    void setProperty(MLSymbol p, const MLProperty& v, bool immediate = false);
	
	void MLPropertySet::setProperty(MLSymbol p, const std::string& v, bool immediate)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, immediate);
	}*/
	
	template <typename T>
	void setProperty(MLSymbol p, T v)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, false);
	}

	template <typename T>
	void setPropertyImmediate(MLSymbol p, T v)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, true);
	}

	void addPropertyListener(MLPropertyListener* pL);
	void removePropertyListener(MLPropertyListener* pToRemove);
    void broadcastAllProperties();
    
	static const MLProperty nullProperty;
	
private:
	std::map<MLSymbol, MLProperty> mProperties;
	std::list<MLPropertyListener*> mpListeners;
	void broadcastProperty(MLSymbol p, bool immediate);
};

// MLPropertyListeners are notified when a Property of an MLPropertySet changes. They do something in
// response by overriding doPropertyChangeAction().

class MLPropertyListener
{
    friend class MLPropertySet;
public:
	MLPropertyListener(MLPropertySet* m) : mpPropertyOwner(m)
    {
        mpPropertyOwner->addPropertyListener(this);
    }
    
    virtual ~MLPropertyListener()
    {
		if(!mpPropertyOwner) return;
		mpPropertyOwner->removePropertyListener(this);
		mpPropertyOwner = nullptr;
    }
    
	// override to do whatever this PropertyListener needs to do based on the values of properties.
	virtual void doPropertyChangeAction(MLSymbol param, const MLProperty & newVal) = 0;
	
	// call periodically to do actions for any properties that have changed since the last call.
	void updateChangedProperties();
	
	// force an update of all properties.
	void updateAllProperties();
    
protected:

    // called by a PropertySet to notify us that one property has changed.
	// if the property is new, or the value has changed, we mark the state as changed.
	// If immediate is true and the state has changed, doPropertyChangeAction() will be called.
	void propertyChanged(MLSymbol p, bool immediate);
    
	// Must be called by the Property owner to notify us in the event it is going away.
    void propertyOwnerClosing();
    
	// PropertyStates represent the state of a single property relative to updates.
	class PropertyState
	{
	public:
		PropertyState() : mChangedSinceUpdate(true) {}
		~PropertyState() {}
		
		bool mChangedSinceUpdate;
		MLProperty mValue;
	};
    
	std::map<MLSymbol, PropertyState> mPropertyStates;
	MLPropertySet* mpPropertyOwner;
};

// an MLPropertyModifier can request that PropertySets make changes to Properties.
// use, for example, to control a Model from a UI or recall it to a saved state.
// These changes propagate to Listeners as pending changes, which can
// trigger actions the next time the Listeners are updated.

/*
class MLPropertyModifier
{
public:
	MLPropertyModifier(MLPropertySet* m) : mpPropertyOwner(m) {}
	virtual ~MLPropertyModifier() {}
    void requestPropertyChange(MLSymbol p, const float& v);
    void requestPropertyChange(MLSymbol p, const std::string& v);
    void requestPropertyChange(MLSymbol p, const MLSignal& v);
    void requestPropertyChange(MLSymbol p, const MLProperty& v, bool immediate = true); // TODO sort out immediate
    
private:
    MLPropertySet* mpPropertyOwner;
};
*/

#endif // __ML_PROPERTY__

