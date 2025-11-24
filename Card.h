#ifndef CARD_H
#define CARD_H

#include <string>

class Card {
private:
    std::string cardNumber;
    std::string accountNumber;

public:
    Card(const std::string& cardNum, const std::string& accNum)
        : cardNumber(cardNum), accountNumber(accNum) {}

    std::string getCardNumber() const { return cardNumber; }
    std::string getAccountNumber() const { return accountNumber; }
};

#endif // CARD_H