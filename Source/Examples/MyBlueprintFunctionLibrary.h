// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sound/MySoundWave.h"
#include "MyBlueprintFunctionLibrary.generated.h"


DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSoundWaveDelegate, UMySoundWave*, SoundWave);

UCLASS()
class EXAMPLES_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	 UFUNCTION(BlueprintCallable, Category = "Example")
	 static void CreateSoundWaveFromFile(const FString& FilePath, const FOnSoundWaveDelegate& SoundWaveCallback);
};
