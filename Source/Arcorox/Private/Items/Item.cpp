// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Item.h"
#include "Characters/ArcoroxCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Curves/CurveVector.h"

AItem::AItem() :
	ItemName(FString("Item")),
	ItemCount(0),
	ItemRarity(EItemRarity::EIR_Common),
	ItemState(EItemState::EIS_Pickup),
	ZCurveTime(0.7f),
	ItemInterpStartLocation(FVector(0.f)),
	bIsInterpolating(false),
	ItemInterpX(30.f),
	ItemInterpY(30.f),
	InterpInitialYawOffset(0.f),
	ItemType(EItemType::EIT_MAX),
	InterpLocationIndex(1),
	MaterialIndex(0),
	bCanChangeCustomDepth(true),
	MaterialPulseCurveTime(5.f),
	GlowAmount(150.f),
	FresnelExponent(3.f),
	FresnelReflectFraction(4.f),
	InventorySlotIndex(0),
	bCharacterInventoryFull(false)
{
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetEnableGravity(false);
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(GetRootComponent());
}

void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Interpolate item when in interpolating state 
	ItemInterpolation(DeltaTime);

	UpdateMaterialPulse();
}

void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	HidePickupWidget();
	SetActiveStars();

	//Setup overlap for sphere component
	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	OverlapSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);

	SetItemProperties(ItemState);
	InitializeCustomDepth();
	StartMaterialPulseTimer();
}

void AItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GetItemRarityDataTableInfo();
	InitializeDynamicMaterialInstance();
}

void AItem::InitializeDynamicMaterialInstance()
{
	if (MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
		EnableGlowMaterial();
	}
}

void AItem::GetItemRarityDataTableInfo()
{
	FString ItemRarityDataTablePath(TEXT("/Script/Engine.DataTable'/Game/Dynamic/Blueprints/DataTables/ItemRarityDataTable.ItemRarityDataTable'"));
	UDataTable* ItemRarityDataTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *ItemRarityDataTablePath));
	if (ItemRarityDataTableObject)
	{
		FItemRarityTable* RarityRow = nullptr;
		switch (ItemRarity)
		{
		case EItemRarity::EIR_Damaged:
			RarityRow = ItemRarityDataTableObject->FindRow<FItemRarityTable>(FName("Damaged"), TEXT(""));
			break;
		case EItemRarity::EIR_Common:
			RarityRow = ItemRarityDataTableObject->FindRow<FItemRarityTable>(FName("Common"), TEXT(""));
			break;
		case EItemRarity::EIR_Uncommon:
			RarityRow = ItemRarityDataTableObject->FindRow<FItemRarityTable>(FName("Uncommon"), TEXT(""));
			break;
		case EItemRarity::EIR_Rare:
			RarityRow = ItemRarityDataTableObject->FindRow<FItemRarityTable>(FName("Rare"), TEXT(""));
			break;
		case EItemRarity::EIR_Legendary:
			RarityRow = ItemRarityDataTableObject->FindRow<FItemRarityTable>(FName("Legendary"), TEXT(""));
			break;
		}
		if (RarityRow)
		{
			GlowColor = RarityRow->GlowColor;
			LightColor = RarityRow->LightColor;
			DarkColor = RarityRow->DarkColor;
			NumStars = RarityRow->NumStars;
			BackgroundIcon = RarityRow->IconBackground;
			if (GetItemMesh()) GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
		}
	}
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		ArcoroxCharacter = Cast<AArcoroxCharacter>(OtherActor);
		if (ArcoroxCharacter)ArcoroxCharacter->IncrementOverlappedItemCount(1);
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		ArcoroxCharacter = Cast<AArcoroxCharacter>(OtherActor);
		if (ArcoroxCharacter)
		{
			ArcoroxCharacter->IncrementOverlappedItemCount(-1);
			ArcoroxCharacter->UnhighlightInventorySlot();
		}
	}
}

