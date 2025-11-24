#ifndef WITHDRAWALTRANSACTION_H
#define WITHDRAWALTRANSACTION_H

#include "Transaction.h"

class WithdrawalTransaction : public Transaction {
public:
    WithdrawalTransaction(const std::string& transID, const std::string& accNum, double amt)
        : Transaction(transID, accNum, amt) {}

    std::string printReceipt() const override {
        std::stringstream ss;
        ss << "======== WITHDRAWAL RECEIPT =========\n";
        ss << "Transaction ID: " << transactionID << "\n";
        ss << "Account Number: " << accountNumber << "\n";
        ss << "Type: WITHDRAWAL\n";
        ss << "Amount: $" << std::fixed << std::setprecision(2) << amount << "\n";
        ss << "Date/Time: " << timestamp << "\n";
        ss << "=====================================\n";
        return ss.str();
    }

    std::string getType() const override {
        return "WITHDRAWAL";
    }
};

#endif // WITHDRAWALTRANSACTION_H