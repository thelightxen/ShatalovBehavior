// (c) XenFFly

#include "Behavior.h"

#include "BehAnim.h"
#include "BehMove.h"
#include "GameplayTasksComponent.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogBehavior);

UBehavior::UBehavior(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), TaskQueue(nullptr), bIsInterrupted(false), bSelectingTask(false)
{
	bTickingTask = true;
}

void UBehavior::Activate()
{
	Super::Activate();

	BehStart();

	Behaviors.StableSort([](const FBehaviorData& Lhs, const FBehaviorData& Rhs) {
		return Lhs.RandomWeight < Rhs.RandomWeight;
		});
}

void UBehavior::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	BehTick(DeltaTime);

	if (!IsInterrupted() && m_bNeedToFinish)
	{
		EndTask();
		m_bNeedToFinish = false;
		return;
	}

	// Wait for behavior Queue
	if (!IsInterrupted() && IsValid(TaskQueue))
	{
		if (TaskQueue->Type == BT_Base)
		{
			UBehavior* BehaviorBase = GetBehaviorOwner()->GetChildBehavior();
			if (IsValid(BehaviorBase) && !BehaviorBase->IsFinished())
				BehaviorBase->FinishBehavior(BR_Skipped, "OverrideTask");
		}
		else if (GetChildBehavior() && TaskQueue->Priority > GetChildBehavior()->Priority)
			return;

		if (TaskQueue->GetState() == EGameplayTaskState::Uninitialized)
		{
			TaskQueue->InitTask(TaskQueue->Type == BT_Base ? *GetBehaviorOwner() : *this, TaskQueue->Priority);
			TaskQueue->ReadyForActivation();
			TaskQueue = nullptr;
		}
	}

	if (Type == BT_Base)
	{
		// Reduce cooldown
		for (int i = 0; i < Behaviors.Num(); i++)
			Behaviors[i].CurrentCooldown = FMath::Clamp(Behaviors[i].CurrentCooldown - DeltaTime, 0.f, FLT_MAX);

		SelectBehavior();
	}
}

/*void UBehavior::NotifyAnim_Implementation(const UNativeAnimNotify* AnimNotify, class UAnimSequenceBase* Animation)
{
	if (GetUsedActor())
		Cast<AFlatActor>(GetUsedActor())->Notify(AnimNotify, Animation);
}*/

UBehavior* UBehavior::RunBehavior(TSubclassOf<UBehavior> Behavior, bool bReady)
{
	if (!IsValid(Behavior))
	{
		UE_LOG(LogBehavior, Error, TEXT("Behavior is invalid: %s)."), *GetFullName());
		return nullptr;
	}

	UBehavior* BehNew = NewObject<UBehavior>(this, Behavior);

	switch (BehNew->Type)
	{
	case BT_Parallel:
		BehNew->InitTask(*GetGameplayTasksComponent(), BehNew->Priority);
		if (bReady)
			BehNew->ReadyForActivation();
		break;
	case BT_Default:
		if (!IsValid(GetChildBehavior()) || BehNew->Priority <= GetChildBehavior()->Priority && !GetChildBehavior()->IsInterrupted())
		{
			if (IsValid(GetChildBehavior()) && !GetChildBehavior()->IsFinished())
				GetChildBehavior()->FinishBehavior(BR_Skipped, "OverrideTask");

			BehNew->InitTask(*this, BehNew->Priority);
			if (bReady)
				BehNew->ReadyForActivation();
		}
		else
		{
			if (TaskQueue) TaskQueue->MarkPendingKill();
			TaskQueue = BehNew;
		}
		break;
	case BT_Base:
		if (!GetBehaviorOwner())
			break;

		if (!IsInterrupted())
		{
			BehNew->InitTask(*GetBehaviorOwner(), BehNew->Priority);
			if (bReady)
				BehNew->ReadyForActivation();
		}
		else
		{
			if (TaskQueue) TaskQueue->MarkPendingKill();
			TaskQueue = BehNew;
		}
		break;
	}

	return BehNew;
}

void UBehavior::FinishBehavior(TEnumAsByte<EBehaviorResult> Result, const FString& FailedCode)
{
	if (IsBehaviorValid())
	{
		if (bIsFinishing)
			return;

		bIsFinishing = true;

		// We finish child tasks ourselves, because we need to use FinishBehavior() instead EndTask() 
		UBehavior* Child = GetChildBehavior();
		if (IsValid(Child) && Child->IsBehaviorValid())
		{
			Child->FinishBehavior(BR_Skipped, "BehAbort");
		}

		FinishResult = Result;
		FinishFailedCode = FailedCode;

		if (IsInterrupted())
		{
			m_bNeedToFinish = true;
			return;
		}
		else EndTask();
	}
	else UE_LOG(LogBehavior, Warning, TEXT("The task is already finished or invalid: %s)."), *GetFullName());
}

void UBehavior::OnDestroy(bool bInOwnerFinished)
{
	OnBehaviorFinished(FinishResult, FinishFailedCode);

	UBehavior* Parent = GetParentBehavior();
	if (IsValid(Parent))
	{
		Parent->OnChildBehaviorFinished(GetClass(), FinishResult, *FinishFailedCode);

		if (Parent->Type == BT_Base && Parent->GetChildBehavior() == this && bOwnedByBase)
		{
			if (Parent->RepeatCount < Parent->MaxRandomRepeat)
			{
				Parent->RepeatCount++;
				UBehavior* NewBeh = Parent->RunBehavior(
					Parent->Behaviors[Parent->SelectedIndex].Behavior, true);
			}
			else
			{
				// Cooldown after end repeat
				Parent->Behaviors[Parent->SelectedIndex].CurrentCooldown =
					Parent->Behaviors[Parent->SelectedIndex].Cooldown;
			}
		}
	}

	if (IsBehaviorValid())
		Super::OnDestroy(bInOwnerFinished);
}

