// (c) XenFFly

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Sosed.generated.h"

UCLASS()
class SHATALOVBEHAVIOR_API ASosed : public ACharacter
{
	GENERATED_BODY()

public:
	ASosed();

protected:

	virtual void BeginPlay() override;
	virtual void Destroyed() override; // Very important

public:
	
	UPROPERTY(VisibleInstanceOnly, DisplayName="GameplayTasksComponent")
	class UGameplayTasksComponent* GameplayTasksComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBehavior* Behavior;
	
	UFUNCTION(BlueprintCallable)
	void PauseMove();
	UFUNCTION(BlueprintCallable)
	void ResumeMove();
};
