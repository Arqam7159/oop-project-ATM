#include "AtmInterface.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cctype>
#include <chrono>
#include <ctime>

using namespace std;

// Helper function to check if a string is a valid number
static bool isValidNumber(const string& str) {
    if (str.empty()) return false;
    
    bool hasDecimal = false;
    bool hasDigit = false;
    
    for (size_t i = 0; i < str.length(); i++) {
        if (isdigit(str[i])) {
            hasDigit = true;
        } else if (str[i] == '.' && !hasDecimal) {
            hasDecimal = true;
        } else {
            return false; // Invalid character
        }
    }
    
    return hasDigit; // Must have at least one digit
}

static std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

ATMInterface::ATMInterface()
    : window(sf::VideoMode(900, 650), "ATM Simulator - Project Group 32"),
      bank(),
      atmMachine(bank),
      admin(),
      currentState(STATE_WELCOME),
      currentAccount(nullptr),
      currentCard(nullptr),
      scrollOffset(0),
      transactionPage(0),
      transactionPageSize(4),
      adminActionMode(ADMIN_ACTION_NONE),
      previousMenuState(STATE_MAIN_MENU) {
    
    window.setFramerateLimit(60);
    window.requestFocus();
    
    if (!mainFont.loadFromFile("assets/fonts/arialn.ttf")) {
        cerr << "Error loading font!" << endl;
    }

    frameTextureLoaded = atmFrameTexture.loadFromFile("assets/atm_frame.png");
    if (frameTextureLoaded) {
        atmFrameSprite.setTexture(atmFrameTexture);
        sf::Vector2u texSize = atmFrameTexture.getSize();
        sf::Vector2u winSize = window.getSize();
        atmFrameSprite.setScale(
            static_cast<float>(winSize.x) / static_cast<float>(texSize.x),
            static_cast<float>(winSize.y) / static_cast<float>(texSize.y)
        );
    }
    
    setupUI();
}

void ATMInterface::setupUI() {
    uiOffset = sf::Vector2f(35.f, 28.f);

    atmBody.setSize(sf::Vector2f(850, 600));
    atmBody.setPosition(25, 25);
    atmBody.setFillColor(sf::Color(20, 24, 30));
    atmBody.setOutlineThickness(5);
    atmBody.setOutlineColor(sf::Color(70, 90, 120));
    
    titleText.setFont(mainFont);
    titleText.setCharacterSize(30);
    titleText.setFillColor(sf::Color(230, 240, 245));
    titleText.setString("ATM Simulator");
    titleText.setPosition(140, 110);

    // --- Insert-card visuals setup ---
    slot_.setSize(sf::Vector2f(260.f, 12.f));
    slot_.setFillColor(sf::Color(70, 80, 90));
    slot_.setOutlineThickness(2.f);
    slot_.setOutlineColor(sf::Color(150, 170, 190));
    slot_.setPosition(500.f, 540.f);

    // Card starts off-screen and slides up into the slot
    card_.setSize(sf::Vector2f(120.f, 80.f));
    card_.setFillColor(sf::Color(220, 225, 230));
    card_.setOutlineThickness(2.f);
    card_.setOutlineColor(sf::Color(160, 170, 185));
    cardYStart_ = 700.f; // below the window
    cardYEnd_   = slot_.getPosition().y - 30.f;
    card_.setPosition(slot_.getPosition().x + 70.f, cardYStart_);
    cardAnimating_ = false;
    animClock_.restart();
}

void ATMInterface::run() {
    sf::Clock frameClock;
    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();
        handleEvents();
        if (currentState == STATE_INSERT_CARD) {
            updateInsertCard(dt);
        }
        render();
    }
}

void ATMInterface::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                window.requestFocus();
                sf::Vector2f mousePos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                handleMouseClick(mousePos);
            }
        }

        if (event.type == sf::Event::MouseEntered) {
            window.requestFocus();
        }
        
        if (event.type == sf::Event::TextEntered) {
            if (currentState == STATE_CARD_INPUT || currentState == STATE_ENTER_NAME ||
                currentState == STATE_ENTER_PIN || currentState == STATE_LOGIN || 
                currentState == STATE_WITHDRAW || currentState == STATE_DEPOSIT || 
                currentState == STATE_ADMIN_LOGIN || currentState == STATE_TRANSFER ||
                currentState == STATE_TRANSFER_AMOUNT || currentState == STATE_CHANGE_PIN_CURRENT ||
                currentState == STATE_CHANGE_PIN_NEW || currentState == STATE_CHANGE_PIN_CONFIRM ||
                currentState == STATE_ADMIN_ADD_INTEREST) {
                handleTextInput(event.text.unicode);
            }
        }
        
        if (event.type == sf::Event::MouseMoved) {
            sf::Vector2f mousePos = window.mapPixelToCoords(
                sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
            for (auto& button : screenButtons) {
                button.update(mousePos);
            }
        }

        if (event.type == sf::Event::MouseWheelScrolled) {
            if (currentState == STATE_ADMIN_VIEW_ACCOUNTS) {
                scrollOffset -= event.mouseWheelScroll.delta * 20;
                if (scrollOffset < 0) scrollOffset = 0;
            }
        }
    }
}

void ATMInterface::handleMouseClick(sf::Vector2f mousePos) {
    for (auto& button : screenButtons) {
        if (button.contains(mousePos)) {
            button.trigger();
            return;
        }
    }
}

void ATMInterface::handleTextInput(sf::Uint32 unicode) {
    if (unicode == 8) { // Backspace
        if (!currentInput.empty()) {
            currentInput.pop_back();
        }
    } else if (unicode == 13) { // Enter
        if (currentState == STATE_CARD_INPUT) {
            checkCardNumber();
        } else if (currentState == STATE_ENTER_NAME) {
            if (!currentInput.empty()) {
                nameInput = currentInput;
                currentInput.clear();
                setScreen(STATE_SELECT_ACCOUNT_TYPE);
            }
        } else if (currentState == STATE_ENTER_PIN) {
            createNewAccount();
        } else if (currentState == STATE_LOGIN) {
            enterPIN(currentInput);
        } else if (currentState == STATE_WITHDRAW) {
            processWithdrawal(currentInput);
        } else if (currentState == STATE_DEPOSIT) {
            processDeposit(currentInput);
        } else if (currentState == STATE_TRANSFER) {
            if (currentInput.length() == 7 && bank.accountExists(currentInput)) {
                if (currentInput != currentAccount->getAccountNumber()) {
                    transferRecipientAccount = currentInput;
                    currentInput.clear();
                    setScreen(STATE_TRANSFER_AMOUNT);
                } else {
                    transactionMessage = "Cannot transfer to same account!";
                    currentInput.clear();
                    setScreen(STATE_TRANSACTION_COMPLETE);
                }
            } else {
                transactionMessage = "Invalid account number!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
            }
        } else if (currentState == STATE_TRANSFER_AMOUNT) {
            processTransfer(currentInput);
        } else if (currentState == STATE_CHANGE_PIN_CURRENT) {
            if (currentInput.length() >= 4) {
                currentPinInput = currentInput;
                currentInput.clear();
                setScreen(STATE_CHANGE_PIN_NEW);
            } else {
                transactionMessage = "PIN must be at least 4 digits!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
            }
        } else if (currentState == STATE_CHANGE_PIN_NEW) {
            if (currentInput.length() >= 4) {
                newPinInput = currentInput;
                currentInput.clear();
                setScreen(STATE_CHANGE_PIN_CONFIRM);
            } else {
                transactionMessage = "PIN must be at least 4 digits!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
            }
        } else if (currentState == STATE_CHANGE_PIN_CONFIRM) {
            processPinChange();
        }
    } else if (unicode == '.' && (currentState == STATE_WITHDRAW || currentState == STATE_DEPOSIT ||
                                  currentState == STATE_TRANSFER || currentState == STATE_TRANSFER_AMOUNT ||
                                  currentState == STATE_ADMIN_ADD_INTEREST)) {
        // Allow one decimal point in numeric entry states
        if (currentInput.find('.') == string::npos && currentInput.length() < 30) {
            currentInput += static_cast<char>(unicode);
        }
    } else if (unicode >= 48 && unicode <= 57) { // Numbers 0-9
        bool isPinState = (currentState == STATE_ENTER_PIN || currentState == STATE_LOGIN || 
                           currentState == STATE_CHANGE_PIN_CURRENT || currentState == STATE_CHANGE_PIN_NEW ||
                           currentState == STATE_CHANGE_PIN_CONFIRM);
        bool isAmountState = (currentState == STATE_WITHDRAW || currentState == STATE_DEPOSIT ||
                              currentState == STATE_TRANSFER || currentState == STATE_TRANSFER_AMOUNT ||
                              currentState == STATE_ADMIN_ADD_INTEREST);

        if (currentState == STATE_CARD_INPUT && currentInput.length() < 7) {
            currentInput += static_cast<char>(unicode);
        } else if (isPinState && currentInput.length() < 4) {
            currentInput += static_cast<char>(unicode);
        } else if (isAmountState && currentInput.length() < 30) {
            currentInput += static_cast<char>(unicode);
        }
    } else if ((unicode >= 65 && unicode <= 90) || (unicode >= 97 && unicode <= 122) || unicode == 32) { // Letters and space
        if (currentState == STATE_ENTER_NAME && currentInput.length() < 50) {
            currentInput += static_cast<char>(unicode);
        } else if (currentState == STATE_ADMIN_LOGIN && currentInput.length() < 30) {
            currentInput += static_cast<char>(unicode);
        }
    }
}

