#include <FastLED.h>
#define BUFFER_SIZE 512
#define NUM_LEDS 50
#define MESSAGE_START '!'
#define MESSAGE_END '$'

/* Pin Definitions */
#define DATA_PIN 5
#define OLED_RESET_PIN 3

/* OLED Display Props */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define FLASH_DURATION 60000 // flash for 60 seconds
#define FLASH_PERIOD 60 // flash every 60 seconds

CRGB leds[NUM_LEDS];

char buffer[BUFFER_SIZE];
bool new_data = false;

enum LedState { OFF, ON, FADE, FLASH };

struct Message { 
    uint8_t id;
    enum LedState type;
    CRGB color;
    uint8_t percentage;
};

void processBuffer() {
    int flash = beatsin16(FLASH_PERIOD, 0, 255); // precompute sin value for flash.

    if (new_data) {
        struct Message message;

        // Tokenize the string so we can parse sections of "TYPE,ID,COLOR,PERCENTAGE;"
        char* command = strtok(buffer, ";");
        while (command != NULL) {
            uint32_t color;
            // ty @mon :^)
            if (sscanf(command, "%d,%hhu,%lu,%hhu", &message.type, &message.id, &color, &message.percentage) == 4) {
                message.color = CRGB(color); // being careful...
                if (message.type == FLASH) {
                    message.percentage = 255;
                }

                processMessage(&message);
            }
            command = strtok(NULL, ";");
        }
    new_data = false;
    }
}

void processMessage(Message* message) {
    switch(message->type) {
        case OFF:
            leds[message->id] = CRGB::Black;
            Serial.print("[State] OFF -> led id: " );
            Serial.println(message->id);
            break;
        case ON:
            leds[message->id] = message->color;
            Serial.print("[State] ON -> led id: " );
            Serial.println(message->id);
            break;
        case FADE:
            leds[message->id] = message->color;
            leds[message->id].fadeLightBy(message->percentage);
            Serial.print("[State] FADE -> led id: ");
            Serial.println(message->id);
            break;
        case FLASH:
            leds[message->id] = message->color;
            // CRGB addition with FastLED does NOT overflow
            Serial.print("[State] FLASH -> led id: ");
            leds[message->id] += CRGB(message->percentage, message->percentage, message->percentage); 
            Serial.println(message->id);
            break;
    }
}

void readSerial() {
    static bool read_state = false;
    static uint8_t index = 0;
    char rc;

    while (Serial.available() > 0 && !new_data) {
        rc = Serial.read();
        if (read_state) {
            if (rc != MESSAGE_END) {
                buffer[index++] = rc;
                if (index >= BUFFER_SIZE - 1) {
                    buffer[index] = '\0';
                    read_state = false;
                    new_data = true;
                    index = 0;
                }
            } else {
                buffer[index] = '\0';
                read_state = false;
                new_data = true;
                index = 0;
            }
        } else if (rc == MESSAGE_START) {
            read_state = true;
        }
    }
}

void setup() {
    Serial.begin(9600);

    // Initialize leds
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

void loop() {
    readSerial();
    processBuffer();
    FastLED.show();

    delay(10);
}
