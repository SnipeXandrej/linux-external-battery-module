#include <cstdio>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <termios.h>

#include <stdint.h>
#include <ryzenadj.h>

using namespace std;

ofstream battery;

// initialize ryzenadj
ryzen_access ry = init_ryzenadj();

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

void set_battery_charging() {
    battery << "charging = 0" << endl;
    set_power_saving(ry);
}

void set_battery_discharging() {
    battery << "charging = 1" << endl;
    set_max_performance(ry);
}

void listenForInput() {
    // Set terminal to non-blocking mode
    termios settings;
    tcgetattr(STDIN_FILENO, &settings);    // Get current terminal settings
    settings.c_lflag &= ~ICANON;           // Disable canonical mode (buffered input)
    settings.c_lflag &= ~ECHO;             // Disable echoing the input
    tcsetattr(STDIN_FILENO, TCSANOW, &settings);  // Apply new settings

    char input;
    while (1) {
        if (read(STDIN_FILENO, &input, 1) > 0) {  // Non-blocking read
            if (input == '\x1b') {  // Escape sequence
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1) > 0 && read(STDIN_FILENO, &seq[1], 1) > 0) {
                    if (seq[0] == '[') {
                        switch (seq[1]) {
                            case 'A':
                                set_battery_discharging();
                                cout << "Set to charging/plugged in" << endl;
                                break;
                            case 'B':
                                set_battery_charging();
                                cout << "Set to discharging/unplugged" << endl;
                                break;
                        }
                    }
                }
            }
        }
    }
}

int main() {
    if (getuid()) {
        cout << "Please run this program as a root user!" << endl;
        return 1;
    }

    if(!ry){
        printf("Unable to init ryzenadj\n");
        return -1;
    }

    ifstream serialStream;
    serialStream.open("/dev/ttyUSB0");
    if ( !serialStream.is_open() ) {
      cout << "Failed to open serialStream" << endl;
    }

    battery.open("/dev/external_battery");
    if ( !battery.is_open() ) {
      cout << "Failed to open battery" << endl;
    }

    // Show as if the charger is disconnected
    // also set the ryzen cpu to a power-saving mode via ryzenadj
    battery << "charging = 0" << endl;
    set_power_saving(ry);

    thread inputThread(listenForInput);

    while (1) {
        if (serialStream.is_open() || !serialStream.fail()) {
            cout << "serialStream is open! reading..." << endl;
            while (getline(serialStream, line) && !serialStream.fail()) {
                try {
                    percentage = stoi(removeString("Battery Voltage: ", line));
                    batteryPercentage(percentage, battery);
                }
                catch (std::invalid_argument const& ex) {
                    cout << "this did an oopsie: " << ex.what() << '\n';
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
