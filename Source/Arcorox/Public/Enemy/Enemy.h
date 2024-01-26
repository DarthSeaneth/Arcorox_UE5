// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Enemy.generated.h"

class UParticleSystem;
class UAnimMontage;
class USphereComponent;
class UBoxComponent;
class AEnemyController;
class UBehaviorTree;
class AArcoroxCharacter;

UCLASS()
class ARCOROX_API AEnemy : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	AEnemy();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Hit_Implementation(FHitResult HitResult) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowHitDamage(int32 Damage, FVector HitLocation, bool bHeadshot);

	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent)
	void ShowHealthBar();

	UFUNCTION(BlueprintImplementableEvent)
	void HideHealthBar();

	UFUNCTION(BlueprintCallable)
	void StoreHitDamage(UUserWidget* Widget, FVector Location);

	UFUNCTION(BlueprintCallable)
	void SetStunned(bool Stunned);

	UFUNCTION(BlueprintCallable)
	void PlayAttackMontage(float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable)
	void ActivateLeftWeapon();

	UFUNCTION(BlueprintCallable)
	void DeactivateLeftWeapon();

	UFUNCTION(BlueprintCallable)
	void ActivateRightWeapon();

	UFUNCTION(BlueprintCallable)
	void DeactivateRightWeapon();

	UFUNCTION(BlueprintCallable)
	void FinishDeath();

	UFUNCTION()
	void DestroyHitDamage(UUserWidget* HitDamage);

	/* Called when an actor overlaps with AggroSphere */
	UFUNCTION()
	void AggroSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* Called when an actor overlaps with AttackRangeSphere */
	UFUNCTION()
	void AttackRangeSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* Called when actor stops overlapping with AttackRangeSphere */
	UFUNCTION()
	void AttackRangeSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/* Called when actor overlaps with LeftWeaponBox */
	UFUNCTION()
	void LeftWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* Called when actor overlaps with RightWeaponBox */
	UFUNCTION()
	void RightWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void DestroyEnemy();

	void ShowHealthBar_Implementation();

	void SetInAttackRange(bool InRange);

	/* Called when Health reaches 0 */
	void Die(const FHitResult& HitResult);
	void Die();

	void ResetHitReactTimer();

	void UpdateHitDamages();

	/* Attempt to stun character on hit */
	void StunCharacter(AArcoroxCharacter* Character, const FHitResult& HitResult);

	void ResetAttackTimer();

private:	
	void PlayImpactSound();
	void SpawnImpactParticles(FHitResult& HitResult);
	void PlayMontageSection(UAnimMontage* Montage, const FName& SectionName, float PlayRate);
	void PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionNames, float PlayRate);
	void PlayHitMontage(FHitResult& HitResult, float PlayRate = 1.f);
	void PlayDeathMontage(const FHitResult& HitResult);
	void InflictDamage(AArcoroxCharacter* ArcoroxCharacter, const FName& WeaponSocket);

	/* Current health of enemy */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float Health;

	/* Maximum health value of enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	/* Particles spawned when hit by bullets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	/* Sound played when hit by bullets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundBase* ImpactSound;

	/* Name of head bone on skeleton for headshot damage functionality */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FString HeadBone;

	/* Hit React animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HitMontage;

	/* Attack animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* AttackMontage;

	/* Death animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* DeathMontage;

	/* Array of Attack Montage section names */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TArray<FName> AttackMontageSections;

	/* How long Health Bar should be displayed when enemy is hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HealthBarDisplayTime;

	/* Minimum hit react delay time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MinHitReactTime;

	/* Maximum hit react delay time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MaxHitReactTime;

	/* Map of Hit Damage widgets and their locations */
	UPROPERTY(VisibleAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TMap<UUserWidget*, FVector> HitDamages;

	/* How long Hit Damage widgets will persist on screen */
	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HitDamageDestroyTime;

	/* Overlap sphere for Enemy to become hostile toward player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USphereComponent* AggroSphere;

	/* Overlap sphere for Enemy to attack player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USphereComponent* AttackRangeSphere;

	/* Is Enemy playing hit react animation */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bStunned;

	/* Chance of Enemy being stunned when hit (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float StunChance;

	/* Is player in attack range of Enemy */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bInAttackRange;

	/* Collision volume for left weapon */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWeaponBox;

	/* Collision volume for right weapon */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWeaponBox;

	/* How much damage the Enemy inflicts per weapon hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float WeaponDamage;

	/* Name of left weapon bone socket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FName LeftWeaponSocket;

	/* Name of right weaponn bone socket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FName RightWeaponSocket;

	/* Can the enemy attack the character */
	UPROPERTY(VisibleAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bCanAttack;

	/* Duration of Attack Timer */
	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float AttackDelayTime;

	/* Delay after death to destroy Enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float DestroyDelay;

	/* Behavior Tree for Enemy AI Character */
	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* BehaviorTree;

	/* Location for Enemy to patrol to */
	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint;

	/* Second location for Enemy to patrol to */
	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint2;

	/* Pointer to Enemy AI Controller instance */
	AEnemyController* EnemyController;

	/* FTimerHandle for displaying the Health Bar */
	FTimerHandle HealthBarDisplayTimer;

	/* FTimerHandle for hit react delay */
	FTimerHandle HitReactTimer;

	/* FTimerHandle for attack delay */
	FTimerHandle AttackTimer;

	/* FTimerHandle for death destroy delay */
	FTimerHandle DestroyTimer;

	/* Can the Hit React montage be played */
	bool bCanHitReact;

	/* Is Enemy dead */
	bool bDead;
};
