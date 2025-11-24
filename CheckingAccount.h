#ifndef CHECKINGACCOUNT_H
#define CHECKINGACCOUNT_H

#include "Account.h"

class CheckingAccount : public Account {
private:
    double overdraftLimit;

public:
    CheckingAccount(const std::string& accNum, const std::string& pinCode, 
                    double initialBalance = 0.0, double overdraft = 500.0)
        : Account(accNum, pinCode, initialBalance), overdraftLimit(overdraft) {}

    std::string displayAccountType() const override {
        return "Checking Account";
    }

    bool withdraw(double amount) override {
        if (amount > 0 && (balance + overdraftLimit) >= amount) {
            balance -= amount;
            return true;
        }
        return false;
    }

    double getOverdraftLimit() const { return overdraftLimit; }
};

#endif // CHECKINGACCOUNT_H