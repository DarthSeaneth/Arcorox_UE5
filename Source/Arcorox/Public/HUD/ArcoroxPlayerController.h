// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArcoroxPlayerController.generated.h"

UCLASS()
class ARCOROX_API AArcoroxPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AArcoroxPlayerController();

protected:
	virtual void BeginPlay() override;

private:
	/* HUD Overlay Widget Blueprint class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widgets, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> HUDOverlayClass;

	/* HUD Overlay Widget object pointer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widgets, meta = (AllowPrivateAccess = "true"))
	UUserWidget* HUDOverlay;
};
