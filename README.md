# ShatalovBehavior
> Developed with **Unreal Engine 4.27**

This is a super very basic task system that was used in the Hello Neighbor game from Prototype to Release. Originally developed by [**Anton Shatalov**](https://dev.epicgames.com/community/profile/bL9nq/BingoBongo).
The system is created on the **UGameplayTask** class, instead of UObject as in Hello Neighbor (due to the absence of UGameplayTask in UE4.8).

> Looking for installation or AI character integration? Check out the [Setup Guide](/Docs/Setup.md).

## Project
The project includes founder/owner examples (`ABehaviorOwner` as `AActor`, `ASosed` as `ACharacter`), **UBehavior** itself, built-in base tasks (`UBehMove`, `UBehAnim`, `UBehWait`), and an example Behavior Tree setup (`Content/BT`).

#### UBehavior has the following main functions:
- **RunBehavior** - Running a new task (`BT_Default`, `BT_Parallel`, or `BT_Base`)
- **RunDelayedBehavior** - Schedules a task to run after a delay once no child/queued task is active
- **FinishBehavior** - Finishes the task with `EBehaviorResult` and an optional `FailedCode`
- **SetIsInterrupted** - Prevents tasks from finishing before the task (including the current one and its parents) in which `bIsInterrupted` was set
- **Ready** - Starts the task if it's in `AwaitingActivation` state and not interrupted
- **BehStart** (`Activate`) - Called when the task starts
- **BehTick** (`TickTask`) - Tick function for this task
- **RunBehMove** (Only for AI) - Runs `UBehMove` (`UAITask_MoveTo` wrapper)
- **RunBehAnim** (Only for AI `ACharacter`) - Runs `UBehAnim` to play a `UAnimSequenceBase`
- **SetUsedActor** / **GetUsedActor** - Stores or retrieves the `AActor` currently used by this behavior

#### And getter / utility functions:
- **GetParentBehavior** - Get reference to parent Behavior.
- **GetChildBehavior** - Get reference to child Behavior.
- **GetBehaviorOwner** - Returns the root/owner Behavior (for example, if state is `"UBehavior (Core) -> BehMain -> BehAction -> BehAnim"`, the function will return a reference to `UBehavior (Core)`).
- **GetBehaviorBase** - Returns the Base Behavior (`BT_Base`) after Owner (for example, if state is `"UBehavior (Core) -> BehMain -> BehAction -> BehAnim"`, the function will return a reference to `BehMain`, or `nullptr` if none).
- **GetLastBehavior** - Returns the last Behavior (for example, if state is `"UBehavior (Core) -> BehMain -> BehAction -> BehAnim"`, the function will return a reference to `BehAnim`).
- **GetBehaviorInQueue** - Get the next Behavior waiting in `TaskQueue`.
- **GetParallelBehaviors** - Get current active parallel (`BT_Parallel`) Behaviors.
- **GetAIController** - Returns the owning Pawn's `AAIController` (if valid).
- **IsBehaviorValid** - Check if the Behavior is still active (not finished or pending kill).
- **IsInterrupted** - Returns whether `bIsInterrupted` is currently set.
- **BehaviorIsOwnedByTasksComponent** (`IsOwnedByTasksComponent`) - Checks if the task is owned directly by `UGameplayTasksComponent`.
- **GetDebugHierarchi** - Returns the current behavior chain names for debugging.

UBehavior also includes a **prioritization system** (where lower `Priority` numeric values have higher priority: equal or higher-priority tasks overwrite the active child task, while lower-priority tasks are queued in `TaskQueue`), task types via `EBehaviorType` (`BT_Default`, `BT_Parallel`, `BT_Base`), and `bOneInstance` to prevent running duplicate instances of the same Behavior class.

> Some of my ideas may not coincide with Anton Shatalov's ideas, for example, I didn't find an explanation for IsInterrupted in Hello Neighbor, so I rethought this variable for my project.

---

# Documentation (WIP)
### Content:
- [Setup & Installation](/Docs/Setup.md)
- [Validating Behaviors **(IMPORTANT)**](#validating-behaviors)
- [Delays](#delays)
- [Binding/Unbinding delegates](#bindingunbinding-delegates)
- [FinishBehavior with Result](#finishbehavior-with-result)
- [Behavior Owner (Actor)](#behavior-owner-actor)
- [Base tasks](#base-tasks)
- [Built-in Tasks](#built-in-tasks)
- [All `_Implementation` events](#all-_implementation-events)

## Validating Behaviors
If you are using Delegates, Tick or some other event that can be called after the task is finished - you need to **check** if the task is still active.

Just put this condition at the beginning of your function:
```cpp
if (!IsBehaviorValid())
	return;

// <code>
```

## Delays
For safe use of `GetWorld()->GetTimerManager().SetTimer()`, use the `BehDelay` template method (it automatically checks `WeakThis.IsValid()` and `IsBehaviorValid()` before executing the lambda):

**Like this:**

```cpp
FTimerHandle TimerHandle; // If possible, move this declaration to the class header.

BehDelay(TimerHandle, [this]()
{
	// <Code>
}, 5.f);
```
---
## Binding/Unbinding delegates
After you bind delegates with, say:
```cpp
GetAIController()->ReceiveMoveCompleted.AddDynamic(this, &UBehMove::OnMoveFinished);
```

**MANDATORY**, override the `OnBehaviorFinished_Implementation` event.

And unbind from delegates like this:
```cpp
void UBehMove::OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode)
{
	Super::OnBehaviorFinished_Implementation(Result, FailedCode);

	if (IsValid(GetAIController()))
		GetAIController()->ReceiveMoveCompleted.RemoveDynamic(this, &UBehMove::OnMoveFinished);
}
```
> ##### Otherwise, this event will be called again later (even after the task is completed, or the level is changed).

## FinishBehavior with Result
You can finish tasks with `Result` state and `FailedCode` to help you track their execution.

**For example:**

```cpp
void UBehCustomChild::SomeEvent()
{
	FinishBehavior(BR_Failed, "CantMove");
}

void UBehCustomParent::OnChildBehaviorFinished_Implementation(TSubclassOf<UBehavior> Behavior, EBehaviorResult Result, const FString& FailedCode)
{
	if (Result == BR_Failed && FailedCode == "CantMove")
	{
		UE_LOG(LogTemp, Error, TEXT("Hello, World!"));
	}
}
```

>### All states:
>- **BR_Success** - Task executed successfully.
>- **BR_Failed** - Failed while executing task.
>- **BR_Skipped** - Task was skipped.


## Behavior Owner (Actor)
Use `BeginPlay` to initialize `UBehavior` and **ALWAYS** finish the base task when the actor is destroyed. For full setup instructions (including `ACharacter` / AI integration and the installer), see [/Docs/Setup.md](/Docs/Setup.md).


```cpp
// .h

virtual void BeginPlay() override;
virtual void Destroyed() override; // Very important

UPROPERTY(VisibleInstanceOnly, DisplayName="GameplayTasksComponent")
	class UGameplayTasksComponent* GameplayTasksComp;

UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBehavior* Behavior;
```

```cpp
// .cpp

void ABehaviorOwner::BeginPlay()
{
	Super::BeginPlay();

	if (!GameplayTasksComp)
	{
		GameplayTasksComp = NewObject<UGameplayTasksComponent>(this, TEXT("GameplayTasksComponent"));
		GameplayTasksComp->RegisterComponent();
	}
	Behavior = UBehavior::NewTask<UBehavior>(GameplayTasksComp);
	Behavior->ReadyForActivation();
}

void ABehaviorOwner::Destroyed()
{
	Super::Destroyed();

	if (Behavior)
		Behavior->FinishBehavior(BR_Skipped);
}
```

## Base tasks
To setup a base task, just fill the `Behaviors` array and set `Type` to `BT_Base` inside the class constructor:
```cpp
UBehHouseWork::UBehHouseWork(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Type = BT_Base;
	Behaviors = {
		FBehaviorData(UBehWait::StaticClass(), 0, 0.2f, 100.f, 0),
		FBehaviorData(UBehJerks::StaticClass(), 0, 0.5f, 0.f, 0),
		// ...
	};
}
```
**FBehaviorData Constructor:**
```cpp
FBehaviorData(TSubclassOf<UBehavior> InBehavior, int32 InMaxPerStage, float InRandomWeight, float InCooldown, int32 InMaxRandRepeat)
{
	Behavior = InBehavior;
	MaxPerStage = InMaxPerStage;
	RandomWeight = InRandomWeight;
	Cooldown = InCooldown;
	MaxRandRepeat = InMaxRandRepeat;
}
```

## Built-in Tasks
The project comes with three ready-to-use base behaviors in `Behavior/Base`:
- **UBehMove** - Moves an AI pawn to `TargetLocation` using `UAITask_MoveTo` (`RunBehMove(TargetLocation, AcceptanceRadius)`). Notifies the parent via `OnMoveCompleted`.
- **UBehAnim** - Plays a `UAnimSequenceBase` on an `ACharacter` pawn (`RunBehAnim(Animation, bLooping, bResetPose)`), optionally restoring the `AnimationBlueprint` mode when finished and broadcasting `OnAnimationFinished`.
- **UBehWait** - Waits for `m_fWaitTime` seconds (`WaitTime`, default `5.f`) using `BehDelay` and finishes with `BR_Success`.

## All `_Implementation` events

```cpp
virtual void OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode);

virtual void OnChildBehaviorFinished_Implementation(TSubclassOf<UBehavior> Behavior, EBehaviorResult Result, const FString& FailedCode);

virtual void OnMoveCompleted_Implementation(EPathFollowingResult::Type Result);
```
