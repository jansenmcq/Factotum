// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Factotum.h"
#include "FactotumGameMode.h"
#include "FactotumCharacter.h"

AFactotumGameMode::AFactotumGameMode()
{
	// set default pawn class to our character
	DefaultPawnClass = AFactotumCharacter::StaticClass();	
}
