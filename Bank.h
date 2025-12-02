#ifndef BANK_H
#define BANK_H

#include <string>
#include <map>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include "Account.h"
#include "SavingsAccount.h"
#include "CheckingAccount.h"

class Bank {
public:
    struct PendingDeposit {
        std::string id;
        std::string accountNumber;
        double amount;
        std::string timestamp;
    };

private:
    std::map<std::string, std::shared_ptr<Account>> accounts;
    std::map<std::string, std::string> accountNames; 
    const std::string dataFile = "bank_accounts.dat";
    const std::string pendingFile = "pending_deposits.dat";
    std::vector<PendingDeposit> pendingDeposits;
    int pendingCounter{0};

    void saveToFile() {
        std::ofstream file(dataFile);
        if (file.is_open()) {
            for (auto const& [key, accPtr] : accounts) {
                std::string name = accountNames.count(key) ? accountNames[key] : "Unknown";
                bool isLocked = accPtr->getIsLocked();
                file << accPtr->getAccountNumber() << ","
                     << accPtr->getPin() << ","
                     << accPtr->getBalance() << ","
                     << accPtr->displayAccountType() << ","
                     << name << ","
                     << (isLocked ? "1" : "0") << ","
                     << accPtr->getFailedLoginAttempts() << "\n";
            }
            file.close();
        }
    }

    void savePendingDeposits() {
        std::ofstream file(pendingFile);
        if (file.is_open()) {
            for (auto const& pd : pendingDeposits) {
                file << pd.id << "," << pd.accountNumber << "," << pd.amount << "," << pd.timestamp << "\n";
            }
            file.close();
        }
    }

    void loadFromFile() {
        std::ifstream file(dataFile);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string accNum, pin, balanceStr, accType, name, lockedStr, attemptsStr;
                
                std::getline(ss, accNum, ',');
                std::getline(ss, pin, ',');
                std::getline(ss, balanceStr, ',');
                std::getline(ss, accType, ',');
                std::getline(ss, name, ',');
                std::getline(ss, lockedStr, ',');
                std::getline(ss, attemptsStr);
                
                double balance = std::stod(balanceStr);
                
                if (accType == "Savings Account") {
                    accounts[accNum] = std::make_shared<SavingsAccount>(accNum, pin, balance);
                } else {
                    accounts[accNum] = std::make_shared<CheckingAccount>(accNum, pin, balance);
                }
                
                accountNames[accNum] = name.empty() ? "Unknown" : name;
                
                bool isLocked = (lockedStr == "1");
                // Load failed attempts if present (for backward compatibility, default to 0)
                int attempts = attemptsStr.empty() ? 0 : std::stoi(attemptsStr);
                accounts[accNum]->setLockedStatus(isLocked, attempts);
            }
            file.close();
        }
    }

    void loadPendingDeposits() {
        std::ifstream file(pendingFile);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            PendingDeposit pd;
            std::string amountStr;
            std::getline(ss, pd.id, ',');
            std::getline(ss, pd.accountNumber, ',');
            std::getline(ss, amountStr, ',');
            std::getline(ss, pd.timestamp);
            if (!pd.id.empty() && !pd.accountNumber.empty() && !amountStr.empty()) {
                pd.amount = std::stod(amountStr);
                pendingDeposits.push_back(pd);
                // track counter based on numeric part if present
                if (pd.id.size() > 2) {
                    try {
                        int num = std::stoi(pd.id.substr(2));
                        pendingCounter = std::max(pendingCounter, num);
                    } catch (...) {}
                }
            }
        }
        file.close();
    }

    std::string nextPendingId() {
        return "PD" + std::to_string(++pendingCounter);
    }

public:
    Bank() {
        loadFromFile();
        loadPendingDeposits();
    }

    bool createAccount(const std::string& cardNumber, const std::string& pin, const std::string& accountType, const std::string& holderName, double initialBalance = 0.0) {
        if (accounts.find(cardNumber) != accounts.end()) {
            return false; // Account already exists
        }
        
        if (accountType == "Savings") {
            accounts[cardNumber] = std::make_shared<SavingsAccount>(cardNumber, pin, initialBalance);
        } else {
            accounts[cardNumber] = std::make_shared<CheckingAccount>(cardNumber, pin, initialBalance);
        }
        
        accountNames[cardNumber] = holderName;
        saveToFile();
        return true;
    }

    void addAccount(std::shared_ptr<Account> account) {
        accounts[account->getAccountNumber()] = account;
        saveToFile();
    }

    std::shared_ptr<Account> getAccount(const std::string& accountNumber) const {
        auto it = accounts.find(accountNumber);
        if (it != accounts.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::string getAccountName(const std::string& accountNumber) const {
        auto it = accountNames.find(accountNumber);
        if (it != accountNames.end()) {
            return it->second;
        }
        return "Unknown";
    }

    const std::map<std::string, std::shared_ptr<Account>>& getAllAccounts() const {
        return accounts;
    }

    bool verifyPIN(const std::string& accountNumber, const std::string& pin) {
        auto account = getAccount(accountNumber);
        if (account) {
            return account->getPin() == pin;
        }
        return false;
    }

    bool accountExists(const std::string& accountNumber) const {
        return accounts.find(accountNumber) != accounts.end();
    }

    bool isAccountLocked(const std::string& accountNumber) const {
        auto account = getAccount(accountNumber);
        if (account) {
            return account->getIsLocked();
        }
        return false;
    }


bool setAccountLock(const std::string& accountNumber, bool locked) {
    std::shared_ptr<Account> account = getAccount(accountNumber);
    
    if (!account) { 
        return false;
    }
    
    // update card state for locking and stuff
    if (locked) {
        account->lockCard();
    } else {
        // This is the UNLOCK action, which also resets failed attempts
        account->unlockCard(); 
    }
    
    updateAccountData(); 
    
    return true;
}

    void updateAccountData() {
        saveToFile();
    }

    const std::vector<PendingDeposit>& getPendingDeposits() const {
        return pendingDeposits;
    }

    std::string addPendingDeposit(const std::string& accountNumber, double amount, const std::string& timestamp) {
        PendingDeposit pd;
        pd.id = nextPendingId();
        pd.accountNumber = accountNumber;
        pd.amount = amount;
        pd.timestamp = timestamp;
        pendingDeposits.push_back(pd);
        savePendingDeposits();
        return pd.id;
    }

    bool takePendingDeposit(const std::string& id, PendingDeposit& out) {
        for (auto it = pendingDeposits.begin(); it != pendingDeposits.end(); ++it) {
            if (it->id == id) {
                out = *it;
                pendingDeposits.erase(it);
                savePendingDeposits();
                return true;
            }
        }
        return false;
    }
};

#endif // BANK_H