void ATMInterface::render() {
    window.clear(sf::Color::Black);

    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);

    if (frameTextureLoaded) {
        window.draw(atmFrameSprite);
    } else {
        window.draw(atmBody);
    }

    sf::View uiView = defaultView;
    uiView.reset(sf::FloatRect(-uiOffset.x, -uiOffset.y, defaultView.getSize().x, defaultView.getSize().y));
    window.setView(uiView);

    if (currentState == STATE_WELCOME) {
        window.draw(titleText);
    }
    
    switch (currentState) {
        case STATE_WELCOME:
            drawWelcomeScreen();
            break;
        case STATE_INSERT_CARD:
            drawInsertCardScreen();
            break;
        case STATE_CARD_INPUT:
            drawCardInputScreen();
            break;
        case STATE_CHECK_ACCOUNT:
            drawCheckAccountScreen();
            break;
        case STATE_ENTER_NAME:
            drawEnterNameScreen();
            break;
        case STATE_SELECT_ACCOUNT_TYPE:
            drawSelectAccountTypeScreen();
            break;
        case STATE_ENTER_PIN:
            drawEnterPinScreen();
            break;
        case STATE_CONFIRM_ACCOUNT:
            drawConfirmAccountScreen();
            break;
        case STATE_LOGIN:
            drawLoginScreen();
            break;
        case STATE_MAIN_MENU:
            drawMainMenu();
            break;
        case STATE_BALANCE_CHECK:
            drawBalanceScreen();
            break;
        case STATE_WITHDRAW:
            drawWithdrawScreen();
            break;
        case STATE_DEPOSIT:
            drawDepositScreen();
            break;
        case STATE_TRANSACTION_COMPLETE:
            drawTransactionScreen(transactionMessage);
            break;
        case STATE_ADMIN_LOGIN:
            drawAdminLoginScreen();
            break;
        case STATE_ADMIN_MENU:
            drawAdminMenu();
            break;
        case STATE_ADMIN_VIEW_ACCOUNTS:
            drawAdminViewAccounts();
            break;
        case STATE_ADMIN_SELECT_ACCOUNT:
            drawAdminSelectAccount();
            break;
        case STATE_VIEW_TRANSACTIONS:
            drawViewTransactionsScreen();
            break;
        case STATE_ADMIN_VIEW_ALL_TRANSACTIONS:
            drawAdminViewAllTransactionsScreen();
            break;
        case STATE_TRANSFER:
            drawTransferScreen();
            break;
        case STATE_TRANSFER_AMOUNT:
            drawTransferAmountScreen();
            break;
        case STATE_CARD_LOCKED:
            drawCardLockedScreen();
            break;
        case STATE_ADMIN_VIEW_LOCKED_CARDS:
            drawViewLockedCardsScreen();
            break;
        case STATE_ADMIN_PENDING_DEPOSITS:
            drawAdminPendingDeposits();
            break;
        case STATE_ADMIN_ADD_INTEREST:
            drawAdminAddInterestScreen();
            break;
        case STATE_CHANGE_PIN_CURRENT:
            drawChangePinCurrentScreen();
            break;
        case STATE_CHANGE_PIN_NEW:
            drawChangePinNewScreen();
            break;
        case STATE_CHANGE_PIN_CONFIRM:
            drawChangePinConfirmScreen();
            break;
    }
    
    for (auto& button : screenButtons) {
        button.draw(window);
    }
    
    window.display();
}

void ATMInterface::checkCardNumber() {
    if (currentInput.length() != 7) {
        transactionMessage = "Invalid card number! Must be exactly 7 digits.";
        currentInput.clear();
        return;
    }
    
    cardNumberInput = currentInput;
    currentInput.clear();
    
    if (bank.accountExists(cardNumberInput)) {
        // Check if card is locked
        if (bank.isAccountLocked(cardNumberInput)) {
            setScreen(STATE_CARD_LOCKED);
        } else {
            currentCard = make_unique<Card>(cardNumberInput, cardNumberInput);
            setScreen(STATE_LOGIN);
        }
    } else {
        setScreen(STATE_CHECK_ACCOUNT);
    }
}

void ATMInterface::createNewAccount() {
    if (currentInput.length() != 4) {
        transactionMessage = "PIN must be exactly 4 digits!";
        currentInput.clear();
        return;
    }
    
    if (bank.createAccount(cardNumberInput, currentInput, accountTypeInput, nameInput, 0.0)) {
        currentCard = make_unique<Card>(cardNumberInput, cardNumberInput);
        transactionMessage = "Account created successfully! Please login.";
        currentInput.clear();
        setScreen(STATE_LOGIN);
    } else {
        transactionMessage = "Error creating account!";
        currentInput.clear();
        setScreen(STATE_WELCOME);
    }
}

void ATMInterface::enterPIN(const string& pin) {
    if (currentCard) {
        if (pin.length() != 4) {
            transactionMessage = "PIN must be exactly 4 digits.";
            currentInput.clear();
            return;
        }
        if (bank.isAccountLocked(currentCard->getAccountNumber())) {
            transactionMessage = "This card is locked.\nPlease contact an administrator to unlock it.";
            currentInput.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
            return;
        }

        if (bank.verifyPIN(currentCard->getAccountNumber(), pin)) {
            // Get account and reset failed attempts on successful login
            currentAccount = bank.getAccount(currentCard->getAccountNumber());
            if (currentAccount) {
                currentAccount->resetFailedAttempts();
                bank.updateAccountData();
            }
            currentInput.clear();
            setScreen(STATE_MAIN_MENU);
        } else {
            // Wrong PIN - increment failed attempts
            auto account = bank.getAccount(currentCard->getAccountNumber());
            if (account) {
                account->incrementFailedAttempts();
                int attempts = account->getFailedLoginAttempts();
                
                // Lock card after 3 failed attempts
                if (attempts >= 3) {
                    account->lockCard();
                    bank.updateAccountData();
                    transactionMessage = "Invalid PIN!\nCard locked due to 3 failed PIN attempts.\nPlease contact an administrator.";
                    currentInput.clear();
                    setScreen(STATE_TRANSACTION_COMPLETE);
                } else {
                    // Show remaining attempts
                    int attemptsLeft = 3 - attempts;
                    transactionMessage = "Invalid PIN!\n" + to_string(attemptsLeft) + " attempt(s) remaining before card is locked.";
                    currentInput.clear();
                }
            }
        }
    }
}

