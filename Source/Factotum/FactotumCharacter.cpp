// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Factotum.h"
#include "FactotumCharacter.h"
#include "ProjectileA.h"
#include "PaperFlipbookComponent.h"

//////////////////////////////////////////////////////////////////////////
// AFactotumCharacter

AFactotumCharacter::AFactotumCharacter()
{ 
	PrimaryActorTick.bCanEverTick = true;

	// Setup the assets
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> RunningAnimationAsset;
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> IdleAnimationAsset;
		FConstructorStatics()
			: RunningAnimationAsset(TEXT("/Game/2dSideScroller/Sprites/RunningAnimation.RunningAnimation"))
			, IdleAnimationAsset(TEXT("/Game/2dSideScroller/Sprites/IdleAnimation.IdleAnimation"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	RunningAnimation = ConstructorStatics.RunningAnimationAsset.Get();
	IdleAnimation = ConstructorStatics.IdleAnimationAsset.Get();
	GetSprite()->SetFlipbook(IdleAnimation);

	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->AttachTo(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

	isDashing = false;

	elapsedTime = 0.0f;

	elapsedTimeInterval = 0.0f;

	FacingDirection = EDirection::Right;



	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void AFactotumCharacter::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeed = PlayerVelocity.Size();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = (PlayerSpeed > 0.0f) ? RunningAnimation : IdleAnimation;

	GetSprite()->SetFlipbook(DesiredAnimation);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFactotumCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	InputComponent->BindAxis("MoveRight", this, &AFactotumCharacter::MoveRight);
	InputComponent->BindAction("Dash", IE_Pressed, this, &AFactotumCharacter::Dash);

	InputComponent->BindTouch(IE_Pressed, this, &AFactotumCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &AFactotumCharacter::TouchStopped);
}

// Called every frame
void AFactotumCharacter::Tick(float DeltaTime)
{
	if (isDashing) {
		
		elapsedTime += DeltaTime;
		elapsedTimeInterval += DeltaTime;
		UE_LOG(LogTemp, Warning, TEXT("Elapsed Time is: %f, and friction is: %f"), elapsedTime, GetCharacterMovement()->GroundFriction);
		if (elapsedTime > 0.8f) {
			UE_LOG(LogTemp, Warning, TEXT("Setting dashing to false"));
			isDashing = false;
			elapsedTime = 0.0f;
			elapsedTimeInterval = 0.0f;
			GetCharacterMovement()->GroundFriction = 0.0f;
		}
		else if (elapsedTimeInterval > 0.3f) {
			UE_LOG(LogTemp, Warning, TEXT("Launching character"));
			if (FacingDirection == EDirection::Right) {
				Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
				LaunchCharacter(FVector(1200.0f, 0.0f, 0.0f), true, false);
			}
			else {
				Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
				LaunchCharacter(FVector(-1200.0f, 0.0f, 0.0f), true, false);
			}
			elapsedTimeInterval = 0.0f;
		}
	}
}

void AFactotumCharacter::MoveRight(float Value)
{
	// Update animation to match the motion
	UpdateAnimation();

	if (!isDashing) {
		// Set the rotation so that the character faces his direction of travel.
		if (Controller != nullptr)
		{
			if (Value < 0.0f)
			{
				Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
				FacingDirection = EDirection::Left;
			}
			else if (Value > 0.0f)
			{
				FacingDirection = EDirection::Right;
				Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
			}
		}
		//GetCharacterMovement()->MaxWalkSpeed = isDashing ? 1200.0f : 600.0f;
		// Apply the input to the character motion
		AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
	}
}

void AFactotumCharacter::Dash() {
	UE_LOG(LogTemp, Warning, TEXT("Launching character"));
	if (FacingDirection == EDirection::Right) {
		Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		LaunchCharacter(FVector(1200.0f, 0.0f, 0.0f), true, false);
	}
	else {
		Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		LaunchCharacter(FVector(-1200.0f, 0.0f, 0.0f), true, false);
	}
	isDashing = true;

	GetCharacterMovement()->GroundFriction = 0.0f;

}


void AFactotumCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// jump on any touch
	Jump();
}

void AFactotumCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}
