// Fill out your copyright notice in the Description page of Project Settings.

#include "SmartSpider.h"
#include "SmartSpiderCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"


// Sets default values
ASmartSpiderCharacter::ASmartSpiderCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	TracingOffset_Eye = 15;
	TracingOffset_Bottom = -5;
	TracingOffset_BottomAssistor = -3;

	TracingDegreesOffset_ForwardBackward = 45;
	TracingDegreesOffset_LeftRight = 45;
	TracingDistance_Stick = 50;
	TracingDistance_Surface = 100;
	TracingDistanceTestToleranceSq = 9; // 3 * 3

	bStickToSurfaceIfOnAir = true;
	bForwardOffsetWhenCrossWithConvexSurface = true;
	bTracingEnvWithHasVelocityOnly = true;
	bForceStickToSurfaceAtBegin = true;
	bRotationToMovement = true;
	bDisableMovementWhenTransition = true;
	bUseCustomRotationRate = false;
	RotateRateInDegrees = 540;
	TransitionRateInDegrees = 540;
	StickToSurfaceSpeed = 50;

	TracingColor_Forward = FColor::Red;
	TracingColor_Backward = FColor::Green;
	TracingColor_Center = FColor::Blue;
	TracingColor_Bottom = FColor::Yellow;
	TracingColor_BottomAssistor = FColor::Cyan;
	TracingColor_LeftSide = FColor::Magenta;
	TracingColor_RightSide = FColor::Orange;

	QueryObjectsType.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	bTraceComplex = false;

	SightsDistanceSq = 1000 * 1000;
	HearingDistanceSq = 1100 * 1100;

#if WITH_EDITORONLY_DATA
	SightsSensorRadius = CreateEditorOnlyDefaultSubobject<USphereComponent>("SightsRadius");
	SightsSensorRadius->InitSphereRadius(FMath::Sqrt(SightsDistanceSq));

	HearingSensorRadius = CreateEditorOnlyDefaultSubobject<USphereComponent>("HearingRadius");
	HearingSensorRadius->InitSphereRadius(FMath::Sqrt(HearingDistanceSq));
#endif
}

void ASmartSpiderCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTracingEnvWithHasVelocityOnly && GetVelocity().SizeSquared() > 0)
	{
		TraceEnvHandle(DeltaSeconds);
	}
}

// Called when the game starts or when spawned
void ASmartSpiderCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitTracingArgs();

	if (bForceStickToSurfaceAtBegin)
	{
		FHitResult HitResult;
		FVector ActorLocation = GetActorLocation();
		FVector EndLocation = ActorLocation - GetActorUpVector() * 100000000;
		if (DoLineTrace(HitResult, ActorLocation, EndLocation, TracingColor_Center))
		{
			TransitionToSurface(CalcDesireStickLocation(HitResult.ImpactPoint, HitResult.ImpactNormal), HitResult.ImpactNormal);
		}
	}
}

void ASmartSpiderCharacter::InitTracingArgs()
{
	AcceptableDistanceSq_SurfaceDetected = CalcSurfaceTracingDistance().SizeSquared();
	AcceptableDistanceSq_TransitionDetected = (GetCharacterMovement()->GetActorFeetLocation() - GetActorLocation()).SizeSquared();
}

FVector ASmartSpiderCharacter::CalcSurfaceTracingDistance()
{
	FVector EyePosition = GetEyePosition();
	FVector EndPosition = EyePosition + GetTraceDirForward() * TracingDistance_Surface;
	FVector PlaneLocation = GetCharacterMovement()->GetActorFeetLocation();
	FPlane Plane(PlaneLocation, GetActorUpVector());

	FVector IntersectPoint;
	float t;
	if (UKismetMathLibrary::LinePlaneIntersection(EyePosition, EndPosition, Plane, t, IntersectPoint))
	{
		return (IntersectPoint - EyePosition);
	}

	return FVector::ZeroVector;
}

bool ASmartSpiderCharacter::IsSurfaceConvex(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	//FAcceptableHitResult AssistorHitResult;
	//TraceBottomAssistor(AssistorHitResult);

	return bottom.AcceptableDistance == EAcceptableDistance::GreaterThan; //AssistorHitResult.bAcceptable;
}