void ATMInterface::processWithdrawal(const string& amountStr) {
    if (isValidNumber(amountStr)) {
        double amount = stod(amountStr);
        
        if (currentAccount && atmMachine.canDispense(amount)) {
            if (currentAccount->withdraw(amount)) {
                atmMachine.dispenseCash(amount);
                bank.updateAccountData();
                
                auto trans = make_shared<WithdrawalTransaction>(
                    atmMachine.generateTransactionID(),
                    currentAccount->getAccountNumber(),
                    amount
                );
                currentAccount->addTransaction(trans);
                TransactionLog::logTransaction(trans, "WITHDRAWAL");
                
                stringstream ss;
                ss << "Withdrawal Successful!\n\n";
                ss << trans->printReceipt();
                ss << "\nRemaining Balance: $" << fixed << setprecision(2) 
                   << currentAccount->getBalance();
                
                transactionMessage = ss.str();
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
            } else {
                transactionMessage = "Insufficient funds!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
            }
        } else {
            transactionMessage = "ATM cannot dispense this amount!";
            currentInput.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
        }
    } else {
        transactionMessage = "Invalid amount entered!";
        currentInput.clear();
    }
}

void ATMInterface::processDeposit(const string& amountStr) {
    if (isValidNumber(amountStr)) {
        double amount = stod(amountStr);
        
        if (currentAccount && amount > 0) {
            if (amount > 7500.0) {
                std::string reqId = bank.addPendingDeposit(currentAccount->getAccountNumber(), amount, currentTimestamp());
                atmMachine.acceptCash(amount);

                stringstream ss;
                ss << "Deposit of $" << fixed << setprecision(2) << amount
                   << " submitted.\nAwaiting admin approval.\nRequest ID: " << reqId;
                transactionMessage = ss.str();
                currentInput.clear();
                previousMenuState = STATE_MAIN_MENU;
                setScreen(STATE_TRANSACTION_COMPLETE);
                return;
            }

            currentAccount->deposit(amount);
            atmMachine.acceptCash(amount);
            bank.updateAccountData();
            
            auto trans = make_shared<DepositTransaction>(
                atmMachine.generateTransactionID(),
                currentAccount->getAccountNumber(),
                amount
            );
            currentAccount->addTransaction(trans);
            TransactionLog::logTransaction(trans, "DEPOSIT");
            
            stringstream ss;
            ss << "Deposit Successful!\n\n";
            ss << trans->printReceipt();
            ss << "\nNew Balance: $" << fixed << setprecision(2) 
               << currentAccount->getBalance();
            
            transactionMessage = ss.str();
            currentInput.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
        } else {
            transactionMessage = "Invalid deposit amount!";
            currentInput.clear();
        }
    } else {
        transactionMessage = "Invalid amount entered!";
        currentInput.clear();
    }
}

void ATMInterface::processTransfer(const string& amountStr) {
    if (isValidNumber(amountStr)) {
        double amount = stod(amountStr);
        
        if (currentAccount && amount > 0) {
            // Check if recipient account exists
            if (!bank.accountExists(transferRecipientAccount)) {
                transactionMessage = "Recipient account not found!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
                return;
            }
            
            // Check if recipient account is locked
            if (bank.isAccountLocked(transferRecipientAccount)) {
                transactionMessage = "Cannot transfer to a locked account!\nRecipient account is temporarily locked.";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
                return;
            }
            
            // Check if sender has sufficient funds
            if (currentAccount->getBalance() < amount) {
                transactionMessage = "Insufficient funds for transfer!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
                return;
            }
            
            // Perform transfer
            currentAccount->withdraw(amount);
            auto recipientAcc = bank.getAccount(transferRecipientAccount);
            if (recipientAcc) {
                recipientAcc->deposit(amount);
            }
            
            bank.updateAccountData();
            
            // Log transaction for sender (withdrawal)
            auto trans = make_shared<WithdrawalTransaction>(
                atmMachine.generateTransactionID(),
                currentAccount->getAccountNumber(),
                amount
            );
            currentAccount->addTransaction(trans);
            TransactionLog::logTransaction(trans, "TRANSFER_OUT");
            
            // Log transaction for recipient (deposit)
            auto recipientTrans = make_shared<DepositTransaction>(
                atmMachine.generateTransactionID(),
                transferRecipientAccount,
                amount
            );
            if (recipientAcc) {
                recipientAcc->addTransaction(recipientTrans);
            }
            TransactionLog::logTransaction(recipientTrans, "TRANSFER_IN");
            
            stringstream ss;
            ss << "Transfer Successful!\n\n";
            ss << "From: " << currentAccount->getAccountNumber() << "\n";
            ss << "To: " << transferRecipientAccount << "\n";
            ss << "Amount: $" << fixed << setprecision(2) << amount << "\n\n";
            ss << "Your New Balance: $" << fixed << setprecision(2) 
               << currentAccount->getBalance();
            
            transactionMessage = ss.str();
            currentInput.clear();
            transferRecipientAccount.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
        } else {
            transactionMessage = "Invalid transfer amount!";
            currentInput.clear();
        }
    } else {
        transactionMessage = "Invalid amount entered!";
        currentInput.clear();
    }
}

void ATMInterface::drawWelcomeScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(26);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Welcome to HU Bank ATM\n\nPlease select an option:");
    displayText.setPosition(200, 180);
    window.draw(displayText);
    
    screenButtons.emplace_back("Insert Card", mainFont, sf::Vector2f(200, 50), sf::Vector2f(140, 360));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        cardNumberInput.clear();
        setScreen(STATE_INSERT_CARD);
    });
    
    screenButtons.emplace_back("Admin Login", mainFont, sf::Vector2f(200, 50), sf::Vector2f(380, 360));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_ADMIN_LOGIN);
    });
}

