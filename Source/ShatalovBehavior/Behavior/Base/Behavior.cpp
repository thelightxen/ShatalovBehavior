// (c) XenFFly

#include "Behavior.h"

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

	// Wait for behavior Queue
	if (!IsInterrupted() && IsValid(TaskQueue))
	{
		if (GetChildBehavior() && TaskQueue->Priority > GetChildBehavior()->Priority)
			return;
		
		if (TaskQueue->GetState() == EGameplayTaskState::Uninitialized)
		{
			TaskQueue->InitTask(*this, TaskQueue->Priority);
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
		if (!IsValid(GetChildBehavior()) || (!GetChildBehavior()->IsInterrupted() && BehNew->Priority <= GetChildBehavior()->Priority))
		{
			BehNew->InitTask(*this, BehNew->Priority);
			if (bReady)
				BehNew->ReadyForActivation();
		}
		else TaskQueue = BehNew;
		break;
	case BT_Base:
		if (!IsInterrupted())
		{
			BehNew->InitTask(*GetBehaviorOwner(), BehNew->Priority);
			if (bReady)
				BehNew->ReadyForActivation();
		}
		else TaskQueue = BehNew;
		break;
	}

	return BehNew;
}

void UBehavior::FinishBehavior()
{
	if (IsValid(this))
	{
		OnFinishBehavior();
		if (IsValid(GetParentBehavior()))
		{
			GetParentBehavior()->OnChildFinish(GetClass());

			if (GetParentBehavior()->Type == BT_Base && GetParentBehavior()->GetChildBehavior() == this)
			{
				if (GetParentBehavior()->RepeatCount < GetParentBehavior()->MaxRandomRepeat)
				{
					GetParentBehavior()->RepeatCount++;
					UBehavior* NewBeh = GetParentBehavior()->RunBehavior(
						GetParentBehavior()->Behaviors[GetParentBehavior()->SelectedIndex].Behavior, true);
				}
				else
				{
					// Cooldown after end repeat
					GetParentBehavior()->Behaviors[GetParentBehavior()->SelectedIndex].CurrentCooldown =
						GetParentBehavior()->Behaviors[GetParentBehavior()->SelectedIndex].Cooldown;
				}
			}
		}
			
		EndTask();
	}
	else UE_LOG(LogBehavior, Warning, TEXT("The task is already finished or invalid: %s)."), *GetFullName());
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

void UBehavior::Ready()
{
	if (GetState() == EGameplayTaskState::AwaitingActivation)
		ReadyForActivation();
	else UE_LOG(LogBehavior, Error, TEXT("Cannot set state: Ready (for %s)."), *GetFullName());
}

void UBehavior::SelectBehavior()
{
	if (Behaviors.Num() > 0 && !bSelectingTask && !IsValid(GetChildBehavior()))
	{
		SelectedIndex = 0;
		bSelectingTask = true;
		float TotalWeight = 0.f;

		// Sum weight
		for (const FBehaviorData& BehData : Behaviors)
			if (BehData.CurrentCooldown == 0.f) // replace with a separate pure function
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
			if (!(Behaviors[i].CurrentCooldown == 0.f && (Behaviors[i].MaxPerStage != Behaviors[i].CurrentPerStage ||
					Behaviors[i].MaxPerStage == 0))) // move it to a separate pure function
				continue;
			
			AccumulatedWeight += Behaviors[i].RandomWeight;
			if (RandomPoint <= AccumulatedWeight)
			{
				SelectedIndex = i;
				MaxRandomRepeat = UKismetMathLibrary::RandomIntegerInRange(0, Behaviors[i].MaxRandRepeat);
				RepeatCount = 0;
				RunBehavior(Behaviors[i].Behavior, true);
				Behaviors[i].CurrentPerStage++;
				bSelectingTask = false;
				return;
			}
		}
	}
	else if (Behaviors.Num() == 0)
		UE_LOG(LogBehavior, Error, TEXT("Behavior Type is BT_Base, but Behaviors array is empty: %s"), *GetFullName());
}
