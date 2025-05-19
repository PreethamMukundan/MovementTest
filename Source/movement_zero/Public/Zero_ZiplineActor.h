// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zero_ZiplineActor.generated.h"

UCLASS()
class MOVEMENT_ZERO_API AZero_ZiplineActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AZero_ZiplineActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
