// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosive/Explosive.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

AExplosive::AExplosive()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AExplosive::BeginPlay()
{
	Super::BeginPlay();
	
}

void AExplosive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AExplosive::SpawnExplosionParticles(FHitResult& HitResult)
{
	if (ExplosionParticles) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticles, HitResult.Location);
}

void AExplosive::PlayExplosionSound()
{
	if (ExplosionSound) UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
}

void AExplosive::Hit_Implementation(FHitResult HitResult)
{
	PlayExplosionSound();
	SpawnExplosionParticles(HitResult);
	Destroy();
}

