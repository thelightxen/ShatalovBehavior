// (c) XenFFly, MissingEntity

#pragma once

#include "CoreMinimal.h"
#include "Behavior.h"
#include "BehMove.generated.h"

/**
 * 
 */
UCLASS()
class SHATALOVBEHAVIOR_API UBehMove : public UBehavior
{
	GENERATED_BODY()

public:
	UBehMove(const FObjectInitializer& ObjectInitializer);
	
	void Activate() override;

	virtual void OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode) override;

	UFUNCTION()
		void OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result);
public:
	FVector TargetLocation;
	float AcceptanceRadius = 50.f;

	UPROPERTY()
	FAIMoveRequest MoveRequest;

	UPROPERTY()
	class UAITask_MoveTo* MoveTask;
};
