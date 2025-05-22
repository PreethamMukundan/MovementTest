// Fill out your copyright notice in the Description page of Project Settings.


#include "movement_zero/Public/ZeroCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "movement_zero/movement_zeroCharacter.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Zero_ZiplineActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SplineComponent.h"

// Helper Macros
#if 1
float MacroDuration = 2.f;
#define ZLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::Yellow, x);
#define ZPOINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration);
#define ZLINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration);
#define ZCAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, c, !MacroDuration, MacroDuration);
#else
#define ZLOG(x)
#define ZPOINT(x, c)
#define ZLINE(x1, x2, c)
#define ZCAPSULE(x, c)
#endif



#pragma region SavedMove
UZeroCharacterMovementComponent::FSavedMove_Zero::FSavedMove_Zero()
{
}

bool UZeroCharacterMovementComponent::FSavedMove_Zero::CanCombineWith(const FSavedMovePtr& NewMove,ACharacter* InCharacter, float MaxDelta) const
{
	//Getting the new move 
	FSavedMove_Zero* NewZeroMove = static_cast<FSavedMove_Zero*>(NewMove.Get());

	//check if the new move is same as the old one
	if(Saved_bWantsToSprint != NewZeroMove->Saved_bWantsToSprint)
	{
		return false;
	}
	if(Saved_bWantsToDash != NewZeroMove->Saved_bWantsToDash)
	{
		return false;
	}
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

//Just clear saved move(FSavedMove_Zero)
void UZeroCharacterMovementComponent::FSavedMove_Zero::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToSprint = 0;
	Saved_bPrevWantsToCrouch = 0;
	Saved_bWantsToDash = 0;
	Saved_bTransitionFinished =0;
	Saved_bPressedZeroJump =0;
	Saved_bHadAnimRootMotion =0;
}


//Compress saved bools into a 8bit uint
uint8 UZeroCharacterMovementComponent::FSavedMove_Zero::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if(Saved_bWantsToSprint)
	{
		Result |= FLAG_Sprint;
	}
	if(Saved_bWantsToDash)
	{
		Result |= FLAG_Dash;
	}
	if(Saved_bPressedZeroJump)
	{
		Result |= FLAG_JumpPressed;
	}
	return Result;
}


//sets saved variables in the SaveMoveZero using the safe bools
void UZeroCharacterMovementComponent::FSavedMove_Zero::SetMoveFor(ACharacter* C, float InDeltaTime,	FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UZeroCharacterMovementComponent* CharMovementComp =Cast<UZeroCharacterMovementComponent>(C->GetCharacterMovement());
	
	Saved_bWantsToSprint = CharMovementComp->Safe_bWantsToSprint;
	Saved_bPrevWantsToCrouch = CharMovementComp->Safe_bPrevWantsToCrouch;
	Saved_bWantsToDash = CharMovementComp->Safe_bWantsToDash;
	Saved_bPressedZeroJump = CharMovementComp->ZeroCharacter_Owner->bPressedZeroJump;

	Saved_bHadAnimRootMotion = CharMovementComp->Safe_bHadAnimRootMotion;
	Saved_bTransitionFinished = CharMovementComp->Safe_bTransitionFinished;

	
}

//Opposite of setmove
void UZeroCharacterMovementComponent::FSavedMove_Zero::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UZeroCharacterMovementComponent* CharMovementComp =Cast<UZeroCharacterMovementComponent>(C->GetCharacterMovement());
	
	CharMovementComp->Safe_bWantsToSprint = Saved_bWantsToSprint ;
	CharMovementComp->Safe_bPrevWantsToCrouch = Saved_bPrevWantsToCrouch;
	CharMovementComp->Safe_bWantsToDash = Saved_bWantsToDash;

	CharMovementComp->ZeroCharacter_Owner->bPressedZeroJump = Saved_bPressedZeroJump;

	CharMovementComp->Safe_bHadAnimRootMotion = Saved_bHadAnimRootMotion;
	CharMovementComp->Safe_bTransitionFinished = Saved_bTransitionFinished;

	
}
#pragma endregion

#pragma region PredictionData
UZeroCharacterMovementComponent::FNetworkPredictionData_Client_Zero::FNetworkPredictionData_Client_Zero(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
}
//change to make our fSavedMove class is used 
FSavedMovePtr UZeroCharacterMovementComponent::FNetworkPredictionData_Client_Zero::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Zero());
}
#pragma endregion

