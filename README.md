# ATM Simulator (OOP Project)

Desktop ATM simulator with a GUI built using C++17 and SFML. It supports card-based signup, persistent accounts, deposits/withdrawals, transfers, transaction history, and an admin panel for managing accounts and approving pending deposits.

## Features
- Card-based signup with PIN creation and account type selection (Savings/Checking).
- Login with locked-card handling after repeated failures.
- Balance check, withdrawals, deposits (with pending queue), transfers, and PIN change.
- Transaction logging and paginated history.
- Admin dashboard: view accounts, lock/unlock cards, reset PINs, approve pending deposits, add interest.

Default admin credentials: `admin` / `admin`

## Requirements
- C++17 compiler
- SFML 2.6 (graphics, window, system, audio modules)

## Building
### macOS (Homebrew)
```bash
brew install sfml@2
c++ -std=c++17 main.cpp AtmInterface.cpp -I. -I/opt/homebrew/opt/sfml@2/include \
  -L/opt/homebrew/opt/sfml@2/lib -Wl,-rpath,/opt/homebrew/opt/sfml@2/lib \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -o atm_simulator
```

### Linux (apt-based)
```bash
sudo apt-get install g++ libsfml-dev
g++ -std=c++17 main.cpp AtmInterface.cpp -I. \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -o atm_simulator
```

### Windows (MSYS2 MinGW)
1) Install MSYS2, then in the MSYS2 MinGW64 shell:
```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml
```
2) Build:
```bash
g++ -std=c++17 main.cpp AtmInterface.cpp -I. \
  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -o atm_simulator.exe
```
3) Ensure the SFML `bin` directory is on `PATH` (or copy the SFML `.dll` files next to `atm_simulator.exe`), then run `./atm_simulator.exe` from the same directory as `assets/`.

### Using CMake
```bash
cmake -B build
cmake --build build
```
Binary outputs to `build/atm_simulator` (or platform equivalent).

## Running
```bash
./atm_simulator
```
Requires a GUI environment (SFML window). Ensure `assets/` and data files are alongside the binary.

## Data files
- `bank_accounts.dat` – persisted accounts (created at runtime).
- `pending_deposits.dat` – queued deposits awaiting admin approval (created at runtime).

Delete these files to reset stored state.

## Project layout
- `main.cpp` – entry point and banner.
- `AtmInterface.*` – GUI, state machine, and user interactions.
- `Bank.*` – account storage, persistence, pending deposits.
- `Account.*`, `SavingsAccount.h`, `CheckingAccount.h` – account models.
- `Transaction*`, `TransactionLog.h` – transaction records and logging.
- `Admin.h` – admin actions and credentials.
- `assets/` – ATM frame image and fonts used by the UI.

## Usage tips
- New users: follow on-screen flow to insert card, enter name, pick account type, and set a PIN.
- Admin tasks: log in with default credentials to approve pending deposits, lock/unlock cards, or add interest.
