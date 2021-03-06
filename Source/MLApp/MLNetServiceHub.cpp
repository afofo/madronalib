
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

/*
	Portions adapted from Oscbonjour:
	Copyright (c) 2005-2009 RÈmy Muller. 
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "MLPlatform.h"

#if ML_WINDOWS
	// TODO
#else

#include "MLNetServiceHub.h"

using namespace ZeroConf;

//------------------------------------------------------------------------------------------------------------

MLNetServiceHub::MLNetServiceHub() :
	browser(0),
	resolver(0),
	service(0)
{
}

MLNetServiceHub::~MLNetServiceHub()
{
	if(browser) delete browser;
	if(resolver)delete resolver;
	if(service) delete service;
}

void MLNetServiceHub::Browse(const char *type, const char *domain)
{
	if(browser) delete browser;
	browser = 0;
	browser = new NetServiceBrowser();
	browser->setListener(this);
	browser->searchForServicesOfType(type, domain);
}

void MLNetServiceHub::Resolve(const char *name, const char *type, const char *domain)
{
	if(resolver) delete resolver;
	resolver = 0;
	resolver = new NetService(domain,type,name);
	resolver->setListener(this);
	resolver->resolveWithTimeout(10.0, false);  // ML temp
}


bool MLNetServiceHub::pollService(DNSServiceRef dnsServiceRef, double timeOutInSeconds, DNSServiceErrorType &err)
{
	assert(dnsServiceRef);

	err = kDNSServiceErr_NoError;
	
  fd_set readfds;
	FD_ZERO(&readfds);
	
  int dns_sd_fd = DNSServiceRefSockFD(dnsServiceRef);
  int nfds = dns_sd_fd+1;
	FD_SET(dns_sd_fd, &readfds);
	
  struct timeval tv;
  tv.tv_sec = long(floor(timeOutInSeconds));
  tv.tv_usec = long(1000000*(timeOutInSeconds - tv.tv_sec));
	
	int result = select(nfds,&readfds,NULL,NULL,&tv);
	if(result>0 && FD_ISSET(dns_sd_fd, &readfds))
	{
		err = DNSServiceProcessResult(dnsServiceRef);
		return true;
	}
	
	return false;
}

void MLNetServiceHub::PollNetServices()
{
	if(resolver && resolver->getDNSServiceRef())
	{
printf("PollNetServices::polling...\n");
		DNSServiceErrorType err = kDNSServiceErr_NoError;
		if(pollService(resolver->getDNSServiceRef(), 0.001, err))
		{
printf("PollNetServices::stop.\n");
			resolver->stop();
		}
	}
}

void MLNetServiceHub::publishUDPService(const char *name, int port)
{
	if(service) delete service;
	service = 0;
	service = new NetService("local.", "_osc._udp", name, port);
	service->setListener(this);
	service->publish(false);
}

void MLNetServiceHub::didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
	veciterator it = std::find(services.begin(),services.end(), pNetService->getName());
	if(it!=services.end()) return; // we already have it
	services.push_back(pNetService->getName());
}

void MLNetServiceHub::didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
	veciterator it = std::find(services.begin(),services.end(), pNetService->getName());
	if(it==services.end()) return;      // we don't have it
	//long index = it-services.begin();   // store the position
	services.erase(it);
}

void MLNetServiceHub::didResolveAddress(NetService *pNetService)
{
//	const std::string& hostName = pNetService->getHostName();
//	int port = pNetService->getPort();
	
}

void MLNetServiceHub::didPublish(NetService *pNetService)
{
}

#endif // ML_WINDOWS
