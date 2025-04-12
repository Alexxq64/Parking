#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <iomanip>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>

using namespace std;

enum class VehicleType {
    MOTORCYCLE,
    SMALL,
    LARGE,
    DISABLED
};

enum class ParkingSpotType {
    MOTORCYCLE,
    SMALL,
    LARGE,
    DISABLED,
    VIP
};

class Vehicle {
private:
    string licensePlate;
    VehicleType type;
    bool isVIP;
    bool isDisabled;

public:
    Vehicle(VehicleType type, bool isVIP, bool isDisabled, std::string plate = "")
        : type(type), isVIP(isVIP), isDisabled(isDisabled) {
        if (plate.empty()) {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(100, 999);
            licensePlate = std::to_string(dis(gen)) + char('A' + dis(gen) % 26) + char('A' + dis(gen) % 26);
        }
        else {
            licensePlate = plate;
        }
    }

    VehicleType getType() const { return type; }
    bool getIsVIP() const { return isVIP; }
    bool getIsDisabled() const { return isDisabled; }
    string getLicensePlate() const { return licensePlate; }

    string getTypeString() const {
        switch (type) {
        case VehicleType::MOTORCYCLE: return "Motorcycle";
        case VehicleType::SMALL: return "Small";
        case VehicleType::LARGE: return "Large";
        case VehicleType::DISABLED: return "Disabled";
        default: return "Unknown";
        }
    }
};

class ParkingSpot {
private:
    int floor;
    int number;
    ParkingSpotType type;
    Vehicle* parkedVehicle;
    std::mutex mtx;

public:
    ParkingSpot(int floor, int number, ParkingSpotType type)
        : floor(floor), number(number), type(type), parkedVehicle(nullptr) {
    }

    bool isOccupied() const { return parkedVehicle != nullptr; }
    ParkingSpotType getType() const { return type; }
    int getFloor() const { return floor; }
    int getNumber() const { return number; }
    Vehicle* getVehicle() const { return parkedVehicle; }

    string getTypeString() const {
        switch (type) {
        case ParkingSpotType::MOTORCYCLE: return "Motorcycle";
        case ParkingSpotType::SMALL: return "Small";
        case ParkingSpotType::LARGE: return "Large";
        case ParkingSpotType::DISABLED: return "Disabled";
        case ParkingSpotType::VIP: return "VIP";
        default: return "Unknown";
        }
    }

    bool parkVehicle(Vehicle* vehicle) {
        std::lock_guard<std::mutex> lock(mtx);
        if (parkedVehicle != nullptr) return false;

        if (vehicle->getIsVIP() && type != ParkingSpotType::VIP) return false;
        if (!vehicle->getIsVIP() && type == ParkingSpotType::VIP) return false;
        if (vehicle->getIsDisabled() && type != ParkingSpotType::DISABLED) return false;

        switch (vehicle->getType()) {
        case VehicleType::MOTORCYCLE:
            if (type != ParkingSpotType::MOTORCYCLE &&
                type != ParkingSpotType::SMALL &&
                type != ParkingSpotType::LARGE) return false;
            break;
        case VehicleType::SMALL:
            if (type != ParkingSpotType::SMALL &&
                type != ParkingSpotType::LARGE) return false;
            break;
        case VehicleType::LARGE:
            if (type != ParkingSpotType::LARGE) return false;
            break;
        case VehicleType::DISABLED:
            if (type != ParkingSpotType::DISABLED) return false;
            break;
        }

        parkedVehicle = vehicle;
        return true;
    }

    bool unparkVehicle() {
        lock_guard<mutex> lock(mtx);
        if (parkedVehicle == nullptr) return false;
        parkedVehicle = nullptr;
        return true;
    }
};

class ParkingLot {
private:
    vector<vector<ParkingSpot*>> floors;
    map<string, pair<int, int>> vehicleLocation;
    mutable std::mutex mtx;

public:
    ParkingLot() {
        vector<ParkingSpot*> floor1;
        for (int i = 0; i < 5; ++i) floor1.push_back(new ParkingSpot(1, i, ParkingSpotType::MOTORCYCLE));
        for (int i = 5; i < 15; ++i) floor1.push_back(new ParkingSpot(1, i, ParkingSpotType::SMALL));
        for (int i = 15; i < 20; ++i) floor1.push_back(new ParkingSpot(1, i, ParkingSpotType::LARGE));
        for (int i = 20; i < 22; ++i) floor1.push_back(new ParkingSpot(1, i, ParkingSpotType::DISABLED));
        floors.push_back(floor1);

        vector<ParkingSpot*> floor2;
        for (int i = 0; i < 10; ++i) floor2.push_back(new ParkingSpot(2, i, ParkingSpotType::VIP));
        for (int i = 10; i < 12; ++i) floor2.push_back(new ParkingSpot(2, i, ParkingSpotType::DISABLED));
        floors.push_back(floor2);
    }

