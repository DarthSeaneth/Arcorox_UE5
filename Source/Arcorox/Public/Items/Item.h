// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Item.generated.h"

class UBoxComponent;
class UWidgetComponent;
class USphereComponent;
class UCurveFloat;
class UCurveVector;
class AArcoroxCharacter;
class UDataTable;

UENUM(BlueprintType)
enum class EItemState : uint8
{
	EIS_Pickup UMETA(DisplayName = "Pickup"),
	EIS_EquipInterpolating UMETA(DisplayName = "EquipInterpolating"),
	EIS_PickedUp UMETA(DisplayName = "PickedUp"),
	EIS_Equipped UMETA(DisplayName = "Equipped"),
	EIS_Falling UMETA(DisplayName = "Falling"),

	EIS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	EIR_Damaged UMETA(DisplayName = "Damaged"),
	EIR_Common UMETA(DisplayName = "Common"),
	EIR_Uncommon UMETA(DisplayName = "Uncommon"),
	EIR_Rare UMETA(DisplayName = "Rare"),
	EIR_Legendary UMETA(DisplayName = "Legendary"),

	EIR_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	EIT_Weapon UMETA(DisplayName = "Weapon"),
	EIT_Ammo UMETA(DisplayName = "Ammo"),

	EIT_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FItemRarityTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor GlowColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor DarkColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumStars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* IconBackground;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CustomDepthStencil;
};

