/*
	File:			Insomnia.cpp
	Program:		Insomnia
	Author:			Michael Roßberg/Alexey Manannikov/Dominik Wickenhauser/Andrew James
	Description:	Insomnia is a kext module to disable sleep on ClamshellClosed
 
	Insomnia is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
 
	Insomnia is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with Insomnia; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <IOKit/IOLib.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <sys/kern_control.h>


#include <mach/mach_types.h>
#include <sys/systm.h>
#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <sys/kern_control.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/kern_event.h>


#include "Insomnia.h"


#define super IOService

static int sysctl_lidsleep SYSCTL_HANDLER_ARGS;
static unsigned int lidvar = 1;
static unsigned int origstate = lidvar;

SYSCTL_PROC(_kern, OID_AUTO, lidsleep, CTLTYPE_INT|CTLFLAG_RW, &lidvar, 0, &sysctl_lidsleep, "I", "");

struct kern_ctl_reg ep_ctl; // Initialize control 
kern_ctl_ref kctlref;
class Insomnia *myinstance;

OSDefineMetaClassAndStructors(Insomnia, IOService);

#pragma mark -

/* init function for Insomnia, unchanged from orginal Insomnia */
bool Insomnia::init(OSDictionary* properties) {
	
	IOLog("Insomnia:init\n");
	
    if (super::init(properties) == false) {
		IOLog("Insomnia::init: super::init failed\n");
		return false;
    }
	
	myinstance = this;
	
    return true;
}


/* start function for Insomnia, fixed send_event to match other code */
bool Insomnia::start(IOService* provider) {
	
	IOLog("Insomnia:start\n");
	
	lastLidState = true;
	
	if (!super::start(provider)) {
		IOLog("Insomnia::start: super::start failed\n");
		return false;
    }
	
	sysctl_register_oid(&sysctl__kern_lidsleep);
	
	Insomnia::send_event(kIOPMDisableClamshell);// | kIOPMPreventSleep);
	
    return true;
}


/* free function for Insomnia, fixed send_event to match other code */
void Insomnia::free() {
    IOPMrootDomain *root = NULL;
    
    root = getPMRootDomain();
	
    if (!root) {
        IOLog("Insomnia: Fatal error could not get RootDomain.\n");
        return;
    }
    
	/* Reset the system to orginal state */
    Insomnia::send_event(kIOPMAllowSleep | kIOPMEnableClamshell);
	
	
    IOLog("Insomnia: Lid close is now processed again.\n");
	
    super::free();
    return;
}

// ###########################################################################

IOWorkLoop* Insomnia::getWorkLoop() {
    if (!_workLoop)
        _workLoop = IOWorkLoop::workLoop();
	
    return _workLoop;
}

// ###########################################################################
void Insomnia::stop(IOService* provider) {
    if (_workLoop) {
        _workLoop->release();
        _workLoop = 0;
    }
	
	sysctl_unregister_oid(&sysctl__kern_lidsleep);
	
    super::stop(provider);
}

#pragma mark -

/* Send power messages to rootDomain */
bool Insomnia::send_event(UInt32 msg) {
    IOPMrootDomain *root = NULL;
	IOReturn		ret=kIOReturnSuccess;
	char			err_str[100];
	
    IOLog("Insomina: Sending event\n");
	
	root = getPMRootDomain();
    if (!root) {
        IOLog("Insomnia: Fatal error could not get RootDomain.\n");
        return false;
    }
	
	
	ret = root->receivePowerNotification(msg);
	
	IOLog("Insomnia: root returns %d\n", ret);
	
	if(ret!=kIOReturnSuccess)
	{
		sprintf(err_str, "Insomina: Error sending event: %d\n", ret);
		IOLog(err_str);
	}
	else
		IOLog("Insomnia: Message sent to root\n");
	
	return true;
}


/* kIOPMMessageClamshallStateChange Notification */
IOReturn Insomnia::message(UInt32 type, IOService * provider, void * argument) {
	
	if (type == kIOPMMessageClamshellStateChange) {
		IOLog("========================\n");
		IOLog("Insomnia: Clamshell State Changed\n");
		
		/* If lid was opened */
		if ( ( argument && kClamshellStateBit) & (!lastLidState)) {
			IOLog("Insomnia: kClamshellStateBit set - lid was opened\n");
			lastLidState = true;
			
			Insomnia::send_event( kIOPMClamshellOpened);
			
		/* If lastLidState is true - lid closed */
		} else if (lastLidState) {
			IOLog("Insomnia: kClamshellStateBit not set - lid was closed \n");
			lastLidState = false;
			
			// - send kIOPMDisableClamshell | kIOPMPreventSleep here?
			if(origstate==1)
				Insomnia::send_event(kIOPMDisableClamshell | kIOPMPreventSleep); 
		}
		
		/*		detection of system sleep probably not needed ...
			
			if ( argument && kClamshellSleepBit) {
				IOLog("Insomnia: kClamshellSleepBit set\n - now awake \n");
			} else {
				IOLog("Insomnia: kClamshellSleepBit not set - now sleeping \n");
			}
		*/
		
	}
	
	return super::message(type, provider, argument);
}

// ###########################################################################

static int sysctl_lidsleep SYSCTL_HANDLER_ARGS {
	int error;

	error = sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2,  req);

	if(lidvar!=1 && lidvar!=0)
		return 1;
	
	if (!error && req->newptr) {//we get value
        if(lidvar != origstate) {
			/*
			if(lidvar)//activate
				myinstance->send_event(kIOPMDisableClamshell | kIOPMPreventSleep);
			if(!lidvar)//deactivate
				myinstance->send_event(kIOPMAllowSleep | kIOPMEnableClamshell);
			*/
			origstate = lidvar;
			}
    } else if (req->newptr) {
        /* Something was wrong with the write request */
    } else {
        /* Read request.  */
        SYSCTL_OUT(req, &lidvar, sizeof(lidvar));
    }
	
	

	return error;
}
