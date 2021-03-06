
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


#ifndef __ML_NET_SERVICE_HUB_H_
#define __ML_NET_SERVICE_HUB_H_

//#include "MLDebug.h"
//#include "MLTime.h"

//#include "OscOutboundPacketStream.h"
//#include "UdpSocket.h"

#if ML_WINDOWS
	// TODO
#else

#include "NetService.h"
#include "NetServiceBrowser.h"

#include <assert.h>
#include <math.h>
#include <vector>
#include <string>
#include <list>
#include <tr1/memory>

using namespace ZeroConf;

/*
//------------------------------------------------------------------------------------------------------------
class MLNetServiceHubListener
{
public:



};
*/

//------------------------------------------------------------------------------------------------------------
class MLNetServiceHub : 
	public NetServiceListener, 
	public NetServiceBrowserListener
{
//	MLNetServiceHubListener *mpListener;
public:
	NetServiceBrowser *browser;
	NetService *resolver;
	NetService *service;
	std::vector<std::string> services;
	typedef std::vector<std::string>::iterator veciterator;

	MLNetServiceHub();
	~MLNetServiceHub();

	virtual void Browse(const char *type, const char *domain);
	virtual void Resolve(const char *name, const char *type, const char *domain);
	virtual void publishUDPService(const char *name, int port);

	bool pollService(DNSServiceRef dnsServiceRef, double timeOutInSeconds, DNSServiceErrorType &err);
	void PollNetServices(); // ML

	virtual void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);
	virtual void didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);

	virtual void didResolveAddress(NetService *pNetService);
	virtual void didPublish(NetService *pNetService);

private:
	virtual void willPublish(NetService *) {}
	virtual void didNotPublish(NetService *) {}
	virtual void willResolve(NetService *) {}
	virtual void didNotResolve(NetService *) {}
	virtual void didUpdateTXTRecordData(NetService *) {}		
	virtual void didStop(NetService *) {}
	virtual void didFindDomain(NetServiceBrowser *, const std::string &, bool ) {}
	virtual void didRemoveDomain(NetServiceBrowser *, const std::string &, bool ) {}		
	virtual void willSearch(NetServiceBrowser *) {}
	virtual void didNotSearch(NetServiceBrowser *) {}
	virtual void didStopSearch(NetServiceBrowser *) {}
};

#endif // ML_WINDOWS
#endif // __ML_NET_SERVICE_HUB_H_
