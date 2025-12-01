#ifndef ATMINTERFACE_H
#define ATMINTERFACE_H

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include "Atm.h"
#include "Bank.h"
#include "Card.h"
#include "Account.h"
#include "Transaction.h"
#include "DepositTransaction.h"
#include "WithdrawalTransaction.h"
#include "Admin.h"
#include "Button.h"
#include "TransactionLog.h"

enum ScreenState {
    STATE_WELCOME,
    STATE_INSERT_CARD,            //! TESTING: insert-card animation state
    STATE_CARD_INPUT,
    STATE_CHECK_ACCOUNT,
    STATE_ENTER_NAME,
    STATE_SELECT_ACCOUNT_TYPE,
    STATE_ENTER_PIN,
    STATE_CONFIRM_ACCOUNT,
    STATE_LOGIN,
    STATE_CARD_LOCKED,
    STATE_MAIN_MENU,
    STATE_BALANCE_CHECK,
    STATE_WITHDRAW,
    STATE_DEPOSIT,
    STATE_TRANSACTION_COMPLETE,
    STATE_ADMIN_LOGIN,
    STATE_ADMIN_MENU,
    STATE_ADMIN_VIEW_ACCOUNTS,
    STATE_ADMIN_SELECT_ACCOUNT,
    STATE_ADMIN_VIEW_LOCKED_CARDS,
    STATE_VIEW_TRANSACTIONS,
    STATE_ADMIN_VIEW_ALL_TRANSACTIONS,
    STATE_TRANSFER,
    STATE_TRANSFER_AMOUNT,
    STATE_CHANGE_PIN_CURRENT,
    STATE_CHANGE_PIN_NEW,
    STATE_CHANGE_PIN_CONFIRM
};

class ATMInterface {
private:
    //! Visual elements for future insert-card animation
    sf::RectangleShape slot_;     //? the ATM slot
    sf::RectangleShape card_;     //? the card
    bool  cardAnimating_ = false;
    float cardYStart_    = 0.f;
    float cardYEnd_      = 0.f;
    sf::Clock animClock_;

    sf::RenderWindow window;
    Bank bank;
    ATM atmMachine;
    Admin admin;
    
    std::shared_ptr<Account> currentAccount;
    std::unique_ptr<Card> currentCard;
    
    ScreenState currentState{ STATE_WELCOME };   // start on Welcome
    std::string currentInput;
    std::string cardNumberInput;
    std::string nameInput;
    std::string transactionMessage;
    std::string accountTypeInput;
    
    sf::Font mainFont;
    sf::Text titleText;
    sf::Text displayText;
    sf::RectangleShape atmBody;
    sf::RectangleShape screen;
    sf::RectangleShape screenGlow;
    sf::RectangleShape screenGlass;
    sf::Texture atmFrameTexture;
    sf::Sprite atmFrameSprite;
    bool frameTextureLoaded{ false };
    sf::Vector2f uiOffset;
    
    std::vector<Button> screenButtons;
    int scrollOffset;
    std::vector<TransactionRecord> currentTransactions;
    int transactionPage;
    int transactionPageSize;
    std::string transferRecipientAccount;
    std::string currentPinInput;
    std::string newPinInput;
    ScreenState previousMenuState; // Track previous menu for transaction complete screen
    
    enum AdminActionMode {
        ADMIN_ACTION_NONE,
        ADMIN_ACTION_RESET_PIN,
        ADMIN_ACTION_LOCK_CARD,
        ADMIN_ACTION_UNLOCK_CARD
    };
    AdminActionMode adminActionMode;

    void setupUI();
    void handleEvents();
    void render();
    void handleMouseClick(sf::Vector2f mousePos);
    void handleTextInput(sf::Uint32 unicode);
    
    // Screen drawing functions
    void drawWelcomeScreen();
    void drawInsertCardScreen();                 // NEW: add draw for insert-card state
    void drawCardInputScreen();
    void drawCheckAccountScreen();
    void drawEnterNameScreen();
    void drawSelectAccountTypeScreen();
    void drawEnterPinScreen();
    void drawConfirmAccountScreen();
    void drawLoginScreen();
    void drawMainMenu();
    void drawBalanceScreen();
    void drawWithdrawScreen();
    void drawDepositScreen();
    void drawTransactionScreen(const std::string& message);
    void drawAdminLoginScreen();
    void drawAdminMenu();
    void drawAdminViewAccounts();
    void drawAdminSelectAccount();
    void drawViewTransactionsScreen();
    void drawAdminViewAllTransactionsScreen();
    void drawTransferScreen();
    void drawTransferAmountScreen();
    void drawCardLockedScreen();
    void drawViewLockedCardsScreen();
    void drawChangePinCurrentScreen();
    void drawChangePinNewScreen();
    void drawChangePinConfirmScreen();
    
    // Transaction processing
    void checkCardNumber();
    void createNewAccount();
    void enterPIN(const std::string& pin);
    void processWithdrawal(const std::string& amountStr);
    void processDeposit(const std::string& amountStr);
    void processTransfer(const std::string& amountStr);
    void processPinChange();
    
    // Helper functions
    void updateInsertCard(float dt);             // NEW: update for insert-card animation
    void setScreen(ScreenState newState);
    void logout();
    void startAdminAccountAction(AdminActionMode mode);
    std::string getAdminActionLabel() const;

public:
    ATMInterface();
    void run();
};

#endif 