void ATMInterface::drawCardInputScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Please enter your 7-digit card number:");
    displayText.setPosition(150, 180);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(28);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString(currentInput);
    inputText.setPosition(350, 250);
    window.draw(inputText);
    
    screenButtons.emplace_back("Continue", mainFont, sf::Vector2f(200, 50), sf::Vector2f(200, 370));
    screenButtons.back().setAction([this]() { checkCardNumber(); });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(200, 50), sf::Vector2f(440, 370));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawCheckAccountScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("This card is not registered.\nWould you like to create a new account?");
    displayText.setPosition(150, 220);
    window.draw(displayText);
    
    screenButtons.emplace_back("Yes, Create Account", mainFont, sf::Vector2f(200, 50), sf::Vector2f(200, 340));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        nameInput.clear();
        accountTypeInput = "";
        setScreen(STATE_ENTER_NAME);
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(200, 50), sf::Vector2f(440, 340));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        cardNumberInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawEnterNameScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Please enter your full name:");
    displayText.setPosition(150, 180);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(24);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString(currentInput);
    inputText.setPosition(150, 250);
    window.draw(inputText);
    
    screenButtons.emplace_back("Continue", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 370));
    screenButtons.back().setAction([this]() {
        if (!currentInput.empty()) {
            nameInput = currentInput;
            currentInput.clear();
            setScreen(STATE_SELECT_ACCOUNT_TYPE);
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 370));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawSelectAccountTypeScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Select Account Type:");
    displayText.setPosition(240, 180);
    window.draw(displayText);
    
    // Show selected type
    if (!accountTypeInput.empty()) {
        sf::Text selectedText;
        selectedText.setFont(mainFont);
        selectedText.setCharacterSize(22);
        selectedText.setFillColor(sf::Color::Green);
        selectedText.setString("Selected: " + accountTypeInput);
        selectedText.setPosition(300, 250);
        window.draw(selectedText);
    }
    
    screenButtons.emplace_back("Savings Account", mainFont, sf::Vector2f(200, 50), sf::Vector2f(200, 310));
    screenButtons.back().setAction([this]() { 
        accountTypeInput = "Savings";
    });
    
    screenButtons.emplace_back("Checking Account", mainFont, sf::Vector2f(200, 50), sf::Vector2f(430, 310));
    screenButtons.back().setAction([this]() { 
        accountTypeInput = "Checking";
    });
    
    screenButtons.emplace_back("Proceed", mainFont, sf::Vector2f(180, 50), sf::Vector2f(240, 420));
    screenButtons.back().setAction([this]() { 
        if (!accountTypeInput.empty()) {
            setScreen(STATE_CONFIRM_ACCOUNT);
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 420));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        accountTypeInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawConfirmAccountScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(20);
    displayText.setFillColor(sf::Color::White);
    stringstream ss;
    ss << "Confirm Account Creation\n\n";
    ss << "Name: " << nameInput << "\n";
    ss << "Card Number: " << cardNumberInput << "\n";
    ss << "Account Type: " << accountTypeInput << "\n\n";
    ss << "Are you sure you want to create this account?";
    displayText.setString(ss.str());
    displayText.setPosition(160, 160);
    window.draw(displayText);
    
    screenButtons.emplace_back("Yes, Create", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 420));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_ENTER_PIN);
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 420));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        accountTypeInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawEnterPinScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Create your PIN (min 4 digits):");
    displayText.setPosition(150, 180);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    string maskedInput(currentInput.length(), '*');
    inputText.setString(maskedInput);
    inputText.setPosition(350, 250);
    window.draw(inputText);
    
    screenButtons.emplace_back("Create Account", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 370));
    screenButtons.back().setAction([this]() { createNewAccount(); });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 370));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawLoginScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Please enter your PIN:");
    displayText.setPosition(150, 180);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    string maskedInput(currentInput.length(), '*');
    inputText.setString(maskedInput);
    inputText.setPosition(350, 250);
    window.draw(inputText);
    
    screenButtons.emplace_back("Submit", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 370));
    screenButtons.back().setAction([this]() { enterPIN(currentInput); });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 370));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        currentCard.reset();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawMainMenu() {
    screenButtons.clear();
    
    if (!currentAccount) {
        setScreen(STATE_WELCOME);
        return;
    }
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(20);
    displayText.setFillColor(sf::Color::White);
    stringstream ss;
    ss << "Account: " << currentAccount->getAccountNumber() << " | ";
    ss << currentAccount->displayAccountType();
    displayText.setString(ss.str());
    displayText.setPosition(140, 120);
    window.draw(displayText);
    
    // 3 buttons per row, 2 rows, plus eject at bottom (tighter spacing to stay in frame)
    float col1X = 180, col2X = 360, col3X = 540;
    float row1Y = 230, row2Y = 300, row3Y = 370;
    float btnWidth = 140, btnHeight = 50;
    
    screenButtons.emplace_back("Check Balance", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col1X, row1Y));
    screenButtons.back().setAction([this]() { 
        previousMenuState = STATE_MAIN_MENU;
        setScreen(STATE_BALANCE_CHECK); 
    });
    
    screenButtons.emplace_back("Withdraw", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col2X, row1Y));
    screenButtons.back().setAction([this]() { 
        currentInput.clear(); 
        setScreen(STATE_WITHDRAW); 
    });
    
    screenButtons.emplace_back("Deposit", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col3X, row1Y));
    screenButtons.back().setAction([this]() { 
        currentInput.clear(); 
        setScreen(STATE_DEPOSIT); 
    });

    screenButtons.emplace_back("Transactions", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col1X, row2Y));
    screenButtons.back().setAction([this]() { 
        transactionPage = 0;
        currentTransactions = TransactionLog::readTransactions(currentAccount->getAccountNumber());
        setScreen(STATE_VIEW_TRANSACTIONS); 
    });
    
    screenButtons.emplace_back("Transfer", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col2X, row2Y));
    screenButtons.back().setAction([this]() { 
        currentInput.clear();
        transferRecipientAccount.clear();
        setScreen(STATE_TRANSFER); 
    });
    
    screenButtons.emplace_back("Change PIN", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col3X, row2Y));
    screenButtons.back().setAction([this]() { 
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_CHANGE_PIN_CURRENT); 
    });
    
    screenButtons.emplace_back("Eject Card", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col2X, row3Y));
    screenButtons.back().setAction([this]() { logout(); });
}

void ATMInterface::drawBalanceScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Current Balance:");
    displayText.setPosition(250, 180);
    window.draw(displayText);
    
    displayText.setCharacterSize(40);
    displayText.setFillColor(sf::Color::Green);
    stringstream ss;
    ss << "$" << fixed << setprecision(2) << currentAccount->getBalance();
    displayText.setString(ss.str());
    displayText.setPosition(280, 280);
    window.draw(displayText);
    
    screenButtons.emplace_back("Return to Customer Menu", mainFont, sf::Vector2f(220, 50), sf::Vector2f(315, 420));
    screenButtons.back().setAction([this]() { setScreen(STATE_MAIN_MENU); });
}

void ATMInterface::drawWithdrawScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Enter withdrawal amount:");
    displayText.setPosition(150, 150);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(36);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString("$" + currentInput);
    inputText.setPosition(250, 270);
    window.draw(inputText);
    
    screenButtons.emplace_back("Submit", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() { processWithdrawal(currentInput); });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() { setScreen(STATE_MAIN_MENU); });
}

void ATMInterface::drawDepositScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Enter deposit amount:");
    displayText.setPosition(150, 150);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(36);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString("$" + currentInput);
    inputText.setPosition(250, 270);
    window.draw(inputText);
    
    screenButtons.emplace_back("Submit", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() { processDeposit(currentInput); });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() { setScreen(STATE_MAIN_MENU); });
}

void ATMInterface::drawTransactionScreen(const string& message) {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(14);
    displayText.setFillColor(sf::Color::White);
    displayText.setString(message);
    displayText.setPosition(130, 120);
    window.draw(displayText);
    
    // Determine button text and destination based on previous menu
    string buttonText;
    ScreenState returnState;
    
    if (previousMenuState == STATE_ADMIN_MENU) {
        buttonText = "Return to Admin Panel";
        returnState = STATE_ADMIN_MENU;
    } else {
        buttonText = "Return to Customer Menu";
        returnState = STATE_MAIN_MENU;
    }
    
    screenButtons.emplace_back(buttonText, mainFont, sf::Vector2f(220, 50), sf::Vector2f(315, 450));
    screenButtons.back().setAction([this, returnState]() { setScreen(returnState); });
}

