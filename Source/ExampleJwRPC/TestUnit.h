#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "JwRPC.h"


#include "TestUnit.generated.h"

class UTestUnitConnection;

UCLASS()
class ATestUnit_GM : public AGameModeBase
{
	GENERATED_BODY()
	
	ATestUnit_GM();
	
	UPROPERTY()
	UTestUnitConnection* Conn;

	UFUNCTION(Exec)
	void Start(FString url = "ws:\\127.0.0.1:3499");

	void BeginPlay() override;

};

UCLASS()
class UTestUnitConnection : public UJwRpcConnection
{
	GENERATED_BODY()
public:
	UPROPERTY()
	ATestUnit_GM* GameMode;


	UTestUnitConnection();

	void BindCallbacks()
	{
		RegisterRequestCallback("testEcho"				, FRequestCallbackBase::CreateUObject(this, &UTestUnitConnection::OnTestEcho));
		RegisterRequestCallback("testTimeout"			, FRequestCallbackBase::CreateUObject(this, &UTestUnitConnection::OnTestTimeout));
		RegisterRequestCallback("testLongTime"			, FRequestCallbackBase::CreateUObject(this, &UTestUnitConnection::OnTestLongTime));
		RegisterRequestCallback("testError666"			, FRequestCallbackBase::CreateUObject(this, &UTestUnitConnection::OnTestError666));
		RegisterRequestCallback("testRequestCounter"    , FRequestCallbackBase::CreateUObject(this, &UTestUnitConnection::OnRequestCounter));

		RegisterNotificationCallback("notiHi", FNotifyCallbackBase::CreateUObject(this, &UTestUnitConnection::NotiHi));
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

		}, 20, false);
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