// (c) XenFFly

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BehaviorOwner.generated.h"

UCLASS()
class SHATALOVBEHAVIOR_API ABehaviorOwner : public AActor
{
	GENERATED_BODY()

public:
	ABehaviorOwner();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleInstanceOnly, DisplayName="GameplayTasksComponent")
		class UGameplayTasksComponent* GameplayTasksComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UBehavior* Behavior;
};
