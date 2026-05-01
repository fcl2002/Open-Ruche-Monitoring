/* Edge Impulse ingestion SDK
 * Code adapted for beehive monitoring - TinyML
 */

#define EIDSP_QUANTIZE_FILTERBANK   0
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4

#include <PDM.h>
#include <Beehive_Audio_Classifier_inferencing.h>

/** Alert Settings */
float CONFIDENCE_THRESHOLD = 0.7; 
float ANOMALY_THRESHOLD = 0.3; // Above 0.3 is considered an unknown sound

// Thresholds for triggering alerts
int SWARMING_THRESHOLD = 15; 
int MISSING_QUEEN_THRESHOLD = 10; 

int swarming_counter = 0;
int missing_queen_counter = 0;

/** Communication Codes for the ESP32 */
#define CODE_NORMAL 1
#define CODE_SWARMING 2
#define CODE_MISSING_QUEEN 3

/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static bool record_ready = false;

#define PDM_BUFFER_SIZE 2048
static signed short sampleBuffer[PDM_BUFFER_SIZE];

static bool debug_nn = false; 
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
  
/* Function Prototypes */
static void pdm_data_ready_inference_callback(void);
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);

void setup() {
    // USB Serial (Debug)
    Serial.begin(115200);
    
    // Serial1 (TX/RX Pins for ESP32)
    Serial1.begin(115200); 

    // ADJUSTMENT: Do not block the code if no USB is connected (important for field use)
    unsigned long start_time = millis();
    while (!Serial && (millis() - start_time < 1000)); // Wait at most 1s for USB
    
    Serial.println("Beehive Monitor - beginning...");

    run_classifier_init();
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        Serial.println("ERR: Failure allocating audio buffer");
        return;
    }
}

void loop() {
    bool m = microphone_inference_record();
    if (!m) return;

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) return;

    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
        
        int8_t current_status = CODE_NORMAL; 
        bool is_anomaly = false;

        #if EI_CLASSIFIER_HAS_ANOMALY == 1
            if(result.anomaly > ANOMALY_THRESHOLD) {
                is_anomaly = true;
                if (swarming_counter > 0) swarming_counter--;
                if (missing_queen_counter > 0) missing_queen_counter--;
            }
        #endif

        if (!is_anomaly) {
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                if (strcmp(result.classification[ix].label, "swarming") == 0) {
                    if (result.classification[ix].value > CONFIDENCE_THRESHOLD) swarming_counter++;
                    else if (swarming_counter > 0) swarming_counter--;
                }
                
                if (strcmp(result.classification[ix].label, "no queen") == 0) {
                    if (result.classification[ix].value > CONFIDENCE_THRESHOLD) missing_queen_counter++;
                    else if (missing_queen_counter > 0) missing_queen_counter--;
                }
            }
        }

        // Determine the final status
        if (swarming_counter >= SWARMING_THRESHOLD) {
            current_status = CODE_SWARMING;
        } else if (missing_queen_counter >= MISSING_QUEEN_THRESHOLD) {
            current_status = CODE_MISSING_QUEEN;
        }

        // --- DATA TRANSFER TO ESP32 ---
        // Option A: Send the binary value (more efficient for processing)
        Serial1.write(current_status); 
        
        // Option B: If your ESP32 reads text (Serial.readString), keep the println:
        // Serial1.println(current_status);

        // Computer debug to verify TX output
        Serial.print("Status sent to TX: ");
        Serial.println(current_status);

        print_results = 0;
    }
}
/** * PDM Microphone Initialization with Adjusted Gain */
static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));
    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL || inference.buffers[1] == NULL) return false;

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    PDM.onReceive(&pdm_data_ready_inference_callback);
    PDM.setBufferSize((n_samples >> 1) * sizeof(int16_t));

    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
        return false;
    }

    PDM.setGain(30); 

    record_ready = true;
    return true;
}

static void pdm_data_ready_inference_callback(void) {
    int bytesAvailable = PDM.available();
    
    if (bytesAvailable > (PDM_BUFFER_SIZE * sizeof(signed short))) {
        bytesAvailable = PDM_BUFFER_SIZE * sizeof(signed short);
    }

    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (record_ready == true) {
        for (int i = 0; i < (bytesRead >> 1); i++) {
            inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];
            
            if (inference.buf_count >= inference.n_samples) {
                inference.buf_select ^= 1;
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
    }
}

static bool microphone_inference_record(void) {
    bool ret = true;
    if (inference.buf_ready == 1) {
        ei_printf("Warning: Buffer overrun (delayed processing).\n");
        ret = false;
    }
    
    while (inference.buf_ready == 0) {
        delay(1);
    }
    
    inference.buf_ready = 0;
    return true; 
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);
    return 0;
}