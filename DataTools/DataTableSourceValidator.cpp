// Copyright (c) 2024 Mirror-Touch Media

#include "DataTableSourceValidator.h"

#include "DataTableEditorUtils.h"
#include "EditorFramework/AssetImportData.h"
#include "Serialization/JsonSerializer.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"


UDataTableSourceValidator::UDataTableSourceValidator()
{
	bIsEnabled = true;
}

bool UDataTableSourceValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject*	InObject, FDataValidationContext& InContext) const
{
	return DataTablesToValidate.Contains(InObject);
}

EDataValidationResult UDataTableSourceValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UE_LOG(LogTemp, Display, TEXT("Running DataTableSourceValidator on Data Table: %s"), *InAsset->GetName());
	if(UDataTable* DataTable = Cast<UDataTable>(InAsset))
	{
		const bool bChangeRequiredFromTagRenames = AutoNameDataTableRows(DataTable);

		// If there is an existing source file for this DT, export the DT data to the source file on save
		if(const FString SourceFilePath = DataTable->AssetImportData.Get()->GetFirstFilename(); !SourceFilePath.IsEmpty())
		{
			const bool bSourceDataChanged = SaveDataTableToSourceIfModified(DataTable);
			if(bChangeRequiredFromTagRenames || bSourceDataChanged)
			{
				UE_LOG(LogTemp, Display, TEXT("Source File for %s updated, resaving"), *InAsset->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("Source File for %s not updated, no re-save required"), *InAsset->GetName());
			}
		}
	}
	
	AssetPasses(InAsset);
	return EDataValidationResult::Valid;
}

bool UDataTableSourceValidator::MakeDerivedRowName_Implementation(const UDataTable* DataTable, const FName OldName, FName& NewName)
{
	NewName = OldName;
	return false;
}

bool UDataTableSourceValidator::AutoNameDataTableRows(UDataTable* DataTable)
{
	bool bChangeWasMade = false;

	// For each row in the Data Table, see if the Row Name matches the derived tag name. If not, rename the row.
	for(auto CurrentRowName : DataTable->GetRowNames())
	{
		FName DerivedRowName;

		// Check if the name getter is true (i.e., has been overridden); if so, get derived row names for each row and rename them accordingly.
		if (MakeDerivedRowName(DataTable, CurrentRowName, DerivedRowName))
		{
			if(CurrentRowName != DerivedRowName)
			{
				UE_LOG(LogTemp, Display, TEXT("Matching row name: %s (old)  vs. %s (new)"), *CurrentRowName.ToString(), *DerivedRowName.ToString());
				if(!FDataTableEditorUtils::RenameRow(DataTable, CurrentRowName, DerivedRowName))
				{
					UE_LOG(LogTemp, Display, TEXT("Row rename failed, please review: %s  vs. %s. New name may be invalid or non-unique"), *CurrentRowName.ToString(), *DerivedRowName.ToString());
				}
				bChangeWasMade = true;
			}
		}
	}

	return bChangeWasMade;
}

bool UDataTableSourceValidator::GetSourceFilePath(const UDataTable* DataTable, FString& SourcePath)
{
	if(const FString SourceFilePath = DataTable->AssetImportData.Get()->GetFirstFilename(); !SourceFilePath.IsEmpty())
	{
		SourcePath = FPaths::ConvertRelativePathToFull(SourceFilePath); // Convert to full path if necessary
		return true;
	}
	return false;
}

bool UDataTableSourceValidator::GetDataTableSourceInfo(const UDataTable* DataTable, EDataTableSourceType& SourceType, FString& SourcePath)
{
	if (GetSourceFilePath(DataTable, SourcePath))
	{
		if(SourcePath.EndsWith(TEXT(".json")))
		{
			SourceType = EDataTableSourceType::JSON;
			return true;
		}
		if(SourcePath.EndsWith(TEXT(".csv")))
		{
			SourceType = EDataTableSourceType::CSV;
			return true;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Source info not found for Data Table: %s"), *DataTable->GetName());
	return false;
}

bool UDataTableSourceValidator::SaveDataTableToSourceIfModified(const UDataTable* DataTable)
{
	EDataTableSourceType SourceType;
	FString SourcePath;
	FString DataTableAsString;

	// Check for source info, returning false if it fails (no source file, getting path fails, etc.).
	if (!GetDataTableSourceInfo(DataTable, SourceType, SourcePath))
	{
		return false;
	}

	// Check if the Data table as string is empty before proceeding
	if(SourceType == EDataTableSourceType::CSV)
	{
		DataTableAsString = DataTable->GetTableAsCSV();
	}
	else if(SourceType == EDataTableSourceType::JSON)
	{
		DataTableAsString = DataTable->GetTableAsJSON();
	}

	// First, check if the DT data and source data are identical, or if either string is empty. If so, skip the rest.
	FString SourceDataAsString;
	if(FFileHelper::LoadFileToString(SourceDataAsString, *SourcePath))
	{
		if(SourceDataAsString == DataTableAsString)
		{
			UE_LOG(LogTemp, Display, TEXT("Data Table and Source strings identical: %s"), *DataTable->GetName());
			return false;
		}
		if (SourceDataAsString.IsEmpty() || DataTableAsString.IsEmpty())
		{

		}
		// Check out the JSON file if it's under source control (the DataTable will automatically check out) and save the new content to it
		if (ISourceControlModule::Get().IsEnabled() && !SourceControlHelpers::CheckOutFile(SourcePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not check out source file for: %s"), *DataTable->GetName());
			return false;
		}
		if (!FFileHelper::SaveStringToFile(DataTableAsString, *SourcePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not save string to file: %s"), *DataTable->GetName());
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load source file for Data Table: %s"), *DataTable->GetName());
		return false;
	}

	// If no errors are returned, return true and proceed;

	return true;
}