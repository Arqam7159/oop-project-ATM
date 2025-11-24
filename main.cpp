#include "AtmInterface.h"
#include <iostream>

int main() {
    std::cout << "========================================\n";
    std::cout << "   ATM Simulator - Project Group 40\n";
    std::cout << "   Team Members:\n";
    std::cout << "   - Mohammad Arqam Nakhuda \n";
    std::cout << "   - Bilal Sheraz \n";
    std::cout << "   - Syed Arham Hussain \n";
    std::cout << "========================================\n\n";
    std::cout << "FEATURES:\n";
    std::cout << "- Card-based signup for new customers\n";
    std::cout << "- Persistent account storage\n";
    std::cout << "- Admin panel with GUI\n";
    std::cout << "- PIN management\n\n";
    std::cout << "Admin Login: Username: admin, Password: admin\n";
    std::cout << "========================================\n\n";

    try {
        ATMInterface atm;
        atm.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}