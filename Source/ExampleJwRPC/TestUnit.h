#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "JwRPC.h"
#include "UserWidget.h"


#include "TestUnit.generated.h"

class UTestUnitConnection;
class UTextBlock;
class UButton;
class USpinBox;
class UEditableTextBox;

UCLASS()
class UTestUnitWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TxtInfo;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* BtnStartRequesting;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* BtnStopRequesting;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USpinBox* BoxUnitLoopDelay;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* TargetURLInput;

	void NativeConstruct() override;

	UFUNCTION()
	void OnStopRequestingClicked();
	UFUNCTION()
	void OnBtnStartClicked();
};

UCLASS(config = Game)
class ATestUnit_GM : public AGameModeBase
{
	GENERATED_BODY()
public:
	ATestUnit_GM();
	
	UPROPERTY(EditAnywhere, Config)
	TSubclassOf<UTestUnitWidget> WidgetClass;

	UPROPERTY(EditAnywhere, Config)
	float TestUnitLoopDelay;
	UPROPERTY(EditAnywhere, Config)
	FString TargetURL;

	UPROPERTY()
	UTestUnitConnection* Conn;
	UPROPERTY()
	UTestUnitWidget* Widget;

	UFUNCTION(Exec)
	void StartRequesting();
	UFUNCTION(Exec)
	void StopRequesting();

	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

};

UCLASS()
class UTestUnitConnection : public UJwRpcConnection
{
	GENERATED_BODY()
public:
	UPROPERTY()
	ATestUnit_GM* GameMode;

	int NumRequestSentAtAll = 0;
	FTimerHandle THRequesting;

	UTestUnitConnection();

	void BindCallbacks()
	{
		RegisterRequestCallback("testEcho"				, FRequestCB::CreateUObject(this, &UTestUnitConnection::OnTestEcho));
		RegisterRequestCallback("testTimeout"			, FRequestCB::CreateUObject(this, &UTestUnitConnection::OnTestTimeout));
		RegisterRequestCallback("testLongTime"			, FRequestCB::CreateUObject(this, &UTestUnitConnection::OnTestLongTime));
		RegisterRequestCallback("testError666"			, FRequestCB::CreateUObject(this, &UTestUnitConnection::OnTestError666));
		RegisterRequestCallback("testRequestCounter"    , FRequestCB::CreateUObject(this, &UTestUnitConnection::OnRequestCounter));

		RegisterNotificationCallback("notiHi", FNotifyCB::CreateUObject(this, &UTestUnitConnection::NotiHi));
	}

	void OnTestEcho(TSharedPtr<FJsonValue> params, FJwRpcIncomingRequest& request)
	{
		request.FinishSuccess(params);
	}
	void OnTestTimeout(TSharedPtr<FJsonValue> params, FJwRpcIncomingRequest& request)
	{
		
	}
	void OnTestLongTime(TSharedPtr<FJsonValue> params, FJwRpcIncomingRequest& request)
	{
		FTimerHandle th;
		GameMode->GetWorldTimerManager().SetTimer(th, [=]() {
			request.FinishSuccess("{}");

		}, 6, false);
	}
	void OnTestError666(TSharedPtr<FJsonValue> params, FJwRpcIncomingRequest& request)
	{
		request.FinishError(666);
	}

	int TestCounter = 0;
	void OnRequestCounter(TSharedPtr<FJsonValue> params, FJwRpcIncomingRequest& request)
	{
		TestCounter++;
		request.FinishSuccess(MakeShared<FJsonValueNumber>(TestCounter));
	}
	void NotiHi(TSharedPtr<FJsonValue> params)
	{
		check(params->AsString() == FString("hi"));
	}

	void OnConnected(bool bReconnect) override;
	void OnConnectionError(const FString& error, bool bReconnect) override;
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean) override;

	void QueueTasks();
};