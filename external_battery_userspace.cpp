#include <cstdio>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace std;

string line;
int percentage = 0;
int percentageSmooth = 0;
int loop_count = 0;

string removeString(const string toRemove, string str)
{
    size_t pos = str.find(toRemove);
    str.erase(pos, toRemove.length());

    // cout << str << "\n";
    return str;
}

void batteryPercentage(int value, ofstream &battery) {
    percentageSmooth = percentageSmooth + value;
    loop_count++;
    // cout << "loop_count: " << loop_count << endl;
    // cout << "percentage: " << percentageSmooth << endl;

    if (loop_count >= 60) {
        percentageSmooth = percentageSmooth / 60;

        battery << "capacity0 = " << percentage << endl;

        percentageSmooth = 0;
        loop_count = 0;
    }
}

int main() {
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
    battery << "charging = 0" << endl;

    while (1) {
        if (serialStream.is_open() || !serialStream.fail()) {
            cout << "serialStream is open! reading..." << endl;
            while (getline(serialStream, line) && !serialStream.fail()) {
                try {
                    percentage = stoi(removeString("Battery Voltage: ", line));
                    batteryPercentage(percentage, battery);
                }
                catch (std::invalid_argument const& ex) {
                    std::cout << ex.what() << '\n';
                }
            }
        }

        if (!serialStream.is_open() || serialStream.fail()) {
            cout << "serialStream is/was closed!" << endl;
            cout << "retrying in 3 seconds..." << "\n" << endl;
            sleep(3);
            serialStream.close();
            serialStream.open("/dev/ttyUSB0");
        }
    }
}
