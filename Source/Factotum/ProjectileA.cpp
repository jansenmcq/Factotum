// Fill out your copyright notice in the Description page of Project Settings.

#include "Factotum.h"
#include "ProjectileA.h"


// Sets default values
AProjectileA::AProjectileA()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AProjectileA::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProjectileA::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