#pragma region CharMoveOverrides
UZeroCharacterMovementComponent::UZeroCharacterMovementComponent()
{
	Sprint_MaxSpeed = 1200;
	Walk_MaxSpeed = 600;
	NavAgentProps.bCanCrouch = true;

	SlideFriction=1.3f;
	SlideMinSpeed=350;
	SlideEnterImpulse=500;
	SlideGravityForce =5000;

	DashImpulse = 1000.0f;
	DashCoolDownDuration =1.f;
	AuthDashCoolDownDuration =0.9f;

	WallBounceImpluse = 1000.0f;

	MantleMaxDistance = 200.0f;
	MantleReachHeight =50.0f;
	MinMantleDepth = 30.0f;
	MantleMinWallSteepnessAngle = 75.0f;
	MantleMaxSurfaceAngle = 40.0f;
	MantleMaxAlignmentAngle =45.0f;
}

void UZeroCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ZeroCharacter_Owner = Cast<Amovement_zeroCharacter>(GetOwner());
}

void UZeroCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Zero::FLAG_Sprint) != 0;
	Safe_bWantsToDash = (Flags & FSavedMove_Zero::FLAG_Dash) != 0;
}

void UZeroCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if(MovementMode == MOVE_Walking)
	{
		if(Safe_bWantsToSprint)
		{
			MaxWalkSpeed = Sprint_MaxSpeed;
		}
		else
		{
			MaxWalkSpeed = Walk_MaxSpeed;
		}
	}

	Safe_bPrevWantsToCrouch = bWantsToCrouch;
}

//Crouch is being handled here so slide can work before crouch overwrites it
void UZeroCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	//WallJump ? Mantle
	if(ZeroCharacter_Owner->bPressedZeroJump)
	{
		if(TryMantle())
		{
			ZeroCharacter_Owner->StopJumping();
		}
		else if(TryWallBounce())
		{
			ZeroCharacter_Owner->StopJumping();
		}
		else
		{
			ZLOG("RevertToJumping");
			ZeroCharacter_Owner->bPressedZeroJump = false;
			CharacterOwner->bPressedJump = true;
			CharacterOwner->CheckJumpInput(DeltaSeconds);
		}
	}
	if(ZeroCharacter_Owner->ZeroJumpHoldTIme > ZiplineMinKeyPressTime && ZeroCharacter_Owner->bStillJumpKeyDown)
	{
		if(TryZipLine())
		{
			ZeroCharacter_Owner->StopJumping();
			//StartZipline();
			SetMovementMode(MOVE_Custom,CMOVE_Zipline);
			if(!CharacterOwner->HasAuthority())
			{
				if(ZiplineActorRef)
				{
					Server_EnterZipline(ZiplineActorRef->GetZiplineComponent(),bZiplineMoveingToEnd);
				}
			}
		}
	}
	//Dash
	bool bAuthProxy = CharacterOwner->HasAuthority() && !CharacterOwner->IsLocallyControlled();
	if(Safe_bWantsToDash && CanDash())
	{
		if(!bAuthProxy || GetWorld()->GetTimeSeconds() - DashStartTime > AuthDashCoolDownDuration)
		{
			PerformDash();
			Safe_bWantsToDash = false;
			Proxy_bDashStart = !Proxy_bDashStart;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Tried Cheating Dash"));
		}
	}
	
	//slide
	if(MovementMode == MOVE_Walking && bWantsToCrouch && Safe_bPrevWantsToCrouch)
	{
		FHitResult PotenialSurface;
		if(Velocity.SizeSquared() > pow(SlideMinSpeed,2) && GetSlideSurface(PotenialSurface))
		{
			//EnterSlide();
			SetMovementMode(MOVE_Custom,CMOVE_Slide);
			return;
		}
	}
	if(IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
	}
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UZeroCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
	

	if (GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		Velocity = FVector::ZeroVector;
		SetMovementMode(MOVE_Walking);
		RemoveRootMotionSourceByID(TransitionRMS_ID);
		Safe_bTransitionFinished = true;
	}

	
	Safe_bHadAnimRootMotion = HasAnimRootMotion();
}