void ATMInterface::drawAdminLoginScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Admin Login\n\nUsername: admin\nPassword: admin\n\nEnter password:");
    displayText.setPosition(150, 150);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(28);
    inputText.setFillColor(sf::Color::Yellow);
    string maskedInput(currentInput.length(), '*');
    inputText.setString(maskedInput);
    inputText.setPosition(350, 350);
    window.draw(inputText);
    
    screenButtons.emplace_back("Login", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 400));
    screenButtons.back().setAction([this]() {
        if (admin.login("admin", currentInput)) {
            currentInput.clear();
            setScreen(STATE_ADMIN_MENU);
        } else {
            transactionMessage = "Invalid admin credentials!";
            currentInput.clear();
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 400));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawAdminMenu() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(28);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("Admin Panel");
    displayText.setPosition(300, 120);
    window.draw(displayText);
    
    float col1X = 150, col2X = 330, col3X = 510;
    float row1Y = 220, row2Y = 300, row3Y = 380;
    float btnWidth = 160, btnHeight = 48;
    
    // Row 1
    screenButtons.emplace_back("View Accounts", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col1X, row1Y));
    screenButtons.back().setAction([this]() {
        scrollOffset = 0;
        setScreen(STATE_ADMIN_VIEW_ACCOUNTS);
    });

    screenButtons.emplace_back("View Transactions", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col2X, row1Y));
    screenButtons.back().setAction([this]() {
        transactionPage = 0;
        currentTransactions = TransactionLog::readTransactions();
        setScreen(STATE_ADMIN_VIEW_ALL_TRANSACTIONS);
    });

    screenButtons.emplace_back("Add Interest", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col3X, row1Y));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_ADMIN_ADD_INTEREST);
    });
    
    // Row 2
    screenButtons.emplace_back("Reset PIN", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col1X, row2Y));
    screenButtons.back().setAction([this]() {
        startAdminAccountAction(ADMIN_ACTION_RESET_PIN);
    });

    screenButtons.emplace_back("Lock Card", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col2X, row2Y));
    screenButtons.back().setAction([this]() {
        startAdminAccountAction(ADMIN_ACTION_LOCK_CARD);
    });
    
    // Row 3
    screenButtons.emplace_back("View Locked Cards", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col1X, row3Y));
    screenButtons.back().setAction([this]() {
        scrollOffset = 0;
        setScreen(STATE_ADMIN_VIEW_LOCKED_CARDS);
    });

    screenButtons.emplace_back("Approve Deposits", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col2X, row3Y));
    screenButtons.back().setAction([this]() {
        scrollOffset = 0;
        setScreen(STATE_ADMIN_PENDING_DEPOSITS);
    });
    
    screenButtons.emplace_back("Logout", mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(col3X, row3Y));
    screenButtons.back().setAction([this]() {
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawAdminViewAccounts() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(20);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("All Bank Accounts (Scroll with mouse wheel)");
    displayText.setPosition(180, 130);
    window.draw(displayText);
    
    auto& accounts = bank.getAllAccounts();
    int yPos = 170 - scrollOffset;
    
    for (auto const& [key, accPtr] : accounts) {
        if (yPos > 120 && yPos < 560) {
            sf::Text accText;
            accText.setFont(mainFont);
            accText.setCharacterSize(13);
            accText.setFillColor(sf::Color::White);
            
            stringstream ss;
            ss << "Name: " << bank.getAccountName(accPtr->getAccountNumber()) << " | ";
            ss << "Card: " << accPtr->getAccountNumber() << " | ";
            ss << "Type: " << accPtr->displayAccountType() << " | ";
            ss << "Balance: $" << fixed << setprecision(2) << accPtr->getBalance() << " | ";
            ss << (bank.isAccountLocked(accPtr->getAccountNumber()) ? "Status: LOCKED" : "Status: Active");
            
            accText.setString(ss.str());
            accText.setPosition(140, yPos);
            window.draw(accText);
        }
        yPos += 40;
    }
    
    screenButtons.emplace_back("Back", mainFont, sf::Vector2f(150, 45), sf::Vector2f(325, 540));
    screenButtons.back().setAction([this]() { setScreen(STATE_ADMIN_MENU); });
}

void ATMInterface::drawAdminSelectAccount() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString(getAdminActionLabel());
    displayText.setPosition(200, 140);
    window.draw(displayText);
    
    auto& accounts = bank.getAllAccounts();
    float col1X = 170, col2X = 340, col3X = 510;
    float row1Y = 210, row2Y = 280;
    float btnWidth = 150, btnHeight = 45;
    int count = 0;
    
    for (auto const& [key, accPtr] : accounts) {
        if (count < 6) {
            string label = accPtr->getAccountNumber().substr(0, 7);
            if (bank.isAccountLocked(accPtr->getAccountNumber())) {
                label += " (Locked)";
            }
            float xPos, yPos;
            if (count < 3) {
                xPos = (count == 0) ? col1X : (count == 1) ? col2X : col3X;
                yPos = row1Y;
            } else {
                xPos = ((count - 3) == 0) ? col1X : ((count - 3) == 1) ? col2X : col3X;
                yPos = row2Y;
            }
            
            screenButtons.emplace_back(label, mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(xPos, yPos));
            
            string accNum = accPtr->getAccountNumber();
            screenButtons.back().setAction([this, accNum]() {
                switch (adminActionMode) {
                    case ADMIN_ACTION_RESET_PIN:
                        if (admin.resetPIN(bank, accNum, "0000")) {
                            bank.updateAccountData();
                            transactionMessage = "PIN reset to 0000 for account:\n" + accNum;
                        } else {
                            transactionMessage = "Unable to reset PIN for account:\n" + accNum;
                        }
                        break;
                    case ADMIN_ACTION_LOCK_CARD:
                        if (bank.isAccountLocked(accNum)) {
                            transactionMessage = "Account already locked:\n" + accNum;
                        } else if (bank.setAccountLock(accNum, true)) {
                            transactionMessage = "Account locked successfully:\n" + accNum;
                        } else {
                            transactionMessage = "Unable to lock account:\n" + accNum;
                        }
                        break;
                    default:
                        transactionMessage = "No admin action selected.";
                        break;
                }
                adminActionMode = ADMIN_ACTION_NONE;
                previousMenuState = STATE_ADMIN_MENU;
                setScreen(STATE_TRANSACTION_COMPLETE);
            });
            count++;
        }
    }
    
    screenButtons.emplace_back("Back", mainFont, sf::Vector2f(150, 45), sf::Vector2f(325, 360));
    screenButtons.back().setAction([this]() { 
        adminActionMode = ADMIN_ACTION_NONE;
        setScreen(STATE_ADMIN_MENU); 
    });
}

