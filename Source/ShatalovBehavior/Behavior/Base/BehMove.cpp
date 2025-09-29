// (c) XenFFly

#include "BehMove.h"
#include "Tasks/AITask_MoveTo.h"

UBehMove::UBehMove(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), MoveTask(nullptr)
{
}

void UBehMove::Activate()
{
	Super::Activate();

	if (!IsValid(GetAIController()))
	{
		UE_LOG(LogBehavior, Error, TEXT("BehMove: Can't use task without APawn & AIController"));
		FinishBehavior(BR_Failed, "AIController_Error");
		return;
	}

	if (IsValid(GetParentBehavior()) && GetParentBehavior()->IsA(StaticClass()))
	{
		UE_LOG(LogBehavior, Error, TEXT("BehMove was called as a child task of BehMove."));
		FinishBehavior(BR_Failed, "RepeatBehMove");
		return;
	}

	GetAIController()->ReceiveMoveCompleted.AddDynamic(this, &UBehMove::OnMoveFinished);

	MoveTask = UAITask_MoveTo::AIMoveTo(
	   GetAIController(),
	   TargetLocation,
	   nullptr,
	   AcceptanceRadius,
	   EAIOptionFlag::Default, // StopOnOverlap
	   EAIOptionFlag::Default, // AcceptPartialPath
	   true // bUsePathfinding
   );
	MoveTask->ReadyForActivation();

	if (MoveTask->GetState() == EGameplayTaskState::Finished && !MoveTask->WasMoveSuccessful())
	{
		UE_LOG(LogBehavior, Error, TEXT("BehMove: Move failed (%s)"),
		       *EnumToString("EPathFollowingResult", MoveTask->GetMoveResult()));
		FinishBehavior(BR_Failed, "CantMove");
	}
}

void UBehMove::OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode)
{
	Super::OnBehaviorFinished_Implementation(Result, FailedCode);

	// Unbind the delegate to avoid double subscription in the future.
	if (IsValid(GetAIController()))
		GetAIController()->ReceiveMoveCompleted.RemoveDynamic(this, &UBehMove::OnMoveFinished);
	
	// Stop movement
	if (IsValid(MoveTask))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BehMove::OnBehaviorFinished_Implementation()"));
		MoveTask->EndTask();
	}
}

void UBehMove::OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (!IsBehaviorValid())
		return;
		
	switch (Result)
	{
	case EPathFollowingResult::Success:
		FinishBehavior(BR_Success);
		break;
	case EPathFollowingResult::Blocked:
		FinishBehavior(BR_Failed, "Blocked");
		break;
	case EPathFollowingResult::OffPath:
		FinishBehavior(BR_Failed, "OffPath");
		break;
	case EPathFollowingResult::Aborted:
		FinishBehavior(BR_Failed, "Aborted");
		break;
	case EPathFollowingResult::Invalid:
		FinishBehavior(BR_Failed, "Invalid");
		break;
	}

	if (GetParentBehavior())
		GetParentBehavior()->OnMoveCompleted(Result);
}