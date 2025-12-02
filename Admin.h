#ifndef ADMIN_H
#define ADMIN_H

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Bank.h"

class Admin {
private:
    std::string username;
    std::string password;

public:
    Admin(const std::string& user = "admin", const std::string& pass = "admin")
        : username(user), password(pass) {}

    bool login(const std::string& user, const std::string& pass) const {
        return (username == user && password == pass);
    }
    

    void addInterest(Bank& bank, double rate) {
        for (auto const& [key, accPtr] : bank.getAllAccounts()) {
            if (accPtr->displayAccountType() == "Savings Account") {
                accPtr->applyInterest(rate);
            }
        }
    }

    bool resetPIN(Bank& bank, const std::string& accountNumber, const std::string& newPIN) {
        auto account = bank.getAccount(accountNumber);
        if (account) {
            account->setPin(newPIN);
            return true;
        }
        return false;
    }
    bool unlockCard(Bank& bank, const std::string& accountNumber) {
        return bank.setAccountLock(accountNumber, false);
    }

    // Placeholder for future admin-specific approval logic if needed
};

#endif // ADMIN_H
