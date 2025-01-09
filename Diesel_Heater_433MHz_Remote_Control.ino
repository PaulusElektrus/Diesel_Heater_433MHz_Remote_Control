/* Diesel Heater 433 MHz Remote Control */

#define VERSION             "1.0.0 - PaulusElektrus"             

/* Temperature Sensor Library */
#include <DHT.h>
/* 433 MHz Library */
#include <RCSwitch.h>

/* Pin Assignment */
#define LED_PIN             2
#define DHT_PIN             3
#define TRANSMITTER_PIN     4
#define SWITCH_FROST_GUARD  8
#define SWITCH_AT_HOME      9

/* User Settings */
#define FROST_ON_TEMP               3.0     /* °C */
#define FROST_OFF_TEMP              10.0    /* °C */
#define FROST_PAUSE_TIME            30      /* Minutes */
#define FROST_SWITCH_ON_TIME        10      /* Minutes */
#define FROST_SWITCH_ON_DELTA_T     3.0     /* °C */
#define FROST_MAX_ON_TIME           30      /* Minutes */
#define FROST_RETRY_COUNT           1       /* times */
#define FROST_TIMES_MINUS_KNOB      5       /* times */

#define HOME_ON_TEMP                15.0    /* °C */
#define HOME_OFF_TEMP               25.0    /* °C */
#define HOME_PAUSE_TIME             30      /* Minutes */
#define HOME_SWITCH_ON_TIME         10      /* Minutes */
#define HOME_SWITCH_ON_DELTA_T      3.0     /* °C */
#define HOME_MAX_ON_TIME            30      /* Minutes */
#define HOME_RETRY_COUNT            0       /* times */
#define HOME_TIMES_MINUS_KNOB       5       /* times */

/* Conversion factor from msec to sec */
#define MIN_TO_MSEC                 60000
/* Interval to send serial report */
#define REPORT_TIME                 5000    /* msec */
/* Check temperature interval */
#define TEMP_CHECK_TIME             1000    /* msec */

/* Remote Codes */
#define REMOTE_ON           "10100110100011010001000110110000"
#define REMOTE_OFF          "10100110100011010000010100001000"
#define REMOTE_PLUS         "10100110100011010000100011110000"
#define REMOTE_MINUS        "10100110100011010000001010001000"
#define REMOTE_PULSLENGTH   257

/* States */
enum operatingModes
{
    OFF_MODE,
    FROST_GUARD_MODE,
    AT_HOME_MODE
};

enum heaterStates
{
    OFF, 
    ON,
    PAUSE,
    NO_OPERATION,
    ERROR_STATE,
    SWITCH_ON_PROCESS,
};

const char* heaterStatesStrings[6] = 
{
    "Off",
    "On",
    "Pause",
    "No Operation",
    "Error",
    "Switch On Process"
};

struct settings
{
    float           onTemp;
    float           offTemp;
    unsigned long   minTimeDelay;
    unsigned long   switchOnTime;
    float           switchOnDeltaT;
    unsigned long   maxOnTime;
    unsigned int    retryCount;
    unsigned int    timesMinusKnob;
};

/* Global Variables */
operatingModes  mode            = OFF_MODE;
heaterStates    state           = OFF;
bool            heaterOn        = false;
bool            debugMode       = false;   
float           temp            = NAN;
String          serialInput     = "";
String          errorString     = "No Errors";
settings        frostGuardSettings;
settings        atHomeSettings;
settings        actualSettings;

/* Init Devices*/
DHT         dht(DHT_PIN, DHT22);
RCSwitch    transmitter = RCSwitch();

