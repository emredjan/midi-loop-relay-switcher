#include <SD.h>
#include <SPI.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

const char *filename = "/config.cfg";

const int NUM_LOOPS = 6;
const int NUM_SWITCHES = 2;
const int NUM_PRESETS = 18;

const int MOMENTARY_MS = 150;

const int CS_PIN = 10;

const uint8_t LOOP_PINS[NUM_LOOPS] = {2, 3, 4, 5, 6, 7};
const uint8_t SWITCH_PINS[NUM_SWITCHES] = {8, 9};

const uint8_t ACTIVATE_LOOP_MIN = 111;
const uint8_t ACTIVATE_LOOP_MAX = ACTIVATE_LOOP_MIN + NUM_LOOPS - 1;
const uint8_t BYPASS_LOOP_MIN = 101;
const uint8_t BYPASS_LOOP_MAX = BYPASS_LOOP_MIN + NUM_LOOPS - 1;
const uint8_t ACTIVATE_SWITCH_MIN = ACTIVATE_LOOP_MIN + NUM_LOOPS;
const uint8_t ACTIVATE_SWITCH_MAX = ACTIVATE_SWITCH_MIN + NUM_SWITCHES - 1;
const uint8_t BYPASS_SWITCH_MIN = BYPASS_LOOP_MIN + NUM_LOOPS;
const uint8_t BYPASS_SWITCH_MAX = BYPASS_SWITCH_MIN + NUM_SWITCHES - 1;

struct Preset
{
    uint8_t loops[NUM_LOOPS];
    uint8_t switches[NUM_SWITCHES];
};

struct Config
{
    uint8_t MIDI_CHANNEL;
    uint8_t switchTypes[NUM_SWITCHES];
};

Preset presets[NUM_PRESETS];
Config config;

void setup()
{
    while (!SD.begin(CS_PIN))
    {
        Serial.println(F("Failed to initialize SD library"));
        delay(1000);
    }

    loadConfiguration(filename);

    // Set loop & switch pins
    for (uint8_t i = 0; i < NUM_LOOPS; i++)
        pinMode(LOOP_PINS[i], OUTPUT);
    for (uint8_t i = 0; i < NUM_SWITCHES; i++)
        pinMode(SWITCH_PINS[i], OUTPUT);

    MIDI.begin(config.MIDI_CHANNEL);
    MIDI.setHandleProgramChange(handleProgramChange);

    // Initialize serial port
    // Disable when activating MIDI
    // Serial.begin(9600);
    // while (!Serial)
    //     continue;

    // printConfig();
}

void loop()
{
    MIDI.read();
}

void handleProgramChange(uint8_t channel, uint8_t number)
{
    if (channel == config.MIDI_CHANNEL)
    {
        if (number == 0)
            resetAll();
        else if (number <= NUM_PRESETS)
            loadPreset(number - 1);
        else if (
            (number >= ACTIVATE_LOOP_MIN && number <= ACTIVATE_LOOP_MAX) ||
            (number >= BYPASS_LOOP_MIN && number <= BYPASS_LOOP_MAX))
        {
            uint8_t loopNum = (number % 10) - 1;
            uint8_t loopState = number / 10 % 10;
            digitalWrite(LOOP_PINS[loopNum], loopState);
        }
        else if (
            (number >= ACTIVATE_SWITCH_MIN && number <= ACTIVATE_SWITCH_MAX) ||
            (number >= BYPASS_SWITCH_MIN && number <= BYPASS_SWITCH_MAX))
        {
            uint8_t switchNum = (number % 10) - 1 - NUM_LOOPS;
            uint8_t switchState = number / 10 % 10;
            setSwitch(SWITCH_PINS[switchNum], config.switchTypes[switchNum], switchState);
        }
    }
}

void loadPreset(uint8_t presetNum)
{
    for (uint8_t i = 0; i < NUM_LOOPS; i++)
        digitalWrite(LOOP_PINS[i], presets[presetNum].loops[i]);

    for (uint8_t i = 0; i < NUM_SWITCHES; i++)
        setSwitch(SWITCH_PINS[i], config.switchTypes[i], presets[presetNum].switches[i]);
}

void setSwitch(uint8_t switchPin, uint8_t switchType, uint8_t switchVal)
{

    if (switchType == 1)
    {
        digitalWrite(switchPin, switchVal);
    }
    else
    {
        if (switchVal == 1)
        {
            digitalWrite(switchPin, switchVal);
            delay(MOMENTARY_MS);
            digitalWrite(switchPin, !switchVal);
        }
    }
}

void resetAll()
{
    for (uint8_t i = 0; i < NUM_LOOPS; i++)
        digitalWrite(LOOP_PINS[i], 0);

    for (uint8_t i = 0; i < NUM_SWITCHES; i++)
        setSwitch(SWITCH_PINS[i], config.switchTypes[i], 0);
}

// Loads the configuration from a file
void loadConfiguration(char *filename)
{
    File file = SD.open(filename);
    if (!file)
    {
        Serial.println(F("Failed to read file"));
        return;
    }

    // Read MIDI channel
    String midiChannelStr = "";
    midiChannelStr += (char)file.read() - 48;
    midiChannelStr += (char)file.read() - 48;

    config.MIDI_CHANNEL = midiChannelStr.toInt();
    file.read();
    file.read();

    // Read relay switch types
    // 1: Latching
    // 0: Momentary
    for (uint8_t i = 0; i < NUM_SWITCHES; i++)
        config.switchTypes[i] = (char)file.read() - 48;
    file.read();
    file.read();

    // Read presets
    for (uint8_t i = 0; i < NUM_PRESETS; i++)
    {
        if (file.available())
        {
            // Read loop status
            for (uint8_t j = 0; j < NUM_LOOPS; j++)
                presets[i].loops[j] = (char)file.read() - 48;
            file.read();

            // Read switch status
            for (uint8_t j = 0; j < NUM_SWITCHES; j++)
                presets[i].switches[j] = (char)file.read() - 48;
            file.read();
            file.read();
        }
    }

    file.close();
}

void printConfig()
{
    Serial.print("MIDI Channel#: ");
    Serial.println(config.MIDI_CHANNEL);

    Serial.println("Switch Types:");
    for (uint8_t i = 0; i < NUM_SWITCHES; i++)
    {
        Serial.print("\tSwitch #");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(config.switchTypes[i]);
    }

    Serial.println("Programs:");
    for (uint8_t i = 0; i < NUM_PRESETS; i++)
    {
        Serial.print("\tProgram #");
        Serial.print(i + 1);
        Serial.print(" | ");

        Serial.print("Loops: ");
        for (uint8_t j = 0; j < NUM_LOOPS; j++)
            Serial.print(presets[i].loops[j]);
        Serial.print(" | ");

        Serial.print("Switches: ");
        for (uint8_t j = 0; j < NUM_SWITCHES; j++)
            Serial.print(presets[i].switches[j]);
        Serial.println("");
    }
}
