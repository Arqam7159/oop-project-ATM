#ifndef TRANSACTIONLOG_H
#define TRANSACTIONLOG_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include "Transaction.h"

struct TransactionRecord {
    std::string transactionID;
    std::string accountNumber;
    std::string type;
    double amount;
    std::string timestamp;
};

class TransactionLog {
private:
    static inline const std::string LOG_FILE = "transaction_log.csv";

public:
    static void logTransaction(const std::shared_ptr<Transaction>& trans, const std::string& type) {
        std::ofstream file(LOG_FILE, std::ios::app);
        if (file.is_open()) {
            file << trans->getTransactionID() << ","
                 << trans->getAccountNumber() << ","
                 << type << ","
                 << std::fixed << std::setprecision(2) << trans->getAmount() << ","
                 << trans->getTimestamp() << "\n";
            file.close();
        }
    }

    static std::vector<TransactionRecord> readTransactions(const std::string& accountNumber = "") {
        std::vector<TransactionRecord> records;
        std::ifstream file(LOG_FILE);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                
                std::stringstream ss(line);
                std::string id, account, type, amount, timestamp;
                
                if (std::getline(ss, id, ',') &&
                    std::getline(ss, account, ',') &&
                    std::getline(ss, type, ',') &&
                    std::getline(ss, amount, ',') &&
                    std::getline(ss, timestamp, ',')) {
                    
                    if (accountNumber.empty() || account == accountNumber) {
                        TransactionRecord rec;
                        rec.transactionID = id;
                        rec.accountNumber = account;
                        rec.type = type;
                        rec.amount = std::stod(amount);
                        rec.timestamp = timestamp;
                        records.push_back(rec);
                    }
                }
            }
            file.close();
        }
        return records;
    }
};

#endif // TRANSACTIONLOG_H
