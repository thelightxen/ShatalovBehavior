// (c) XenFFly

#include "BehAnim.h"
#include "GameFramework/Character.h"

void UBehAnim::Activate()
{
	Super::Activate();

	if (!Animation)
	{
		FinishBehavior(BR_Failed, "AnimNotSelected");
		return;
	}

	AI = Cast<ACharacter>(GetAIController()->GetPawn());

	if (!IsValid(AI))
	{
		FinishBehavior(BR_Failed, "FindAI_Invalid");
		return;
	}

	AI->GetMesh()->PlayAnimation(Animation, bLooping);

	if (!bLooping)
	{
		FTimerHandle TimerHandle;
		BehDelay(TimerHandle, [=](){
			if (!IsValid(AI))
				return;
			
			if (bResetPose)
				AI->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			
			bPlayed = true;
			OnAnimationFinished.Broadcast(Animation, bPlayed);

			if (!bIsFinishing)
				FinishBehavior(BR_Success);
		}, Animation->GetPlayLength());
	}
}

void UBehAnim::OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode)
{
	Super::OnBehaviorFinished_Implementation(Result, FailedCode);
	
	if (bResetPose && IsValid(AI))
		AI->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	if (IsBehaviorValid() && !bPlayed)
		OnAnimationFinished.Broadcast(Animation, bPlayed);
}

