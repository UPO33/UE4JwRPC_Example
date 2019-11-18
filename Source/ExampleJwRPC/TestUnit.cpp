#include "TestUnit.h"
#include "ExampleJwRPC.h"

ATestUnit_GM::ATestUnit_GM()
{

}

void ATestUnit_GM::Start(FString url)
{
	UTestUnitConnection* pConn = UJwRpcConnection::CreateAndConnect<UTestUnitConnection>(url);
	pConn->GameMode = this;
	this->Conn = pConn;

	
}

void ATestUnit_GM::BeginPlay()
{
	Super::BeginPlay();
	Start("ws://127.0.0.1:3476");
}

UTestUnitConnection::UTestUnitConnection()
{
	this->DefaultTimeout = 10;
	
	BindCallbacks();
}

void UTestUnitConnection::OnConnected(bool bReconnect)
{
	Super::OnConnected(bReconnect);
	
	FTimerHandle th;
	GameMode->GetWorld()->GetTimerManager().SetTimer(th, this, &UTestUnitConnection::QueueTasks, 1, false);
}

void UTestUnitConnection::OnConnectionError(const FString& error, bool bReconnect)
{
	Super::OnConnectionError(error, bReconnect);

}

void UTestUnitConnection::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	Super::OnClosed(StatusCode, Reason, bWasClean);
}

void UTestUnitConnection::QueueTasks()
{
	if (true)
	{
		this->Notify("notiHi", MakeShared<FJsonValueString>("hi"));
		//
		this->Notify("notiHi", nullptr);
		//sending wrong json
		this->Notify("notiHi", "[1,2,3, sdfsdfdfsdfsdf]");
	}


	if (true)
	{
		TSharedPtr<FJsonObject> sumParams = MakeShared<FJsonObject>();
		const float valA = FMath::RandRange(-1000, 1000);
		const float valB = FMath::RandRange(-1000, 1000);
		const float valSum = valA + valB;
		sumParams->SetNumberField("a", valA);
		sumParams->SetNumberField("b", valB);
		this->Request("reqSum", MakeShared<FJsonValueObject>(sumParams), FSuccessCB::CreateLambda([=](TSharedPtr<FJsonValue> result) {
			verify(result->AsNumber() == valSum);

			}), FErrorCB::CreateLambda([](const FJwRPCError & err) {
				verify(false);
		}));
	}




	if (true) 
	{
		this->Request("reqTimeout", "{}", FSuccessCB::CreateLambda([](TSharedPtr<FJsonValue> result) {
			verify(false);
			}), FErrorCB::CreateLambda([](const FJwRPCError& err) {
				verify(err.Code == FJwRPCError::Timeout.Code);
			}));
	}

	if (true)
	{
		this->Request("reqError666", "{}", FSuccessCB::CreateLambda([](TSharedPtr<FJsonValue> result) {
			verify(false);
			}), FErrorCB::CreateLambda([](const FJwRPCError & err) {
				verify(err.Code == 666);
			}));
	}
}
