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
    

    std::string generateReport(const Bank& bank) const {
        std::stringstream report;
        report << "\n========== BANK REPORT ==========\n";
        report << "Total Accounts: " << bank.getAllAccounts().size() << "\n\n";
        
        for (auto const& [key, accPtr] : bank.getAllAccounts()) {
            report << "Account: " << accPtr->getAccountNumber() << "\n";
            report << "Type: " << accPtr->displayAccountType() << "\n";
            report << "Balance: $" << std::fixed << std::setprecision(2) << accPtr->getBalance() << "\n";
            report << "---------------------------------\n";
        }
        report << "=================================\n";
        return report.str();
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
};

#endif // ADMIN_H
