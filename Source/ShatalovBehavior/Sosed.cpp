// (c) XenFFly

#include "Sosed.h"

#include "BehaviorOwner.h"
#include "GameplayTasksComponent.h"
#include "Behavior/Base/Behavior.h"

ASosed::ASosed()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASosed::BeginPlay()
{
	if (!GameplayTasksComp)
	{
		GameplayTasksComp = NewObject<UGameplayTasksComponent>(this, TEXT("GameplayTasksComponent"));
		GameplayTasksComp->RegisterComponent();
	}
	Behavior = UBehavior::NewTask<UBehavior>(GameplayTasksComp);
	Behavior->ReadyForActivation();
	
	Super::BeginPlay();
}

void ASosed::Destroyed()
{
	Super::Destroyed();

	if (Behavior)
		Behavior->FinishBehavior(BR_Skipped);
}

void ASosed::PauseMove()
{
	if (Behavior && Behavior->GetAIController())
		Behavior->GetAIController()->GetPathFollowingComponent()->PauseMove(FAIRequestID::AnyRequest, EPathFollowingVelocityMode::Reset);
}

void ASosed::ResumeMove()
{
	if (Behavior && Behavior->GetAIController())
		Behavior->GetAIController()->GetPathFollowingComponent()->ResumeMove();
}
