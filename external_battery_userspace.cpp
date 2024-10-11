#include <cstdio>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <stdint.h>
#include <ryzenadj.h>

using namespace std;

string line;
int percentage = 0;


#define WINDOW_SIZE 180
float   windowVoltage[WINDOW_SIZE] = {0};  // Initialize the window with zeros
int     rolAvgCountVoltage = 0;  // To keep track of the number of elements added
float   rolAvgVoltage = 0;

string removeString(const string toRemove, string str)
{
    size_t pos = str.find(toRemove);
    str.erase(pos, toRemove.length());

    // cout << str << "\n";
    return str;
}

float calculate_moving_average(float window[], int size) {
    float sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += window[i];
    }
    return sum / size;
}

void batteryPercentage(int value, ofstream &battery) {
    // cout << "value: " << percentageSmooth << endl;

    windowVoltage[rolAvgCountVoltage % WINDOW_SIZE] = value;
    rolAvgCountVoltage++;
    if (rolAvgCountVoltage > (WINDOW_SIZE * 1000)) {
        rolAvgCountVoltage = 1;
    }

    int sizeVoltage = rolAvgCountVoltage < WINDOW_SIZE ? rolAvgCountVoltage : WINDOW_SIZE;
    rolAvgVoltage = calculate_moving_average(windowVoltage, sizeVoltage);

    battery << "capacity0 = " << (int)rolAvgVoltage << endl;
    // cout << "percentage: " <<         rolAvgVoltage << endl;
}

int main() {
    if (getuid()) {
        cout << "Please run this program as a root user!" << endl;
        return 1;
    }

    // initialize ryzenadj
    ryzen_access ry = init_ryzenadj();

    ifstream serialStream;
    serialStream.open("/dev/ttyUSB0");
    if ( !serialStream.is_open() ) {
      cout << "Failed to open serialStream" << endl;
    }

    ofstream battery;
    battery.open("/dev/external_battery");
    if ( !battery.is_open() ) {
      cout << "Failed to open battery" << endl;
    }

    // Show as if the charger is disconnected
    // also set the ryzen cpu to a power-saving mode via ryzenadj
    battery << "charging = 0" << endl;
    set_power_saving(ry);

    while (1) {
        if (serialStream.is_open() || !serialStream.fail()) {
            cout << "serialStream is open! reading..." << endl;
            while (getline(serialStream, line) && !serialStream.fail()) {
                try {
                    percentage = stoi(removeString("Battery Voltage: ", line));
                    batteryPercentage(percentage, battery);
                }
                catch (std::invalid_argument const& ex) {
                    std::cout << "this did an oopsie: " << ex.what() << '\n';
                }
            }
        }

        if (!serialStream.is_open() || serialStream.fail()) {
            cout << "serialStream is/was closed!" << endl;
            cout << "retrying every 5 seconds..." << "\n" << endl;

            while(serialStream.fail()) {
                serialStream.close();
                serialStream.open("/dev/ttyUSB0");
                sleep(5);
            }
        }
    }
}
