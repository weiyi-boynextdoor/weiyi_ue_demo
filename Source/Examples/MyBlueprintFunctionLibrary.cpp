// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "SoundFileIO/SoundFileIO.h"
#include "Examples.h"

// Refer to USoundFactory::CreateObject in SoundFactory.cpp
static USoundWave* _CreateSoundWaveFromFile(const TArray<uint8>& RawWaveData)
{
    FWaveModInfo WaveInfo;
    FString ErrorMessage;
    if (!WaveInfo.ReadWaveInfo(RawWaveData.GetData(), RawWaveData.Num(), &ErrorMessage))
    {
        UE_LOG(LogMyExamples, Error, TEXT("Unable to read wave file - \"%s\""), *ErrorMessage);
        return nullptr;
    }

    USoundWave* Sound = NewObject<USoundWave>();
    int32 ChannelCount = (int32)*WaveInfo.pChannels;
    check(ChannelCount >0);
    int32 SizeOfSample = (*WaveInfo.pBitsPerSample) / 8;
    int32 NumSamples = WaveInfo.SampleDataSize / SizeOfSample;
    int32 NumFrames = NumSamples / ChannelCount;

    if (ChannelCount > 2)
    {
        UE_LOG(LogMyExamples, Error, TEXT("Wave file has unsupported number of channels %d"), ChannelCount);
        return nullptr;
    }
    else
    {
        // For mono and stereo assets, just copy the data into the buffer
        // Clone directly as a param so that if anyone MoveToUniques it then its a steal not a copy.
        Sound->RawData.UpdatePayload(FSharedBuffer::Clone(RawWaveData.GetData(), RawWaveData.Num()));

    }

    Sound->Duration = (float)NumFrames / *WaveInfo.pSamplesPerSec;
    Sound->SetImportedSampleRate(*WaveInfo.pSamplesPerSec);
    Sound->SetSampleRate(*WaveInfo.pSamplesPerSec);
    Sound->NumChannels = ChannelCount;
    Sound->TotalSamples = *WaveInfo.pSamplesPerSec * Sound->Duration;

    // Compressed data is now out of date.
    Sound->InvalidateCompressedData(true /* bFreeResources */);

    // If stream caching is enabled, we need to make sure this asset is ready for playback.
    if (Sound->IsStreaming(nullptr))
    {
        Sound->LoadZerothChunk();
    }

    return Sound;
}

USoundWave* UMyBlueprintFunctionLibrary::CreateSoundWaveFromFile(const FString& FilePath)
{
    TArray<uint8> FileContent;
    if (!FFileHelper::LoadFileToArray(FileContent, *FilePath))
    {
        UE_LOG(LogMyExamples, Error, TEXT("Failed to load file at path: %s"), *FilePath);
        return nullptr;
    }
    USoundWave* SoundWave = nullptr;
    if (FilePath.ToLower().EndsWith(".wav"))
    {
        SoundWave = _CreateSoundWaveFromFile(FileContent);
    }
    else
    {
        TArray<uint8> RawWaveData;
        if (Audio::SoundFileUtils::ConvertAudioToWav(FileContent, RawWaveData))
        {
            SoundWave = _CreateSoundWaveFromFile(RawWaveData);
        }
    }
    if (!SoundWave)
    {
        UE_LOG(LogMyExamples, Error, TEXT("Failed to create sound wave from file at path: %s"), *FilePath);
    }
    return SoundWave;
}
