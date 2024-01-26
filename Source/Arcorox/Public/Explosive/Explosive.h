// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/HitInterface.h"
#include "Explosive.generated.h"

class UParticleSystem;

UCLASS()
class ARCOROX_API AExplosive : public AActor, public IHitInterface
{
	GENERATED_BODY()
	
public:	
	AExplosive();

	virtual void Tick(float DeltaTime) override;
	virtual void Hit_Implementation(FHitResult HitResult) override;

protected:
	virtual void BeginPlay() override;

private:	
	void SpawnExplosionParticles(FHitResult& HitResult);
	void PlayExplosionSound();

	/* Particles for explosion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ExplosionParticles;

	/* Sound for explosion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundBase* ExplosionSound;
};
