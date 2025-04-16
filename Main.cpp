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
#include <memory>
#include <atomic>
#include <conio.h> // Только для Windows

using namespace std;

enum class VehicleType { MOTORCYCLE, SMALL, LARGE };
enum class ParkingSpotType { MOTORCYCLE, SMALL, LARGE, DISABLED, VIP };

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
            licensePlate = to_string(dis(gen)) + char('A' + dis(gen) % 26) + char('A' + dis(gen) % 26);
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
        default: return "Unknown";
        }
    }
};

class ParkingSpot {
private:
    int floor;
    int number;
    ParkingSpotType type;
    shared_ptr<Vehicle> parkedVehicle;
    mutex mtx;

public:
    ParkingSpot(int floor, int number, ParkingSpotType type)
        : floor(floor), number(number), type(type), parkedVehicle(nullptr) {
    }

    bool isOccupied() const { return parkedVehicle != nullptr; }
    ParkingSpotType getType() const { return type; }
    int getFloor() const { return floor; }
    int getNumber() const { return number; }
    shared_ptr<Vehicle> getVehicle() const { return parkedVehicle; }

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

    bool parkVehicle(shared_ptr<Vehicle> vehicle) {
        lock_guard<mutex> lock(mtx);
        if (parkedVehicle) return false;

        if (type == ParkingSpotType::VIP && !vehicle->getIsVIP()) return false;
        if (type == ParkingSpotType::DISABLED && !vehicle->getIsDisabled()) return false;

        switch (vehicle->getType()) {
        case VehicleType::MOTORCYCLE:
            if (type == ParkingSpotType::LARGE || type == ParkingSpotType::SMALL || type == ParkingSpotType::MOTORCYCLE || type == ParkingSpotType::VIP || type == ParkingSpotType::DISABLED)
                break;
            else return false;
        case VehicleType::SMALL:
            if (type == ParkingSpotType::LARGE || type == ParkingSpotType::SMALL || type == ParkingSpotType::VIP || type == ParkingSpotType::DISABLED)
                break;
            else return false;
        case VehicleType::LARGE:
            if (type == ParkingSpotType::LARGE || type == ParkingSpotType::VIP || type == ParkingSpotType::DISABLED)
                break;
            else return false;
        }

        parkedVehicle = vehicle;
        return true;
    }

    bool unparkVehicle() {
        lock_guard<mutex> lock(mtx);
        if (!parkedVehicle) return false;
        parkedVehicle = nullptr;
        return true;
    }
};

class ParkingLot {
private:
    vector<vector<ParkingSpot*>> floors;
    vector<shared_ptr<Vehicle>> ownedVehicles;
    mutable mutex mtx;
    mutable mutex vehiclesMtx;

public:
    ParkingLot() {
        int totalFloors = 3;
        int spotsPerFloor = 10;
        for (int floor = 1; floor <= totalFloors; ++floor) {
            vector<ParkingSpot*> floorSpots;
            for (int number = 0; number < spotsPerFloor; ++number) {
                ParkingSpotType type;
                if (number < 2)
                    type = ParkingSpotType::MOTORCYCLE;
                else if (number < 4)
                    type = ParkingSpotType::SMALL;
                else if (number < 7)
                    type = ParkingSpotType::LARGE;
                else if (number == 7)
                    type = ParkingSpotType::DISABLED;
                else
                    type = ParkingSpotType::VIP;

                floorSpots.push_back(new ParkingSpot(floor, number, type));
            }
            floors.push_back(floorSpots);
        }
    }

    ~ParkingLot() {
        for (auto& floor : floors)
            for (auto spot : floor)
                delete spot;
    }

    bool parkVehicle(shared_ptr<Vehicle> vehicle) {
        lock_guard<mutex> lock(mtx);
        for (auto& floor : floors) {
            for (auto spot : floor) {
                if (!spot->isOccupied() && spot->parkVehicle(vehicle)) {
                    lock_guard<mutex> vehiclesLock(vehiclesMtx);
                    ownedVehicles.push_back(vehicle);
                    return true;
                }
            }
        }
        return false;
    }

    bool unparkVehicle(const string& licensePlate) {
        lock_guard<mutex> lock(mtx);
        for (auto& floor : floors) {
            for (auto spot : floor) {
                auto vehicle = spot->getVehicle();
                if (vehicle && vehicle->getLicensePlate() == licensePlate) {
                    spot->unparkVehicle();
                    lock_guard<mutex> vehiclesLock(vehiclesMtx);
                    auto it = find(ownedVehicles.begin(), ownedVehicles.end(), vehicle);
                    if (it != ownedVehicles.end())
                        ownedVehicles.erase(it);
                    return true;
                }
            }
        }
        return false;
    }

