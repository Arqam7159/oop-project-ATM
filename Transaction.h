#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

class Transaction {
protected:
    std::string transactionID;//Records the no of transactions
    std::string accountNumber;//Accoun number for which the transaction is done
    double amount;//How much the transaction is for
    std::string timestamp;//Takes the date and time from the system

    std::string getCurrentTimestamp() 
    {
        //Gets the current time from the system
        time_t now = time(0);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

public:
//Constructor for our transaction
    Transaction(const std::string& transID, const std::string& accNum, double amt)
        : transactionID(transID), accountNumber(accNum), amount(amt) {
        timestamp = getCurrentTimestamp();
    }
//Destructor for transaction
    virtual ~Transaction() = default;


    virtual std::string printReceipt() const = 0;
    virtual std::string getType() const = 0;
    
    std::string getTransactionID() const { return transactionID; }
    std::string getAccountNumber() const { return accountNumber; }
    double getAmount() const { return amount; }
    std::string getTimestamp() const { return timestamp; }
};

#endif // TRANSACTION_H