#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameFramework/Volume.h"
#include "AkAcousticPortalState.h"
#include "AkAcousticPortal.generated.h"

class UAkPortalComponent;

UCLASS(Blueprintable)
class AKAUDIO_API AAkAcousticPortal : public AVolume {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, meta=(AllowPrivateAccess=true))
    UAkPortalComponent* Portal;
    
private:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    AkAcousticPortalState InitialState;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, meta=(AllowPrivateAccess=true))
    bool bRequiresStateMigration;
    
public:
    AAkAcousticPortal(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable)
    void SetExtent(const FVector& Extent);
    
    UFUNCTION(BlueprintCallable)
    void OpenPortal();
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    AkAcousticPortalState GetCurrentState() const;
    
    UFUNCTION(BlueprintCallable)
    void ClosePortal();
    
};

