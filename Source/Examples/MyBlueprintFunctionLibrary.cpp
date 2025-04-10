// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "SoundFileIO/SoundFileIO.h"
#include "Sound/SoundWaveProcedural.h"
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

    USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>();
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
        // This code is editor only
        // Sound->RawData.UpdatePayload(FSharedBuffer::Clone(RawWaveData.GetData(), RawWaveData.Num()));
        Sound->QueueAudio(WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);
    }

    Sound->Duration = (float)NumFrames / *WaveInfo.pSamplesPerSec;
    Sound->SetImportedSampleRate(*WaveInfo.pSamplesPerSec);
    Sound->SetSampleRate(*WaveInfo.pSamplesPerSec);
    Sound->NumChannels = ChannelCount;
    Sound->TotalSamples = *WaveInfo.pSamplesPerSec * Sound->Duration;

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
    double LoadingStartTime = FPlatformTime::Seconds();
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
    UE_LOG(LogMyExamples, Log, TEXT("CreateSoundWaveFromFile in %lf seconds"), FPlatformTime::Seconds() - LoadingStartTime);
    return SoundWave;
}
