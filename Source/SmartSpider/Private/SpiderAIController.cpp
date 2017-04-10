// Fill out your copyright notice in the Description page of Project Settings.

#include "SmartSpider.h"
#include "SpiderAIController.h"
#include "SmartSpiderCharacter.h"

void ASpiderAIController::Possess(APawn* InPawn)
{
	SpiderCharacter = Cast<ASmartSpiderCharacter>(InPawn);
	if (!SpiderCharacter)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Spider controller can only possesss ASmartSpiderCharacter£¡"));
	}
	else
	{
		Super::Possess(InPawn);
	}
}

void ASpiderAIController::UnPossess()
{
	Super::UnPossess();

	SpiderCharacter = nullptr;
}

//void ASpiderAIController::SetDestination(FVector InDestination, float AcceptableDistance)
//{
//	Destination = InDestination;
//	AcceptableDistanceSq = AcceptableDistance * AcceptableDistance;
//}
//
//bool ASpiderAIController::MoveToDestination()
//{
//	if (IsArrvied(Destination)) return true;
//
//	FindPathForMoveRequest();
//
//	MoveTo();
//
//
//	FTraceResult TraceResult = SpiderCharacter->TraceEnv();
//	switch (TraceResult.SurfaceType)
//	{
//		default:
//			break;
//	}
//
//	return false;
//}

bool ASpiderAIController::FindReversePath(FVector Dest, float AcceptanceRadius, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	FAIMoveRequest MoveReq(Dest);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : DefaultNavigationFilterClass);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(true);

	FPathFindingQuery PFQuery;

	const bool bValidQuery = BuildPathfindingQuery(MoveReq, PFQuery);
	if (bValidQuery)
	{
		FNavPathSharedPtr Path;
		FindPathForMoveRequest(MoveReq, PFQuery, Path);
		if (Path->IsValid())
		{
			//Path->re
		}
	}

	return true; 
}

//
//bool ASpiderAIController::IsArrvied(FVector InDestination)
//{
//	if (!SpiderCharacter) return true;
//
//	return (SpiderCharacter->GetActorLocation() - InDestination).SizeSquared() < AcceptableDistanceSq;
//}


