// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/ArcoroxCharacter.h"
#include "Items/Item.h"
#include "Items/Weapon.h"
#include "Items/Ammo.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Particles/ParticleSystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Arcorox/Arcorox.h"
#include "Enemy/Enemy.h"
#include "Enemy/EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"

AArcoroxCharacter::AArcoroxCharacter() :
	//Is Aiming
	bAiming(false),
	bAimButtonPressed(false),
	//Camera field of view
	CameraDefaultFOV(0.f),
	CameraZoomedFOV(25.f),
	CameraCurrentFOV(0.f),
	ZoomInterpolationSpeed(25.f),
	//Look sensitivity
	LookScale(1.f),
	HipLookScale(1.f),
	AimingLookScale(0.6f),
	//Crosshair spread
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),
	ShootTimeDuration(0.05f),
	bFiringWeapon(false),
	//Automatic weapon fire
	bShouldFire(true),
	bFireButtonPressed(false),
	bShouldTraceForItems(false),
	//Camera interp location variables
	CameraInterpDistance(200.f),
	CameraInterpElevation(50.f),
	//Default ammo amounts
	Starting9mmAmmo(120),
	Starting556Ammo(90),
	//Combat state
	CombatState(ECombatState::ECS_Unoccupied),
	//Crouching
	bCrouching(false),
	//Movement speed
	DefaultMovementSpeed(650.f),
	CrouchMovementSpeed(300.f),
	//Capsule half height
	DefaultCapsuleHalfHeight(88.f),
	CrouchingCapsuleHalfHeight(44.f),
	//Ground friction
	DefaultGroundFriction(2.f),
	CrouchingGroundFriction(100.f),
	//Sound timer properties
	bShouldPlayPickupSound(true),
	bShouldPlayEquipSound(true),
	PickupSoundTime(0.2f),
	EquipSoundTime(0.2f),
	//Highlight Icon animation property
	HighlightedInventorySlot(-1),
	//Health
	Health(100.f),
	MaxHealth(100.f),
	//Stun Chance
	StunChance(0.25f)
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	//Make Character Mesh move according to movement direction
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 240.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 35.f, 80.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComponent"));

	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetCamera());
	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComp1->SetupAttachment(GetCamera());
	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComp2->SetupAttachment(GetCamera());
	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComp3->SetupAttachment(GetCamera());
	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComp4->SetupAttachment(GetCamera());
	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComp5->SetupAttachment(GetCamera());
	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComp6->SetupAttachment(GetCamera());

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AArcoroxCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SetupEnhancedInput();

	if (GetCamera())
	{
		CameraDefaultFOV = GetCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;
	EquipWeapon(SpawnDefaultWeapon());
	if (EquippedWeapon)
	{
		Inventory.Add(EquippedWeapon);
		EquippedWeapon->SetInventorySlotIndex(0);
		EquippedWeapon->SetArcoroxCharacter(this);
		EquippedWeapon->DisableGlowMaterial();
		EquippedWeapon->DisableCustomDepth();
	}
	InitializeAmmoMap();
	InitializeInterpLocations();
}

void AArcoroxCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraZoomInterpolation(DeltaTime);
	SetLookScale();
	CalculateCrosshairSpread(DeltaTime);
	ItemTrace();
	InterpolateCapsuleHalfHeight(DeltaTime);
}

void AArcoroxCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Bind functions to specific Input Actions 
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArcoroxCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArcoroxCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AArcoroxCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(FireWeaponAction, ETriggerEvent::Started, this, &AArcoroxCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireWeaponAction, ETriggerEvent::Completed, this, &AArcoroxCharacter::FireButtonReleased);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AArcoroxCharacter::AimButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AArcoroxCharacter::AimButtonReleased);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AArcoroxCharacter::InteractButtonPressed);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AArcoroxCharacter::InteractButtonReleased);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AArcoroxCharacter::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AArcoroxCharacter::CrouchButtonPressed);
		EnhancedInputComponent->BindAction(FAction, ETriggerEvent::Started, this, &AArcoroxCharacter::FKeyPressed);
		EnhancedInputComponent->BindAction(OneAction, ETriggerEvent::Started, this, &AArcoroxCharacter::OneKeyPressed);
		EnhancedInputComponent->BindAction(TwoAction, ETriggerEvent::Started, this, &AArcoroxCharacter::TwoKeyPressed);
		EnhancedInputComponent->BindAction(ThreeAction, ETriggerEvent::Started, this, &AArcoroxCharacter::ThreeKeyPressed);
		EnhancedInputComponent->BindAction(FourAction, ETriggerEvent::Started, this, &AArcoroxCharacter::FourKeyPressed);
		EnhancedInputComponent->BindAction(FiveAction, ETriggerEvent::Started, this, &AArcoroxCharacter::FiveKeyPressed);
	}
}

void AArcoroxCharacter::Jump()
{
	if (bCrouching)
	{
		bCrouching = false;
		if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;
		return;
	}
	Super::Jump();
}

void AArcoroxCharacter::Hit_Implementation(FHitResult HitResult)
{

}

float AArcoroxCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();
		AEnemyController* EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController && EnemyController->GetBlackboardComponent())
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("PlayerDead"), true);
		}
	}
	else Health -= DamageAmount;
	return DamageAmount;
}

void AArcoroxCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	OverlappedItemCount += Amount;
	bShouldTraceForItems = OverlappedItemCount > 0;
}

void AArcoroxCharacter::GetPickupItem(AItem* Item)
{
	if (Item) Item->PlayEquipSound();
	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		if (Inventory.Num() < InventoryCapacity)
		{
			Inventory.Add(Weapon);
			Weapon->SetInventorySlotIndex(Inventory.Num() - 1);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else SwapWeapon(Weapon);
	}
	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo) PickupAmmo(Ammo);
}

FInterpLocation AArcoroxCharacter::GetInterpLocation(int32 Index)
{
	if(InterpLocations.Num() >= Index) return InterpLocations[Index];
	return FInterpLocation();
}

void AArcoroxCharacter::IncrementInterpLocationItemCount(int32 Index)
{
	if (InterpLocations.Num() >= Index) InterpLocations[Index].ItemCount++;
}

void AArcoroxCharacter::DecrementInterpLocationItemCount(int32 Index)
{
	if (InterpLocations.Num() >= Index) InterpLocations[Index].ItemCount--;
}

void AArcoroxCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(PickupSoundTimer, this, &AArcoroxCharacter::ResetPickupSoundTimer, PickupSoundTime);
}

void AArcoroxCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AArcoroxCharacter::ResetEquipSoundTimer, EquipSoundTime);
}

float AArcoroxCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void AArcoroxCharacter::PlayMeleeImpactSound()
{
	if (MeleeImpactSound) UGameplayStatics::PlaySoundAtLocation(this, MeleeImpactSound, GetActorLocation());
}

void AArcoroxCharacter::SpawnBloodParticles(const FTransform& SocketTransform)
{
	if (BloodParticles) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BloodParticles, SocketTransform);
}

void AArcoroxCharacter::Stun(const FHitResult& HitResult)
{
	if (Health <= 0.f) return;
	CombatState = ECombatState::ECS_Stunned;
	PlayHitReactMontage(HitResult);
}

void AArcoroxCharacter::PlayHitReactMontage(const FHitResult& HitResult)
{
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	FVector Diff = HitResult.Location - GetActorLocation();
	Diff.Normalize();
	const FVector HitLocation = Diff;
	const float ForwardDotProduct = FVector::DotProduct(Forward, HitLocation);
	const float RightDotProduct = FVector::DotProduct(Right, HitLocation);
	FName SectionName = FName("Front");
	if (ForwardDotProduct >= 0.5f && ForwardDotProduct <= 1.f) SectionName = FName("Front");
	else if (ForwardDotProduct >= -1.f && ForwardDotProduct <= -0.5f) SectionName = FName("Back");
	else if (RightDotProduct >= 0.f && RightDotProduct <= 1.f) SectionName = FName("Right");
	else if (RightDotProduct >= -1.f && RightDotProduct <= -0.f) SectionName = FName("Left");
	PlayMontageSection(HitReactMontage, SectionName);
}

void AArcoroxCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (GetController())
	{
		//Get Controller Rotation and use Rotation Matrix to get Controller Forward and Right Vectors
		const FRotator ControllerRotation = GetControlRotation();
		const FRotator YawRotation(0.f, ControllerRotation.Yaw, 0.f);
		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardVector, MovementVector.Y);
		AddMovementInput(RightVector, MovementVector.X);
	}
}

void AArcoroxCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (GetController())
	{
		AddControllerPitchInput(LookScale * LookAxisVector.Y);
		AddControllerYawInput(LookScale * LookAxisVector.X);
	}
}

void AArcoroxCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr || CombatState != ECombatState::ECS_Unoccupied) return;
	if (WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunfireMontage();
		StartCrosshairShootTimer();
		EquippedWeapon->DecrementAmmo();
		StartAutoFireTimer();
		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol) EquippedWeapon->StartPistolSlideTimer();
	}
}

void AArcoroxCharacter::ReloadWeapon()
{
	if (EquippedWeapon == nullptr || CombatState != ECombatState::ECS_Unoccupied) return;
	if (CarryingAmmo() && !EquippedWeapon->FullMagazine())
	{
		if (bAiming) StopAiming();
		CombatState = ECombatState::ECS_Reloading;
		PlayReloadMontage();
	}
}

void AArcoroxCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void AArcoroxCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AArcoroxCharacter::AimButtonPressed()
{
	bAimButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping && CombatState != ECombatState::ECS_Stunned) Aim();
}

void AArcoroxCharacter::AimButtonReleased()
{
	bAimButtonPressed = false;
	StopAiming();
}

void AArcoroxCharacter::InteractButtonPressed()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
		TraceHitItem = nullptr;
	}
}

void AArcoroxCharacter::InteractButtonReleased()
{

}

void AArcoroxCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AArcoroxCharacter::CrouchButtonPressed()
{
	if (GetCharacterMovement() == nullptr) return;
	if (!GetCharacterMovement()->IsFalling()) bCrouching = !bCrouching;
	if (bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		if (!bAiming) GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;
		GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
	}
}

void AArcoroxCharacter::FKeyPressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetInventorySlotIndex() == 0) return;
	ExchangeInventoryItems(EquippedWeapon->GetInventorySlotIndex(), 0);
}

void AArcoroxCharacter::OneKeyPressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetInventorySlotIndex() == 1) return;
	ExchangeInventoryItems(EquippedWeapon->GetInventorySlotIndex(), 1);
}

void AArcoroxCharacter::TwoKeyPressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetInventorySlotIndex() == 2) return;
	ExchangeInventoryItems(EquippedWeapon->GetInventorySlotIndex(), 2);
}

void AArcoroxCharacter::ThreeKeyPressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetInventorySlotIndex() == 3) return;
	ExchangeInventoryItems(EquippedWeapon->GetInventorySlotIndex(), 3);
}

void AArcoroxCharacter::FourKeyPressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetInventorySlotIndex() == 4) return;
	ExchangeInventoryItems(EquippedWeapon->GetInventorySlotIndex(), 4);
}

void AArcoroxCharacter::FiveKeyPressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetInventorySlotIndex() == 5) return;
	ExchangeInventoryItems(EquippedWeapon->GetInventorySlotIndex(), 5);
}

