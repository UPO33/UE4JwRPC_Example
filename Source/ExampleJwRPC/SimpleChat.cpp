#include "SimpleChat.h"
#include "JsonBP.h"
#include "TimerManager.h"

ASimpleChat_PC::ASimpleChat_PC()
{
	static ConstructorHelpers::FClassFinder<UWidgetSimpleChat> simpleChatWC(TEXT("/Game/SimpleChatCPP/WBP_SimpleChat"));
	static ConstructorHelpers::FClassFinder<UWidgetChatEntry>  chatEntryWC(TEXT("/Game/SimpleChatCPP/WBP_ChatEntry"));

	ChatWidgetClass = simpleChatWC.Class;
	ChatEntryWidgetClass = chatEntryWC.Class;

	bShowMouseCursor = true;
}

void ASimpleChat_PC::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		UWidgetSimpleChat* pWidget = CreateWidget<UWidgetSimpleChat>(this, ChatWidgetClass);
		if (ensureAlways(pWidget))
		{
			pWidget->AddToViewport();
			ChatWidget = pWidget;
		}
	}
}

ASimpleChat_GM::ASimpleChat_GM()
{
	PlayerControllerClass = ASimpleChat_PC::StaticClass();
}

USimpleChatConnection::USimpleChatConnection()
{
	BindCallbacks();
}

void USimpleChatConnection::BindCallbacks()
{
	this->RegisterNotificationCallback("userLogout"		, FNotifyCallbackBase::CreateUObject(this, &USimpleChatConnection::OnUserLogout));
	this->RegisterNotificationCallback("userLogin"		, FNotifyCallbackBase::CreateUObject(this, &USimpleChatConnection::OnUserLogin));
	this->RegisterNotificationCallback("userIsTyping"	, FNotifyCallbackBase::CreateUObject(this, &USimpleChatConnection::OnUserIsTyping));
	this->RegisterNotificationCallback("message"		, FNotifyCallbackBase::CreateUObject(this, &USimpleChatConnection::OnMessage));
}

void USimpleChatConnection::OnPostLogin()
{
	Player->ChatWidget->ChatsBox->ClearChildren();

	//fetch the stored messages
	this->Request("getStoredMessages", MakeShared<FJsonValueNull>(), UJwRpcConnection::FSuccessCB::CreateLambda([this](TSharedPtr<FJsonValue> result) {

		auto& messages = result->AsArray();
		for (TSharedPtr<FJsonValue> msg : messages)
			OnMessage(msg);

	}), nullptr);
}

void USimpleChatConnection::OnUserLogout(TSharedPtr<FJsonValue> params)
{
	GetChatWidget()->InsertMessage(FString(), FString::Printf(TEXT("---------- %s Left ----------"), *params->AsString()));
}

void USimpleChatConnection::OnUserLogin(TSharedPtr<FJsonValue> params)
{
	GetChatWidget()->InsertMessage(FString(), FString::Printf(TEXT("---------- %s Entered ----------"), *params->AsString()));
}

void USimpleChatConnection::OnUserIsTyping(TSharedPtr<FJsonValue> params)
{
	if (!GetChatWidget())
		return;

	auto username = params->AsObject()->GetStringField("username");
	auto isTyping = params->AsObject()->GetBoolField("isTyping");

	if (isTyping) 
		GetChatWidget()->TypingUsers.AddUnique(username);
	else
		GetChatWidget()->TypingUsers.Remove(username);
}

void USimpleChatConnection::OnMessage(TSharedPtr<FJsonValue> params)
{
	if (!GetChatWidget())
		return;

	auto username = params->AsObject()->GetStringField("username");
	auto message = params->AsObject()->GetStringField("message");
	GetChatWidget()->InsertMessage(username, message);
}

void USimpleChatConnection::OnConnected(bool bReconnect)
{
	Super::OnConnected(bReconnect);

	auto loginParams = MakeShared<FJsonObject>();
	loginParams->SetStringField("username", Player->ChatWidget->UsernameInput->GetText().ToString());
	loginParams->SetStringField("password", "qwerty123");

	this->Request("login", MakeShared<FJsonValueObject>(loginParams)
		, FSuccessCB::CreateUObject(this, &USimpleChatConnection::OnLoginSuccess)
		, FErrorCB::CreateUObject(this, &USimpleChatConnection::OnLoginError));

}

void USimpleChatConnection::OnConnectionError(const FString& error, bool bReconnect)
{
	Super::OnConnectionError(error, bReconnect);

	check(Player);
	Player->ChatWidget->TxtLoginStatus->SetText(FText::FromString(error));
	Player->ChatWidget->VBoxLogin->SetIsEnabled(true);
}

void USimpleChatConnection::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	Super::OnClosed(StatusCode, Reason, bWasClean);

	//go back to login menu
	Player->ChatWidget->MainSwitcher->SetActiveWidgetIndex(0);
}


