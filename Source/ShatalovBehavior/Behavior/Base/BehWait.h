// (c) XenFFly, MissingEntity

#pragma once

#include "CoreMinimal.h"
#include "Behavior.h"
#include "BehWait.generated.h"

UCLASS()
class SHATALOVBEHAVIOR_API UBehWait : public UBehavior
{
	GENERATED_BODY()

public:

	virtual void Activate() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn="true"), DisplayName="WaitTime")
		float m_fWaitTime = 5.f;

protected:
	FTimerHandle TimerHandle;
};