bool ASmartSpiderCharacter::IsSurfaceConcave(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	return Forward.AcceptableDistance == EAcceptableDistance::LessThan;
}

bool ASmartSpiderCharacter::IsOnAir(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	return Forward.AcceptableDistance == EAcceptableDistance::GreaterThan && 
			Backward.AcceptableDistance == EAcceptableDistance::GreaterThan && 
			bottom.AcceptableDistance == EAcceptableDistance::GreaterThan;
}

bool ASmartSpiderCharacter::IsSurfacePlane(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	FAcceptableHitResult AssistorHitResult;
	TraceBottomAssistor(AssistorHitResult);

	return AssistorHitResult.HitResult.bBlockingHit && 
			bottom.HitResult.bBlockingHit && 
			AssistorHitResult.HitResult.ImpactNormal.Equals(bottom.HitResult.ImpactNormal, 0.01f);
}

EEnvironmentSurface ASmartSpiderCharacter::OnSurfaceHandlePlane(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	SurfaceNormal = bottom.HitResult.bBlockingHit? bottom.HitResult.ImpactNormal: FVector::UpVector;
	bNeedStickToSurface = true;

	return Surface;
}

EEnvironmentSurface ASmartSpiderCharacter::OnSurfaceHandleConvex(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	OnCrossSurfaceBegin();

	if (bForwardOffsetWhenCrossWithConvexSurface)
	{
		AddActorLocalOffset(-GetActorForwardVector()*TracingOffset_BottomAssistor, false, nullptr, ETeleportType::TeleportPhysics);
	}

	FRotator LocalRotation = UKismetMathLibrary::MakeRotator(0, -TransitionRateInDegrees * DeltaTime, 0);
	AddActorLocalRotation(LocalRotation, false, nullptr, ETeleportType::TeleportPhysics);

	return Surface;
}

EEnvironmentSurface ASmartSpiderCharacter::OnSurfaceHandleConcave(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	//SurfaceNormal = Forward.HitResult.ImpactNormal;
	TransitionToSurface(GetActorLocation(), Forward.HitResult.ImpactNormal);
	return Surface;
}

EEnvironmentSurface ASmartSpiderCharacter::OnAirHandle(float DeltaTime, EEnvironmentSurface& Surface, FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	if (bStickToSurfaceIfOnAir)
	{
		StickToSurface(bottom.HitResult.bBlockingHit ? bottom.HitResult.ImpactNormal : Forward.HitResult.bBlockingHit ? Forward.HitResult.ImpactNormal : FVector::UpVector);
	}

	return Surface;
}

bool ASmartSpiderCharacter::IsStickAndAlignWithSurface(FVector QueryPosition, FVector InSurfaceNornal)
{
	if (!GetActorUpVector().Equals(InSurfaceNornal)) return false;

	FVector ActorLocation = GetActorLocation();
	float DistanceToActor = FVector::Dist(QueryPosition, ActorLocation);

	return FMath::IsNearlyEqual(DistanceToActor, GetFeetOffset(), .1f);
}

FVector ASmartSpiderCharacter::CalcDesireStickLocation(FVector QueryLocation, FVector InSurfaceNornal)
{
	return QueryLocation + InSurfaceNornal * GetFeetOffset();
}

void ASmartSpiderCharacter::RotationToMovement(float DeltaTime)
{
	//FRotator Rotation = GetActorRotation();
	//GetCharacterMovement()->ComputeOrientToMovementRotation(Rotation, DeltaTime, );

	FVector MovementVel = GetVelocity();
	if (MovementVel.Equals(FVector::ZeroVector)) return;

	FTransform Trans = GetActorTransform();
	FVector LocalMovementDir = UKismetMathLibrary::InverseTransformDirection(Trans, MovementVel.GetSafeNormal());
	FVector DesireDir = FVector(LocalMovementDir.X, LocalMovementDir.Y, 0);

	FVector InterpDir = UKismetMathLibrary::VLerp(FVector::ForwardVector, DesireDir, DeltaTime * RotateRateInDegrees);
	FVector WorldDir = UKismetMathLibrary::TransformDirection(Trans, InterpDir.GetSafeNormal());
	SetActorRotation(WorldDir.Rotation());


	//FQuat Rotation = Trans.GetRotation();
	//FQuat DesireRotation = FQuat::FastLerp(Rotation, FQuat(MovementVel.Rotation()), RotateRateInDegrees);
	//SetActorRotation(DesireRotation);
}

