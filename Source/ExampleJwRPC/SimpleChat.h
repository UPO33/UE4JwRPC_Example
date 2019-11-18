#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "JwRPC.h"
#include "UMG.h"

#include "SimpleChat.generated.h"

class UVerticalBox;
class UButton;
class USimpleChatConnection;

//widget to show a single message
UCLASS()
class UWidgetChatEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TxtUsername;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TxtMessage;
};

//widget for login and chat 
UCLASS()
class UWidgetSimpleChat : public UUserWidget
{
	GENERATED_BODY()
public:
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* BtnLogin;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* BtnExit;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* VBoxLogin;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UWidgetSwitcher* MainSwitcher;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* URLInput;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* UsernameInput;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TxtLoginStatus;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UScrollBox* ChatsBox;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UEditableTextBox* ChatBoxInput;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TxtChatInfo;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* BtnLeave;

	//usernames of the users who are typing at the moment
	TArray<FString> TypingUsers;
	//true means the user is typing 
	bool bIsTyping;
	//
	FTimerHandle THRestTypingState;

	
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void InsertMessage(const FString& username, const FString& message);
	void ResetTypingState();

	UFUNCTION()
	void OnChatBoxTextChanged(const FText& txt);
	UFUNCTION()
	void OnChatBoxTextCommited(const FText& text, ETextCommit::Type commitMethod);
	UFUNCTION()
	void BtnLoginClicked();
	UFUNCTION()
	void BtnExitClicked();
	UFUNCTION()
	void BtnLeaveClicked();
	UFUNCTION()
	FText GetTypingUsersText();

	USimpleChatConnection* GetConnection() const;
	

};

UCLASS()
class ASimpleChat_PC : public APlayerController
{
	GENERATED_BODY()
public:
	UPROPERTY()
	UWidgetSimpleChat* ChatWidget;
	UPROPERTY()
	TSubclassOf<UWidgetSimpleChat> ChatWidgetClass;
	UPROPERTY()
	TSubclassOf<UWidgetChatEntry> ChatEntryWidgetClass;
	UPROPERTY()
	USimpleChatConnection* TheConnection;

	UPROPERTY()
	FString SavedUsername;

	ASimpleChat_PC();

	void BeginPlay() override;
};

UCLASS()
class ASimpleChat_GM : public AGameModeBase
{
	GENERATED_BODY()
public:
	ASimpleChat_GM();
};


UCLASS()
class USimpleChatConnection : public UJwRpcConnection
{
	GENERATED_BODY()
public:
	USimpleChatConnection();

	UPROPERTY()
	ASimpleChat_PC* Player;
	

	UWidgetSimpleChat* GetChatWidget() const { return Player ? Player->ChatWidget : nullptr; };

	void BindCallbacks();
	void OnPostLogin();

	void OnUserLogout(TSharedPtr<FJsonValue> params);
	void OnUserLogin(TSharedPtr<FJsonValue> params);
	void OnUserIsTyping(TSharedPtr<FJsonValue> params);
	void OnMessage(TSharedPtr<FJsonValue> params);

	void OnLoginSuccess(TSharedPtr<FJsonValue> result);
	void OnLoginError(const FJwRPCError& error);

	void OnConnected(bool bReconnect) override;
	void OnConnectionError(const FString& error, bool bReconnect) override;
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean) override;



};

