// Copyright Epic Games, Inc. All Rights Reserved.

#include "AerosimConnector.h"
#include "CesiumIonServer.h"

#define LOCTEXT_NAMESPACE "FAerosimConnectorModule"

DEFINE_LOG_CATEGORY(LogAerosimConnector);

void FAerosimConnectorModule::StartupModule()
{
	ModifyCesiumTokenDataAsset();
}

void FAerosimConnectorModule::ShutdownModule()
{
}

void FAerosimConnectorModule::ModifyCesiumTokenDataAsset()
{
	// Retrieve the Cesium token variable
	FString EnvVarValue = FPlatformMisc::GetEnvironmentVariable(TEXT("AEROSIM_CESIUM_TOKEN"));
	if (EnvVarValue.IsEmpty())
	{
		UE_LOG(LogAerosimConnector, Warning, TEXT("Environment variable AEROSIM_CESIUM_TOKEN is not set or empty."));
		UE_LOG(LogAerosimConnector, Log, TEXT("Reading command line for 'CesiumToken' argument."));
		if (!FParse::Value(FCommandLine::Get(), TEXT("CesiumToken="), EnvVarValue))
		{
			UE_LOG(LogAerosimConnector, Error, TEXT("'CesiumToken' argument not found!"));
		}
	}

	if (EnvVarValue.IsEmpty())
	{
		return;
	}

	UCesiumIonServer* Server = UCesiumIonServer::GetDefaultServer();
	if (!Server)
	{
		UE_LOG(LogAerosimConnector, Warning, TEXT("Failed to get default UCesiumIonServer."));
		return;
	}

	Server->DefaultIonAccessToken = EnvVarValue;
	Server->Modify();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAerosimConnectorModule, AerosimConnector)
