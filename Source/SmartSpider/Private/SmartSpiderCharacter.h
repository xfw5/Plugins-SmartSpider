// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "EnvironmentTraceHit.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SmartSpiderCharacter.generated.h"

/* Only draw tracing rays in case of editor. */
#if WITH_EDITOR
	#define ENV_TRACE_TYPE EDrawDebugTrace::ForOneFrame
#else
	#define ENV_TRACE_TYPE EDrawDebugTrace::None
#endif

USTRUCT(BlueprintType)
struct FTraceResult
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FAcceptableHitResult HitResultForward;

	UPROPERTY(EditAnywhere)
	FAcceptableHitResult HitResultBackward;

	UPROPERTY(EditAnywhere)
	FAcceptableHitResult HitResultBottom;

	UPROPERTY(EditAnywhere)
	EEnvironmentSurface SurfaceType;

	FTraceResult(FAcceptableHitResult Forward, FAcceptableHitResult Backward, FAcceptableHitResult bottom, EEnvironmentSurface TargetSurface)
	{
		HitResultForward = Forward;
		HitResultBackward = Backward;
		HitResultBottom = bottom;

		SurfaceType = TargetSurface;
	}

	FTraceResult()
	{
		HitResultForward = FAcceptableHitResult();
		HitResultBackward = FAcceptableHitResult();
		HitResultBottom = FAcceptableHitResult();

		SurfaceType = EEnvironmentSurface::Plane;
	}
};

