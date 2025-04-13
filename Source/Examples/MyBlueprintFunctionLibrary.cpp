// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "SoundFileIO/SoundFileIO.h"
#include "Sound/MySoundWave.h"
#include "Examples.h"
#include "Interfaces/IAudioFormat.h"
#include "Decoders/VorbisAudioInfo.h"

static UMySoundWave* _CreateSoundWaveFromWav(const TArray<uint8>& RawWaveData)
{
    FWaveModInfo WaveInfo;
    FString ErrorMessage;
    if (!WaveInfo.ReadWaveInfo(RawWaveData.GetData(), RawWaveData.Num(), &ErrorMessage))
    {
        UE_LOG(LogMyExamples, Error, TEXT("Unable to read wave file - \"%s\""), *ErrorMessage);
        return nullptr;
    }

    UMySoundWave* Sound = NewObject<UMySoundWave>();
    int32 ChannelCount = (int32)*WaveInfo.pChannels;
    check(ChannelCount > 0);
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
        Sound->SetAudio(WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);
    }

    Sound->Duration = (float)NumFrames / *WaveInfo.pSamplesPerSec;
    Sound->SetImportedSampleRate(*WaveInfo.pSamplesPerSec);
    Sound->SetSampleRate(*WaveInfo.pSamplesPerSec);
    Sound->NumChannels = ChannelCount;
    Sound->TotalSamples = *WaveInfo.pSamplesPerSec * Sound->Duration;

    return Sound;
}

static UMySoundWave* _CreateSoundWaveFromOgg(const TArray<uint8>& OggData)
{
    FVorbisAudioInfo	AudioInfo;
    FSoundQualityInfo	QualityInfo;
    if (!AudioInfo.ReadCompressedInfo(OggData.GetData(), OggData.Num(), &QualityInfo))
    {
        return nullptr;
    }
    TArray<uint8> PCMData;
    PCMData.AddUninitialized(QualityInfo.SampleDataSize);
    AudioInfo.ReadCompressedData(PCMData.GetData(), false, QualityInfo.SampleDataSize);
    UMySoundWave* Sound = NewObject<UMySoundWave>();
    int32 ChannelCount = QualityInfo.NumChannels;
    check(ChannelCount > 0);
    Sound->Duration = QualityInfo.Duration;
    Sound->SetImportedSampleRate(QualityInfo.SampleRate);
    Sound->SetSampleRate(QualityInfo.SampleRate);
    Sound->NumChannels = QualityInfo.NumChannels;
    Sound->TotalSamples = QualityInfo.Duration * QualityInfo.Duration;
    Sound->SetAudio(PCMData.GetData(), PCMData.Num());
    return Sound;
}

UMySoundWave* UMyBlueprintFunctionLibrary::CreateSoundWaveFromFile(const FString& FilePath)
{
    TArray<uint8> FileContent;
    if (!FFileHelper::LoadFileToArray(FileContent, *FilePath))
    {
        UE_LOG(LogMyExamples, Error, TEXT("Failed to load file at path: %s"), *FilePath);
        return nullptr;
    }
    double LoadingStartTime = FPlatformTime::Seconds();
    UMySoundWave* SoundWave = nullptr;
    if (FilePath.ToLower().EndsWith(".wav"))
    {
        SoundWave = _CreateSoundWaveFromWav(FileContent);
    }
    else if (FilePath.ToLower().EndsWith(".ogg"))
    {
        SoundWave = _CreateSoundWaveFromOgg(FileContent);
    }
    else
    {
        // UE runtime only supports wav or ogg
    }
    if (!SoundWave)
    {
        UE_LOG(LogMyExamples, Error, TEXT("Failed to create sound wave from file at path: %s"), *FilePath);
    }
    UE_LOG(LogMyExamples, Log, TEXT("CreateSoundWaveFromFile in %lf seconds"), FPlatformTime::Seconds() - LoadingStartTime);
    return SoundWave;
}
