// (c) XenFFly

#pragma once

#include "CoreMinimal.h"
#include "Behavior.h"
#include "BehAnim.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnimationFinished, UAnimSequenceBase*, Animation, bool, bFullPlayed);

UCLASS()
class SHATALOVBEHAVIOR_API UBehAnim : public UBehavior
{
	GENERATED_BODY()

public:
	virtual void Activate() override;
	virtual void OnBehaviorFinished_Implementation(EBehaviorResult Result, const FString& FailedCode) override;

	UPROPERTY(BlueprintAssignable)
		FOnAnimationFinished OnAnimationFinished;
	
	UAnimSequenceBase* Animation;
	bool bLooping;
	bool bResetPose;

private:
	class ACharacter* AI;
	bool bPlayed;
};