    void displayVisual() const {
        lock_guard<mutex> lock(mtx);
        cout << "\n=== Parking Lot Map ===\n";
        for (const auto& floor : floors) {
            cout << "Floor " << floor[0]->getFloor() << ": ";
            for (const auto& spot : floor) {
                string t = spot->getTypeString().substr(0, 1);
                cout << (spot->isOccupied() ? "\033[1;31mX" : "\033[1;32m-") << t << "\033[0m ";
            }
            cout << "\n";
        }
    }
};

void simulateParking(ParkingLot& lot, int id) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> typeDist(0, 2);
    uniform_int_distribution<> boolDist(0, 1);
    uniform_int_distribution<> waitDist(5, 15);
    uniform_int_distribution<> retryDelay(100, 300);

    auto type = static_cast<VehicleType>(typeDist(gen));
    bool isVIP = boolDist(gen);
    bool isDisabled = boolDist(gen);

    auto vehicle = make_shared<Vehicle>(type, isVIP, isDisabled);
    bool parked = false;

    for (int attempt = 0; attempt < 10; ++attempt) {
        if (lot.parkVehicle(vehicle)) {
            parked = true;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(retryDelay(gen)));
    }

    if (!parked) {
        cout << "Thread " << id << ": No space for " << vehicle->getLicensePlate() << "\n";
        return;
    }

    cout << "Thread " << id << ": Parked " << vehicle->getLicensePlate()
        << (isVIP ? " [VIP]" : "") << (isDisabled ? " [Disabled]" : "") << "\n";

    this_thread::sleep_for(chrono::seconds(waitDist(gen)));

    if (lot.unparkVehicle(vehicle->getLicensePlate())) {
        cout << "Thread " << id << ": " << vehicle->getLicensePlate() << " left\n";
    }
}

int main() {
    ParkingLot lot;
    atomic<int> threadId{ 0 };
    atomic<double> trafficDelay{ 2.0 };
    atomic<bool> running{ true };
    thread generator, inputThread;

    while (true) {
        cout << "\n--- Menu ---\n";
        cout << "1 - Park a vehicle\n";
        cout << "2 - Unpark a vehicle\n";
        cout << "3 - Show parking status\n";
        cout << "4 - Start simulation\n";
        cout << "0 - Exit\n";
        cout << "Your choice: ";

        int choice;
        cin >> choice;

        if (choice == 0) {
            running = false;
            cout << "Exiting...\n";
            break;
        }

        else if (choice == 1) {
            int t, v, d;
            cout << "Enter vehicle type (0 - Motorcycle, 1 - Small, 2 - Large): ";
            cin >> t;
            cout << "VIP? (1 - yes, 0 - no): ";
            cin >> v;
            cout << "Disabled? (1 - yes, 0 - no): ";
            cin >> d;

            auto vehicle = make_shared<Vehicle>(static_cast<VehicleType>(t), v, d);
            if (lot.parkVehicle(vehicle)) {
                cout << "Vehicle parked! License Plate: " << vehicle->getLicensePlate() << "\n";
            }
            else {
                cout << "No available parking spots!\n";
            }
        }

        else if (choice == 2) {
            string plate;
            cout << "Enter vehicle license plate: ";
            cin >> plate;

            if (lot.unparkVehicle(plate)) {
                cout << "Vehicle with license plate " << plate << " has left.\n";
            }
            else {
                cout << "Vehicle not found.\n";
            }
        }

        else if (choice == 3) {
            lot.displayVisual();
        }

        else if (choice == 4) {
            if (generator.joinable() || inputThread.joinable()) {
                cout << "Simulation already running.\n";
                continue;
            }

            cout << "Simulation started! Press + / - to change intensity, 0 to stop.\n";

            generator = thread([&]() {
                while (running) {
                    thread(simulateParking, ref(lot), threadId++).detach();
                    this_thread::sleep_for(chrono::duration<double>(trafficDelay));
                }
                });

            inputThread = thread([&]() {
                while (running) {
                    if (_kbhit()) {
                        char c = _getch();
                        if (c == '+') {
                            trafficDelay.store(max(0.1, trafficDelay.load() - 0.1));
                            cout << "\n[Input] Traffic UP: delay = " << trafficDelay.load() << "s\n";
                        }
                        else if (c == '-') {
                            trafficDelay.store(trafficDelay.load() + 0.1);
                            cout << "\n[Input] Traffic DOWN: delay = " << trafficDelay.load() << "s\n";
                        }
                        else if (c == '0') {
                            running = false;
                            cout << "\n[Input] Stopping simulation...\n";
                        }
                    }
                    this_thread::sleep_for(chrono::milliseconds(100));
                }
                });

            // Visualization
            while (running) {
                lot.displayVisual();
                this_thread::sleep_for(chrono::seconds(2));
            }

            if (generator.joinable()) generator.join();
            if (inputThread.joinable()) inputThread.join();
        }

        else {
            cout << "Invalid input. Please try again.\n";
        }
    }

    return 0;
}




