#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Account.h"

class Customer {
private:
    int customerId;
    std::string name;
    std::string address;
    std::vector<std::shared_ptr<Account>> accounts;

public:
    Customer(int id, const std::string& n, const std::string& addr) 
        : customerId(id), name(n), address(addr) {}

    int getID() const { return customerId; } 

    std::string getName() const { return name; }
    std::string getAddress() const { return address; }
    const std::vector<std::shared_ptr<Account>>& getAccounts() const { return accounts; }

    void addAccount(std::shared_ptr<Account> account) {
        accounts.push_back(account);
    }
};