void setup()
{
    Serial.begin(115200);
    Serial.print("\n\nDiesel Heater Control - Version: ");
    Serial.print(VERSION);
    Serial.println("\n");

    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SWITCH_FROST_GUARD, INPUT_PULLUP);
    pinMode(SWITCH_AT_HOME, INPUT_PULLUP);

    dht.begin();

    transmitter.enableTransmit(TRANSMITTER_PIN);
    transmitter.setPulseLength(REMOTE_PULSLENGTH);

    frostGuardSettings.onTemp           = FROST_ON_TEMP;
    frostGuardSettings.offTemp          = FROST_OFF_TEMP;
    frostGuardSettings.minTimeDelay     = FROST_PAUSE_TIME * MIN_TO_MSEC;
    frostGuardSettings.switchOnTime     = FROST_SWITCH_ON_TIME * MIN_TO_MSEC;
    frostGuardSettings.switchOnDeltaT   = FROST_SWITCH_ON_DELTA_T;
    frostGuardSettings.maxOnTime        = FROST_MAX_ON_TIME * MIN_TO_MSEC;
    frostGuardSettings.retryCount       = FROST_RETRY_COUNT;
    frostGuardSettings.timesMinusKnob   = FROST_TIMES_MINUS_KNOB;

    atHomeSettings.onTemp               = HOME_ON_TEMP;
    atHomeSettings.offTemp              = HOME_OFF_TEMP;
    atHomeSettings.minTimeDelay         = HOME_PAUSE_TIME * MIN_TO_MSEC;
    atHomeSettings.switchOnTime         = HOME_SWITCH_ON_TIME * MIN_TO_MSEC;
    atHomeSettings.switchOnDeltaT       = HOME_SWITCH_ON_DELTA_T;
    atHomeSettings.maxOnTime            = HOME_MAX_ON_TIME * MIN_TO_MSEC;
    atHomeSettings.retryCount           = HOME_RETRY_COUNT;
    atHomeSettings.timesMinusKnob       = HOME_TIMES_MINUS_KNOB;

    checkOperatingMode();
}

void loop()
{
    checkAdminMode();
    checkOperatingMode();
    checkTemperature();
    setHeater();
    serialReport();
}

void checkAdminMode()
{
    if (Serial.available() > 0)
    {
        char received = Serial.read();

        if (received == 'n')
        {
            adminMode(serialInput);
            serialInput = "";
        } 
        else 
        {
        serialInput += received;
        }

        if (serialInput.length() >= 5)
        {
            serialInput = "";
        }
    }
}

void adminMode(String userInput)
{
    bool adminModeOn = 1;

    userInput.trim();
    if (!userInput.equals("Admi") )
    {
        return;
    }

    Serial.println("\n### Admin Mode ###");
    printAdminMenu();

    while (adminModeOn) 
    {
        if (Serial.available() > 0)
        {
            char received = Serial.read();

            int receivedInt = received - '0';

            switch (receivedInt)
            {
            case 1:
                switchOn();
                Serial.println("Switched manually On");
                break;
            case 2:
                switchOff();
                Serial.println("Switched manually Off");
                break;
            case 3:
                setDebugTemperature();
                break;
            case 4:
                debugMode = false;
                Serial.println("Disabled Debug Mode");
                break;
            case 5:
                error("Test Error");
                Serial.println("Triggered Test Error");
                break;
            case 6:
                printSettings();
                break;
            case 7:
                state = OFF;
                Serial.println("Reset state to Off");
                break;
            case 8:
                serialReport();
                Serial.println("Triggered serial Report function");
                break;
            default:
                adminModeOn = 0;
                Serial.println("\n### Closed Admin Mode ###\n");
                break;
            }
            printAdminMenu();
        }
    }
}

void printAdminMenu()
{
    Serial.println("\n1) Switch On\n2) Switch Off\n3) Set Debug Temperature\n4) Disable Debug\n5) Trigger Error\n6) Print actual Settings\n7) Reset State to Off\n8) Trigger serial Report\n\nEvery other key ends Admin Mode.");
}

void setDebugTemperature()
{
    char buffer[4];
    float debugTemp;

    Serial.println("Set temperature for debug tests in format: (+/-XX)");

    while (Serial.available() < 3)
    {
        /* Wait */
    }
    for (int i = 0; i < 3; i++)
    {
        buffer[i] = Serial.read();
    }

    buffer[3] = '\0';

    if ((buffer[0] == '-' || buffer[0] == '+') && isdigit(buffer[1]) && isdigit(buffer[2]))
    {
        debugTemp = atof(buffer);
        Serial.print("Debug Temperature set to: ");
        Serial.println(debugTemp);
        temp = debugTemp;
        debugMode = true;
        return;
    }

    Serial.println("Please try again...");
}

void printSettings()
{
    Serial.println("\n### Settings ###\n");
    Serial.print("- On Temp: ");
    Serial.println(actualSettings.onTemp);
    Serial.print("- Off Temp: ");
    Serial.println(actualSettings.offTemp);
    Serial.print("- Min Time Delay: ");
    Serial.println(actualSettings.minTimeDelay);
    Serial.print("- Switch On Time: ");
    Serial.println(actualSettings.switchOnTime);
    Serial.print("- Switch On Delta T: ");
    Serial.println(actualSettings.switchOnDeltaT);
    Serial.print("- Max On Time: ");
    Serial.println(actualSettings.maxOnTime);
    Serial.print("- Retry Count: ");
    Serial.println(actualSettings.retryCount);
}

