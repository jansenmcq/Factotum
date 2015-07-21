// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FactotumCharacter.h"
#include "GameFramework/Actor.h"
#include "ProjectileA.generated.h"

UCLASS()
class FACTOTUM_API AProjectileA : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileA();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	
	
};
