#ifndef DEPOSITTRANSACTION_H
#define DEPOSITTRANSACTION_H

#include "Transaction.h"

class DepositTransaction : public Transaction {
public:
    DepositTransaction(const std::string& transID, const std::string& accNum, double amt)
        : Transaction(transID, accNum, amt) {}

    std::string printReceipt() const override {
        std::stringstream ss;
        ss << "========== DEPOSIT RECEIPT ==========\n";
        ss << "Transaction ID: " << transactionID << "\n";
        ss << "Account Number: " << accountNumber << "\n";
        ss << "Type: DEPOSIT\n";
        ss << "Amount: $" << std::fixed << std::setprecision(2) << amount << "\n";
        ss << "Date/Time: " << timestamp << "\n";
        ss << "=====================================\n";
        return ss.str();
    }

    std::string getType() const override {
        return "DEPOSIT";
    }
};

#endif // DEPOSITTRANSACTION_H