// (c) XenFFly

#include "BehaviorOwner.h"

#include "GameplayTasksComponent.h"
#include "Behavior/Base/Behavior.h"

ABehaviorOwner::ABehaviorOwner()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
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

void ABehaviorOwner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

