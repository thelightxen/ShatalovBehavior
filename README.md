# ShatalovBehavior
> Developed with **Unreal Engine 4.27**

This is a super very basic task system that was used in the Hello Neighbor game from Prototype to Release. Originally developed by [**Anton Shatalov**](https://dev.epicgames.com/community/profile/bL9nq/BingoBongo).
The system is created on the **UGameplayTask** class, instead of UObject as in Hello Neighbor (due to the absence of UGameplayTask in UE4.8).
## Project
The project includes a founder class (as AActor) and **UBehavior** itself.

#### UBehavior has the following main functions:
- **RunBehavior** - Running a new task
- **FinishBehavior** - Finishes the task
- **SetIsInterrupted** - Prevents all tasks from finishing before the task (including the current one) in which IsInterrupted was set
- **Ready** - Starts the task if it's not started yet.
- **BehStart** (Activate)
- **BehTick** (TickTask)
- **RunBehMove** (Only for AI) - Runs AI_MoveTo Task.<br>
#### And pure functions:
- **GetParentBehavior** - Get reference to parent Behavior.
- **GetChildBehavior** - Get reference to child Behavior.
- **GetBehaviorOwner** - Returns the first Behavior (for example, if state is "BehMain -> BehAction -> BehAnim", the function will return a reference to BehMain).
- **GetLastBehavior** - Returns the last Behavior (for example, if state is "BehMain -> BehAction -> BehAnim", the function will return a reference to BehAnim).
- **GetBehaviorInQueue** - Get the next Behavior after finishing current.
- **GetParallelBehaviors** - Get current parallel Behaviors.

UBehavior also includes a **prioritization system**, where lower priority tasks are queued and high priority tasks overwrite other tasks.

> Some of my ideas may not coincide with Anton Shatalov's ideas, for example, I didn't find an explanation for IsInterrupted in Hello Neighbor, so I rethought this variable for my project.

---

# Documentation (WIP)
### Content:
- [Delays](#delays)
- [Binding/Unbinding delegates](#bindingunbinding-delegates)
- [FinishBehavior with Result](#finishbehavior-with-result)
- [Behavior Owner (Actor)](#behavior-owner-actor)
- [Base tasks](#base-tasks)
- [All `_Implementation` events](#all-_implementation-events)


## Delays
For safe use of `GetWorldTimerManager()->SetTimer()` - use special macro **BehDelay();**

**Like this:**

```cpp
FTimerHandle TimerHandle; // If possible, move this declaration to the class header.

BehDelay(TimerHandle, 5.f, {
    <Code>
});
```
---
## Binding/Unbinding delegates
After you bind delegates with say: 
```cpp
GetAIController()->ReceiveMoveCompleted.AddDynamic(this, &UBehMove::OnMoveFinished);
```

**MANDATORY**, override the `OnBehaviorFinished_Implementation` event.

And unbind from delegates like this:
```cpp
GetAIController()->ReceiveMoveCompleted.RemoveDynamic(this, &UBehMove::OnMoveFinished);
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
Use `BeginPlay` to initialize `UBehavior` and **ALWAYS** finish the base task when the actor is destroyed.


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
To setup base task, just fill `Behaviors` array and set `Type` to `BT_Base` inside the class constructor:
```cpp
UBehHouseWork::UBehHouseWork(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Type = BT_Base;
	Behaviors = {
		FBehaviorData(UBehWait::StaticClass(), 0, 0.2f, 100.f,0),
		FBehaviorData(UBehJerks::StaticClass(), 0, 0.5f, 0.f,0),
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

## All `_Implementation` events

```cpp
virtual void OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode);

virtual void OnChildBehaviorFinished_Implementation(TSubclassOf<UBehavior> Behavior, EBehaviorResult Result, const FString& FailedCode);

virtual void OnMoveCompleted_Implementation(EPathFollowingResult::Type Result);
```