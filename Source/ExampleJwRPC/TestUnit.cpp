#include "TestUnit.h"
#include "ExampleJwRPC.h"
#include "Engine/Engine.h"
#include "ConstructorHelpers.h"
#include "TextBlock.h"
#include "Button.h"
#include "EditableTextBox.h"
#include "SpinBox.h"

ATestUnit_GM::ATestUnit_GM()
{
	this->TargetURL = "ws://127.0.0.1:3476";
	this->TestUnitLoopDelay = 0.01f;

	static ConstructorHelpers::FClassFinder<UTestUnitWidget> CF_Widget(TEXT("/Game/TestUnit/TestUnitWidget"));
	WidgetClass = CF_Widget.Class;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	this->DefaultPawnClass = nullptr;
}

void ATestUnit_GM::StartRequesting()
{
	UTestUnitConnection* pConn = UJwRpcConnection::CreateAndConnect<UTestUnitConnection>(TargetURL);
	pConn->GameMode = this;
	this->Conn = pConn;

	
}

void ATestUnit_GM::StopRequesting()
{
	if (Conn) 
	{
		this->GetWorld()->GetTimerManager().ClearTimer(Conn->THRequesting);
		
	}
}

void ATestUnit_GM::BeginPlay()
{
	Super::BeginPlay();

	UTestUnitWidget* pWidget = CreateWidget<UTestUnitWidget>(GetWorld(), WidgetClass);
	if (ensureAlways(pWidget))
	{
		pWidget->AddToViewport();
		Widget = pWidget;
	}

	GetWorld()->GetFirstPlayerController()->bShowMouseCursor = true;
}

void ATestUnit_GM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (Conn && Widget)
	{
		FString debugStr = FString::Printf(TEXT("NumRequestSentAtAll:%d"), Conn->NumRequestSentAtAll);
		Widget->TxtInfo->SetText(FText::FromString(debugStr));
	}
}

UTestUnitConnection::UTestUnitConnection()
{
	this->DefaultTimeout = 30;


	BindCallbacks();
}

void UTestUnitConnection::OnConnected(bool bReconnect)
{
	Super::OnConnected(bReconnect);
	
	//for (size_t i = 0; i < 30; i++)
	//{
	//	UTestUnitConnection::QueueTasks();
	//}

	
	GameMode->GetWorld()->GetTimerManager().SetTimer(THRequesting, this, &UTestUnitConnection::QueueTasks, GameMode->TestUnitLoopDelay, true);
	
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
	if (false)
	{
		this->Notify("notiHi", MakeShared<FJsonValueString>("hi"));
		//
		this->Notify("notiHi", nullptr);
		//sending wrong json
		this->Notify("notiHi", "[1,2,3, sdfsdfdfsdfsdf]");
		this->Notify("notiHi", MakeShared<FJsonValueString>("hehe\n\nhehe"));
	}


	if (true)
	{
		TSharedPtr<FJsonObject> sumParams = MakeShared<FJsonObject>();
		const float valA = FMath::RandRange(-1000, 1000);
		const float valB = FMath::RandRange(-1000, 1000);
		const float valSum = valA + valB;
		sumParams->SetNumberField("a", valA);
		sumParams->SetNumberField("b", valB);
		NumRequestSentAtAll++;
		this->Request("reqSum", MakeShared<FJsonValueObject>(sumParams), FSuccessCB::CreateLambda([=](TSharedPtr<FJsonValue> result) {
			checkf(result->AsNumber() == valSum, TEXT("number is not valid"));

			}), FErrorCB::CreateLambda([](const FJwRPCError & err) {
				checkf(false, TEXT("reqSum error %s"), *err.Message);
		}));
	}




	if (true) 
	{
		NumRequestSentAtAll++;
		this->Request("reqTimeout", "{}", FSuccessCB::CreateLambda([](TSharedPtr<FJsonValue> result) {
			checkf(false, TEXT("reqTimeout returned success result"));
			}), FErrorCB::CreateLambda([](const FJwRPCError& err) {
				checkf(err.Code == FJwRPCError::Timeout.Code, TEXT("reqTimeout error.code:%d error.message:%s"), err.Code, *err.Message);
			}));
	}

	if (true)
	{
		NumRequestSentAtAll++;
		this->Request("reqError666", "{}", FSuccessCB::CreateLambda([](TSharedPtr<FJsonValue> result) {
			checkf(false, TEXT("this should not happen"));
			}), FErrorCB::CreateLambda([](const FJwRPCError & err) {
				checkf(err.Code == 666, TEXT("reqError666 error.code:%d error.message:%s"), err.Code, *err.Message);
			}));
	}
}

void UTestUnitWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BtnStopRequesting->OnClicked.AddDynamic(this, &UTestUnitWidget::OnStopRequestingClicked);
	BtnStartRequesting->OnClicked.AddDynamic(this, &UTestUnitWidget::OnBtnStartClicked);
}

void UTestUnitWidget::OnStopRequestingClicked()
{
	GetWorld()->GetAuthGameMode<ATestUnit_GM>()->StopRequesting();
}

void UTestUnitWidget::OnBtnStartClicked()
{
	auto pGM = GetWorld()->GetAuthGameMode<ATestUnit_GM>();
	pGM->TargetURL = this->TargetURLInput->GetText().ToString();
	pGM->TestUnitLoopDelay = this->BoxUnitLoopDelay->GetValue();
	pGM->StartRequesting();
}