EEnvironmentSurface ASmartSpiderCharacter::GetSurfaceType(FAcceptableHitResult& Forward, FAcceptableHitResult& Backward, FAcceptableHitResult& bottom)
{
	if (IsSurfaceConcave(Forward, Backward, bottom)) return EEnvironmentSurface::Concave;

	if (IsSurfaceConvex(Forward, Backward, bottom)) return EEnvironmentSurface::Convex;

	return EEnvironmentSurface::Plane;
}

void ASmartSpiderCharacter::TraceEnvHandle(float DeltaTime)
{
	bNeedStickToSurface = false;
	SurfaceNormal = GetActorUpVector();

	FAcceptableHitResult HitResultForwarwd;
	TraceForward(HitResultForwarwd);

	FAcceptableHitResult HitResultBackward;
	TraceBackward(HitResultBackward);

	FAcceptableHitResult HitResultBottom;
	TraceBottom(HitResultBottom);

	EEnvironmentSurface CurrentSurface = GetSurfaceType(HitResultForwarwd, HitResultBackward, HitResultBottom);
	CurrentSurface = OnSurfaceHandle(DeltaTime, CurrentSurface, HitResultForwarwd, HitResultBackward, HitResultBottom);
	if (CurrentSurface != LastSurfaceType)
	{
		OnSurfaceChange(LastSurfaceType, CurrentSurface, SurfaceNormal);
		if (LastSurfaceType == EEnvironmentSurface::Convex) 
		{
			OnCrossSurfaceEnd();
		}
	}

	LastSurfaceType = CurrentSurface;

	if (SurfaceNormal.Equals(FVector::UpVector))
	{
		Cast<UPrimitiveComponent>(GetRootComponent())->SetEnableGravity(true);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	else
	{
		Cast<UPrimitiveComponent>(GetRootComponent())->SetEnableGravity(false);
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	if (bNeedStickToSurface)
	{
		StickToSurface(SurfaceNormal);
	}

	if (bUseCustomRotationRate)
	{
		OnCustomRotationUpdate();
	}
	else
	{
		UpdateRotationRate();
	}

	if (bRotationToMovement && LastSurfaceType == EEnvironmentSurface::Plane) 
	{
		RotationToMovement(DeltaTime);
	}
}

FTraceResult ASmartSpiderCharacter::TraceEnv()
{
	FAcceptableHitResult HitResultForwarwd;
	TraceForward(HitResultForwarwd);

	FAcceptableHitResult HitResultBackward;
	TraceBackward(HitResultBackward);

	FAcceptableHitResult HitResultBottom;
	TraceBottom(HitResultBottom);

	EEnvironmentSurface SurfaceType = GetSurfaceType(HitResultForwarwd, HitResultBackward, HitResultBottom);
	return FTraceResult(HitResultForwarwd, HitResultBackward, HitResultBottom, SurfaceType);
}

void ASmartSpiderCharacter::StickToSurface(FVector InSurfaceNormal)
{
	FHitResult HitResult;
	if (TraceCenter(HitResult))
	{
		if (!IsStickAndAlignWithSurface(HitResult.ImpactPoint, InSurfaceNormal))
		{
			FVector UpDir = GetActorUpVector();
			FVector InterpNormal = UKismetMathLibrary::VInterpTo(UpDir, InSurfaceNormal, UGameplayStatics::GetWorldDeltaSeconds(this), StickToSurfaceSpeed);
			TransitionToSurface(CalcDesireStickLocation(HitResult.ImpactPoint, InterpNormal), InterpNormal);
		}
	}
}

void ASmartSpiderCharacter::TransitionToSurface(FVector TransitionLocation, FVector InSurfaceNormal)
{
	FVector RightDir = GetActorRightVector();
	FVector ForwardDir = FVector::CrossProduct(RightDir, InSurfaceNormal);
	FVector DesireRightDir = FVector::CrossProduct(InSurfaceNormal, ForwardDir);

	FRotator Rotation = UKismetMathLibrary::MakeRotationFromAxes(ForwardDir, DesireRightDir, InSurfaceNormal);
	FTransform DesireTransform = FTransform(Rotation, TransitionLocation);
	SetActorTransform(DesireTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void ASmartSpiderCharacter::UpdateRotationRate()
{
	FVector Up = GetActorUpVector()* RotateRateInDegrees;
	GetCharacterMovement()->RotationRate = Up.Rotation();
}

bool ASmartSpiderCharacter::TraceForward(FAcceptableHitResult& OutHitResult)
{
	FVector EyePos = GetEyePosition();
	FVector EndPos = EyePos + GetTraceDirForward() * TracingDistance_Surface;
	if (DoLineTrace(OutHitResult.HitResult, EyePos, EndPos, TracingColor_Forward))
	{
		OutHitResult.TestAgainstHitResult(AcceptableDistanceSq_SurfaceDetected, TracingDistanceTestToleranceSq);
	}

	return OutHitResult.HitResult.bBlockingHit;
}

bool ASmartSpiderCharacter::TraceBackward(FAcceptableHitResult& OutHitResult)
{
	FVector EyePos = GetEyePosition();
	FVector EndPos = EyePos + GetTraceDirBackward() * TracingDistance_Surface;
	if (DoLineTrace(OutHitResult.HitResult, EyePos, EndPos, TracingColor_Backward))
	{
		OutHitResult.TestAgainstHitResult(AcceptableDistanceSq_SurfaceDetected, TracingDistanceTestToleranceSq);
	}

	return OutHitResult.HitResult.bBlockingHit;
}

bool ASmartSpiderCharacter::TraceCenter(FHitResult& OutHitResult)
{
	FVector ActorLocation = GetActorLocation();
	FVector EndLocation = ActorLocation - GetActorUpVector() * TracingDistance_Stick;
	return DoLineTrace(OutHitResult, ActorLocation, EndLocation, TracingColor_Center);
}

bool ASmartSpiderCharacter::TraceBottom(FAcceptableHitResult& OutHitResult)
{
	FVector BottomLocation = GetBottomLocation();
	FVector EndLocation = BottomLocation - GetActorUpVector() * TracingDistance_Surface;
	if (DoLineTrace(OutHitResult.HitResult, BottomLocation, EndLocation, TracingColor_Bottom))
	{
		OutHitResult.TestAgainstHitResult(AcceptableDistanceSq_SurfaceDetected, TracingDistanceTestToleranceSq);
	}

	return OutHitResult.HitResult.bBlockingHit;
}

bool ASmartSpiderCharacter::TraceBottomAssistor(FAcceptableHitResult& OutHitResult)
{
	FVector BottomLocation = GetBottomAssistorLocation();
	FVector EndLocation = BottomLocation - GetActorUpVector() * TracingDistance_Surface;
	if (DoLineTrace(OutHitResult.HitResult, BottomLocation, EndLocation, TracingColor_BottomAssistor))
	{
		OutHitResult.TestAgainstHitResult(AcceptableDistanceSq_SurfaceDetected, TracingDistanceTestToleranceSq);
	}

	return OutHitResult.HitResult.bBlockingHit;
}

#if WITH_EDITOR
void ASmartSpiderCharacter::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.MemberProperty)
	{
		FString PropertyName = PropertyChangedEvent.MemberProperty->GetName();
		if (PropertyName.Equals(GET_MEMBER_NAME_STRING_CHECKED(ASmartSpiderCharacter, SightsDistanceSq)))
		{
			SightsSensorRadius->SetSphereRadius(FMath::Sqrt(SightsDistanceSq));
		}else if (PropertyName.Equals(GET_MEMBER_NAME_STRING_CHECKED(ASmartSpiderCharacter, HearingDistanceSq)))
		{
			HearingSensorRadius->SetSphereRadius(FMath::Sqrt(HearingDistanceSq));
		}
	}
}

#endif