bool AArcoroxCharacter::GetBeamEndLocation(const FVector& BarrelSocketLocation, FHitResult& OutHit)
{
	FVector OutBeamLocation;
	//Check for crosshair trace hit
	FHitResult CrosshairHitResult;
	if (CrosshairLineTrace(CrosshairHitResult, OutBeamLocation))
	{
		OutBeamLocation = CrosshairHitResult.Location;
	}
	//Perform line trace from weapon barrel
	const FVector WeaponTraceStart{ BarrelSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - BarrelSocketLocation };
	const FVector WeaponTraceEnd{ BarrelSocketLocation + StartToEnd * 1.25f };
	GetWorld()->LineTraceSingleByChannel(OutHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);
	if (!OutHit.bBlockingHit) //No object between weapon barrel and beam end point?
	{
		OutHit.Location = OutBeamLocation;
		return false;
	}
	return true;
}

void AArcoroxCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
		FHitResult BeamHitResult;
		SpawnMuzzleFlash(SocketTransform);
		if (GetBeamEndLocation(SocketTransform.GetLocation(), BeamHitResult))
		{
			if (BeamHitResult.GetActor())
			{
				//Does the hit Actor implement the HitInterface
				IHitInterface* HitInterface = Cast<IHitInterface>(BeamHitResult.GetActor());
				if (HitInterface) HitInterface->Hit_Implementation(BeamHitResult);
				else
				{
					SpawnImpactParticles(BeamHitResult.Location);
					SpawnBeamParticles(SocketTransform, BeamHitResult.Location);
				}
				//Is the hit Actor an Enemy
				AEnemy* Enemy = Cast<AEnemy>(BeamHitResult.GetActor());
				if (Enemy)
				{
					float Damage = EquippedWeapon->GetDamage();
					bool bHeadshot = false;
					if (BeamHitResult.BoneName.ToString().Equals(Enemy->GetHeadBone()))
					{
						bHeadshot = true;
						Damage *= EquippedWeapon->GetHeadshotMultiplier();
					}
					UGameplayStatics::ApplyDamage(BeamHitResult.GetActor(), Damage, GetController(), this, UDamageType::StaticClass());
					Enemy->ShowHitDamage(Damage, BeamHitResult.Location, bHeadshot);
				}
			}
		}
	}
}

void AArcoroxCharacter::ExchangeInventoryItems(int32 CurrentSlotIndex, int32 TargetSlotIndex)
{
	const bool CannotExchangeItems = ((CombatState != ECombatState::ECS_Unoccupied && CombatState != ECombatState::ECS_Equipping) || EquippedWeapon == nullptr || CurrentSlotIndex == TargetSlotIndex || TargetSlotIndex >= Inventory.Num());
	if (CannotExchangeItems) return;
	if (bAiming) StopAiming();
	AWeapon* CurrentEquippedWeapon = EquippedWeapon;
	AWeapon* NewEquippedWeapon = Cast<AWeapon>(Inventory[TargetSlotIndex]);
	EquipWeapon(NewEquippedWeapon);
	NewEquippedWeapon->ForcePlayEquipSound();
	CurrentEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
	CombatState = ECombatState::ECS_Equipping;
	PlayEquipMontage();
}

bool AArcoroxCharacter::CrosshairLineTrace(FHitResult& OutHit, FVector& OutHitLocation)
{
	//Get size of viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(ViewportSize);
	//Get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition, CrosshairWorldDirection;
	//Get crosshairs world position and direction
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld) //was deprojection successful
	{
		//Trace outward from crosshair location
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50000 };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility);
		if (OutHit.bBlockingHit)
		{
			OutHitLocation = OutHit.Location;
			return true;
		}
	}
	return false;
}

void AArcoroxCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	//Calculate CrosshairVelocityFactor
	FVector2D WalkSpeedRange{ 0.f, 600.f };
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
	//Calculate CrosshairInAirFactor
	if (GetCharacterMovement()->IsFalling()) CrosshairInAirFactor = FMath::FInterpTo<float>(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	else CrosshairInAirFactor = FMath::FInterpTo<float>(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	//Calculate CrosshairAimFactor
	if (bAiming) CrosshairAimFactor = FMath::FInterpTo<float>(CrosshairAimFactor, -0.6f, DeltaTime, 30.f);
	else CrosshairAimFactor = FMath::FInterpTo<float>(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	//Calculate CrosshairShootFactor
	if (bFiringWeapon) CrosshairShootingFactor = FMath::FInterpTo<float>(CrosshairShootingFactor, 0.4f, DeltaTime, 60.f);
	else CrosshairShootingFactor = FMath::FInterpTo<float>(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor + CrosshairShootingFactor;
}

void AArcoroxCharacter::ItemTrace()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		if (CrosshairLineTrace(ItemTraceResult, HitLocation))
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			const AWeapon* TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if (TraceHitWeapon)
			{
				if (HighlightedInventorySlot == -1) HighlightInventorySlot();
			}
			else
			{
				if (HighlightedInventorySlot != -1) UnhighlightInventorySlot();
			}
			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterpolating) TraceHitItem = nullptr;
			if (TraceHitItem)
			{
				TraceHitItem->ShowPickupWidget();
				TraceHitItem->EnableCustomDepth();
				TraceHitItem->SetCharacterInventoryFull(Inventory.Num() >= InventoryCapacity);
			}
			
			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->HidePickupWidget();
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->HidePickupWidget();
		TraceHitItemLastFrame->DisableCustomDepth();
		TraceHitItemLastFrame = nullptr;
	}
}

void AArcoroxCharacter::StartCrosshairShootTimer()
{
	bFiringWeapon = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AArcoroxCharacter::FinishedCrosshairShootTimer, ShootTimeDuration);
}

void AArcoroxCharacter::StartAutoFireTimer()
{
	if (EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_Firing;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AArcoroxCharacter::AutoFireReset, EquippedWeapon->GetFireRate());
}

AWeapon* AArcoroxCharacter::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();
	if (World && DefaultWeaponClass) return World->SpawnActor<AWeapon>(DefaultWeaponClass);
	return nullptr;
}

void AArcoroxCharacter::EquipWeapon(AWeapon* Weapon, bool bSwapping)
{
	if (Weapon)
	{
		const USkeletalMeshSocket* WeaponSocket = GetMesh()->GetSocketByName(FName("WeaponSocket"));
		if (WeaponSocket)
		{
			WeaponSocket->AttachActor(Weapon, GetMesh());
			if (EquippedWeapon == nullptr) EquipItemDelegate.Broadcast(-1, Weapon->GetInventorySlotIndex());
			else if(!bSwapping) EquipItemDelegate.Broadcast(EquippedWeapon->GetInventorySlotIndex(), Weapon->GetInventorySlotIndex());
			EquippedWeapon = Weapon;
			EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
		}
	}
}

void AArcoroxCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AArcoroxCharacter::SwapWeapon(AWeapon* Weapon)
{
	if (EquippedWeapon == nullptr) return;
	if (Inventory.Num() - 1 >= EquippedWeapon->GetInventorySlotIndex())
	{
		Inventory[EquippedWeapon->GetInventorySlotIndex()] = Weapon;
		Weapon->SetInventorySlotIndex(EquippedWeapon->GetInventorySlotIndex());
	}
	DropWeapon();
	EquipWeapon(Weapon, true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

int32 AArcoroxCharacter::GetEmptyInventorySlot() const
{
	if (Inventory.Num() >= InventoryCapacity) return -1;
	for (int32 i = 0; i < Inventory.Num(); i++)	if (Inventory[i] == nullptr) return i;
	if (Inventory.Num() < InventoryCapacity) return Inventory.Num();
	return -1;
}

void AArcoroxCharacter::HighlightInventorySlot()
{
	HighlightedInventorySlot = GetEmptyInventorySlot();
	HighlightIconDelegate.Broadcast(HighlightedInventorySlot, true);
}

void AArcoroxCharacter::UnhighlightInventorySlot()
{
	if (HighlightedInventorySlot == -1) return;
	HighlightIconDelegate.Broadcast(HighlightedInventorySlot, false);
	HighlightedInventorySlot = -1;
}

void AArcoroxCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_556, Starting556Ammo);
}

bool AArcoroxCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon) return EquippedWeapon->GetAmmo() > 0;
	return false;
}

