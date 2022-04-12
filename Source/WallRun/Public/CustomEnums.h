// Copyright 2020-Present Oyintare Ebelo. All Rights Reserved.   

#pragma once

#include "CoreMinimal.h"
#include "CustomEnums.generated.h"

// Custom movement modes
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_PlaceHolder UMETA(DisplayName = "PlaceHolder"),
	CMOVE_WallRunning UMETA(DisplayName = "WallRunning"),
	CMOVE_Sliding UMETA(DisplayName = "Sliding")
};

//The side of the character the wall is on
UENUM(BlueprintType)
enum EWallRunSide
{
	Side_Left UMETA(DisplayName = "Left"),
	Side_Right UMETA(DisplayName = "Right")
};