void UBehavior::SetIsInterrupted(bool IsInterrupted)
{
	bIsInterrupted = IsInterrupted;

	if (IsValid(GetParentBehavior()))
		GetParentBehavior()->SetIsInterrupted(IsInterrupted);
}

TArray<FString> UBehavior::GetDebugHierarchi()
{
	if (IsPendingKill())
		return {};

	TArray<FString> Result;
	if (GetParentBehavior())
		Result.Add(GetFName().GetPlainNameString());

	if (Cast<UBehavior>(GetChildTask()))
		Result.Append(Cast<UBehavior>(GetChildTask())->GetDebugHierarchi());

	return Result;
}

UBehavior* UBehavior::GetParentBehavior()
{
	if (IsValid(TaskOwner.GetObject()) && TaskOwner.GetObject()->IsA(UBehavior::StaticClass()))
		return Cast<UBehavior>(TaskOwner.GetObject());
	else return nullptr;
}

UBehavior* UBehavior::GetChildBehavior()
{
	return Cast<UBehavior>(GetChildTask());
}

UBehavior* UBehavior::GetBehaviorOwner()
{
	if (IsValid(GetParentBehavior()))
		return GetParentBehavior();
	else if (BehaviorIsOwnedByTasksComponent())
		return this;
	else return nullptr;
}

UBehavior* UBehavior::GetLastBehavior()
{
	if (IsValid(GetChildBehavior()))
		return GetChildBehavior()->GetLastBehavior();
	else return this;
}

void UBehavior::Ready()
{
	if (IsInterrupted())
		return;

	if (GetState() == EGameplayTaskState::AwaitingActivation)
		ReadyForActivation();
	else UE_LOG(LogBehavior, Error, TEXT("Cannot set state: Ready (for %s)."), *GetFullName());
}

TArray<UBehavior*> UBehavior::GetParallelBehaviors()
{
	TArray<UBehavior*> Result;
	if (GetGameplayTasksComponent())
		for (auto Task = GetGameplayTasksComponent()->GetKnownTaskIterator(); Task; ++Task)
		{
			UBehavior* BehCast = Cast<UBehavior>(*Task);
			if (IsValid(BehCast) && BehCast->Type == BT_Parallel)
				Result.Add(BehCast);
		}
	return Result;
}

class AAIController* UBehavior::GetAIController()
{
	APawn* PawnOwner = Cast<APawn>(GetOwnerActor());
	if (IsValid(PawnOwner))
	{
		return Cast<AAIController>(PawnOwner->GetController());
	}
	else return nullptr;
}

void UBehavior::SelectBehavior()
{
	if (IsBehaviorValid() && Behaviors.Num() > 0 && !bSelectingTask && !IsValid(GetChildBehavior()))
	{
		SelectedIndex = 0;
		bSelectingTask = true;
		float TotalWeight = 0.f;

		// Sum weight
		for (const FBehaviorData& BehData : Behaviors)
			if (CanExecuteBehavior(BehData))
				TotalWeight += BehData.RandomWeight;

		if (TotalWeight <= 0.f)
		{
			UE_LOG(LogBehavior, Warning, TEXT("All tasks have zero weight: %s"), *GetFullName());
			return;
		}

		float RandomPoint = FMath::FRandRange(0.f, TotalWeight);

		float AccumulatedWeight = 0.f;
		for (int i = 0; i < Behaviors.Num(); i++)
		{
			if (!CanExecuteBehavior(Behaviors[i]))
				continue;

			AccumulatedWeight += Behaviors[i].RandomWeight;
			if (RandomPoint <= AccumulatedWeight)
			{
				SelectedIndex = i;
				MaxRandomRepeat = UKismetMathLibrary::RandomIntegerInRange(0, Behaviors[i].MaxRandRepeat);
				RepeatCount = 0;
				UBehavior* BehRandom = RunBehavior(Behaviors[i].Behavior, false);
				BehRandom->bOwnedByBase = true;
				BehRandom->Ready();
				Behaviors[i].CurrentPerStage++;
				bSelectingTask = false;
				return;
			}
		}
	}
	else if (Behaviors.Num() == 0)
		UE_LOG(LogBehavior, Error, TEXT("Behavior Type is BT_Base, but Behaviors array is empty: %s"), *GetFullName());
}

bool UBehavior::CanExecuteBehavior(const FBehaviorData& Behavior)
{
	return (Behavior.CurrentCooldown == 0.f && (Behavior.MaxPerStage != Behavior.CurrentPerStage ||
		Behavior.MaxPerStage == 0));
}

// Custom
void UBehavior::RunBehMove(FVector TargetLocation, float AcceptanceRadius)
{
	UBehMove* BehMove = Cast<UBehMove>(RunBehavior(UBehMove::StaticClass(), false));
	BehMove->TargetLocation = TargetLocation;
	BehMove->AcceptanceRadius = AcceptanceRadius;
	BehMove->Ready();
}

UBehAnim* UBehavior::RunBehAnim(UAnimSequence* Animation, bool bLooping, bool bResetPose)
{
	UBehAnim* BehAnim = Cast<UBehAnim>(RunBehavior(UBehAnim::StaticClass(), false));
	if (!BehAnim)
		return nullptr;
	BehAnim->Animation = Animation;
	BehAnim->bLooping = bLooping;
	BehAnim->bResetPose = bResetPose;
	BehAnim->Ready();
	return BehAnim;
}

// ~Custom
