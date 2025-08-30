// (c) XenFFly


#include "BehWait.h"

void UBehWait::Activate()
{
	Super::Activate();
	
	BehDelay(m_fWaitTime, {
		FinishBehavior(BR_Success);
	});
}
