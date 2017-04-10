// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "SpiderAIController.generated.h"

class ASmartSpiderCharacter;

/**
 * 
 */
UCLASS(ClassGroup = AI, BlueprintType, Blueprintable)
class SMARTSPIDER_API ASpiderAIController : public AAIController
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(BlueprintReadOnly, category = "SpiderAI")
	ASmartSpiderCharacter* SpiderCharacter;

	FVector Destination;
	float AcceptableDistanceSq;

public:
	//UFUNCTION(BlueprintCallable, category = "SpiderAI")
	//bool MoveToDestination();

	bool FindReversePath(FVector Dest, float AcceptanceRadius, TSubclassOf<UNavigationQueryFilter> FilterClass);

	//
	//UFUNCTION(BlueprintCallable, category = "SpiderAI")
	//void SetDestination(FVector InDestination, float AcceptableDistance);

	virtual void Possess(APawn* InPawn) override;
	virtual void UnPossess() override;

	//bool IsArrvied(FVector InDestination);
};