void AItem::SetActiveStars()
{
	for (int32 i = 0; i < 6; i++) ActiveStars.Add(false);
	switch (ItemRarity)
	{
		case EItemRarity::EIR_Damaged:
			ActiveStars[1] = true;
			break;
		case EItemRarity::EIR_Common:
			for (int32 i = 1; i < 3; i++) ActiveStars[i] = true;
			break;
		case EItemRarity::EIR_Uncommon:
			for (int32 i = 1; i < 4; i++) ActiveStars[i] = true;
			break;
		case EItemRarity::EIR_Rare:
			for (int32 i = 1; i < 5; i++) ActiveStars[i] = true;
			break;
		case EItemRarity::EIR_Legendary:
			for (int32 i = 1; i < 6; i++) ActiveStars[i] = true;
			break;
		default:
			for (int32 i = 1; i < 3; i++) ActiveStars[i] = true;
			break;
	}
}

void AItem::SetItemProperties(EItemState State)
{
	switch (State)
	{
		case EItemState::EIS_Pickup:
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			DisableMeshCollision();
			OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			DisableBoxCollision();
			CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			break;
		case EItemState::EIS_Equipped:
			HidePickupWidget();
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			DisableMeshCollision();
			DisableSphereCollision();
			DisableBoxCollision();
			break;
		case EItemState::EIS_Falling:
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetEnableGravity(true);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			DisableSphereCollision();
			DisableBoxCollision();
			break;
		case EItemState::EIS_EquipInterpolating:
			HidePickupWidget();
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			DisableMeshCollision();
			DisableSphereCollision();
			DisableBoxCollision();
			break;
		case EItemState::EIS_PickedUp:
			HidePickupWidget();
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(false);
			DisableMeshCollision();
			DisableSphereCollision();
			DisableBoxCollision();
			break;
	}
}

void AItem::ItemInterpolation(float DeltaTime)
{
	if (!bIsInterpolating) return;
	if (ArcoroxCharacter && ItemZCurve && ItemScaleCurve)
	{
		//Time since ItemInterpolationTimer started
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpolationTimer);
		//Z Curve value corresponding to elapsed time
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);
		//Initial location of Item when timer started
		FVector ItemLocation = ItemInterpStartLocation;
		//Target location in front of camera
		const FVector CameraInterpLocation = GetInterpLocation();
		//Vector from Item to CameraInterpLocation, X and Y components zeroed out
		const FVector ItemToCamera{ FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z) };
		//Scale factor for curve value
		const float DeltaZ = ItemToCamera.Size();
		//Current item location
		const FVector CurrentLocation{ GetActorLocation() };
		//Interpolated X value
		const float InterpX = FMath::FInterpTo<float>(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, ItemInterpX);
		//Interpolated Y value
		const float InterpY = FMath::FInterpTo<float>(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, ItemInterpY);
		//Set Item Location interpolated X and Y values
		ItemLocation.X = InterpX;
		ItemLocation.Y = InterpY;
		//Add curve value to Z component of ItemLocation scaled by DeltaZ
		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);
		//Current Camera rotation
		const FRotator CameraRotation{ ArcoroxCharacter->GetCamera()->GetComponentRotation() };
		//Target Item rotation
		FRotator ItemRotation{ 0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f };
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);
		//Scale Curve value corresponding to elapsed time
		const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
		//Scale item according to scale curve value
		SetActorScale3D(FVector(ScaleCurveValue));
	}
}

void AItem::FinishInterpolating()
{
	bIsInterpolating = false;
	if (ArcoroxCharacter)
	{
		ArcoroxCharacter->DecrementInterpLocationItemCount(InterpLocationIndex);
		ArcoroxCharacter->GetPickupItem(this);
		ArcoroxCharacter->UnhighlightInventorySlot();
	}
	//Scale item to original size
	SetActorScale3D(FVector(1.f));
	bCanChangeCustomDepth = true;
	DisableGlowMaterial();
	DisableCustomDepth();
}

FVector AItem::GetInterpLocation()
{
	if (ArcoroxCharacter)
	{
		if (ItemType == EItemType::EIT_Ammo) return ArcoroxCharacter->GetInterpLocation(InterpLocationIndex).SceneComponent->GetComponentLocation();
		else if (ItemType == EItemType::EIT_Weapon) return ArcoroxCharacter->GetInterpLocation(0).SceneComponent->GetComponentLocation();
	}
	return FVector(0.f);
}

