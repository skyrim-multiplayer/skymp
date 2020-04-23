#pragma once

#include "common/IInterlockedLong.h"

class IEvent
{
	public:
		static const UInt32 kDefaultTimeout = 1000 * 10;

		IEvent();
		~IEvent();

		bool	Block(void);
		bool	UnBlock(void);
		bool	Wait(UInt32 timeout = kDefaultTimeout);

		bool	IsBlocked(void)	{ return blockCount.Get() > 0; }

	private:
		HANDLE				theEvent;
		IInterlockedLong	blockCount;
};
