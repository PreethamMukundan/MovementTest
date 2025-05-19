
#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ZeroPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class MOVEMENT_ZERO_API AZeroPlayerCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere)
    float CrouchBlendDuration =0.5f;
    UPROPERTY(EditAnywhere)
    float CrouchBlendTime;
                                                
public:
    AZeroPlayerCameraManager();
                                               
    virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
                                               
                                                
};