void USimpleChatConnection::OnLoginSuccess(TSharedPtr<FJsonValue> result)
{
	this->Player->ChatWidget->VBoxLogin->SetIsEnabled(true);
	this->Player->ChatWidget->MainSwitcher->SetActiveWidgetIndex(1);
	this->Player->ChatWidget->TxtLoginStatus->SetText(FText());
	this->OnPostLogin();
}

void USimpleChatConnection::OnLoginError(const FJwRPCError& error)
{
	this->Player->ChatWidget->TxtLoginStatus->SetText(FText::FromString(error.Message));
	this->Player->ChatWidget->VBoxLogin->SetIsEnabled(true);
	this->Player->SavedUsername = "";
	this->Close();
}

void UWidgetSimpleChat::NativeConstruct()
{
	Super::NativeConstruct();
	
	BtnLogin->OnClicked.AddDynamic(this, &UWidgetSimpleChat::BtnLoginClicked);
	BtnExit->OnClicked.AddDynamic(this, &UWidgetSimpleChat::BtnExitClicked);
	BtnLeave->OnClicked.AddDynamic(this, &UWidgetSimpleChat::BtnLeaveClicked);

	ChatBoxInput->OnTextCommitted.AddDynamic(this, &UWidgetSimpleChat::OnChatBoxTextCommited);
	ChatBoxInput->OnTextChanged.AddDynamic(this, &UWidgetSimpleChat::OnChatBoxTextChanged);
}


void UWidgetSimpleChat::InsertMessage(const FString& username, const FString& message)
{
	ASimpleChat_PC* pPC = GetOwningPlayer<ASimpleChat_PC>();
	UWidgetChatEntry* pWidget = CreateWidget<UWidgetChatEntry>(pPC, pPC->ChatEntryWidgetClass);
	if (ensureAlways(pWidget))
	{
		//fill the widget
		pWidget->TxtUsername->SetText(FText::FromString(username));
		pWidget->TxtMessage->SetText(FText::FromString(message));
		//add it to the scrollbar
		ChatsBox->AddChild(pWidget);
		ChatsBox->ScrollToEnd();
	}
}

void UWidgetSimpleChat::ResetTypingState()
{
	bIsTyping = false;

	if (GetConnection())
	{
		//tell the server that I am not typing anymore
		GetConnection()->Notify("typing", MakeShared<FJsonValueBoolean>(false));
	}
	
}

void UWidgetSimpleChat::OnChatBoxTextChanged(const FText& txt)
{
	if (!bIsTyping)
	{
		bIsTyping = true;
		//tell the server that I am typing
		GetConnection()->Notify("typing", MakeShared<FJsonValueBoolean>(true));
	}

	GetWorld()->GetTimerManager().SetTimer(THRestTypingState, this, &UWidgetSimpleChat::ResetTypingState, 3);
}

void UWidgetSimpleChat::OnChatBoxTextCommited(const FText& text, ETextCommit::Type commitMethod)
{
	if (commitMethod == ETextCommit::OnEnter) 
	{
		GetConnection()->Notify("message", MakeShared<FJsonValueString>(text.ToString()));
		ChatBoxInput->SetText(FText());
	}
}

void UWidgetSimpleChat::BtnLoginClicked()
{
	ChatsBox->ClearChildren();
	VBoxLogin->SetIsEnabled(false);

	ASimpleChat_PC* pPC = GetOwningPlayer<ASimpleChat_PC>();
	pPC->SavedUsername = UsernameInput->GetText().ToString();

	USimpleChatConnection* pConnection = UJwRpcConnection::CreateAndConnect<USimpleChatConnection>(URLInput->GetText().ToString());
	check(pConnection);
	pConnection->Player = pPC;
	pPC->TheConnection = pConnection;

	
	
}

void UWidgetSimpleChat::BtnExitClicked()
{
	GetOwningPlayer()->ConsoleCommand("quit");
}

void UWidgetSimpleChat::BtnLeaveClicked()
{
	ASimpleChat_PC* pPC = GetOwningPlayer<ASimpleChat_PC>();
	if (pPC->TheConnection)
	{
		pPC->TheConnection->Close();
		pPC->TheConnection = nullptr;

	}
	
	MainSwitcher->SetActiveWidgetIndex(0);
}

FText UWidgetSimpleChat::GetTypingUsersText()
{
	if (TypingUsers.Num() == 0) //no one is typing?
		return FText();

	if (TypingUsers.Num() == 1) //one user is typing ?
		return FText::FromString(FString::Printf(TEXT("%s Is Typing ..."), *TypingUsers[0]));

	FString str;
	if (TypingUsers.Num() >= 2) //many are typing ?
	{
		for (int i = 0; i < TypingUsers.Num(); i++)
		{
			str += TypingUsers[i];
			
			if (i != TypingUsers.Num() - 1)
				str += TEXT(", ");
		}
	}
	str += TEXT(" Are Typing ...");
	return FText::FromString(str);
}

USimpleChatConnection* UWidgetSimpleChat::GetConnection() const
{
	return GetOwningPlayer<ASimpleChat_PC>()->TheConnection;
}

void UWidgetSimpleChat::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TxtChatInfo->SetText(GetTypingUsersText());
}
