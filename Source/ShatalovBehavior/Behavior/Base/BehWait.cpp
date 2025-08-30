// (c) XenFFly


#include "BehWait.h"

void UBehWait::Activate()
{
	Super::Activate();
	
	BehDelay(TimerHandle, m_fWaitTime, {
		FinishBehavior(BR_Success);
	});
}
