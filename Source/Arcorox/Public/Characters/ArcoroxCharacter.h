// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Items/AmmoType.h"
#include "Interfaces/HitInterface.h"
#include "ArcoroxCharacter.generated.h"

//Forward declarations to avoid including unnecessary header files
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UParticleSystem;
class UAnimMontage;
class AItem;
class AWeapon;
class AAmmo;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_Firing UMETA(DisplayName = "Firing"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
	ECS_Equipping UMETA(DisplayName = "Equipping"),
	ECS_Stunned UMETA(DisplayName = "Stunned"),

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FInterpLocation
{
	GENERATED_BODY()

	/* Scene component for interpolation location */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SceneComponent;

	/* Number of items interpolating to scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ItemCount;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipItemDelegate, int32, CurrentSlotIndex, int32, NewSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHighlightIconDelegate, int32, InventorySlotIndex, bool, bStartAnimation);

UCLASS()
class ARCOROX_API AArcoroxCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	AArcoroxCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;
	virtual void Hit_Implementation(FHitResult HitResult) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void IncrementOverlappedItemCount(int8 Amount);

	void GetPickupItem(AItem* Item);

	FInterpLocation GetInterpLocation(int32 Index);

	/* Returns index in interp locations array with lowest item count */
	int32 GetInterpLocationIndex();

	/* Increment item count of specified element of interp locations array */
	void IncrementInterpLocationItemCount(int32 Index);

	/* Decrement item count of specified element of interp locations array */
	void DecrementInterpLocationItemCount(int32 Index);

	/* Start sound timers */
	void StartPickupSoundTimer();
	void StartEquipSoundTimer();

	/* Broadcasts inventory slot info using delegate to play the highlight icon animation */
	void HighlightInventorySlot();

	/* Broadcasts inventory slot info using delegate to stop playing the highlight icon animation */
	void UnhighlightInventorySlot();

	UFUNCTION(BlueprintCallable)
	float GetCrosshairSpreadMultiplier() const;

	void PlayMeleeImpactSound();
	void SpawnBloodParticles(const FTransform& SocketTransform);

	void Stun(const FHitResult& HitResult);

	void PlayHitReactMontage(const FHitResult& HitResult);

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }
	FORCEINLINE bool IsAiming() const { return bAiming; }
	FORCEINLINE int8 GetOverlappedItemCount() const { return OverlappedItemCount; }
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	FORCEINLINE bool IsCrouching() const { return bCrouching; }
	FORCEINLINE bool ShouldPlayPickupSound() const { return bShouldPlayPickupSound; }
	FORCEINLINE bool ShouldPlayEquipSound() const { return bShouldPlayEquipSound; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE float GetStunChance() const { return StunChance; }

protected:
	virtual void BeginPlay() override;

	/* Input callback functions */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void FireButtonPressed();
	void FireButtonReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	void InteractButtonPressed();
	void InteractButtonReleased();
	void ReloadButtonPressed();
	void CrouchButtonPressed();
	void FKeyPressed();
	void OneKeyPressed();
	void TwoKeyPressed();
	void ThreeKeyPressed();
	void FourKeyPressed();
	void FiveKeyPressed();

	void FireWeapon();
	void ReloadWeapon();

	bool GetBeamEndLocation(const FVector& BarrelSocketLocation, FHitResult& OutHit);
	void SendBullet();

	void ExchangeInventoryItems(int32 CurrentSlotIndex, int32 TargetSlotIndex);

	/* Line trace for items behind the crosshairs*/
	bool CrosshairLineTrace(FHitResult& OutHit, FVector& OutHitLocation);
	void CalculateCrosshairSpread(float DeltaTime);

	/* Trace for Items if OverlappedItemCount > 0 */
	void ItemTrace();

	/* Weapon fire timers */
	void StartCrosshairShootTimer();
	void StartAutoFireTimer();

	/* Spawn default weapon for character in BeginPlay */
	AWeapon* SpawnDefaultWeapon();

	/* Attach weapon to character's weapon socket */
	void EquipWeapon(AWeapon* Weapon, bool bSwapping = false);

	/* Detach weapon and have it fall to ground */
	void DropWeapon();

	/* Drops equipped weapon and equips TraceHitItem if it's a weapon */
	void SwapWeapon(AWeapon* Weapon);

	/* Returns index of empty inventory slot or -1 if full */
	int32 GetEmptyInventorySlot() const;

	/* Initialize Ammo Map with default ammo values */
	void InitializeAmmoMap();

	/* Checks if the player's equipped weapon has ammo */
	bool WeaponHasAmmo();

	/* Checks if player has ammo of same type as equipped weapon */
	bool CarryingAmmo();

	/* Interpolates Capsule half height when going between standing and crouching */
	void InterpolateCapsuleHalfHeight(float DeltaTime);

	void Aim();
	void StopAiming();

	/* Pick up the ammo and increment the ammo map */
	void PickupAmmo(AAmmo* Ammo);

	void InitializeInterpLocations();

	void Die();

	UFUNCTION()
	void AutoFireReset();

	UFUNCTION()
	void FinishedCrosshairShootTimer();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FinishEquipping();

	UFUNCTION(BlueprintCallable)
	void FinishDeath();

	/* Called in response to Grab Clip notify in the character Animation Blueprint */
	UFUNCTION(BlueprintCallable)
	void GrabClip();

	/* Called in response to Release Clip notify in the character Animation Blueprint */
	UFUNCTION(BlueprintCallable)
	void ReleaseClip();

	/* Returns physical surface type of the ground upon every character footstep */
	UFUNCTION(BlueprintCallable)
	EPhysicalSurface GetSurfaceType();

	/* Sets the combat state back to unoccupied after being stunned */
	UFUNCTION(BlueprintCallable)
	void EndStun();

	/* Enhanced Input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* ArcoroxContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FireWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* OneAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* TwoAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ThreeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FourAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FiveAction;

private:	
	void PlayFireSound();
	void SpawnMuzzleFlash(const FTransform& SocketTransform);
	void SpawnImpactParticles(const FVector& BeamEnd);
	void SpawnBeamParticles(const FTransform& SocketTransform, const FVector& BeamEnd);
	void PlayMontageSection(UAnimMontage* Montage, const FName& SectionName);
	void PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionNames);
	void PlayGunfireMontage();
	void PlayReloadMontage();
	void PlayEquipMontage();
	void PlayDeathMontage();
	void CameraZoomInterpolation(float DeltaTime);
	void SetupEnhancedInput();
	void SetLookScale();
	void ResetPickupSoundTimer();
	void ResetEquipSoundTimer();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float LookScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HipLookScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float AimingLookScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HipFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* EquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TArray<FName> HipFireMontageSections;

	UPROPERTY(EditAnywhere, Category = Combat)
	TArray<FName> DeathMontageSections;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float ZoomInterpolationSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairShootingFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	AItem* TraceHitItemLastFrame;

	/* Currently equipped weapon */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon;

	/* Default weapon class to spawn for character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	/* Item currently hit by line trace */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	AItem* TraceHitItem;

	/* Distance outward from camera for item to interpolate to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpDistance;

	/* Distance upward from camera for item to interpolate to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpElevation;

	/* Map for ammo types */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	/* Default amount of 9mm ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmo;

	/* Default ammount of 5.56 ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
	int32 Starting556Ammo;

	/* Combat state of character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState;

	/* Initial transform of gun clip when reloading */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FTransform WeaponClipTransform;

	/* Scene component for character hand to move the gun clip along with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandSceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* WeaponInterpComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComp1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComp2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComp3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComp4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComp5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComp6;

	/* TArray of interpolation location structs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Interpolation, meta = (AllowPrivateAccess = "true"))
	TArray<FInterpLocation> InterpLocations;

	/* Is character crouching */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bCrouching;

	/* Default movement speed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float DefaultMovementSpeed;

	/* Movement speed when crouching */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchMovementSpeed;

	/* Capsule half height when standing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float DefaultCapsuleHalfHeight;

	/* Capsule half height when crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchingCapsuleHalfHeight;

	/* Ground friction when standing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float DefaultGroundFriction;

	/* Ground friction when crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchingGroundFriction;

	/* Default camera field of view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraDefaultFOV;

	/* Camera field of view when aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraZoomedFOV;

	/* Time to wait between playing pickup sounds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float PickupSoundTime;

	/* Time to wait between playing equip sounds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float EquipSoundTime;

	/* TArray of items for character inventory */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	TArray<AItem*> Inventory;

	/* Delegate for sending inventory slot info to Inventory Bar Widget */
	UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
	FEquipItemDelegate EquipItemDelegate;

	/* Delegate for sending inventory slot info to play the highlight icon animation for the Inventory Bar Widget */
	UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
	FHighlightIconDelegate HighlightIconDelegate;

	/* Inventory slot index that is currently playing the Highlight Icon animation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	int32 HighlightedInventorySlot;

	/* Health of character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float Health;

	/* Maximum health of character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	/* Sound played when character hit by melee attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundBase* MeleeImpactSound;

	/* Particle system for blood when getting hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BloodParticles;

	/* Chance for character to be stunned when hit by enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float StunChance;

	/* Capacity of inventory */
	const int32 InventoryCapacity = 6;

	/* Current camera field of view this frame */
	float CameraCurrentFOV;

	/* Is the player pressing the aim button */
	bool bAimButtonPressed;

	//Crosshair spread
	float ShootTimeDuration;
	bool bFiringWeapon;
	FTimerHandle CrosshairShootTimer;

	//Automatic Weapon Fire
	bool bFireButtonPressed;
	bool bShouldFire;
	FTimerHandle AutoFireTimer;

	bool bShouldTraceForItems;
	int8 OverlappedItemCount;

	/* Sound timer properties */
	FTimerHandle PickupSoundTimer;
	FTimerHandle EquipSoundTimer;
	bool bShouldPlayPickupSound;
	bool bShouldPlayEquipSound;
};