void ATMInterface::drawAdminAddInterestScreen() {
    screenButtons.clear();

    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("Add Interest to Savings Accounts");
    displayText.setPosition(140, 140);
    window.draw(displayText);

    displayText.setCharacterSize(18);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Enter interest percentage then pick a Savings account:");
    displayText.setPosition(120, 200);
    window.draw(displayText);

    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString(currentInput + "%");
    inputText.setPosition(300, 240);
    window.draw(inputText);

    std::vector<std::shared_ptr<Account>> savings;
    for (auto const& [key, accPtr] : bank.getAllAccounts()) {
        if (accPtr && accPtr->displayAccountType() == "Savings Account") {
            savings.push_back(accPtr);
        }
    }

    if (savings.empty()) {
        displayText.setCharacterSize(20);
        displayText.setFillColor(sf::Color::Yellow);
        displayText.setString("No savings accounts found.");
        displayText.setPosition(250, 320);
        window.draw(displayText);
    } else {
        float col1X = 150, col2X = 340, col3X = 530;
        float row1Y = 320, row2Y = 390;
        float btnWidth = 160, btnHeight = 45;
        for (size_t i = 0; i < savings.size() && i < 6; ++i) {
            float xPos = (i % 3 == 0) ? col1X : (i % 3 == 1) ? col2X : col3X;
            float yPos = (i < 3) ? row1Y : row2Y;
            std::string label = savings[i]->getAccountNumber().substr(0, 7);
            screenButtons.emplace_back(label, mainFont, sf::Vector2f(btnWidth, btnHeight), sf::Vector2f(xPos, yPos));
            std::string accNum = savings[i]->getAccountNumber();
            screenButtons.back().setAction([this, accNum]() {
                if (currentInput.empty() || !isValidNumber(currentInput)) {
                    transactionMessage = "Enter a valid percentage.";
                    previousMenuState = STATE_ADMIN_MENU;
                    setScreen(STATE_TRANSACTION_COMPLETE);
                    return;
                }
                double percent = stod(currentInput);
                if (percent <= 0) {
                    transactionMessage = "Percentage must be greater than 0.";
                    previousMenuState = STATE_ADMIN_MENU;
                    setScreen(STATE_TRANSACTION_COMPLETE);
                    return;
                }
                double rate = percent / 100.0;
                auto acc = bank.getAccount(accNum);
                if (acc && acc->displayAccountType() == "Savings Account") {
                    acc->applyInterest(rate);
                    bank.updateAccountData();
                    stringstream ss;
                    ss << "Added " << fixed << setprecision(2) << percent << "% interest\n"
                       << "to savings account " << accNum;
                    transactionMessage = ss.str();
                } else {
                    transactionMessage = "Account not found or not Savings.";
                }
                currentInput.clear();
                previousMenuState = STATE_ADMIN_MENU;
                setScreen(STATE_TRANSACTION_COMPLETE);
            });
        }
    }

    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(160, 45), sf::Vector2f(320, 470));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        setScreen(STATE_ADMIN_MENU);
    });
}

void ATMInterface::drawAdminPendingDeposits() {
    screenButtons.clear();

    displayText.setFont(mainFont);
    displayText.setCharacterSize(26);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("Pending Deposit Requests");
    displayText.setPosition(200, 120);
    window.draw(displayText);

    const auto& pending = bank.getPendingDeposits();
    if (pending.empty()) {
        displayText.setCharacterSize(20);
        displayText.setFillColor(sf::Color::White);
        displayText.setString("No pending deposits found.");
        displayText.setPosition(260, 250);
        window.draw(displayText);
    } else {
        int yPos = 170;
        int shown = 0;
        const int maxShow = 5;
        for (const auto& pd : pending) {
            if (shown >= maxShow) break;
            sf::Text row;
            row.setFont(mainFont);
            row.setCharacterSize(16);
            row.setFillColor(sf::Color::White);

            stringstream ss;
            ss << pd.id << " | Acc: " << pd.accountNumber
               << " | Amount: $" << fixed << setprecision(2) << pd.amount
               << " | " << pd.timestamp;
            row.setString(ss.str());
            row.setPosition(80, yPos);
            window.draw(row);

            screenButtons.emplace_back("Approve", mainFont, sf::Vector2f(120, 40), sf::Vector2f(620, yPos - 5));
            std::string reqId = pd.id;
            screenButtons.back().setAction([this, reqId]() {
                Bank::PendingDeposit pdOut;
                if (bank.takePendingDeposit(reqId, pdOut)) {
                    auto acc = bank.getAccount(pdOut.accountNumber);
                    if (acc) {
                        acc->deposit(pdOut.amount);
                        bank.updateAccountData();
                        auto trans = make_shared<DepositTransaction>(
                            atmMachine.generateTransactionID(),
                            pdOut.accountNumber,
                            pdOut.amount
                        );
                        acc->addTransaction(trans);
                        TransactionLog::logTransaction(trans, "DEPOSIT");
                        stringstream ss;
                        ss << "Approved deposit $" << fixed << setprecision(2) << pdOut.amount
                           << " for account " << pdOut.accountNumber;
                        transactionMessage = ss.str();
                    } else {
                        transactionMessage = "Account not found for pending request.";
                    }
                } else {
                    transactionMessage = "Pending request not found.";
                }
                previousMenuState = STATE_ADMIN_MENU;
                setScreen(STATE_TRANSACTION_COMPLETE);
            });

            yPos += 70;
            shown++;
        }
    }

    screenButtons.emplace_back("Back", mainFont, sf::Vector2f(160, 45), sf::Vector2f(340, 470));
    screenButtons.back().setAction([this]() { setScreen(STATE_ADMIN_MENU); });
}

void ATMInterface::drawViewTransactionsScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(26);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("Your Transaction History");
    displayText.setPosition(200, 120);
    window.draw(displayText);
    
    if (currentTransactions.empty()) {
        displayText.setCharacterSize(20);
        displayText.setFillColor(sf::Color::Yellow);
        displayText.setString("No transactions found");
        displayText.setPosition(300, 300);
        window.draw(displayText);
    } else {
        int startIdx = transactionPage * transactionPageSize;
        int endIdx = min(startIdx + transactionPageSize, (int)currentTransactions.size());
        
        int yPos = 160;
        for (int i = startIdx; i < endIdx; i++) {
            auto& trans = currentTransactions[i];
            stringstream ss;
            ss << "[" << trans.type << "] $" << fixed << setprecision(2) << trans.amount;
            
            displayText.setCharacterSize(15);
            displayText.setFillColor(sf::Color::Green);
            displayText.setString(ss.str());
            displayText.setPosition(140, yPos);
            window.draw(displayText);
            
            displayText.setCharacterSize(13);
            displayText.setFillColor(sf::Color::Cyan);
            displayText.setString(trans.timestamp);
            displayText.setPosition(140, yPos + 20);
            window.draw(displayText);
            
            yPos += 65;
        }
        
        // Page info
        displayText.setCharacterSize(14);
        displayText.setFillColor(sf::Color::Magenta);
        stringstream pageInfo;
        pageInfo << "Page " << (transactionPage + 1) << " of " 
                 << ((currentTransactions.size() + transactionPageSize - 1) / transactionPageSize);
        displayText.setString(pageInfo.str());
        displayText.setPosition(350, 475);
        window.draw(displayText);
    }
    
    float buttonY = 500;
    screenButtons.emplace_back("Back", mainFont, sf::Vector2f(140, 50), sf::Vector2f(120, buttonY));
    screenButtons.back().setAction([this]() { setScreen(STATE_MAIN_MENU); });
    
    if (transactionPage > 0) {
        screenButtons.emplace_back("Prev", mainFont, sf::Vector2f(140, 50), sf::Vector2f(360, buttonY));
        screenButtons.back().setAction([this]() { transactionPage--; });
    }
    
    if ((transactionPage + 1) * transactionPageSize < (int)currentTransactions.size()) {
        screenButtons.emplace_back("Next", mainFont, sf::Vector2f(140, 50), sf::Vector2f(600, buttonY));
        screenButtons.back().setAction([this]() { transactionPage++; });
    }
}

