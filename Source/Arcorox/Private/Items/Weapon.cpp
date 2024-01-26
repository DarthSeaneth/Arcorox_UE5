// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon.h"
#include "Items/Weapon.h"

AWeapon::AWeapon():
	ThrowWeaponTime(0.7f),
	bIsFalling(false),
	Ammo(30),
	MagazineCapacity(30),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9mm),
	ReloadMontageSection(FName(TEXT("Reload SMG"))),
	ClipBoneName(FName(TEXT("smg_clip"))),
	PistolSlideDisplacement(0.f),
	PistolSlideTime(0.2f),
	bDisplacingPistolSlide(false),
	PistolSlideDistance(4.f),
	TargetPistolRecoilRotation(20.f),
	PistolRecoilRotation(0.f),
	bAutomaticWeapon(true)
{
	PrimaryActorTick.bCanEverTick = true;


}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetItemState() == EItemState::EIS_Falling && bIsFalling)
	{
		const FRotator Rotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
		GetItemMesh()->SetWorldRotation(Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	UpdatePistolSlideDisplacement();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	SetItemType(EItemType::EIT_Weapon);

	if (BoneToHide != FName()) GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GetWeaponTypeDataTableInfo();
	InitializeDynamicMaterialInstance();
}

void AWeapon::GetWeaponTypeDataTableInfo()
{
	const FString WeaponTypeDataTablePath(TEXT("/Script/Engine.DataTable'/Game/Dynamic/Blueprints/DataTables/WeaponTypeDataTable.WeaponTypeDataTable'"));
	UDataTable* WeaponTypeDataTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTypeDataTablePath));
	if (WeaponTypeDataTableObject)
	{
		FWeaponTypeTable* WeaponTypeRow = nullptr;
		switch (WeaponType)
		{
		case EWeaponType::EWT_SubmachineGun:
			WeaponTypeRow = WeaponTypeDataTableObject->FindRow<FWeaponTypeTable>(FName("SubmachineGun"), TEXT(""));
			break;
		case EWeaponType::EWT_AssaultRifle:
			WeaponTypeRow = WeaponTypeDataTableObject->FindRow<FWeaponTypeTable>(FName("AssaultRifle"), TEXT(""));
			break;
		case EWeaponType::EWT_Pistol:
			WeaponTypeRow = WeaponTypeDataTableObject->FindRow<FWeaponTypeTable>(FName("Pistol"), TEXT(""));
			break;
		}
		SetDataTableProperties(WeaponTypeRow);
	}
}

void AWeapon::SetDataTableProperties(FWeaponTypeTable* WeaponTypeRow)
{
	if (WeaponTypeRow)
	{
		AmmoType = WeaponTypeRow->AmmoType;
		Ammo = WeaponTypeRow->AmmoCount;
		MagazineCapacity = WeaponTypeRow->MagazineCapacity;
		ClipBoneName = WeaponTypeRow->ClipBoneName;
		ReloadMontageSection = WeaponTypeRow->ReloadMontageSection;
		TopCrosshair = WeaponTypeRow->TopCrosshair;
		BottomCrosshair = WeaponTypeRow->BottomCrosshair;
		MidCrosshair = WeaponTypeRow->MidCrosshair;
		LeftCrosshair = WeaponTypeRow->LeftCrosshair;
		RightCrosshair = WeaponTypeRow->RightCrosshair;
		FireRate = WeaponTypeRow->FireRate;
		MuzzleFlash = WeaponTypeRow->MuzzleFlash;
		FireSound = WeaponTypeRow->FireSound;
		BoneToHide = WeaponTypeRow->BoneToHide;
		bAutomaticWeapon = WeaponTypeRow->bAutomaticWeapon;
		Damage = WeaponTypeRow->Damage;
		HeadshotMultiplier = WeaponTypeRow->HeadshotMultiplier;
		SetPickupSound(WeaponTypeRow->PickupSound);
		SetEquipSound(WeaponTypeRow->EquipSound);
		GetItemMesh()->SetSkeletalMesh(WeaponTypeRow->WeaponMesh);
		SetItemName(WeaponTypeRow->WeaponName);
		SetItemIcon(WeaponTypeRow->InventoryIcon);
		SetAmmoIcon(WeaponTypeRow->AmmoIcon);
		SetMaterialInstance(WeaponTypeRow->MaterialInstance);
		PreviousMaterialIndex = GetMaterialIndex();
		if (GetItemMesh())
		{
			GetItemMesh()->SetAnimInstanceClass(WeaponTypeRow->AnimationBlueprint);
			GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
		}
		SetMaterialIndex(WeaponTypeRow->MaterialIndex);
	}
}

void AWeapon::ThrowWeapon()
{
	FRotator Rotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
	GetItemMesh()->SetWorldRotation(Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	const FVector ForwardVector{ GetItemMesh()->GetForwardVector() };
	const FVector RightVector{ GetItemMesh()->GetRightVector() };
	FVector ImpulseVector = RightVector.RotateAngleAxis(-20.f, ForwardVector);
	float RandomRotation{ FMath::FRandRange(10.f, 60.f) };
	ImpulseVector = ImpulseVector.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
	ImpulseVector *= 10000.f;
	GetItemMesh()->AddImpulse(ImpulseVector);
	bIsFalling = true;
	EnableGlowMaterial();
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);
}

void AWeapon::DecrementAmmo()
{
	if (Ammo - 1 < 0) Ammo = 0;
	else --Ammo;
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Reload with more than magazine capacity"));
	Ammo += Amount;
}

bool AWeapon::FullMagazine()
{
	return Ammo == MagazineCapacity;
}

void AWeapon::StartPistolSlideTimer()
{
	bDisplacingPistolSlide = true;
	GetWorldTimerManager().SetTimer(PistolSlideTimer, this, &AWeapon::FinishPistolSlideDisplacement, PistolSlideTime);
}

void AWeapon::StopFalling()
{
	bIsFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	StartMaterialPulseTimer();
}

void AWeapon::FinishPistolSlideDisplacement()
{
	bDisplacingPistolSlide = false;
}

void AWeapon::UpdatePistolSlideDisplacement()
{
	if (!bDisplacingPistolSlide || PistolSlideCurve == nullptr) return;
	const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PistolSlideTimer);
	const float CurveValue = PistolSlideCurve->GetFloatValue(ElapsedTime);
	PistolSlideDisplacement = CurveValue * PistolSlideDistance;
	PistolRecoilRotation = CurveValue * TargetPistolRecoilRotation;
}
