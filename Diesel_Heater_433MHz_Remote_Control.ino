/* Diesel Heater 433 MHz Remote Control */

#define VERSION             "0.0.1"             

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
#define FROST_ON_TEMP           0   /* °C */
#define FROST_OFF_TEMP          10  /* °C */
#define FROST_MIN_TIME_DELAY    30  /* Minutes */
#define FROST_SWITCH_ON_TIME    5   /* Minutes */
#define FROST_SWITCH_OFF_TIME   5   /* Minutes */
#define FROST_MAX_ON_TIME       30  /* Minutes */
#define FROST_RETRY_COUNT       0   /* times */

#define HOME_ON_TEMP            15  /* °C */
#define HOME_OFF_TEMP           25  /* °C */
#define HOME_MIN_TIME_DELAY     30  /* Minutes */
#define HOME_SWITCH_ON_TIME     5   /* Minutes */
#define HOME_SWITCH_OFF_TIME    5   /* Minutes */
#define HOME_MAX_ON_TIME        30  /* Minutes */
#define HOME_RETRY_COUNT        0   /* times */

#define MIN_TO_MSEC             60000
#define REPORT_TIME             5000

/* Remote Codes */
#define REMOTE_ON           "10100110100011010001000110110000"
#define REMOTE_OFF          "10100110100011010000010100001000"
#define REMOTE_PLUS         "10100110100011010000100011110000"
#define REMOTE_Minus        "10100110100011010000001010001000"
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
    SWITCH_ON_PROCESS,
    ON,
    SWITCH_OFF_PROCESS,
    ERROR_STATE
};

struct settings
{
    int             onTemp;
    int             offTemp;
    unsigned long   minTimeDelay;
    unsigned long   switchOnTime;
    unsigned long   switchOffTime;
    unsigned long   maxOnTime;
    unsigned int    retryCount;
};


/* Global Variables */
operatingModes  mode            = OFF_MODE;
heaterStates    state           = OFF;
bool            heaterOn        = 0;   
float           temp            = NAN;
unsigned long   time            = millis();
String          serialInput     = "";
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
    Serial.println("\n\n");

    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SWITCH_FROST_GUARD, INPUT_PULLUP);
    pinMode(SWITCH_AT_HOME, INPUT_PULLUP);

    checkOperatingMode();

    dht.begin();

    transmitter.enableTransmit(TRANSMITTER_PIN);
    transmitter.setPulseLength(REMOTE_PULSLENGTH);

    frostGuardSettings.onTemp           = FROST_ON_TEMP;
    frostGuardSettings.offTemp          = FROST_OFF_TEMP;
    frostGuardSettings.minTimeDelay     = FROST_MIN_TIME_DELAY * MIN_TO_MSEC;
    frostGuardSettings.switchOnTime     = FROST_SWITCH_ON_TIME * MIN_TO_MSEC;
    frostGuardSettings.switchOffTime    = FROST_SWITCH_OFF_TIME * MIN_TO_MSEC;
    frostGuardSettings.maxOnTime        = FROST_MAX_ON_TIME * MIN_TO_MSEC;
    frostGuardSettings.retryCount       = FROST_RETRY_COUNT;

    atHomeSettings.onTemp               = HOME_ON_TEMP;
    atHomeSettings.offTemp              = HOME_OFF_TEMP;
    atHomeSettings.minTimeDelay         = HOME_MIN_TIME_DELAY * MIN_TO_MSEC;
    atHomeSettings.switchOnTime         = HOME_SWITCH_ON_TIME * MIN_TO_MSEC;
    atHomeSettings.switchOffTime        = HOME_SWITCH_OFF_TIME * MIN_TO_MSEC;
    atHomeSettings.maxOnTime            = HOME_MAX_ON_TIME * MIN_TO_MSEC;
    atHomeSettings.retryCount           = HOME_RETRY_COUNT;

    actualSettings                      = frostGuardSettings;

    Serial.println("### Device Report ###\n");
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

    Serial.println("\n### Admin Mode ###\n");
    digitalWrite(LED_BUILTIN, HIGH);

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
                break;
            case 2:
                switchOff();
                break;
            default:
                adminModeOn = 0;
                Serial.println("\n### Closed Admin Mode ###\n");
                break;
            }

        }
    }
    

}

void checkOperatingMode()
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
    else
    {
        mode = OFF_MODE;
        switchOff();
        state = SWITCH_OFF_PROCESS;
    }
}

void checkTemperature()
{
    temp = dht.readTemperature();
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
        if (temp < actualSettings.onTemp)
        {
            switchOn();
            state = SWITCH_ON_PROCESS;
        }
        break;

    case SWITCH_ON_PROCESS:
        if (temp >= actualSettings.onTemp + 1)
        {
            state = ON;
        }
        else if (millis() - lastTime >= actualSettings.switchOnTime)
        {
            if (actualSettings.retryCount >= retryCountOn)
            {
                state = ERROR_STATE;
            }
            else
            {
                state = OFF;
                retryCountOn++;
            }
            lastTime = millis();
        }
        break;

    case ON:
        retryCountOn = 0;
        if (temp > actualSettings.offTemp)
        {
            switchOff();
            state = SWITCH_OFF_PROCESS;
        }
        break;

    case SWITCH_OFF_PROCESS:
        if (temp <= actualSettings.offTemp - 1)
        {
            state = OFF;
        }
        else if (millis() - lastTime >= actualSettings.switchOffTime)
        {
            if (actualSettings.retryCount >= retryCountOff)
            {
                state = ERROR_STATE;
            }
            else
            {
                state = ON;
                retryCountOff++;
            }
            lastTime = millis();
        }
        break;
    
    case ERROR_STATE:
        /* fall through */
    default:
        switchOff();
        Serial.println("Error State - please Reset!");
        state = ERROR_STATE;
        break;
    }
}

void switchOn()
{
    for (int i = 0; i < 4; i++)
    {
        transmitter.send(REMOTE_ON);
        delay(100);
    }
    digitalWrite(LED_PIN, 1);
    
}

void switchOff()
{
    for (int i = 0; i < 4; i++)
    {
        transmitter.send(REMOTE_OFF);
        delay(100);
    }
    digitalWrite(LED_PIN, 0);
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
    for (int i = 0; i < 15; i++)
    {
        transmitter.send(REMOTE_Minus);
        delay(100);
    }
}

void serialReport()
{
    static unsigned long lastReport = millis();

    if (millis() - lastReport >= REPORT_TIME)
    {
        Serial.print("Operating Mode: ");
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
            Serial.println("Off");
        }
        
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println(" °C");

        lastReport = millis();
    }
}
