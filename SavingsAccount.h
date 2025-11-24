#ifndef SAVINGSACCOUNT_H
#define SAVINGSACCOUNT_H

#include "Account.h"

class SavingsAccount : public Account {
private:
    double interestRate;

public:
    SavingsAccount(const std::string& accNum, const std::string& pinCode, 
                   double initialBalance = 0.0, double rate = 0.015)
        : Account(accNum, pinCode, initialBalance), interestRate(rate) {}

    std::string displayAccountType() const override {
        return "Savings Account";
    }

    void applyInterest(double rate) override {
        balance += balance * rate;
    }

    double getInterestRate() const { return interestRate; }
};

#endif // SAVINGSACCOUNT_H