bool AArcoroxCharacter::CarryingAmmo()
{
	if (EquippedWeapon)
	{
		auto AmmoType = EquippedWeapon->GetAmmoType();
		if (AmmoMap.Contains(AmmoType)) return AmmoMap[AmmoType] > 0;
	}
	return false;
}

void AArcoroxCharacter::InterpolateCapsuleHalfHeight(float DeltaTime)
{
	if (GetCapsuleComponent() == nullptr || GetMesh() == nullptr) return;
	const float CurrentCapsuleHalfHeight{ GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };
	float TargetCapsuleHalfHeight;
	bCrouching ? TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight : TargetCapsuleHalfHeight = DefaultCapsuleHalfHeight;
	const float InterpolatedCapsuleHalfHeight{ FMath::FInterpTo<float>(CurrentCapsuleHalfHeight, TargetCapsuleHalfHeight, DeltaTime, 20.f) };
	//Negative if crouching, positive if standing
	const float DeltaCapsuleHalfHeight{ InterpolatedCapsuleHalfHeight - CurrentCapsuleHalfHeight };
	const FVector MeshOffset{ 0.f, 0.f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpolatedCapsuleHalfHeight);
}

void AArcoroxCharacter::Aim()
{
	bAiming = true;
	if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AArcoroxCharacter::StopAiming()
{
	bAiming = false;
	if (GetCharacterMovement() && !bCrouching) GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;
}

void AArcoroxCharacter::PickupAmmo(AAmmo* Ammo)
{
	if (Ammo == nullptr) return;
	if (AmmoMap.Contains(Ammo->GetAmmoType()))
	{
		int32 CurrentAmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		CurrentAmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = CurrentAmmoCount;
	}
	if (EquippedWeapon && EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		if (!WeaponHasAmmo()) ReloadWeapon();
	}
	Ammo->Destroy();
}

void AArcoroxCharacter::InitializeInterpLocations()
{
	InterpLocations.Add(FInterpLocation{ WeaponInterpComp, 0 });
	InterpLocations.Add(FInterpLocation{ InterpComp1, 0 });
	InterpLocations.Add(FInterpLocation{ InterpComp2, 0 });
	InterpLocations.Add(FInterpLocation{ InterpComp3, 0 });
	InterpLocations.Add(FInterpLocation{ InterpComp4, 0 });
	InterpLocations.Add(FInterpLocation{ InterpComp5, 0 });
	InterpLocations.Add(FInterpLocation{ InterpComp6, 0 });
}

void AArcoroxCharacter::Die()
{
	PlayDeathMontage();
}

int32 AArcoroxCharacter::GetInterpLocationIndex()
{
	int32 TargetIndex = 1;
	int32 LowestItemCount = INT_MAX;
	for (int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if (InterpLocations[i].ItemCount < LowestItemCount)
		{
			LowestItemCount = InterpLocations[i].ItemCount;
			TargetIndex = i;
		}
	}
	return TargetIndex;
}

void AArcoroxCharacter::AutoFireReset()
{
	if (CombatState == ECombatState::ECS_Stunned) return;
	CombatState = ECombatState::ECS_Unoccupied;
	if (EquippedWeapon == nullptr) return;
	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed && EquippedWeapon->IsWeaponAutomatic()) FireWeapon();
	}
	else
	{
		ReloadWeapon();
	}
}

void AArcoroxCharacter::FinishedCrosshairShootTimer()
{
	bFiringWeapon = false;
}

void AArcoroxCharacter::FinishReloading()
{
	if (CombatState == ECombatState::ECS_Stunned) return;
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimButtonPressed) Aim();
	int32 CarriedAmmo = AmmoMap[EquippedWeapon->GetAmmoType()];
	const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();
	if (MagEmptySpace > CarriedAmmo)
	{
		EquippedWeapon->ReloadAmmo(CarriedAmmo);
		CarriedAmmo = 0;
		AmmoMap.Add(EquippedWeapon->GetAmmoType(), CarriedAmmo);
	}
	else
	{
		EquippedWeapon->ReloadAmmo(MagEmptySpace);
		CarriedAmmo -= MagEmptySpace;
		AmmoMap.Add(EquippedWeapon->GetAmmoType(), CarriedAmmo);
	}
}

