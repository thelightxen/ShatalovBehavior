# How to Install

Download the installer and follow its instructions.<br />
After installation, rebuild your project.

> <a href="https://gitlab.com/-/project/68916792/uploads/b138da36e232cabe3a384b16afcf5453/SB_Installer.exe">SB Installer.exe</a><br />
> <a href="https://gitlab.com/thelightxen/ShatalovBehavior/-/tree/main/Programs/SB_Installer">(cource code)</a>

#  AI Character Integration

This document outlines the process of integrating the **ShatalovBehavior** system into a project using `ACharacter`. This system leverages the `UGameplayTask` architecture to provide a hierarchical, task-based AI logic flow similar to the Hello Neighbor game.

## Prerequisites

1.  **Module Dependency**: Ensure your project's `.Build.cs` file includes `GameplayTasks` and `AIModule`.
    ```csharp
    PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GameplayTasks", "AIModule" });
    ```
2.  **AI Controller**: Your Character must be possessed by an `AAIController` for movement tasks (`BehMove`) to function correctly.

---

## 1. Character Class Setup (.h)

Your AI Character needs a `UGameplayTasksComponent` to manage behavior execution and a reference to the active `UBehavior`.

```cpp
// Sosed.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Sosed.generated.h"

UCLASS()
class MYPROJECT_API ASosed : public ACharacter
{
    GENERATED_BODY()

public:
    ASosed();

protected:
    virtual void BeginPlay() override;
    virtual void Destroyed() override;

public:
    /** Required component for UGameplayTask execution */
    UPROPERTY(VisibleInstanceOnly, DisplayName="GameplayTasksComponent")
    class UGameplayTasksComponent* GameplayTasksComp;

    /** The runtime instance of the root behavior */
    UPROPERTY(BlueprintReadWrite)
    class UBehavior* Behavior;

    /** The behavior class to instantiate on BeginPlay (e.g., "BehHouseWork") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class UBehavior> BehClass;
};
```

---

## 2. Character Implementation (.cpp)

Proper lifecycle management is critical. You must initialize the component in the constructor and ensure the behavior is terminated when the Actor is destroyed.

```cpp
// Sosed.cpp
#include "Sosed.h"
#include "GameplayTasksComponent.h"
#include "Behavior/Base/Behavior.h"

ASosed::ASosed()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASosed::BeginPlay()
{
    Super::BeginPlay();

    if (!GameplayTasksComp)
	{
		GameplayTasksComp = NewObject<UGameplayTasksComponent>(this, TEXT("GameplayTasksComponent"));
		GameplayTasksComp->RegisterComponent();
	}
	Behavior = UBehavior::NewTask<UBehavior>(GameplayTasksComp);
	Behavior->ReadyForActivation();
    
    if (BehClass && Behavior)
    {
        Behavior->RunBehavior(BehClass, true);
    }
}

void ASosed::Destroyed()
{
    // MANDATORY: Cleanup behavior to prevent dangling delegates or memory leaks
    if (Behavior)
    {
        Behavior->FinishBehavior(BR_Skipped, "OwnerDestroyed");
    }

    Super::Destroyed();
}
```

---

## 3. Creating Custom Behaviors

The system provides three primary behavior types via the `EBehaviorType` enum:

1.  **BT_Default**: Standard sequential tasks. High-priority tasks will overwrite lower-priority ones.
2.  **BT_Parallel**: Tasks that run independently of the main behavior stack.
3.  **BT_Base**: A container/manager that selects and executes child behaviors from a `Behaviors` array based on weights and cooldowns.

### Example: A custom "Search" Behavior
```cpp
// BehSearch.h
UCLASS()
class UBehSearch : public UBehavior
{
    GENERATED_BODY()
public:
    virtual void Activate() override;
};

// BehSearch.cpp
void UBehSearch::Activate()
{
    Super::Activate(); // Essential for initialization

    // Logic: Move to a point, then wait
    RunBehMove(FVector(100, 200, 0), 50.f);
}

// Handle what happens when the movement finishes
void UBehSearch::OnMoveCompleted_Implementation(EPathFollowingResult::Type Result)
{
    if (Result == EPathFollowingResult::Success)
    {
        // Start a 2-second wait task
        UBehWait* WaitTask = Cast<UBehWait>(RunBehavior(UBehWait::StaticClass()));
        WaitTask->m_fWaitTime = 2.0f;
    }
    else
    {
        FinishBehavior(BR_Failed, "MoveFailed");
    }
}
```

---

## 4. Configuring the Randomizer Task (BT_Base)

To create a self-sustaining AI, create a class with `Type = BT_Base`. This acts as the decision-maker.

**In C++ Constructor:**
```cpp
UBehHouseWork::UBehHouseWork(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Type = BT_Base;
    
    // Setup random weighted tasks
    Behaviors.Add(FBehaviorData(UBehToilet::StaticClass(), 0, 0.7f, 0.f, 0));
    Behaviors.Add(FBehaviorData(UBehSleep::StaticClass(), 0, 0.3f, 5.0f, 2));
}
```

---

## 5. Important Implementation Rules

### 1. Validation Check
Always check if the task is still valid when receiving callbacks from timers, delegates, or asynchronous events.
```cpp
if (!IsBehaviorValid()) return;
```

### 2. Priority System
*   **Low Priority (e.g., 255)**: Tasks will be queued if a task is already running.
*   **High Priority (e.g., 0)**: Tasks will immediately stop the current child task and start.

### 3. Cleanup (Delegates)
If you bind to any external delegates (e.g., `OnPerceptionUpdated`), you **must** unbind them in `OnBehaviorFinished_Implementation`.

```cpp
void UBehAttack::OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode)
{
    Super::OnBehaviorFinished_Implementation(Result, FailedCode);
    // Unbind here to prevent crashes after task destruction
}
```
### 4. Animation (BehAnim)
The `RunBehAnim` function assumes the Pawn is an `ACharacter`. It automatically handles the `AnimationBlueprint` toggle if `bResetPose` is true, ensuring the AI returns to its Idle/Walk state after the animation sequence finishes.