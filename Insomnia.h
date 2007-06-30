/*
	File:			Insomnia.h
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

#include <IOKit/IOService.h>

#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOTimerEventSource.h>


class Insomnia : public IOService {
    OSDeclareDefaultStructors(Insomnia);

public:
    // driver startup and shutdown
    virtual bool init(OSDictionary * = 0);
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    virtual void free();
	virtual IOReturn message(UInt32 type, IOService *provider, void *argument = 0);	
	virtual IOWorkLoop* getWorkLoop();
	virtual bool send_event(UInt32 msg);
	
private:
	
	//UInt32				counter;
	bool				lastLidState;
	
	//IOWorkLoop*			myWorkLoop;
	IOWorkLoop*         _workLoop;
};