UCLASS()
class ARCOROX_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	AItem();
	virtual void Tick(float DeltaTime) override;

	virtual void EnableCustomDepth();
	virtual void DisableCustomDepth();
	virtual void InitializeCustomDepth();

	void ShowPickupWidget();
	void HidePickupWidget();
	void DisableSphereCollision();
	void DisableBoxCollision();
	void DisableMeshCollision();
	void SetItemState(EItemState State);
	void StartItemCurve(AArcoroxCharacter* Character);
	void PlayPickupSound();
	void PlayEquipSound();
	void ForcePlayEquipSound();

	void InitializeDynamicMaterialInstance();
	void EnableGlowMaterial();
	void DisableGlowMaterial();

	FORCEINLINE EItemState GetItemState() const { return ItemState; }
	FORCEINLINE USkeletalMeshComponent* GetItemMesh() const { return ItemMesh; }
	FORCEINLINE UBoxComponent* GetCollisionBox() const { return CollisionBox; }
	FORCEINLINE USphereComponent* GetOverlapSphere() const { return OverlapSphere; }
	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }
	FORCEINLINE USoundBase* GetPickupSound() const { return PickupSound; }
	FORCEINLINE USoundBase* GetEquipSound() const { return EquipSound; }
	FORCEINLINE int32 GetItemCount() const { return ItemCount; }
	FORCEINLINE EItemType GetItemType() const { return ItemType; }
	FORCEINLINE int32 GetInventorySlotIndex() const { return InventorySlotIndex; }
	FORCEINLINE UMaterialInstance* GetMaterialInstance() const { return MaterialInstance; }
	FORCEINLINE int32 GetMaterialIndex() const { return MaterialIndex; }
	FORCEINLINE void SetItemType(EItemType Type) { ItemType = Type; }
	FORCEINLINE void SetInventorySlotIndex(int32 Index) { InventorySlotIndex = Index; }
	FORCEINLINE void SetArcoroxCharacter(AArcoroxCharacter* Character) { ArcoroxCharacter = Character; }
	FORCEINLINE void SetCharacterInventoryFull(bool bInventoryFull) { bCharacterInventoryFull = bInventoryFull; }
	FORCEINLINE void SetPickupSound(USoundBase* Sound) { PickupSound = Sound; }
	FORCEINLINE void SetEquipSound(USoundBase* Sound) { EquipSound = Sound; }
	FORCEINLINE void SetItemName(FString Name) { ItemName = Name; }
	FORCEINLINE void SetItemIcon(UTexture2D* Icon) { ItemIcon = Icon; }
	FORCEINLINE void SetAmmoIcon(UTexture2D* Icon) { AmmoIcon = Icon; }
	FORCEINLINE void SetMaterialInstance(UMaterialInstance* MatInst) { MaterialInstance = MatInst; }
	FORCEINLINE void SetMaterialIndex(int32 Index) { MaterialIndex = Index; }

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	void GetItemRarityDataTableInfo();

	/* Sets Item properties based on ItemState */
	virtual void SetItemProperties(EItemState State);

	/* Sets ActiveStars boolean array based on item rarity */
	void SetActiveStars();

	/* Interpolates Item when in interpolation state */
	void ItemInterpolation(float DeltaTime);

	/* Callback for end of ItemInterpolationTimer */
	void FinishInterpolating();

	/* Get interpolation location based on item type and interp location index */
	FVector GetInterpLocation();

	/* Updates dynamic material instance parameters based on pulse vector curve */
	void UpdateMaterialPulse();

	/* Starts the Material Pulse Timer */
	void StartMaterialPulseTimer();

	/* Callback for Material Pulse Timer */
	void ResetMaterialPulseTimer();

	/* Callback for Sphere Component OnComponentBeginOverlap */
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* Callback for Sphere Component OnComponentEndOverlap */
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/* Item Skeletal Mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ItemMesh;

	/* Item collision box component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionBox;

	/* Popup Widget showing item properties */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* PickupWidget;

	/* Sphere component for overlap events */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USphereComponent* OverlapSphere;

	/* Item Name on pickup widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FString ItemName;

	/* Item Count for ammo and other item properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 ItemCount;

	/* Item Rarity - determines star count of pickup widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Rarity", meta = (AllowPrivateAccess = "true"))
	EItemRarity ItemRarity;

	/* Array of booleans to determine which stars to show in the pickup widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	TArray<bool> ActiveStars;

	/* Item State - determines behavior */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	EItemState ItemState;

	/* Curve to use for Item Z location when interpolating */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ItemZCurve;

	/* Curve to scale item when interpolating */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ItemScaleCurve;

	/* Start location for item interpolation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FVector ItemInterpStartLocation;

	/* Is the item interpolating */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	bool bIsInterpolating;

	/* Reference to Arcorox Character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	AArcoroxCharacter* ArcoroxCharacter;

	/* Duration of timer and curve */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float ZCurveTime;

	/* Sound for picking up the item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USoundBase* PickupSound;

	/* Sound for equipping the item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USoundBase* EquipSound;

	/* Type of Item (Weapon or Ammo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	EItemType ItemType;

	/* Index for interp location array that item will interp to */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 InterpLocationIndex;

	/* Index for Material element to change on Item Mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 MaterialIndex;

	/* Dynamic material instance to change at runtime */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	/* Material instance to be used with dynamic material instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* MaterialInstance;

	/* Vector curve to manipulate dynamic material parameters */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UCurveVector* MaterialPulseCurve;

	/* Vector curve to manipulate dynamic material parameters for when item is interpolating */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UCurveVector* MaterialPulseInterpCurve;

	/* Duration of material pulse timer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float MaterialPulseCurveTime;

	/* Glow amount parameter of material instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float GlowAmount;

	/* Fresnel exponent parameter of material instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float FresnelExponent;

	/* Fresnel reflect fraction parameter of material instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float FresnelReflectFraction;

	/* Item icon image in the inventory */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	UTexture2D* ItemIcon;

	/* Ammo icon image in the inventory */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	UTexture2D* AmmoIcon;

	/* Index of item in character inventory */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	int32 InventorySlotIndex;

	/* Is character's inventory full */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool bCharacterInventoryFull;

	/* Item Rarity Data Table */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UDataTable* ItemRarityDataTable;

	/* Color of material glow */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Rarity", meta = (AllowPrivateAccess = "true"))
	FLinearColor GlowColor;

	/* Light color of pickup widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Rarity", meta = (AllowPrivateAccess = "true"))
	FLinearColor LightColor;

	/* Dark color of pickup widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Rarity", meta = (AllowPrivateAccess = "true"))
	FLinearColor DarkColor;

	/* Amount of stars on pickup widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Rarity", meta = (AllowPrivateAccess = "true"))
	int32 NumStars;

	/* Background icon image in the inventory and pickup widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Rarity", meta = (AllowPrivateAccess = "true"))
	UTexture2D* BackgroundIcon;

	/* Timer for dynamic material pulse curve */
	FTimerHandle MaterialPulseTimer;

	/* For interpolating item in X and Y directions */
	float ItemInterpX;
	float ItemInterpY;

	/* Timer for item interpolation */
	FTimerHandle ItemInterpolationTimer;

	/* Initial offset of Yaw between Item and Camera */
	float InterpInitialYawOffset;

	/* To enable and disable outline effect while interpolating */
	bool bCanChangeCustomDepth;

};