void AArcoroxCharacter::FinishEquipping()
{
	if (CombatState == ECombatState::ECS_Stunned) return;
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimButtonPressed) Aim();
}

void AArcoroxCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController) DisableInput(PlayerController);
}

void AArcoroxCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr || HandSceneComponent == nullptr) return;
	int32 ClipBoneIndex{ EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };
	WeaponClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("hand_l")));
	HandSceneComponent->SetWorldTransform(WeaponClipTransform);
	EquippedWeapon->SetMovingClip(true);
}

void AArcoroxCharacter::ReleaseClip()
{
	if (EquippedWeapon == nullptr) return;
	EquippedWeapon->SetMovingClip(false);
}

EPhysicalSurface AArcoroxCharacter::GetSurfaceType()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return EPhysicalSurface::SurfaceType_Default;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;
	World->LineTraceSingleByChannel(HitResult, GetActorLocation(), GetActorLocation() + FVector(0.f, 0.f, -300.f), ECollisionChannel::ECC_Visibility, QueryParams);
	if (HitResult.PhysMaterial == nullptr) return EPhysicalSurface::SurfaceType_Default;
	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AArcoroxCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimButtonPressed) Aim();
}

void AArcoroxCharacter::PlayFireSound()
{
	if (EquippedWeapon->GetFireSound()) UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
}

void AArcoroxCharacter::SpawnMuzzleFlash(const FTransform& SocketTransform)
{
	if (EquippedWeapon->GetMuzzleFlash()) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
}

void AArcoroxCharacter::SpawnImpactParticles(const FVector& BeamEnd)
{
	if (ImpactParticles) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd);
}

void AArcoroxCharacter::SpawnBeamParticles(const FTransform& SocketTransform, const FVector& BeamEnd)
{
	if (BeamParticles)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
		if (Beam) Beam->SetVectorParameter(FName("Target"), BeamEnd);
	}
}

void AArcoroxCharacter::PlayMontageSection(UAnimMontage* Montage, const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Play(Montage);
		AnimInstance->Montage_JumpToSection(SectionName, Montage);
	}
}

void AArcoroxCharacter::PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionNames)
{
	if (SectionNames.Num() <= 0) return;
	const int32 Section = FMath::RandRange(0, SectionNames.Num() - 1);
	PlayMontageSection(Montage, SectionNames[Section]);
}

void AArcoroxCharacter::PlayGunfireMontage()
{
	PlayRandomMontageSection(HipFireMontage, HipFireMontageSections);
}

void AArcoroxCharacter::PlayReloadMontage()
{
	PlayMontageSection(ReloadMontage, EquippedWeapon->GetReloadMontageSection());
}

void AArcoroxCharacter::PlayEquipMontage()
{
	PlayMontageSection(EquipMontage, FName(TEXT("Default")));
}

void AArcoroxCharacter::PlayDeathMontage()
{
	PlayRandomMontageSection(DeathMontage, DeathMontageSections);
}

void AArcoroxCharacter::CameraZoomInterpolation(float DeltaTime)
{
	//Smoothly transition the current camera field of view
	if (bAiming)
	{
		//Interpolate to zoomed field of view
		CameraCurrentFOV = FMath::FInterpTo<float>(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpolationSpeed);
	}
	else
	{
		//Interpolate to default field of view
		CameraCurrentFOV = FMath::FInterpTo<float>(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpolationSpeed);
	}
	if (GetCamera()) GetCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AArcoroxCharacter::SetupEnhancedInput()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		//Link Input Mapping Context to Character Player Controller
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ArcoroxContext, 0);
		}
	}
}

void AArcoroxCharacter::SetLookScale()
{
	if (bAiming) LookScale = AimingLookScale;
	else LookScale = HipLookScale;
}

void AArcoroxCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}

void AArcoroxCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}