void ATMInterface::drawAdminViewAllTransactionsScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("All System Transactions");
    displayText.setPosition(220, 120);
    window.draw(displayText);
    
    if (currentTransactions.empty()) {
        displayText.setCharacterSize(20);
        displayText.setFillColor(sf::Color::Yellow);
        displayText.setString("No transactions found");
        displayText.setPosition(300, 300);
        window.draw(displayText);
    } else {
        int startIdx = transactionPage * transactionPageSize;
        int endIdx = min(startIdx + transactionPageSize, (int)currentTransactions.size());
        
        int yPos = 160;
        for (int i = startIdx; i < endIdx; i++) {
            auto& trans = currentTransactions[i];
            stringstream ss;
            ss << "[" << trans.type << "] " << trans.accountNumber.substr(0, 6) << "... - $" 
               << fixed << setprecision(2) << trans.amount;
            
            displayText.setCharacterSize(14);
            displayText.setFillColor(sf::Color::White);
            displayText.setString(ss.str());
            displayText.setPosition(140, yPos);
            window.draw(displayText);
            
            displayText.setCharacterSize(12);
            displayText.setFillColor(sf::Color::Cyan);
            displayText.setString(trans.timestamp);
            displayText.setPosition(140, yPos + 20);
            window.draw(displayText);
            
            yPos += 65;
        }
        
        // Page info
        displayText.setCharacterSize(13);
        displayText.setFillColor(sf::Color::Magenta);
        stringstream pageInfo;
        pageInfo << "Page " << (transactionPage + 1) << " of " 
                 << ((currentTransactions.size() + transactionPageSize - 1) / transactionPageSize);
        displayText.setString(pageInfo.str());
        displayText.setPosition(350, 475);
        window.draw(displayText);
    }
    
    float buttonY = 500;
    screenButtons.emplace_back("Back", mainFont, sf::Vector2f(140, 50), sf::Vector2f(120, buttonY));
    screenButtons.back().setAction([this]() { setScreen(STATE_ADMIN_MENU); });
    
    if (transactionPage > 0) {
        screenButtons.emplace_back("Prev", mainFont, sf::Vector2f(140, 50), sf::Vector2f(360, buttonY));
        screenButtons.back().setAction([this]() { transactionPage--; });
    }
    
    if ((transactionPage + 1) * transactionPageSize < (int)currentTransactions.size()) {
        screenButtons.emplace_back("Next", mainFont, sf::Vector2f(140, 50), sf::Vector2f(600, buttonY));
        screenButtons.back().setAction([this]() { transactionPage++; });
    }
}

void ATMInterface::drawTransferScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(26);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("Enter Recipient Account");
    displayText.setPosition(220, 120);
    window.draw(displayText);
    
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Account Number (7 digits):");
    displayText.setPosition(140, 180);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString(currentInput);
    inputText.setPosition(280, 270);
    window.draw(inputText);
    
    displayText.setCharacterSize(14);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("(or press ENTER to continue)");
    displayText.setPosition(280, 320);
    window.draw(displayText);
    
    screenButtons.emplace_back("Continue", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() {
        if (currentInput.length() == 7 && bank.accountExists(currentInput)) {
            if (currentInput != currentAccount->getAccountNumber()) {
                transferRecipientAccount = currentInput;
                currentInput.clear();
                setScreen(STATE_TRANSFER_AMOUNT);
            } else {
                transactionMessage = "Cannot transfer to same account!";
                currentInput.clear();
                setScreen(STATE_TRANSACTION_COMPLETE);
            }
        } else {
            transactionMessage = "Invalid account number!";
            currentInput.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() { 
        currentInput.clear();
        setScreen(STATE_MAIN_MENU); 
    });
}

void ATMInterface::drawTransferAmountScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    stringstream ss;
    ss << "From: " << currentAccount->getAccountNumber() << "\n";
    ss << "To: " << transferRecipientAccount;
    displayText.setString(ss.str());
    displayText.setPosition(150, 120);
    window.draw(displayText);
    
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Enter transfer amount:");
    displayText.setPosition(150, 200);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(36);
    inputText.setFillColor(sf::Color::Yellow);
    inputText.setString("$" + currentInput);
    inputText.setPosition(250, 290);
    window.draw(inputText);
    
    screenButtons.emplace_back("Transfer", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() { processTransfer(currentInput); });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() { 
        currentInput.clear();
        setScreen(STATE_MAIN_MENU); 
    });
}

void ATMInterface::drawCardLockedScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(32);
    displayText.setFillColor(sf::Color::Red);
    displayText.setString("CARD LOCKED");
    displayText.setPosition(320, 120);
    window.draw(displayText);
    
    displayText.setCharacterSize(22);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Your card has been locked due to\nmultiple failed PIN attempts.\n\nPlease contact an administrator\nto unlock your card.");
    displayText.setPosition(150, 230);
    window.draw(displayText);
    
    screenButtons.emplace_back("Try Another Card", mainFont, sf::Vector2f(220, 50), sf::Vector2f(200, 420));
    screenButtons.back().setAction([this]() {
        currentCard.reset();
        currentInput.clear();
        setScreen(STATE_WELCOME);
    });
    
    screenButtons.emplace_back("Exit", mainFont, sf::Vector2f(120, 50), sf::Vector2f(540, 420));
    screenButtons.back().setAction([this]() {
        currentCard.reset();
        currentInput.clear();
        setScreen(STATE_WELCOME);
    });
}

void ATMInterface::drawViewLockedCardsScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(28);
    displayText.setFillColor(sf::Color::Cyan);
    displayText.setString("Locked Cards");
    displayText.setPosition(340, 120);
    window.draw(displayText);
    
    // Get all accounts and find locked ones
    const auto& accountsMap = bank.getAllAccounts();
    vector<shared_ptr<Account>> lockedAccounts;
    
    for (const auto& [key, acc] : accountsMap) {
        if (acc && acc->getIsLocked()) {
            lockedAccounts.push_back(acc);
        }
    }
    
    if (lockedAccounts.empty()) {
        displayText.setCharacterSize(22);
        displayText.setFillColor(sf::Color::White);
        displayText.setString("No locked cards found.");
        displayText.setPosition(250, 250);
        window.draw(displayText);
    } else {
        float yPos = 140;
        int displayCount = 0;
        const int maxDisplay = 5;
        
        for (size_t i = scrollOffset; i < lockedAccounts.size() && displayCount < maxDisplay; ++i) {
            displayText.setCharacterSize(18);
            displayText.setFillColor(sf::Color::Yellow);
            string info = "Card: " + lockedAccounts[i]->getAccountNumber() + 
                             " | Name: " + bank.getAccountName(lockedAccounts[i]->getAccountNumber()) +
                             " | Failed Attempts: " + to_string(lockedAccounts[i]->getFailedLoginAttempts());
            displayText.setString(info);
            displayText.setPosition(140, yPos);
            window.draw(displayText);
            
            // Add unlock button for this card
            screenButtons.emplace_back("Unlock", mainFont, sf::Vector2f(100, 35), 
                                      sf::Vector2f(600, yPos - 5));
            string cardNum = lockedAccounts[i]->getAccountNumber();
            screenButtons.back().setAction([this, cardNum]() {
                admin.unlockCard(bank, cardNum);
                transactionMessage = "Card " + cardNum + " has been unlocked successfully!";
                previousMenuState = STATE_ADMIN_MENU;
                setScreen(STATE_TRANSACTION_COMPLETE);
            });
            
            yPos += 60;
            displayCount++;
        }
        
        // Scroll buttons
        if (lockedAccounts.size() > maxDisplay) {
            if (scrollOffset > 0) {
                screenButtons.emplace_back("Scroll Up", mainFont, sf::Vector2f(120, 40), 
                                          sf::Vector2f(100, 420));
                screenButtons.back().setAction([this]() { if (scrollOffset > 0) scrollOffset--; });
            }
            
            if (scrollOffset < lockedAccounts.size() - maxDisplay) {
                screenButtons.emplace_back("Scroll Down", mainFont, sf::Vector2f(140, 40), 
                                          sf::Vector2f(280, 420));
                screenButtons.back().setAction([this, lockedAccounts]() { 
                    if (scrollOffset < lockedAccounts.size() - 5) scrollOffset++; 
                });
            }
        }
    }
    
    screenButtons.emplace_back("Back", mainFont, sf::Vector2f(120, 50), sf::Vector2f(540, 420));
    screenButtons.back().setAction([this]() { setScreen(STATE_ADMIN_MENU); });
}