/*
* Smart Spider climbing without surface limited.
*/
UCLASS(ClassGroup=Spider)
class SMARTSPIDER_API ASmartSpiderCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	/* Objects will tracing when query the world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Environment Tracing")
	TArray<TEnumAsByte<EObjectTypeQuery> > QueryObjectsType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Environment Tracing")
	uint32 bTraceComplex: 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Environment Tracing")
	TArray<AActor*> ActorsToIgnore;

	/* Interpolation speed when needs stick to surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Environment Tracing|Character Ability")
	float StickToSurfaceSpeed;

	/* How far spider can see. In square. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Environment Tracing|Sensor")
	float SightsDistanceSq;

	/* How far spider can hear. In square. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Environment Tracing|Sensor")
	float HearingDistanceSq;

	/* Force spider stick to surface at begin when spawn on the air. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Character Ability")
	uint32 bForceStickToSurfaceAtBegin: 1;

	/* Spider can only rotation along up vector. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Character Ability")
	float RotateRateInDegrees;

	/* Rotation rate when spider cross surface and transition to the desire surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Character Ability")
	float TransitionRateInDegrees;

	/* Override the default rotation behavior, and implement function @OnCustomRotationUpdate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, category = "Environment Tracing|Character Ability")
	uint32 bUseCustomRotationRate: 1;

	/* Slam offset when cross convex surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, category = "Environment Tracing|Character Ability")
	uint32 bForwardOffsetWhenCrossWithConvexSurface : 1;

	/* Should disable movement when spider cross the surface. Disable for get more nature behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, category = "Environment Tracing|Character Ability")
	uint32 bDisableMovementWhenTransition:1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, category = "Environment Tracing|Character Ability")
	uint32 bRotationToMovement: 1;

	/* Enable for more effective tracing and avoid stunning behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, category = "Environment Tracing|Character Ability")
	uint32 bTracingEnvWithHasVelocityOnly : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, category = "Environment Tracing|Character Ability")
	uint32 bStickToSurfaceIfOnAir : 1;

	/* 
	* Offset from eye position to actor location.(EyePosition = Offset + ActorLocation).
	* Mostly, sets as half of character hight with a little offset. 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Offset")
	float TracingOffset_Eye;

	/* Offset from surface transition detected location to actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Offset")
	float TracingOffset_Bottom;

	/* The second surface transition detected offset relative to @TracingOffset_Bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Offset")
	float TracingOffset_BottomAssistor;

	/* How many degrees from actor down vector to forward&backward detected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Offset")
	float TracingDegreesOffset_ForwardBackward;

	/* How many degrees from actor down vector to left&right detected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Offset")
	float TracingDegreesOffset_LeftRight;

	/* Stick to surface detect distance to avoid actor floating when walking on surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Distance")
	float TracingDistance_Stick;

	/* Surface tracing distance allowed when tracing environment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Distance")
	float TracingDistance_Surface;

	/* Tolerance(in square) for test against with tracing distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Tracing Distance")
	float TracingDistanceTestToleranceSq;

#if WITH_EDITORONLY_DATA // Debug Only with editor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_Forward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_Backward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_Bottom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_BottomAssistor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_LeftSide;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Environment Tracing|Debug")
	FColor TracingColor_RightSide;

	UPROPERTY(VisibleDefaultsOnly)
	class USphereComponent* SightsSensorRadius;

	UPROPERTY(VisibleDefaultsOnly)
	class USphereComponent* HearingSensorRadius;
#endif

	/* Acceptable distance square when transition to a new surface. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, category = "Environment Tracing|Runtime")
	float AcceptableDistanceSq_TransitionDetected;

	/* Acceptable distance square when tracing environment with surface. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, category = "Environment Tracing|Runtime")
	float AcceptableDistanceSq_SurfaceDetected;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, category = "Environment Tracing|Runtime")
	EEnvironmentSurface LastSurfaceType;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, category = "Environment Tracing|Runtime")
	FVector SurfaceNormal;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, category = "Environment Tracing|Runtime")
	uint32 bNeedStickToSurface : 1;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, category = "Runtime|Animation")
	uint32 bDeath : 1;

public:
	// Sets default values for this character's properties
	ASmartSpiderCharacter();


	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	void InitTracingArgs();

	FVector CalcSurfaceTracingDistance();
	FORCEINLINE bool DoLineTrace(FHitResult& OutHit, const FVector Start, const FVector End, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool IsOnAir(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool IsSurfacePlane(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool IsSurfaceConvex(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool IsSurfaceConcave(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	EEnvironmentSurface OnAirHandle(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);
	EEnvironmentSurface OnSurfaceHandlePlane(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);
	EEnvironmentSurface OnSurfaceHandleConvex(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);
	EEnvironmentSurface OnSurfaceHandleConcave(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool IsStickAndAlignWithSurface(FVector QueryPosition, FVector InSurfaceNornal);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	FVector CalcDesireStickLocation(FVector QueryLocation, FVector InSurfaceNornal);
	void RotationToMovement(float DeltaTime);

	FORCEINLINE float GetFeetOffset() const { return GetCharacterMovement()->UpdatedComponent->Bounds.BoxExtent.Z; }
public:	
	UFUNCTION(BlueprintImplementableEvent)
	void OnCrossSurfaceBegin();

	UFUNCTION(BlueprintImplementableEvent)
	void OnCrossSurfaceEnd();

	UFUNCTION(BlueprintImplementableEvent)
	void OnSurfaceChange(EEnvironmentSurface LastSurface, EEnvironmentSurface NewSurface, FVector NewSurfaceNormal);
	
	/* Make sure @bUseCustomRotationRate was checked, and implement the custom rotation logic here. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnCustomRotationUpdate();

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	void TraceEnvHandle(float DeltaTime);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	FTraceResult TraceEnv();

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	void StickToSurface(FVector InSurfaceNormal);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	void TransitionToSurface(FVector TransitionLocation, FVector InSurfaceNormal);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	void UpdateRotationRate();

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	EEnvironmentSurface GetSurfaceType(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	FORCEINLINE EEnvironmentSurface OnSurfaceHandle(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool TraceForward(FAcceptableHitResult& OutHitResult);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool TraceBackward(FAcceptableHitResult& OutHitResult);
	
	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool TraceCenter(FHitResult& OutHitResult);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool TraceBottom(FAcceptableHitResult& OutHitResult);

	UFUNCTION(BlueprintCallable, category = "SmartSpider")
	bool TraceBottomAssistor(FAcceptableHitResult& OutHitResult);

	FORCEINLINE FVector GetTraceDirForward() const { return (-GetActorUpVector()).RotateAngleAxis(-TracingDegreesOffset_ForwardBackward, GetActorRightVector()); }
	FORCEINLINE FVector GetTraceDirBackward() const { return (-GetActorUpVector()).RotateAngleAxis(TracingDegreesOffset_ForwardBackward, GetActorRightVector()); }
	FORCEINLINE FVector GetTraceDirLeft() const { return (-GetActorUpVector()).RotateAngleAxis(-TracingDegreesOffset_LeftRight, GetActorForwardVector()); }
	FORCEINLINE FVector GetTraceDirRight() const { return (-GetActorUpVector()).RotateAngleAxis(TracingDegreesOffset_LeftRight, GetActorForwardVector()); }

	FORCEINLINE FVector GetEyePosition() const { return TracingOffset_Eye * GetActorUpVector() + GetActorLocation(); }
	FORCEINLINE FVector GetBottomLocation() const { return GetActorLocation() + GetActorForwardVector() * TracingOffset_Bottom; }
	FORCEINLINE FVector GetBottomAssistorLocation() const { return GetActorLocation() + GetActorForwardVector() * (TracingOffset_Bottom + TracingOffset_BottomAssistor); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

FORCEINLINE EEnvironmentSurface ASmartSpiderCharacter::OnSurfaceHandle(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	switch (Surface)
	{
		case EEnvironmentSurface::OnAir:
			return OnAirHandle(DeltaTime, Surface, Forward, Backward, bottom);

		case EEnvironmentSurface::Plane:
			return OnSurfaceHandlePlane(DeltaTime, Surface, Forward, Backward, bottom);
		
		case EEnvironmentSurface::Concave:
			return OnSurfaceHandleConcave(DeltaTime, Surface, Forward, Backward, bottom);

		case EEnvironmentSurface::Convex:
			return OnSurfaceHandleConvex(DeltaTime, Surface, Forward, Backward, bottom);

		default:
			break;
	}

	return Surface;
}

FORCEINLINE bool ASmartSpiderCharacter::DoLineTrace(FHitResult& OutHit, const FVector Start, const FVector End, FLinearColor TraceColor /* = FLinearColor::Red */, FLinearColor TraceHitColor /* = FLinearColor::Green */)
{
	return UKismetSystemLibrary::LineTraceSingleForObjects(this, Start, End, QueryObjectsType, bTraceComplex, ActorsToIgnore, ENV_TRACE_TYPE, OutHit, true, TraceColor, TraceHitColor, 0.0f);
}