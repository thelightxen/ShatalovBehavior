// (c) XenFFly

#pragma once

#include "CoreMinimal.h"
#include "GameplayTask.h"
#include "AIController.h"
#include "Behavior.generated.h"

/**
 * \brief safe-delay for UBehavior
 * \param TimerHandle FTimerHandle
 * \param DelayTime The delay time in seconds.
 * \param LambdaBody The code to execute. It can be a single expression (for example, `myFunction();`) or a block (`{ ... }`).
 */
#define BehDelay(TimerHandle, DelayTime, LambdaBody) \
		do { \
			TWeakObjectPtr<std::decay<decltype(*this)>::type> WeakThis(this); \
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, [=, this]() { \
				if (WeakThis.IsValid()) \
				{ \
				LambdaBody; \
				} \
			}, DelayTime, false); \
		} while(0)

UENUM(BlueprintType)
enum EBehaviorType
{
	BT_Default UMETA(DisplayName = "Default", ToolTip = "Default task"),
	BT_Parallel UMETA(DisplayName = "Parallel", ToolTip = "Parallel task, runs separately from the main thread."),
	BT_Base UMETA(DisplayName = "Base", ToolTip="A task that contains an array of Default tasks, can call them in random order.")
};

UENUM(BlueprintType)
enum EBehaviorResult
{
	BR_Success UMETA(DisplayName = "Success", ToolTip = "Task executed successfully."),
	BR_Failed UMETA(DisplayName = "Failed", ToolTip = "Failed while executing task."),
	BR_Skipped UMETA(DisplayName = "Skipped", ToolTip = "Task was skipped.")
};

USTRUCT(BlueprintType)
struct FBehaviorData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UBehavior> Behavior;

	/*
	Do Behavior N times per stage (reset after RefreshLevel).
	MaxPerStage = 0 - unlimited times.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPerStage = 0;

	// Random Weight for this Behavior
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RandomWeight = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 0.f;

	// Random repeat Behavior <= N times.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxRandRepeat = 0;

	int32 CurrentPerStage = 0;
	float CurrentCooldown = 0.f;

	FBehaviorData() {};

	FBehaviorData(TSubclassOf<UBehavior> InBehavior, int32 InMaxPerStage, float InRandomWeight, float InCooldown, int32 InMaxRandRepeat)
	{
		Behavior = InBehavior;
		MaxPerStage = InMaxPerStage;
		RandomWeight = InRandomWeight;
		Cooldown = InCooldown;
		MaxRandRepeat = InMaxRandRepeat;
	}
};

DECLARE_LOG_CATEGORY_EXTERN(LogBehavior, Log, All);

UCLASS(Blueprintable, HideFunctions=(EndTask))
class SHATALOVBEHAVIOR_API UBehavior : public UGameplayTask
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TEnumAsByte<EBehaviorType> Type = BT_Default;

	/**
	 * If the priority is lower (or equal) than the new task, the old task is completed and the new one is started.
	 * If the priority is higher, the new task is added to the queue and waits for the previous task to finish.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		uint8 Priority = 127;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Base, meta = (EditCondition = "Type==EBehaviorType::BT_Base"))
		TArray<FBehaviorData> Behaviors;

	UPROPERTY()
		UBehavior* TaskQueue;

	UPROPERTY()
		bool bIsInterrupted;

public:
	UBehavior(const FObjectInitializer& ObjectInitializer);
	virtual void TickTask(float DeltaTime) override;
	virtual void Activate() override;

public: // Blueprints
	// Called when the task starts
	UFUNCTION(BlueprintImplementableEvent, DisplayName="BehStart", Category = Behavior)
		void BehStart();

	// Tick function for this task
	UFUNCTION(BlueprintImplementableEvent, DisplayName="BehTick", Category = Behavior)
		void BehTick(float DeltaTime);

	// For current Behavior
	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnBehaviorFinished(EBehaviorResult Result, const FString& FailedCode);
		virtual void OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode) {};

	// Check any completed BehMove
	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnMoveCompleted(EPathFollowingResult::Type Result);
		virtual void OnMoveCompleted_Implementation(EPathFollowingResult::Type Result) {};

	// Child Behavior has finished.
	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnChildBehaviorFinished(TSubclassOf<UBehavior> Behavior, EBehaviorResult Result, const FString& FailedCode);
		virtual void OnChildBehaviorFinished_Implementation(TSubclassOf<UBehavior> Behavior, EBehaviorResult Result, const FString& FailedCode) {};

	// Work with UBehAnim
	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnAnimationFinished(UAnimSequence* Animation);
		virtual void OnAnimationFinished_Implementation(UAnimSequence* Animation) {};

public:
	UFUNCTION(BlueprintCallable, Category = Behavior)
		UBehavior* RunBehavior(TSubclassOf<UBehavior> Behavior, bool bReady = true);

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void FinishBehavior(TEnumAsByte<EBehaviorResult> Result, const FString& FailedCode = "");

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetIsInterrupted(bool IsInterrupted);
	
	UFUNCTION(BlueprintPure, Category = Behavior)
		bool IsInterrupted() const { return bIsInterrupted; };
	
	UFUNCTION(BlueprintCallable, Category = Behavior)
		TArray<FString> GetDebugHierarchi();

	// Get reference to parent Behavior
	UFUNCTION(BlueprintPure, Category = Behavior)
		UBehavior* GetParentBehavior();

	// Get reference to child Behavior
	UFUNCTION(BlueprintPure, Category = Behavior)
		UBehavior* GetChildBehavior();

	// Returns the first Behavior (for example, if state is "BehMain -> BehAction -> BehAnim", the function will return a reference to BehMain).
	UFUNCTION(BlueprintPure, Category = Behavior)
		UBehavior* GetBehaviorOwner();
		
	// Returns the last Behavior (for example, if state is "BehMain -> BehAction -> BehAnim", the function will return a reference to BehAnim).
	UFUNCTION(BlueprintCallable)
        	UBehavior* GetLastBehavior();

	// Get the next task
	UFUNCTION(BlueprintCallable)
		UBehavior* GetBehaviorInQueue() { return TaskQueue; };

	UFUNCTION(BlueprintPure, DisplayName = "IsOwnedByTasksComponent", Category = Behavior)
		bool BehaviorIsOwnedByTasksComponent() const { return IsOwnedByTasksComponent(); };

	// Activate Behavior
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void Ready();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		TArray<UBehavior*> GetParallelBehaviors();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		AAIController* GetAIController();

private:
	void SelectBehavior();
	bool CanExecuteBehavior(const FBehaviorData& Behavior);

	FBehaviorData LastSelected;
	int32 RepeatCount = 0, MaxRandomRepeat = 0, SelectedIndex = 0;
	bool bSelectingTask;

	TEnumAsByte<EBehaviorResult> FinishResult = BR_Skipped;
	FString FinishFailedCode;
	bool bOwnedByBase;

protected:

	template<typename T>
	FString EnumToString(const FString& enumName, const T value)
	{
		UEnum* pEnum = FindObject<UEnum>(ANY_PACKAGE, *enumName);
		return *(pEnum ? pEnum->GetNameStringByIndex(static_cast<uint8>(value)) : "null");
	}

public:
	// Move to Location implementation.
	UFUNCTION(BlueprintCallable)
		void RunBehMove(FVector TargetLocation, float AcceptanceRadius = 10.f);
};