    ~ParkingLot() {
        for (auto& floor : floors) {
            for (auto spot : floor) {
                delete spot;
            }
        }
    }

    bool parkVehicle(Vehicle* vehicle) {
        lock_guard<mutex> lock(mtx);
        for (auto& floor : floors) {
            for (auto spot : floor) {
                if (!spot->isOccupied() && spot->parkVehicle(vehicle)) {
                    vehicleLocation[vehicle->getLicensePlate()] = make_pair(spot->getFloor(), spot->getNumber());
                    return true;
                }
            }
        }
        return false;
    }

    bool unparkVehicle(const string& licensePlate) {
        lock_guard<mutex> lock(mtx);
        auto it = vehicleLocation.find(licensePlate);
        if (it == vehicleLocation.end()) return false;

        int floor = it->second.first;
        int spotNum = it->second.second;
        bool success = floors[floor - 1][spotNum]->unparkVehicle();
        if (success) {
            vehicleLocation.erase(it);
        }
        return success;
    }

    void displayStatus() const {
        std::lock_guard<mutex> lock(mtx);
        cout << "\nParking Lot Status:\n";
        cout << "-------------------\n";

        for (const auto& floor : floors) {
            cout << "Floor " << floor[0]->getFloor() << ":\n";
            cout << left << setw(12) << "Spot" << setw(12) << "Type" << setw(10) << "Status" << "Vehicle\n";
            cout << string(50, '-') << endl;

            for (const auto spot : floor) {
                cout << left << setw(12) << spot->getNumber()
                    << setw(12) << spot->getTypeString()
                    << setw(10) << (spot->isOccupied() ? "Occupied" : "Free");

                if (spot->isOccupied()) {
                    Vehicle* v = spot->getVehicle();
                    cout << v->getLicensePlate() << " (" << v->getTypeString();
                    if (v->getIsVIP()) cout << ", VIP";
                    if (v->getIsDisabled()) cout << ", Disabled";
                    cout << ")";
                }
                cout << endl;
            }
            cout << endl;
        }
    }

    void displayVisual() const {
        std::lock_guard<mutex> lock(mtx);
        cout << "\nParking Lot Visual:\n";
        cout << "------------------\n";

        for (const auto& floor : floors) {
            cout << "Floor " << floor[0]->getFloor() << ":\n";
            for (const auto spot : floor) {
                char symbol;
                switch (spot->getType()) {
                case ParkingSpotType::MOTORCYCLE: symbol = 'M'; break;
                case ParkingSpotType::SMALL: symbol = 'S'; break;
                case ParkingSpotType::LARGE: symbol = 'L'; break;
                case ParkingSpotType::DISABLED: symbol = 'D'; break;
                case ParkingSpotType::VIP: symbol = 'V'; break;
                default: symbol = '?';
                }

                if (spot->isOccupied()) {
                    cout << "\033[1;31m" << symbol << "\033[0m ";
                }
                else {
                    cout << "\033[1;32m" << symbol << "\033[0m ";
                }
            }
            cout << "\n\n";
        }
    }

    bool isVehicleParked(const string& licensePlate) const {
        std::lock_guard<mutex> lock(mtx);
        return vehicleLocation.find(licensePlate) != vehicleLocation.end();
    }
};