void ATMInterface::drawChangePinCurrentScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Change PIN\n\nEnter your current PIN:");
    displayText.setPosition(200, 150);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    string maskedInput(currentInput.length(), '*');
    inputText.setString(maskedInput);
    inputText.setPosition(350, 280);
    window.draw(inputText);
    
    screenButtons.emplace_back("Continue", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() {
        if (currentInput.length() >= 4) {
            currentPinInput = currentInput;
            currentInput.clear();
            setScreen(STATE_CHANGE_PIN_NEW);
        } else {
            transactionMessage = "PIN must be at least 4 digits!";
            currentInput.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        currentPinInput.clear();
        setScreen(STATE_MAIN_MENU);
    });
}

void ATMInterface::drawChangePinNewScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Enter your new PIN\n(minimum 4 digits):");
    displayText.setPosition(180, 150);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    string maskedInput(currentInput.length(), '*');
    inputText.setString(maskedInput);
    inputText.setPosition(350, 280);
    window.draw(inputText);
    
    screenButtons.emplace_back("Continue", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() {
        if (currentInput.length() >= 4) {
            newPinInput = currentInput;
            currentInput.clear();
            setScreen(STATE_CHANGE_PIN_CONFIRM);
        } else {
            transactionMessage = "PIN must be at least 4 digits!";
            currentInput.clear();
            setScreen(STATE_TRANSACTION_COMPLETE);
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_MAIN_MENU);
    });
}

void ATMInterface::drawChangePinConfirmScreen() {
    screenButtons.clear();
    
    displayText.setFont(mainFont);
    displayText.setCharacterSize(24);
    displayText.setFillColor(sf::Color::White);
    displayText.setString("Confirm your new PIN:");
    displayText.setPosition(200, 150);
    window.draw(displayText);
    
    sf::Text inputText;
    inputText.setFont(mainFont);
    inputText.setCharacterSize(32);
    inputText.setFillColor(sf::Color::Yellow);
    string maskedInput(currentInput.length(), '*');
    inputText.setString(maskedInput);
    inputText.setPosition(350, 280);
    window.draw(inputText);
    
    screenButtons.emplace_back("Confirm", mainFont, sf::Vector2f(180, 50), sf::Vector2f(200, 390));
    screenButtons.back().setAction([this]() {
        if (currentInput.length() >= 4) {
            processPinChange();
        } else {
            transactionMessage = "PIN must be at least 4 digits!";
            currentInput.clear();
            previousMenuState = STATE_MAIN_MENU;
            setScreen(STATE_TRANSACTION_COMPLETE);
        }
    });
    
    screenButtons.emplace_back("Cancel", mainFont, sf::Vector2f(180, 50), sf::Vector2f(430, 390));
    screenButtons.back().setAction([this]() {
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_MAIN_MENU);
    });
}

void ATMInterface::processPinChange() {
    previousMenuState = STATE_MAIN_MENU; // Set for all cases in this function
    
    if (!currentAccount) {
        transactionMessage = "Error: No account selected!";
        currentInput.clear();
        setScreen(STATE_TRANSACTION_COMPLETE);
        return;
    }
    
    // Get confirmation PIN from current input
    string confirmPin = currentInput;
    
    // Verify current PIN
    if (!bank.verifyPIN(currentAccount->getAccountNumber(), currentPinInput)) {
        transactionMessage = "Current PIN is incorrect!\nPIN change cancelled.";
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_TRANSACTION_COMPLETE);
        return;
    }
    
    // Verify new PIN matches confirmation
    if (newPinInput != confirmPin) {
        transactionMessage = "New PIN and confirmation do not match!\nPIN change cancelled.";
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_TRANSACTION_COMPLETE);
        return;
    }
    
    // Verify new PIN is different from current PIN
    if (newPinInput == currentPinInput) {
        transactionMessage = "New PIN must be different from current PIN!\nPIN change cancelled.";
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_TRANSACTION_COMPLETE);
        return;
    }
    
    // Verify new PIN meets exact length requirement
    if (newPinInput.length() != 4) {
        transactionMessage = "New PIN must be exactly 4 digits!\nPIN change cancelled.";
        currentInput.clear();
        currentPinInput.clear();
        newPinInput.clear();
        setScreen(STATE_TRANSACTION_COMPLETE);
        return;
    }
    
    // Update PIN
    currentAccount->setPin(newPinInput);
    bank.updateAccountData();
    
    transactionMessage = "PIN changed successfully!\nYour new PIN is now active.";
    currentInput.clear();
    currentPinInput.clear();
    newPinInput.clear();
    previousMenuState = STATE_MAIN_MENU;
    setScreen(STATE_TRANSACTION_COMPLETE);
}


//! TRYING: ANIMATION
void ATMInterface::drawInsertCardScreen() {
    screenButtons.clear();

    // Instruction text
    sf::Text info;
    info.setFont(mainFont);
    info.setCharacterSize(22);
    info.setFillColor(sf::Color::White);
    info.setString("Inserting card...");
    info.setPosition(320.f, 180.f);
    window.draw(info);

    // Draw only the moving card (slot hidden for cleaner look)
    sf::RenderStates cardStates;
    cardStates.transform.translate(-uiOffset);
    window.draw(card_, cardStates);

    // Allow skipping the animation
    screenButtons.emplace_back("Skip", mainFont, sf::Vector2f(140.f, 45.f), sf::Vector2f(380.f, 440.f));
    screenButtons.back().setAction([this]() {
        cardAnimating_ = false;
        setScreen(STATE_CARD_INPUT);
    });
}

void ATMInterface::updateInsertCard(float dt) {
    (void)dt; // dt reserved for future easing; animation uses clock for simplicity
    if (!cardAnimating_) {
        cardAnimating_ = true;
        animClock_.restart();
        // Ensure starting position is off-screen each time we enter this state
        card_.setPosition(slot_.getPosition().x + 70.f, cardYStart_);
    }

    float t = animClock_.getElapsedTime().asSeconds() / 1.0f; // 1 second animation
    if (t > 1.f) t = 1.f;

    float y = cardYStart_ + (cardYEnd_ - cardYStart_) * t;
    sf::Vector2f pos = card_.getPosition();
    card_.setPosition(pos.x, y);

    if (t >= 1.f) {
        cardAnimating_ = false;
        setScreen(STATE_CARD_INPUT);
    }
}

void ATMInterface::setScreen(ScreenState newState) {
    currentState = newState;
    screenButtons.clear();
}

void ATMInterface::logout() {
    currentAccount.reset();
    currentCard.reset();
    currentInput.clear();
    setScreen(STATE_WELCOME);
}

void ATMInterface::startAdminAccountAction(AdminActionMode mode) {
    adminActionMode = mode;
    setScreen(STATE_ADMIN_SELECT_ACCOUNT);
}

string ATMInterface::getAdminActionLabel() const {
    switch (adminActionMode) {
        case ADMIN_ACTION_RESET_PIN:
            return "Select Account to Reset PIN:";
        case ADMIN_ACTION_LOCK_CARD:
            return "Select Account to Lock Card:";
        default:
            return "Select Account:";
    }
}
