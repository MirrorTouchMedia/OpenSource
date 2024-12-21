// Copyright (c) 2024 Mirror-Touch Media

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "DataTableSourceValidator.generated.h"

/**
 * This is a custom data validator that makes it easier to keep Data Tables in sync with their source files (JSON or CSV)
 *
 * NOTE: Make sure to add your module's API macro to the class declaration (where it says "YOURMODULENAMEHERE_API")
 *
 * To set this up for specific Data Tables, you'll need to create at least one Blueprint that derives from this class, where you can then
 * handle any overrides you need to properly address your specific Data Table(s).
 *
 * The only mandatory setup for your child validators is to add the target Data Table(s) to the DataTablesToValidate property.
 * This will *only* use the part of this validator that syncs the Data Table's content to its source file on edit.
 *
 * If you want to apply additional logic (e.g., implementing MakeDerivedRowName or adding your custom validation), that is also possible.
 *
 * To apply partially different validation to different Data Tables, you can either:
 *
 *		- Override the built-in validation functions of this class and switch logic according to the Data Table or its struct, or
 *		- Create unique child classes of this validator for each Data Table
 *
 * For more info on data validation: https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine
 *
 * NOTE: This is best used in combination with the auto-import Unreal Editor Settings, which will ensure that the source files
 * are kept up to date with the Data Table (via this validator) and the Data Table will be kept up to date with the source files
 * (via automatic import). Otherwise, you'll need to manually Reimport the source file anytime the source file is changed.
 */


// Helper enum to easily switch on the file type of Data Table source files.
UENUM(BlueprintType)
enum class EDataTableSourceType : uint8
{
	CSV,
	JSON,
	None
};

// The validator class itself. You can create child classes of this in both C++ and Blueprint.
UCLASS(Blueprintable)
class YOURMODULENAMEHERE_API UDataTableSourceValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

public:

	UDataTableSourceValidator();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Validation")
	TArray<UDataTable*> DataTablesToValidate;

	/** Optionally override this function to set the condition under which the Data Table should use this validator.
	 * By default, this will check whether the given Data Table is in the array of DataTablesToValidate on this validator.
	 */
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	/** This is where the core logic is implemented.
	 *		1) Automatically rename the rows to their derived names IFF MakeDerivedRowName is implemented.
	 *		2) Export the Data Table's data to its source file via SaveDataTableToSourceIfModified
	 * If you want to add additional logic to the validation, you can; I'd recommend adding it *before* the call to parent function.
	 * That way, any changes are included in the export to source.
	 */
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

	/** Optionally override this function to get the desired row name for auto renaming rows (see AutoNameDataTableRows).
	 * After overriding this function with your logic, ensure the function returns true so the downstream function knows to use it.
	 * This can be a helpful feature of the validator, but isn't critical.
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool MakeDerivedRowName(const UDataTable* DataTable, const FName OldName, FName& NewName);

	/** Will generate row names for entries in the Data Table according to logic in MakeDerivedRowName.
	 * Can be useful to generate intuitive row names derived from properties of the given row.
	 * We often use this to name rows based on a corresponding GameplayTag assigned to the row.
	 */
	bool AutoNameDataTableRows(UDataTable* DataTable);

protected:

	/** Get the file path as an absolute path for read/write and source control purposes etc. */
	static bool GetSourceFilePath(const UDataTable* DataTable, FString& SourcePath);

	/** Gets the source file type for the Data Table, so we know which read/write operations to use. */
	static bool GetDataTableSourceInfo(const UDataTable* DataTable, EDataTableSourceType& SourceType, FString& SourcePath);

	/** Save Data Table data to a CSV/JSON file IFF the DT data and CSV/JSON data are different. Includes VC checkout for the CSV/JSON */
	static bool SaveDataTableToSourceIfModified(const UDataTable* DataTable);

};