// (c) XenFFly

#include "BehWait.h"

void UBehWait::Activate()
{
	Super::Activate();
	
	BehDelay(TimerHandle, [this](){
		FinishBehavior(BR_Success);
	}, m_fWaitTime);
}
