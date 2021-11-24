// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"

#include "MyCharacterMovementComponent.h"

// Sets default values
AMyCharacter::AMyCharacter(const class FObjectInitializer& ObjectInitializer) :
    Super(ObjectInitializer.SetDefaultSubobjectClass<UMyCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 200.0f;

}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