//handles custom movement modes
void UZeroCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	case CMOVE_Zipline:
		PhysZipline(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp,Fatal,TEXT("InvalidMovement MOde"));
	}
}

void UZeroCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
	uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Slide) ExitSlide();
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Zipline) ExitZipline();
	
	if (IsCustomMovementMode(CMOVE_Slide)) EnterSlide();
	if (IsCustomMovementMode(CMOVE_Zipline)) EnterZipline();

	if(D_ChangeInCustomMovementMode.IsBound())
	{
		D_ChangeInCustomMovementMode.Broadcast();
	}
}

bool UZeroCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if ( CharacterOwner && CharacterOwner->CanJump() )
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || !FMath::IsNearlyEqual(FMath::Abs(GetGravitySpaceZ(PlaneConstraintNormal)), 1.f))
		{
			
			const bool bFirstJump = (CharacterOwner->JumpCurrentCountPreJump == 0);

			if (bFirstJump || bDontFallBelowJumpZVelocityDuringJump)
			{
				if (HasCustomGravity())
				{
					SetGravitySpaceZ(Velocity, FMath::Max<FVector::FReal>(GetGravitySpaceZ(Velocity), JumpZVelocity));
				}
				else
				{
					Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, JumpZVelocity);
				}
			}
			
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	
	return false;
}

bool UZeroCharacterMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
}

bool UZeroCharacterMovementComponent::CanCrouchInCurrentState() const
{
	if (!CanEverCrouch())
	{
		return false;
	}

	return (IsFalling() || IsMovingOnGround()) && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}
#pragma endregion 

#pragma region Slide
void UZeroCharacterMovementComponent::EnterSlide()
{
	bWantsToCrouch = true;
	Velocity += Velocity.GetSafeNormal2D() * SlideEnterImpulse;
	
}

void UZeroCharacterMovementComponent::ExitSlide()
{
	bWantsToCrouch = false;

	FQuat NewRot = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(),FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector,NewRot,true,Hit);
}
//takes care of the movement in slide movement mode
void UZeroCharacterMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{
	//delta time is not less than the min tick time 
	if(DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	//Exit if cant slide
	FHitResult SurfaceHit;
	if(!GetSlideSurface(SurfaceHit) || Velocity.SizeSquared() < pow(SlideMinSpeed,2))
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(DeltaTime,Iterations);//starts a new physics in the same frame
		return;
	}

	//Surface Gravity
	Velocity += SlideGravityForce * FVector::DownVector * DeltaTime;

	//Strafe 
	if(FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(),UpdatedComponent->GetRightVector())) >  .5)
	{
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector());//only allow left or right movement
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}

	//Calc velocity
	if(!HasRootMotionSources() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(DeltaTime,SlideFriction,true,GetMaxBrakingDeceleration());
	}
	ApplyRootMotionToVelocity(DeltaTime);


	//Perform Move
	Iterations++;
	bJustTeleported = false;


	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FQuat OldRot = UpdatedComponent->GetComponentRotation().Quaternion();
	FHitResult Hit(1.f);
	FVector Adjusted = Velocity * DeltaTime;
	FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity,SurfaceHit.Normal).GetSafeNormal();
	FQuat NewRot =FRotationMatrix::MakeFromXZ(VelPlaneDir,SurfaceHit.Normal).ToQuat();
	SafeMoveUpdatedComponent(Adjusted,NewRot,true,Hit);//function which moves the character

	if(Hit.Time <1.f)
	{
		HandleImpact(Hit,DeltaTime,Adjusted);
		SlideAlongSurface(Adjusted,(1.f - Hit.Time),Hit.Normal,Hit,true);
	}

	FHitResult NewSurfaceHit;
	if(!GetSlideSurface(NewSurfaceHit)|| Velocity.SizeSquared() < pow(SlideMinSpeed,2))
	{
		SetMovementMode(MOVE_Walking);
	}
	//Update ongoing Velocity and Accer
	if(!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) /DeltaTime;
	}
}

bool UZeroCharacterMovementComponent::GetSlideSurface(FHitResult& OutHit) const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	return GetWorld()->LineTraceSingleByProfile(OutHit,Start, End,ProfileName,ZeroCharacter_Owner->GetIgnoreCharacterParams());
}


