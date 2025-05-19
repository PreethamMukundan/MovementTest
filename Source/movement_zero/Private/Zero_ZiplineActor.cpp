// Fill out your copyright notice in the Description page of Project Settings.


#include "Zero_ZiplineActor.h"


// Sets default values
AZero_ZiplineActor::AZero_ZiplineActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AZero_ZiplineActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AZero_ZiplineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

