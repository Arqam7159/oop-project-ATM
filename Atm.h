#ifndef ATM_H
#define ATM_H

#include <string>
#include "Bank.h"
#include "Card.h"

class ATM {
private:
    Bank& bank;
    double cashAvailable;
    int transactionCounter;

public:
    ATM(Bank& bankRef) : bank(bankRef), cashAvailable(1000000.0), transactionCounter(0) {}

    void refillCash(double amount) {
        if (amount > 0) {
            cashAvailable += amount;
        }
    }

    bool canDispense(double amount) const {
        return amount <= cashAvailable && amount > 0;
    }

    double getCashAvailable() const {
        return cashAvailable;
    }

    bool dispenseCash(double amount) {
        if (canDispense(amount)) {
            cashAvailable -= amount;
            return true;
        }
        return false;
    }

    void acceptCash(double amount) {
        if (amount > 0) {
            cashAvailable += amount;
        }
    }

    Bank& getBank() {
        return bank;
    }

    std::string generateTransactionID() {
        return "TXN" + std::to_string(++transactionCounter);
    }
};

#endif // ATM_H
