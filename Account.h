#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <vector>
#include <memory>

class Transaction;

class Account {
protected:
    std::string accountNumber;
    std::string pin;
    double balance;
    std::vector<std::shared_ptr<Transaction>> transactionHistory;
    bool isLocked;
    int failedLoginAttempts;

public:
    Account(const std::string& accNum, const std::string& pinCode, double initialBalance = 0.0)
        : accountNumber(accNum), pin(pinCode), balance(initialBalance), isLocked(false), failedLoginAttempts(0) {}
    
    virtual ~Account() = default;

    virtual std::string displayAccountType() const = 0;

    std::string getAccountNumber() const { return accountNumber; }
    std::string getPin() const { return pin; }
    double getBalance() const { return balance; }

    void setPin(const std::string& newPin) { pin = newPin; }

    //# start operations 
    virtual bool deposit(double amount) {
        if (amount > 0) {
            balance += amount;
            return true;
        }
        return false;
    }

    virtual bool withdraw(double amount) {
        if (amount > 0 && balance >= amount) {
            balance -= amount;
            return true;
        }
        return false;
    }

    void addTransaction(std::shared_ptr<Transaction> trans) {
        transactionHistory.push_back(trans);
    }

    const std::vector<std::shared_ptr<Transaction>>& getTransactionHistory() const {
        return transactionHistory;
    }
	

	//# might remove interest we will see about this
    virtual void applyInterest(double rate) { 
        // Default implementation does nothing
    }

    // Lock management methods
    bool getIsLocked() const { return isLocked; }
    
    void lockCard() { isLocked = true; }
    
    void unlockCard() {
        isLocked = false;
        failedLoginAttempts = 0;
    }
    
    int getFailedLoginAttempts() const { return failedLoginAttempts; }
    
    void incrementFailedAttempts() { failedLoginAttempts++; }
    
    void resetFailedAttempts() { failedLoginAttempts = 0; }
    
    void setLockedStatus(bool locked, int attempts) {
        isLocked = locked;
        failedLoginAttempts = attempts;
    }
};

#endif // ACCOUNT_H