#pragma endregion

#pragma region Dash
void UZeroCharacterMovementComponent::OnDashCoolDownFinished()
{
	Safe_bWantsToDash = true;
}
bool UZeroCharacterMovementComponent::CanDash() const
{
	//can change later to add stamina 
	return true;
}

void UZeroCharacterMovementComponent::PerformDash()
{
	DashStartTime = GetWorld()->GetTimeSeconds();

	FVector DashDirection = (Acceleration.IsNearlyZero() ? UpdatedComponent->GetForwardVector() : Acceleration).GetSafeNormal2D();
	
	Velocity = DashImpulse * (DashDirection + FVector::UpVector * 0.1f);

	FQuat NewRot = FRotationMatrix::MakeFromXZ(DashDirection,FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector,NewRot,false,Hit);

	SetMovementMode(MOVE_Falling);

	if(DashStartDelegate.IsBound())
	{
		DashStartDelegate.Broadcast();
	}
}

void UZeroCharacterMovementComponent::OnRep_DashStart()
{
	if(Proxy_bDashStart)
	{
		if(DashStartDelegate.IsBound())
		{
			DashStartDelegate.Broadcast();
		}
	}
}
#pragma endregion

#pragma region Mantle
void UZeroCharacterMovementComponent::OnRep_ShortMantle()
{
}

void UZeroCharacterMovementComponent::OnRep_TallMantle()
{
}

bool UZeroCharacterMovementComponent::TryMantle()
{
	if (!(IsMovementMode(MOVE_Walking) && !IsCrouching()) && !IsMovementMode(MOVE_Falling)) return false;

	// Helper Variables
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector Fwd = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = ZeroCharacter_Owner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2+ MantleReachHeight;
	float CosMMWSA = FMath::Cos(FMath::DegreesToRadians(MantleMinWallSteepnessAngle));
	float CosMMSA = FMath::Cos(FMath::DegreesToRadians(MantleMaxSurfaceAngle));
	float CosMMAA = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));


ZLOG("Starting Mantle Attempt")

	// Check Front Face
	FHitResult FrontHit;
	float CheckDistance = FMath::Clamp(Velocity | Fwd, CapR() + 30, MantleMaxDistance);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);
	for (int i = 0; i < 6; i++)
	{
		ZLINE(FrontStart, FrontStart + Fwd * CheckDistance, FColor::Red)
		if (GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + Fwd * CheckDistance, "BlockAll", Params)) break;
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / 5;
	}
	if (!FrontHit.IsValidBlockingHit()) return false;
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	//checking wall angle and player facing the wall
	if (FMath::Abs(CosWallSteepnessAngle) > CosMMWSA || (Fwd | -FrontHit.Normal) < CosMMAA)
	{
		return false;
	}
ZPOINT(FrontHit.Location, FColor::Red);


	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + Fwd + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
ZLINE(TraceStart, FrontHit.Location + Fwd, FColor::Orange)
		if (!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + Fwd, "BlockAll", Params)) return false;
	for (const FHitResult& Hit : HeightHits)
	{
		if (Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;
		}
	}
	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosMMSA) return false;
	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;

ZLOG(FString::Printf(TEXT("Height: %f"), Height))
ZPOINT(SurfaceHit.Location, FColor::Blue);
	
	if (Height > MaxHeight) return false;

	// Check Clearance
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector ClearCapLoc = SurfaceHit.Location + Fwd * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	if (GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params))
	{
ZCAPSULE(ClearCapLoc, FColor::Red)
		return false;
	}
	else
	{
ZCAPSULE(ClearCapLoc, FColor::Green)
	}
ZLOG("Can Mantle")
	
	FVector TransitionTarget = ClearCapLoc;
ZCAPSULE(TransitionTarget, FColor::Yellow)

	// Perform Transition to Mantle
ZCAPSULE(UpdatedComponent->GetComponentLocation(), FColor::Red)

	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());

	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	
	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, .1f, .25f);
ZLOG(FString::Printf(TEXT("Duration: %f"), TransitionRMS->Duration))
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);
	TransitionName = "Mantle";
	return true;
}
#pragma endregion