void AItem::UpdateMaterialPulse()
{
	if (DynamicMaterialInstance == nullptr) return;
	float ElapsedTime;
	FVector CurveValue;
	if (ItemState == EItemState::EIS_Pickup && MaterialPulseCurve)
	{
		ElapsedTime = GetWorldTimerManager().GetTimerElapsed(MaterialPulseTimer);
		CurveValue = MaterialPulseCurve->GetVectorValue(ElapsedTime);
	}
	else if (ItemState == EItemState::EIS_EquipInterpolating && MaterialPulseInterpCurve)
	{
		ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpolationTimer);
		CurveValue = MaterialPulseInterpCurve->GetVectorValue(ElapsedTime);
	}
	DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
	DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
	DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
}

void AItem::EnableGlowMaterial()
{
	if (DynamicMaterialInstance) DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
}

void AItem::DisableGlowMaterial()
{
	if (DynamicMaterialInstance) DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
}

void AItem::EnableCustomDepth()
{
	if(bCanChangeCustomDepth && ItemMesh) ItemMesh->SetRenderCustomDepth(true);
}

void AItem::DisableCustomDepth()
{
	if(bCanChangeCustomDepth && ItemMesh) ItemMesh->SetRenderCustomDepth(false);
}

void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
}

void AItem::ShowPickupWidget()
{
	if(PickupWidget) PickupWidget->SetVisibility(true);
}

void AItem::HidePickupWidget()
{
	if(PickupWidget) PickupWidget->SetVisibility(false);
}

void AItem::DisableSphereCollision()
{
	if (OverlapSphere)
	{
		OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AItem::DisableBoxCollision()
{
	if (CollisionBox)
	{
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AItem::DisableMeshCollision()
{
	if (ItemMesh)
	{
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AItem::SetItemState(EItemState State)
{
	ItemState = State;
	SetItemProperties(ItemState);
}

void AItem::StartItemCurve(AArcoroxCharacter* Character)
{
	ArcoroxCharacter = Character;
	if (!ArcoroxCharacter->GetCamera()) return;
	InterpLocationIndex = ArcoroxCharacter->GetInterpLocationIndex();
	ArcoroxCharacter->IncrementInterpLocationItemCount(InterpLocationIndex);
	ItemInterpStartLocation = GetActorLocation();
	const float CameraYaw = ArcoroxCharacter->GetCamera()->GetComponentRotation().Yaw;
	const float ItemYaw = GetActorRotation().Yaw;
	InterpInitialYawOffset = ItemYaw - CameraYaw;
	bIsInterpolating = true;
	SetItemState(EItemState::EIS_EquipInterpolating);
	GetWorldTimerManager().ClearTimer(MaterialPulseTimer);
	PlayPickupSound();
	bCanChangeCustomDepth = false;
	GetWorldTimerManager().SetTimer(ItemInterpolationTimer, this, &AItem::FinishInterpolating, ZCurveTime);
}

void AItem::PlayPickupSound()
{
	if (ArcoroxCharacter == nullptr || PickupSound == nullptr) return;
	if (ArcoroxCharacter->ShouldPlayPickupSound())
	{
		UGameplayStatics::PlaySound2D(this, PickupSound);
		ArcoroxCharacter->StartPickupSoundTimer();
	}
}

void AItem::PlayEquipSound()
{
	if (ArcoroxCharacter == nullptr || EquipSound == nullptr) return;
	if (ArcoroxCharacter->ShouldPlayEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, EquipSound);
		ArcoroxCharacter->StartEquipSoundTimer();
	}
}

void AItem::ForcePlayEquipSound()
{
	if (ArcoroxCharacter == nullptr || EquipSound == nullptr) return;
	UGameplayStatics::PlaySound2D(this, EquipSound);
}

void AItem::StartMaterialPulseTimer()
{
	if (ItemState == EItemState::EIS_Pickup) GetWorldTimerManager().SetTimer(MaterialPulseTimer, this, &AItem::ResetMaterialPulseTimer, MaterialPulseCurveTime);
}

void AItem::ResetMaterialPulseTimer()
{
	StartMaterialPulseTimer();
}