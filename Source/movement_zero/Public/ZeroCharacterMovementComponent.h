// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ZeroCharacterMovementComponent.generated.h"

/**
 * 
 */

class AZero_ZiplineActor;
class Amovement_zeroCharacter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDashStartDelegate);


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None  UMETA(DisplayName = "None"),
	CMOVE_Slide UMETA(DisplayName = "Slide"),
	CMOVE_Zipline UMETA(DisplayName = "Zipline"),
	CMOVE_Max   UMETA(DisplayName = "Max")
};

UCLASS()
class MOVEMENT_ZERO_API UZeroCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	
	
	class FSavedMove_Zero :public FSavedMove_Character
	{

		public:
		enum CompressedFlags
		{
			FLAG_Sprint  = 0x10,
			FLAG_Dash    = 0x20,
			FLAG_Cust3	 = 0x40,
			FLAG_Cust4   = 0x80,
			
		};
		typedef FSavedMove_Character Super;

		uint8 Saved_bWantsToSprint: 1;
		uint8 Saved_bWantsToDash: 1;
		uint8 Saved_bPressedZeroJump:1;

		uint8 Saved_bPrevWantsToCrouch: 1;
		uint8 Saved_bHadAnimRootMotion:1;
		uint8 Saved_bTransitionFinished:1;

	
		
	public:
		FSavedMove_Zero();

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_Zero :public FNetworkPredictionData_Client_Character
	{
		public:
		FNetworkPredictionData_Client_Zero(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

	//Flags
	bool Safe_bWantsToSprint;
	bool Safe_bPrevWantsToCrouch;

	bool Safe_bWantsToDash;
	bool Safe_bHadAnimRootMotion;

	

	float DashStartTime;//time the dash starts
	FTimerHandle TimerHandle_DashCoolDown;


	

	//replicated
	UPROPERTY(ReplicatedUsing=OnRep_DashStart) bool Proxy_bDashStart;
	UPROPERTY(ReplicatedUsing=OnRep_ShortMantle) bool Proxy_bShortMantle;
	UPROPERTY(ReplicatedUsing=OnRep_TallMantle) bool Proxy_bTallMantle;

	//transient
	UPROPERTY(transient) Amovement_zeroCharacter* ZeroCharacter_Owner;

	bool Safe_bTransitionFinished;
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	FString TransitionName;
	UPROPERTY(Transient) UAnimMontage* TransitionQueuedMontage;
	float TransitionQueuedMontageSpeed;
	int TransitionRMS_ID;
	
public:
	UPROPERTY(EditDefaultsOnly) float Sprint_MaxSpeed;
	UPROPERTY(EditDefaultsOnly) float Walk_MaxSpeed;

	UPROPERTY(EditDefaultsOnly) float SlideMinSpeed;
	UPROPERTY(EditDefaultsOnly) float SlideEnterImpulse;
	UPROPERTY(EditDefaultsOnly) float SlideGravityForce;
	UPROPERTY(EditDefaultsOnly) float SlideFriction;


	UPROPERTY(EditDefaultsOnly) float DashImpulse;
	UPROPERTY(EditDefaultsOnly) float DashCoolDownDuration;
	UPROPERTY(EditDefaultsOnly) float AuthDashCoolDownDuration;

	UPROPERTY(EditDefaultsOnly) float WallBounceImpluse;

	//Mantle
	UPROPERTY(EditDefaultsOnly) float MantleMaxDistance ;
	UPROPERTY(EditDefaultsOnly) float MantleReachHeight ;
	UPROPERTY(EditDefaultsOnly) float MinMantleDepth ;
	UPROPERTY(EditDefaultsOnly) float MantleMinWallSteepnessAngle ;
	UPROPERTY(EditDefaultsOnly) float MantleMaxSurfaceAngle ;
	UPROPERTY(EditDefaultsOnly) float MantleMaxAlignmentAngle;

	UPROPERTY(BlueprintAssignable) FDashStartDelegate DashStartDelegate;

	//Zipline
	UPROPERTY(EditDefaultsOnly) float ZiplineMinKeyPressTime = 0.5f;
	UPROPERTY(EditDefaultsOnly) float ZiplineCheckTickIntervel =0.5f;
	UPROPERTY(EditDefaultsOnly) float ZiplineCheckSphereRadius =110.0f;
	float ZiplineLastTickTime;
	AZero_ZiplineActor* ZiplineActorRef;
	
	UZeroCharacterMovementComponent();

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	virtual void InitializeComponent() override;
	
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	//called after every perform move (kinda like Tick)
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	
public:
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	//virtual bool CanAttemptJump() const override;
	//Slide
	private:
	void EnterSlide();
	void ExitSlide();
	void PhysSlide(float DeltaTime, int32 Iterations);
	bool GetSlideSurface(FHitResult& OutHit) const;

	//Dash
	void OnDashCoolDownFinished();
	bool CanDash() const;
	void PerformDash();
	UFUNCTION()
	void OnRep_DashStart();

	

	UFUNCTION() void OnRep_ShortMantle();
	UFUNCTION() void OnRep_TallMantle();


	bool TryMantle();
	
	//Wall Bounce
	bool TryWallBounce();
	
	//Zipline
	bool TryZipLine();
	UFUNCTION(Server, Reliable) void Server_EnterZipline(AZero_ZiplineActor* ZiplineToUse);
	void EnterZipline();
	void ExitZipline();
	void PhysZipline(float DeltaTime, int32 Iterations);
	

	bool IsServer() const;
	float CapR() const;
	float CapHH() const;
	bool IsMovementMode(EMovementMode inMovementMode)const;
	FVector CharLocation()const;
	FRotator CharRotation()const;
	FVector CamFV()const;
	FVector CamLoc()const;
	FQuat CamQuat()const;
	
public:
	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode inCustomMode) const;
	
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UFUNCTION(BlueprintCallable) void CrouchPressed();
	UFUNCTION(BlueprintCallable) void CrouchReleased();

	UFUNCTION(BlueprintCallable) void DashPressed();
	UFUNCTION(BlueprintCallable) void DashReleased();


	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	

};