#pragma region WallBounce
bool UZeroCharacterMovementComponent::TryWallBounce()
{
	if (!IsMovementMode(MOVE_Falling)) return false;
	
	FHitResult WallHit;
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR() * 1.25, CapHH()/2);
	FVector TraceLocation = UpdatedComponent->GetComponentLocation();
	FQuat RotationX = FQuat::Identity;
	if(GetWorld()->SweepSingleByChannel(WallHit,TraceLocation,TraceLocation,RotationX,ECC_WorldStatic,CapShape,ZeroCharacter_Owner->GetIgnoreCharacterParams()))
	{
ZLOG("Wall Detected");
		//ZPOINT(WallHit.ImpactPoint,FColor::Blue);

		//Launch Dir
		
		FVector WallLaunchDir = WallHit.ImpactNormal.GetSafeNormal() + FVector::UpVector;
ZLINE(WallHit.ImpactPoint,WallHit.ImpactPoint+WallLaunchDir,FColor::Green);
		Velocity = WallLaunchDir *  WallBounceImpluse;
		SetMovementMode(MOVE_Falling);
		return true;
	}
	return false;
}
#pragma endregion

#pragma region Zipline
bool UZeroCharacterMovementComponent::TryZipLine()
{
	if(IsCustomMovementMode(CMOVE_Zipline)) return false;
	if(GetWorld()->TimeSeconds - ZiplineLastTickTime < ZiplineCheckTickIntervel ) return false;
	ZiplineLastTickTime = GetWorld()->TimeSeconds;
	
	FCollisionShape ZipCap = FCollisionShape::MakeSphere(ZiplineCheckSphereRadius);
	FVector TraceLocation = CamLoc() + CamFV() * ZiplineCheckSphereRadius;
	FVector TraceEndLocation = CamLoc() + CamFV() * ZiplineCheckMaxDistance;
	FHitResult ZipHit;
	if(GetWorld()->SweepSingleByObjectType(ZipHit,TraceLocation,TraceEndLocation,CamQuat(),ECC_Vehicle,ZipCap,ZeroCharacter_Owner->GetIgnoreCharacterParams()))
	{
ZPOINT(ZipHit.ImpactPoint,FColor::Magenta);
ZLOG("Hit")
ZLOG(ZipHit.GetActor()->GetName());
		if(Cast<AZero_ZiplineActor>(ZipHit.GetActor()))
		{
			ZiplineActorRef = Cast<AZero_ZiplineActor>(ZipHit.GetActor());
			ZiplineSplineComp = ZiplineActorRef->GetZiplineComponent();
			float maxDis = ZiplineSplineComp->GetSplineLength();
			FVector EndPoint = ZiplineSplineComp->GetLocationAtDistanceAlongSpline(maxDis,ESplineCoordinateSpace::World);
			float Angle = (CharacterOwner->GetActorForwardVector())| (EndPoint - CharLocation() );
			bZiplineMoveingToEnd =false;
			if(Angle> 0)
			{
				bZiplineMoveingToEnd = true;
			}
			//StartZipline();
			return true;
		}
	}
	return false;
}

void UZeroCharacterMovementComponent::Server_EnterZipline_Implementation(USplineComponent* ZiplineToUse,
	bool InSplineDir)
{
	ZiplineSplineComp = ZiplineToUse;
	bZiplineMoveingToEnd = InSplineDir;
	SetMovementMode(MOVE_Custom,CMOVE_Zipline);
}






void UZeroCharacterMovementComponent::EnterZipline()
{
	Velocity = FVector::ZeroVector;
}

void UZeroCharacterMovementComponent::ExitZipline()
{
	FQuat NewRot = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(),FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector,NewRot,true,Hit);
}


void UZeroCharacterMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{
	//delta time is not less than the min tick time 
	if(DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();
	if(!ZiplineSplineComp)
	{
		return;
	}
	//
	if(Safe_bWantsToDash || bWantsToCrouch || ZeroCharacter_Owner->bPressedZeroJump)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(DeltaTime,Iterations);//starts a new physics in the same frame
		return;
	}

	//TODO add a way to find if we reached the end of zipline

	//Perform Move
	Iterations++;
	bJustTeleported = false;
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	float MaxDis = ZiplineSplineComp->GetSplineLength();
	float DistancetoPoint = ZiplineSplineComp->GetDistanceAlongSplineAtLocation(CharLocation(),ESplineCoordinateSpace::World);
	float DistanceToMoveAlongSpline = bZiplineMoveingToEnd? ZiplineSpeed : (ZiplineSpeed * -1);
	FVector TargetLocation = ZiplineSplineComp->GetLocationAtDistanceAlongSpline(DistancetoPoint + DistanceToMoveAlongSpline,ESplineCoordinateSpace::World);
	FVector Adjusted = (TargetLocation - CharLocation()).GetSafeNormal() * DeltaTime * ZiplineSpeed;
	if(MaxDis <= DistancetoPoint || DistancetoPoint <= 0.0f)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(DeltaTime,Iterations);//starts a new physics in the same frame
		return;
	}

	FQuat OldRot = UpdatedComponent->GetComponentRotation().Quaternion();
	FHitResult Hit(1.f);
	//FVector Adjusted = Velocity * DeltaTime;
	//FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity,SurfaceHit.Normal).GetSafeNormal();
	FVector FVofSplinePoint = ZiplineSplineComp->GetDirectionAtDistanceAlongSpline(DistancetoPoint,ESplineCoordinateSpace::World).GetSafeNormal();
	FVofSplinePoint.Z =0;
	if(!bZiplineMoveingToEnd)
	{
		FVofSplinePoint *= -1;
	}
	FQuat NewRot =FRotationMatrix::MakeFromXZ(FVofSplinePoint,FVector::UpVector).ToQuat();
	SafeMoveUpdatedComponent(Adjusted,NewRot,true,Hit);

	if(Hit.Time <1.f)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(DeltaTime,Iterations);//starts a new physics in the same frame
		return;
	}
	//Update ongoing Velocity and Accer
	if(!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) /DeltaTime;
	}
	
}
#pragma endregion

#pragma region Helper Functions
bool UZeroCharacterMovementComponent::IsServer() const
{
	return CharacterOwner->HasAuthority();
}

float UZeroCharacterMovementComponent::CapR() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float UZeroCharacterMovementComponent::CapHH() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

bool UZeroCharacterMovementComponent::IsMovementMode(EMovementMode inMovementMode) const
{
	return inMovementMode == MovementMode;
}

FVector UZeroCharacterMovementComponent::CharLocation() const
{
	return UpdatedComponent->GetComponentLocation();
}

FRotator UZeroCharacterMovementComponent::CharRotation() const
{
	return UpdatedComponent->GetComponentRotation();
}

FVector UZeroCharacterMovementComponent::CamFV() const
{
	return ZeroCharacter_Owner->GetFollowCamera()->GetForwardVector();
}

FVector UZeroCharacterMovementComponent::CamLoc() const
{
	return ZeroCharacter_Owner->GetFollowCamera()->GetComponentLocation();
}
FQuat UZeroCharacterMovementComponent::CamQuat() const
{
	return ZeroCharacter_Owner->GetFollowCamera()->GetComponentRotation().Quaternion();
}




bool UZeroCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode inCustomMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == inCustomMode;
}
#pragma endregion

#pragma region Inputs
void UZeroCharacterMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}

void UZeroCharacterMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}

void UZeroCharacterMovementComponent::CrouchPressed()
{
	bWantsToCrouch = true;
}

void UZeroCharacterMovementComponent::CrouchReleased()
{
	bWantsToCrouch = false;
}

void UZeroCharacterMovementComponent::DashPressed()
{
	float currentTime = GetWorld()->GetTimeSeconds();
	if(currentTime - DashStartTime >= DashCoolDownDuration)
	{
		Safe_bWantsToDash = true;
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_DashCoolDown,this,&ThisClass::OnDashCoolDownFinished,DashCoolDownDuration-(currentTime-DashStartTime));
	}
}

void UZeroCharacterMovementComponent::DashReleased()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashCoolDown);
	Safe_bWantsToDash = false;
}

void UZeroCharacterMovementComponent::GetLifetimeReplicatedProps(
	TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UZeroCharacterMovementComponent,Proxy_bDashStart,COND_SkipOwner);
}
#pragma endregion

class FNetworkPredictionData_Client* UZeroCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if(ClientPredictionData == nullptr)
	{
		UZeroCharacterMovementComponent* MutableThis = const_cast<UZeroCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Zero(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}
	return ClientPredictionData;
}