void checkOperatingMode()
{
    /* No operating mode changes during Switch On */
    if (state < SWITCH_ON_PROCESS)
    {
        if (!digitalRead(SWITCH_FROST_GUARD))
        {
            mode = FROST_GUARD_MODE;
            actualSettings = frostGuardSettings;
        }
        else if (!digitalRead(SWITCH_AT_HOME))
        {
            mode = AT_HOME_MODE;
            actualSettings = atHomeSettings;
        }
    }
}

void checkTemperature()
{
    static unsigned long lastReport = millis();

    if (millis() - lastReport >= TEMP_CHECK_TIME)
    {
        if (false == debugMode)
        {
            temp = dht.readTemperature();
            if (isnan(temp))
            {
                error("No temperature sensor data available");
            }
        }
    }
}

void setHeater()
{
    static unsigned long    lastTime        = millis();
    static unsigned int     retryCountOn    = 0;
    static unsigned int     retryCountOff   = 0;

    switch (state)
    {
    case OFF:
        retryCountOff = 0;
        if (temp <= actualSettings.onTemp)
        {
            switchOn();
            state = SWITCH_ON_PROCESS;
        }
        lastTime = millis();
        break;

    case SWITCH_ON_PROCESS:
        if (temp >= actualSettings.onTemp + actualSettings.switchOnDeltaT)
        {
            state = ON;
        }
        else if (millis() - lastTime >= actualSettings.switchOnTime)
        {
            if (actualSettings.retryCount >= retryCountOn)
            {
                error("Heater has not switched On");
            }
            else
            {
                state = OFF;
                retryCountOn++;
            }
        }
        break;

    case ON:
        retryCountOn = 0;
        if ((temp >= actualSettings.offTemp) || (millis() - lastTime >= actualSettings.maxOnTime))
        {
            switchOff();
            state = PAUSE;
            lastTime = millis();
        }
        break;

    case PAUSE:
        if (millis() - lastTime >= actualSettings.minTimeDelay)
        {
            state = OFF;
        }
        break;

    case NO_OPERATION:
        /* fall through */
    case ERROR_STATE:
        /* fall through */
    default:
        if (heaterOn)
        {
            switchOff();
        }
        break;
    }
}

void switchOn()
{
    digitalWrite(LED_PIN, 1);
    for (int i = 0; i < 1; i++)
    {
        transmitter.send(REMOTE_ON);
        delay(100);
    }
    minimalPower();
    heaterOn = HIGH;
    
}

void switchOff()
{
    digitalWrite(LED_PIN, 0);
    for (int i = 0; i < 5; i++)
    {
        transmitter.send(REMOTE_OFF);
        delay(100);
    }
    heaterOn = LOW;
}

void fullPower()
{
    for (int i = 0; i < 15; i++)
    {
        transmitter.send(REMOTE_PLUS);
        delay(100);
    }
}

void minimalPower()
{
    for (int i = 0; i < actualSettings.timesMinusKnob; i++)
    {
        transmitter.send(REMOTE_MINUS);
        delay(100);
    }
}

void serialReport()
{
    static unsigned long lastReport = millis();

    if (millis() - lastReport >= REPORT_TIME)
    {
        Serial.print("\nOperating Mode: ");
        if (FROST_GUARD_MODE == mode)
        {
            Serial.println("Frost Guard Mode");
        }
        else if (AT_HOME_MODE == mode)
        {
            Serial.println("At Home Mode");
        }
        else
        {
            Serial.println("Off Mode");
        }
        
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println(" °C");

        Serial.print("Heater is: ");
        if (true == heaterOn)
        {
            Serial.println("On");
        }
        else
        {
            Serial.println("Off");
        }

        Serial.print("Device State: ");
        Serial.println(heaterStatesStrings[state]);

        if (ERROR_STATE == state)
        {
            Serial.print("Error Occured: ");
            Serial.println(errorString);
        }

        if (true == debugMode)
        {
            Serial.println("\n### Debug Mode enabled ###");
        }

        lastReport = millis();
    }
}

void error(String newErrorString)
{
    state = ERROR_STATE;
    errorString = newErrorString;
    digitalWrite(LED_BUILTIN, HIGH);
}