void simulateParking(ParkingLot& parkingLot, int threadId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> typeDist(0, 3);
    uniform_int_distribution<> boolDist(0, 1);

    for (int i = 0; i < 3; ++i) {
        VehicleType type = static_cast<VehicleType>(typeDist(gen));
        bool isVIP = boolDist(gen);
        bool isDisabled = boolDist(gen);

        Vehicle* vehicle = new Vehicle(type, isVIP, isDisabled);

        if (parkingLot.parkVehicle(vehicle)) {
            cout << "Thread " << threadId << ": Parked " << vehicle->getTypeString()
                << " (Plate: " << vehicle->getLicensePlate() << ")";
            if (isVIP) cout << " [VIP]";
            if (isDisabled) cout << " [Disabled]";
            cout << endl;

            this_thread::sleep_for(chrono::milliseconds(500 + gen() % 1000));

            if (parkingLot.unparkVehicle(vehicle->getLicensePlate())) {
                cout << "Thread " << threadId << ": Unparked " << vehicle->getLicensePlate() << endl;
            }
            else {
                cout << "Thread " << threadId << ": Failed to unpark " << vehicle->getLicensePlate() << endl;
            }
        }
        else {
            cout << "Thread " << threadId << ": Failed to park " << vehicle->getTypeString()
                << " (no suitable spot)" << endl;
        }

        delete vehicle;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

int main() {
    ParkingLot parkingLot;

    // Случайная начальная инициализация
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> typeDist(0, 3);
    uniform_int_distribution<> boolDist(0, 1);
    uniform_int_distribution<> countDist(5, 10);

    int initialVehicles = countDist(gen);
    for (int i = 0; i < initialVehicles; ++i) {
        VehicleType type = static_cast<VehicleType>(typeDist(gen));
        bool isVIP = boolDist(gen);
        bool isDisabled = boolDist(gen);
        Vehicle* v = new Vehicle(type, isVIP, isDisabled);

        if (!parkingLot.parkVehicle(v)) delete v;
    }

    cout << "\nParking lot initialized with " << initialVehicles << " random vehicles.\n";

    int choice;
    do {
        cout << "\nParking Management System\n";
        cout << "1. Park a vehicle\n";
        cout << "2. Unpark a vehicle\n";
        cout << "3. Display parking status\n";
        cout << "4. Display visual parking map\n";
        cout << "5. Run multithreading simulation\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1: {
            int typeChoice;
            bool isVIP, isDisabled;
            std::string licensePlate;

            cout << "Select vehicle type:\n";
            cout << "1. Motorcycle\n";
            cout << "2. Small car\n";
            cout << "3. Large car\n";
            cout << "4. Disabled vehicle\n";
            cout << "Enter type (1-4): ";
            cin >> typeChoice;

            cout << "Is VIP? (0/1): ";
            cin >> isVIP;
            cout << "Is disabled? (0/1): ";
            cin >> isDisabled;
            cout << "Enter license plate (or leave empty for random): ";
            cin.ignore();
            std::getline(cin, licensePlate);

            VehicleType type;
            switch (typeChoice) {
            case 1: type = VehicleType::MOTORCYCLE; break;
            case 2: type = VehicleType::SMALL; break;
            case 3: type = VehicleType::LARGE; break;
            case 4: type = VehicleType::DISABLED; break;
            default:
                cout << "Invalid type!\n";
                continue;
            }

            Vehicle* vehicle = new Vehicle(type, isVIP, isDisabled, licensePlate);
            if (parkingLot.parkVehicle(vehicle)) {
                cout << "Vehicle parked successfully! Plate: " << vehicle->getLicensePlate() << endl;
            }
            else {
                cout << "Failed to park vehicle - no suitable spot available.\n";
                delete vehicle;
            }
            break;
        }
        case 2: {
            string licensePlate;
            cout << "Enter license plate to unpark: ";
            cin >> licensePlate;

            if (parkingLot.unparkVehicle(licensePlate)) {
                cout << "Vehicle with plate " << licensePlate << " has left the parking.\n";
            }
            else {
                cout << "No vehicle found with plate " << licensePlate << " or error occurred.\n";
            }
            break;
        }
        case 3:
            parkingLot.displayStatus();
            break;
        case 4:
            parkingLot.displayVisual();
            break;
        case 5: {
            const int numThreads = 10;
            thread threads[numThreads];

            cout << "Starting multithreading simulation with " << numThreads << " threads...\n";
            for (int i = 0; i < numThreads; ++i) {
                threads[i] = thread(simulateParking, ref(parkingLot), i + 1);
            }

            for (int i = 0; i < numThreads; ++i) {
                threads[i].join();
            }
            cout << "Simulation completed.\n";
            break;
        }
        case 0:
            cout << "Exiting...\n";
            break;
        default:
            cout << "Invalid choice. Try again.\n";
        }
    } while (choice != 0);

    return 0;
}
