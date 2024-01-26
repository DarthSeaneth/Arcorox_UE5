#pragma once

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm UMETA(DisplayName = "9mm"),
	EAT_556 UMETA(DisplayName = "5.56 x 45mm NATO"),

	EAT_MAX UMETA(DisplayName = "DefaultMAX")
};