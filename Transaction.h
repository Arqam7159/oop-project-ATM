#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

class Transaction {
protected:
    std::string transactionID;
    std::string accountNumber;
    double amount;
    std::string timestamp;

    std::string getCurrentTimestamp() {
        time_t now = time(0);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

public:
    Transaction(const std::string& transID, const std::string& accNum, double amt)
        : transactionID(transID), accountNumber(accNum), amount(amt) {
        timestamp = getCurrentTimestamp();
    }

    virtual ~Transaction() = default;

    // Pure virtual function for polymorphism
    virtual std::string printReceipt() const = 0;
    virtual std::string getType() const = 0;
    
    std::string getTransactionID() const { return transactionID; }
    std::string getAccountNumber() const { return accountNumber; }
    double getAmount() const { return amount; }
    std::string getTimestamp() const { return timestamp; }
};

#endif // TRANSACTION_H
