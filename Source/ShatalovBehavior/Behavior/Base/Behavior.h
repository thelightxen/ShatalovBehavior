// (c) XenFFly

#pragma once

#include "CoreMinimal.h"
#include "GameplayTask.h"
#include "Navigation/PathFollowingComponent.h"
#include "Behavior.generated.h"

UENUM(BlueprintType)
enum EBehaviorType
{
	BT_Default UMETA(DisplayName = "Default", ToolTip = "Default task"),
	BT_Parallel UMETA(DisplayName = "Parallel", ToolTip = "Parallel task, runs separately from the main thread."),
	BT_Base UMETA(DisplayName = "Base", ToolTip="A task that contains an array of Default tasks, can call them in random order.")
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
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	
public: // Blueprints
	// Called when the task starts
	UFUNCTION(BlueprintImplementableEvent, DisplayName="BehStart", Category = Behavior)
		void BehStart();

	// Tick function for this task
	UFUNCTION(BlueprintImplementableEvent, DisplayName="BehTick", Category = Behavior)
		void BehTick(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnFinishBehavior();
		virtual void OnFinishBehavior_Implementation() {};

	// Check any completed BehMove
	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnMoveCompleted(EPathFollowingResult::Type Result);
		virtual void OnMoveCompleted_Implementation(EPathFollowingResult::Type Result) {};

	// Child Behavior has finished.
	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnChildFinish(TSubclassOf<UBehavior> Behavior);
		virtual void OnChildFinish_Implementation(TSubclassOf<UBehavior> Behavior) {};

	UFUNCTION(BlueprintNativeEvent, Category = Behavior)
		void OnAnimationFinished(UAnimSequence* Animation);
		virtual void OnAnimationFinished_Implementation(UAnimSequence* Animation) {};

public:
	UFUNCTION(BlueprintCallable, Category = Behavior)
		UBehavior* RunBehavior(TSubclassOf<UBehavior> Behavior, bool bReady = true);

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void FinishBehavior();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetIsInterrupted(bool IsInterrupted);
	
	UFUNCTION(BlueprintPure, Category = Behavior)
		bool IsInterrupted() const { return bIsInterrupted; };
	
	UFUNCTION(BlueprintCallable, Category = Behavior)
		TArray<FString> GetDebugHierarchi();

	UFUNCTION(BlueprintPure, Category = Behavior)
		UBehavior* GetParentBehavior();

	UFUNCTION(BlueprintPure, Category = Behavior)
		UBehavior* GetChildBehavior();

	UFUNCTION(BlueprintPure, Category = Behavior)
		UBehavior* GetBehaviorOwner();
		
	// Returns the last Behavior (for example, if state is "BehMain -> BehAction -> BehAnim", the function will return a reference to BehAnim).
	UFUNCTION(BlueprintCallable)
        	UBehavior* GetLastBehavior();

	UFUNCTION(BlueprintPure, DisplayName = "IsOwnedByTasksComponent", Category = Behavior)
		bool BehaviorIsOwnedByTasksComponent() const { return IsOwnedByTasksComponent(); };

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void Ready();

private:
	void SelectBehavior();
	bool CanExecuteBehavior(const FBehaviorData& Behavior);

	FBehaviorData LastSelected;
	int32 RepeatCount = 0, MaxRandomRepeat = 0, SelectedIndex = 0;
	bool bSelectingTask;
};
