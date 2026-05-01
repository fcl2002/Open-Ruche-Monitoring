/* Edge Impulse Arduino examples - Modified for BeeGuard Production (Deep Sleep / Latch) */
#include <BeeGuardAI_Hornet_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"

#define UART_TX_PIN 43 // D6 on XIAO ESP32S3
#define UART_RX_PIN 44 // D7 on XIAO ESP32S3
HardwareSerial SerialVespa(1); // Uses ESP32 UART channel 1

// Select camera model for XIAO ESP32S3 Sense
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#if defined(CAMERA_MODEL_XIAO_ESP32S3)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13
#else
#error "Camera model not selected or not supported"
#endif

/* ------------------------------------------------------------------------- */
/* PRODUCTION SETTINGS AND ANTI-FALSE POSITIVE FILTER                        */
/* ------------------------------------------------------------------------- */
#define DEBUG_SERIAL           1     // Set to 1 only if you want to read human-readable error/timing prints
#define CONFIDENCE_THRESHOLD   0.70f 
#define REQUIRED_DETECTIONS    4     
#define TIME_WINDOW_MS         5000  // Time window: must detect 4 times within 5 seconds

const char* TARGET_LABEL = "Hornet"; 

// Native capture resolution
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private Variables ------------------------------------------------------- */
static bool debug_nn = false;
static bool is_initialised = false;
uint8_t *snapshot_buf; 

static unsigned long detection_times[REQUIRED_DETECTIONS] = {0};
static int detections_in_buffer = 0;

// ADDED: Alarm latch variable
static bool alarm_triggered = false; 

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,  .pin_reset = RESET_GPIO_NUM, .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM, .pin_sccb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM, .pin_d6 = Y8_GPIO_NUM, .pin_d5 = Y7_GPIO_NUM, .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM, .pin_d2 = Y4_GPIO_NUM, .pin_d1 = Y3_GPIO_NUM, .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM, .pin_href = HREF_GPIO_NUM, .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000, .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12, .fb_count = 1, .fb_location = CAMERA_FB_IN_PSRAM, .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    SerialVespa.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    if (DEBUG_SERIAL) Serial.println("Starting BeeGuard - Production Mode (With Latch)");
    
    if (ei_camera_init() == false) {
        if (DEBUG_SERIAL) Serial.println("Camera initialization failed!");
        while(1); // Halt if camera fails to start
    }
}

void loop() {
    // ADDED: If hornet already detected, latch here! 
    // Keeps sending '1' and saves battery by not taking new photos, waiting for power cut.
    if (alarm_triggered) {
        SerialVespa.write(1);
        if (DEBUG_SERIAL) Serial.println("LATCHED STATE: Sending 1... Waiting for power cut.");
        ei_sleep(250); // Sends approximately 4 times per second
        return; 
    }

    ei_sleep(10); 
    unsigned long current_time = ei_read_timer_ms();

    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    if(snapshot_buf == nullptr) return;

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (ei_camera_capture((size_t)EI_CAMERA_RAW_FRAME_BUFFER_COLS, (size_t)EI_CAMERA_RAW_FRAME_BUFFER_ROWS, snapshot_buf) == false) {
        free(snapshot_buf);
        return;
    }

    uint8_t *inference_buf = (uint8_t*)malloc(EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CAMERA_FRAME_BYTE_SIZE);
    if(inference_buf == nullptr) {
        free(snapshot_buf); return;
    }

    ei::image::processing::crop_and_interpolate_rgb888(
        snapshot_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        inference_buf, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);

    memcpy(snapshot_buf, inference_buf, EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CAMERA_FRAME_BYTE_SIZE);
    free(inference_buf);

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        free(snapshot_buf); return;
    }

    if (DEBUG_SERIAL) {
        Serial.print("Inference OK | Timing: ");
        Serial.print(result.timing.classification);
        Serial.print("ms | ");
        
        bool saw_something = false;
        for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
            if (result.bounding_boxes[i].value > 0.40) { // Show if at least 40% confidence
                Serial.print("Detected: ");
                Serial.print(result.bounding_boxes[i].label);
                Serial.print(" (");
                Serial.print(result.bounding_boxes[i].value);
                Serial.print(")  ");
                saw_something = true;
            }
        }
        if (!saw_something) {
            Serial.print("Nothing on radar.");
        }
        Serial.println();
    }

    /* --------------------------------------------------------------------- */
    /* NEW LOGIC: SLIDING WINDOW WITH LATCHING                               */
    /* --------------------------------------------------------------------- */
    bool hornet_in_this_frame = false;

    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value >= CONFIDENCE_THRESHOLD && strcmp(bb.label, TARGET_LABEL) == 0) {
            hornet_in_this_frame = true;
            break; 
        }
    }

    if (hornet_in_this_frame) {
        // Shift old history to the left
        for (int i = 0; i < REQUIRED_DETECTIONS - 1; i++) {
            detection_times[i] = detection_times[i+1];
        }
        // Store new detection in the most recent position (right)
        detection_times[REQUIRED_DETECTIONS - 1] = current_time;

        if (detections_in_buffer < REQUIRED_DETECTIONS) {
            detections_in_buffer++;
        }

        // If memory is full (we have 4 recorded detections)
        if (detections_in_buffer == REQUIRED_DETECTIONS) {
            // Compare the most recent detection with the oldest recorded one
            unsigned long oldest_detection = detection_times[0];
            
            if (current_time - oldest_detection <= TIME_WINDOW_MS) {
                // ALARM CONFIRMED! 4 hornets detected within the time window!
                alarm_triggered = true; // ACTIVATE LATCH HERE
                SerialVespa.write(1); 
                
                if(DEBUG_SERIAL) Serial.println("=========================================");
                if(DEBUG_SERIAL) Serial.println("MAXIMUM ALERT: HORNET CONFIRMED ON PIN D6!");
                if(DEBUG_SERIAL) Serial.println("=========================================");
                
            } else {
                // Hornet seen, but old ones have already expired
                SerialVespa.write(0);
            }
        } else {
            SerialVespa.write(0); // Accumulating, haven't reached 4 yet
        }
    } else {
        SerialVespa.write(0); // Nothing in this frame
    }

    free(snapshot_buf);
}


bool ei_camera_init(void) {
    if (is_initialised) return true;
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) return false;
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1); s->set_brightness(s, 1); s->set_saturation(s, 0);
    }
#if defined(CAMERA_MODEL_XIAO_ESP32S3)
    s->set_vflip(s, 1); s->set_hmirror(s, 1);
#endif
    is_initialised = true; return true;
}

void ei_camera_deinit(void) {
    esp_err_t err = esp_camera_deinit();
    if (err == ESP_OK) is_initialised = false;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    if (!is_initialised) return false;
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return false;
    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);
    return converted;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;
    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
        out_ptr_ix++; pixel_ix+=3; pixels_left--;
    }
    return 0;
}