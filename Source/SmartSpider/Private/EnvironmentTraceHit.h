#pragma once

#include "SmartSpider.h"
#include "Engine/EngineTypes.h"
#include "EnvironmentTraceHit.generated.h"

UENUM(BlueprintType)
enum class EEnvironmentSurface : uint8
{
	OnAir,
	Plane,
	Convex,
	Concave
};

UENUM(BlueprintType)
enum class EAcceptableDistance: uint8
{
	Equal = 0,
	GreaterThan,
	LessThan
};

USTRUCT(BlueprintType)
struct FAcceptableHitResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	uint32 bAcceptable:1;

	UPROPERTY(EditAnywhere)
	EAcceptableDistance AcceptableDistance;

	UPROPERTY(EditAnywhere)
	FHitResult HitResult;

	FAcceptableHitResult()
	{
		bAcceptable = false;
		AcceptableDistance = EAcceptableDistance::GreaterThan;
	}

	/* Test against hit result with impact point. */
	bool TestAgainstHitResult(float AcceptableDistanceSq, float Tolreance)
	{
		float ImpactDistanceSq = FVector::DistSquared(HitResult.TraceStart, HitResult.ImpactPoint);
		float AbsDeltaDistanceSq = FMath::Abs(ImpactDistanceSq - AcceptableDistanceSq);

		bAcceptable = false;
		if (AbsDeltaDistanceSq < Tolreance)
		{
			bAcceptable = true;
			AcceptableDistance = EAcceptableDistance::Equal;
		}
		else if (ImpactDistanceSq < AcceptableDistanceSq)
		{
			AcceptableDistance = EAcceptableDistance::LessThan;
		}
		else
		{
			AcceptableDistance = EAcceptableDistance::GreaterThan;
		}

		return bAcceptable;
	}
};