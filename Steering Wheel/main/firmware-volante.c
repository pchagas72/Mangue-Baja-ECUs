#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ssd1309_interface.h"
#include "can_management.h"
//#include "icons.h"

// Hardware configurations
// Check the can_management.h and ssd1309_interface.h for CAN and I2C
#define PIN_BUTTON      GPIO_NUM_0
#define TAG             "DASH_MAIN"

// Settings
#define FILTER_ALPHA    0.1f  // 0.1 = Smooth/Slow, 1.0 = Instant/Jittery
#define TIMEOUT_MS      1500  // Time before "NO DATA" error in ms
                                    
// Different screen modes
typedef enum {
    MODE_PILOT = 0,
    MODE_ENGINEER,
    MODE_ADVENTURE,
    MODE_NIGHT,
    MODE_COUNT
} dash_mode_t;

static dash_mode_t current_mode = MODE_NIGHT;
static uint8_t s_buffer[SSD1309_BUFFER_SIZE];
static int64_t race_start_time = 0;

// Filtering (keep these as floats)
typedef struct {
    float rpm;
    float fuel;
} filter_t;

filter_t signal_filter = {0};

// Graphics

void draw_race_timer(uint8_t *fb, int x, int y) {
    int64_t now = esp_timer_get_time();
    int64_t diff = (now - race_start_time) / 1000000; // Convert micros to seconds
    
    int hour = (diff / 3600) % 60;
    int min = (diff / 60) % 60;
    int sec = diff % 60;
    
    ssd1309_draw_string(fb, x, y, "%02d:%02d:%02d",hour, min, sec);
}

// Draw Arc (Bresenham-ish approximation)
void draw_arc(uint8_t *fb, int cx, int cy, int r, int start_angle, int end_angle) {
    float step = 10.0f; 
    float prev_x = -1, prev_y = -1;
    for (float a = start_angle; a >= end_angle; a -= step) {
        float rad = a * (3.14159f / 180.0f);
        int x = cx + (int)(r * cos(rad));
        int y = cy - (int)(r * sin(rad));
        if (prev_x != -1) ssd1309_draw_line(fb, (int)prev_x, (int)prev_y, x, y, 1);
        prev_x = x; prev_y = y;
    }
}

void draw_dynamic_gauge(uint8_t *fb, int cx, int cy, int r, float val, float max_val, const char* label, float split_pct) {
    int start_deg = 220; // 0% position
    int end_deg = -40;   // 100% position
    int total_sweep = start_deg - end_deg;
    
    // Calculate the angle where the gauge "breaks" (e.g. at 60%)
    int split_deg = start_deg - (int)(total_sweep * split_pct);

    // Determine Visibility
    bool unlocked = (val > (max_val * split_pct * 0.95f)); 

    int current_visible_end = unlocked ? end_deg : split_deg;

    // Draw Arc (Only the visible part)
    draw_arc(fb, cx, cy, r, start_deg, current_visible_end);

    // Draw Ticks
    int num_ticks = 5;
    for (int i = 0; i <= num_ticks; i++) {
        float tick_pct = (float)i / num_ticks;
        
        // If locked, skip ticks that are in the hidden zone
        if (!unlocked && tick_pct > split_pct) continue;

        float angle = start_deg - (tick_pct * total_sweep);
        float rad = angle * (3.14159f / 180.0f);
        
        int x0 = cx + (int)(r * cos(rad));
        int y0 = cy - (int)(r * sin(rad));
        int len = (i==0 || i==num_ticks) ? 6 : 3;
        int x1 = cx + (int)((r - len) * cos(rad));
        int y1 = cy - (int)((r - len) * sin(rad));
        ssd1309_draw_line(fb, x0, y0, x1, y1, 1);
    }

    // Draw Needle
    // Map value to angle
    float current_angle = start_deg - ((val / max_val) * total_sweep);
    if (current_angle > start_deg) current_angle = start_deg;
    if (current_angle < end_deg) current_angle = end_deg;

    float rad = current_angle * (3.14159f / 180.0f);
    int tip_x = cx + (int)((r - 2) * cos(rad));
    int tip_y = cy - (int)((r - 2) * sin(rad));
    ssd1309_draw_line(fb, cx, cy, tip_x, tip_y, 1);
    
    // Center Hub & Text
    ssd1309_draw_rect(fb, cx-2, cy-2, 5, 5, 1, 1);
    
    int txt_x = (val < 10) ? cx-3 : (val < 100) ? cx-6 : cx-9;
    ssd1309_draw_string(fb, txt_x, cy+6, "%.0f", val);
    ssd1309_draw_string(fb, cx-10, cy-8, label);
}

// Drawing Functions

// Pilot feedback
void draw_pilot(uint8_t *fb, car_state_t *car) {
    ssd1309_clear_buffer(fb);
    // Big Digital Speed
    ssd1309_draw_string_large(fb, 45, 10, 4, "%d", car->speed);
    ssd1309_draw_string(fb, 95, 40, "km/h");
    
    // Simple RPM Bar
    ssd1309_draw_rect(fb, 0, 0, 128, 8, 1, 0); 
    int bar_w = (car->rpm * 126) / 3800;
    if(bar_w > 126) bar_w = 126;
    for(int i=2; i<bar_w; i+=2) ssd1309_draw_rect(fb, i, 2, 1, 4, 1, 1);
    bool show_fuel = (car->fuel < 20);
    bool show_bat = (car->voltage < 11.8);
    bool show_cvt = (car->cvt_temp > 90);
    bool show_eng = (car->eng_temp > 90);

    // Warnings

    // Low Fuel Warning
    if (show_fuel) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 30, 56, "F");
            // ssd1309_draw_bitmap(fb, 30, 56, icon_fuel, 16, 16, 1); // I don't know how to draw.
        }
    }

    // Low Fuel Warning
    if (show_bat) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 20, 56, "B");
        }
    }

    // Low Fuel Warning
    if (show_cvt) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 10, 56, "T");
        }
    }

    if (show_eng) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 0, 56, "E");
        }
    }



    // Timing
    draw_race_timer(fb, 80, 56);
}

// Heavy data mode
void draw_engineer(uint8_t *fb, car_state_t *car) {
    ssd1309_clear_buffer(fb);
    ssd1309_draw_string(fb, 0, 0, "SYSTEM: ONLINE");
    ssd1309_draw_line(fb, 0, 10, 128, 10, 1);
    
    ssd1309_draw_string(fb, 0,  15, "RPM:%d", car->rpm);
    ssd1309_draw_string(fb, 65, 15, "SPD:%dkm/h", car->speed);
    ssd1309_draw_string(fb, 0,  28, "ENG:%d C", car->eng_temp);
    ssd1309_draw_string(fb, 65, 28, "CVT:%d C", car->cvt_temp);
    ssd1309_draw_string(fb, 0,  41, "BAT:%.1fV", car->voltage);
    ssd1309_draw_string(fb, 65, 41, "FUEL:%d%%", car->fuel);
    ssd1309_draw_string(fb, 0, 54, "R:%d P:%d", car->roll, car->pitch);
}

// Adventure mode, add more data here
void draw_adventure(uint8_t *fb, car_state_t *car) {
    ssd1309_clear_buffer(fb);
    int cx = 64, cy = 32;
    // Scale: 100 = 10.0 degrees
    float roll_rad = (car->roll / 10.0f) * (3.14159f / 180.0f); 
    int pitch_offset = (car->pitch / 10.0f); 
    
    // Calculate Horizon Line
    float cos_a = cos(roll_rad);
    float sin_a = sin(roll_rad);
    int len = 80; 
    int x0 = cx - (int)(len * cos_a);
    int y0 = (cy + pitch_offset) + (int)(len * sin_a);
    int x1 = cx + (int)(len * cos_a);
    int y1 = (cy + pitch_offset) - (int)(len * sin_a);
    
    ssd1309_draw_line(fb, x0, y0, x1, y1, 1);
    ssd1309_draw_line(fb, 60, 0, 68, 0, 1); // Sky Ref
    ssd1309_draw_line(fb, 44, 32, 84, 32, 1); // Wings
    
    ssd1309_draw_string(fb, 0, 56, "P:%d", car->pitch/10);
    ssd1309_draw_string(fb, 90, 56, "R:%d", car->roll/10);
}

// My mode, saab inspired
// Still needs much tweaking
void draw_night_mode(uint8_t *fb, car_state_t *car) {
    ssd1309_clear_buffer(fb);
    bool show_fuel = (car->fuel < 20);
    bool show_bat = (car->voltage < 11.8);
    bool show_cvt = (car->cvt_temp > 90);
    bool show_eng = (car->eng_temp > 90);

    // Speedometer (Centered Left)
    draw_dynamic_gauge(fb, 32, 32, 28, car->speed, 55.0f, "KPH", 0.65f);

    // Tachometer (Centered Right - Ghost)
    if (car->rpm > 3400) {
        if (xTaskGetTickCount() % 20 < 10) {
            draw_dynamic_gauge(fb, 96, 32, 28, car->rpm, 3800.0f, "RPM", 0.65f);
        }
    } else{
        draw_dynamic_gauge(fb, 96, 32, 28, car->rpm, 3800.0f, "RPM", 0.65f);
    }

    // Low Fuel Warning
    if (show_fuel) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 30, 56, "F");
            // ssd1309_draw_bitmap(fb, 30, 56, icon_fuel, 16, 16, 1); // I don't know how to draw.
        }
    }

    // Low Fuel Warning
    if (show_bat) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 20, 56, "B");
        }
    }

    // Low Fuel Warning
    if (show_cvt) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 10, 56, "T");
        }
    }

    if (show_eng) {
        if (xTaskGetTickCount() % 20 < 10) {
            ssd1309_draw_string(fb, 0, 56, "E");
        }
    }

    draw_race_timer(fb, 80, 56);
}

// Main function, no FreeRTOS needed here
void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t screen_handle;

    // Initialize using new driver [cite: 88, 120, 134]
    ESP_ERROR_CHECK(ssd1309_hw_init(&bus_handle, &screen_handle));
    ssd1309_init(screen_handle);
    can_init(); // Pin 5 (TX) - Pin 18 (RX)

    gpio_set_direction(PIN_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BUTTON, GPIO_PULLUP_ONLY);

    car_state_t car = {0};
    int64_t last_pkt_time = 0;
    int64_t last_btn_time = 0;

    ESP_LOGI(TAG, "Dashboard Initialized.");

    while (gpio_get_level(PIN_BUTTON) !=0) {
        ssd1309_draw_string_large(s_buffer, 10, 20, 2, "MANGUE");
        ssd1309_draw_string_large(s_buffer, 55, 40, 2, "BAJA");
        ssd1309_display_buffer(screen_handle, s_buffer);
    }

    race_start_time = esp_timer_get_time();

    while(1) {
        int64_t now = esp_timer_get_time() / 1000;

        // Updates data if available
        if (can_update_state(&car)) {
            last_pkt_time = now;
            car.link_active = true;
            
            // Apply Low-Pass Filter (EMA) to smooth needles
            signal_filter.rpm = (FILTER_ALPHA * car.rpm) + ((1.0 - FILTER_ALPHA) * signal_filter.rpm);
            signal_filter.fuel = (FILTER_ALPHA * car.fuel) + ((1.0 - FILTER_ALPHA) * signal_filter.fuel);
            
            // Write smoothed values back for display
            car.rpm = (uint16_t)signal_filter.rpm;
            car.fuel = (uint16_t)signal_filter.fuel;
        }

        // Dead link warning
        if (now - last_pkt_time > TIMEOUT_MS) {
            car.link_active = false;
            car.rpm = 0; car.speed = 0; // Kill gauges
        }
        // Check for button input
        if (gpio_get_level(PIN_BUTTON) == 0) {
            if (now - last_btn_time > 300) { 
                current_mode++;
                if (current_mode >= MODE_COUNT) current_mode = 0;
                last_btn_time = now;
            }
        }


        // Render screen
        if (!car.link_active) {
            ssd1309_clear_buffer(s_buffer);
            ssd1309_draw_rect(s_buffer, 0, 0, 128, 64, 1, 0); // Warning border
            ssd1309_draw_string_large(s_buffer, 15, 20, 2, "NO LINK");
            ssd1309_draw_string(s_buffer, 35, 45, "CHECK ECU");
        } else if (car.box_alert) {
            if (xTaskGetTickCount() % 20 < 10) {
                ssd1309_clear_buffer(s_buffer);
                ssd1309_draw_rect(s_buffer, 0, 0, 128, 64, 1, 0); // Warning border
                ssd1309_draw_string_large(s_buffer, 15, 20, 2, "BOX BOX!");
                switch (car.box_alert_message) {
                    case CVT: ssd1309_draw_string(s_buffer, 35, 45, "CVT ISSUE"); break;
                    case FUEL: ssd1309_draw_string(s_buffer, 35, 45, "REFUEL"); break;
                    case BAT: ssd1309_draw_string(s_buffer, 35, 45, "BAT SWITCH"); break;
                }
            }
        } else {
            switch(current_mode) {
                case MODE_PILOT:     draw_pilot(s_buffer, &car); break;
                case MODE_ENGINEER:  draw_engineer(s_buffer, &car); break;
                case MODE_ADVENTURE: draw_adventure(s_buffer, &car); break;
                case MODE_NIGHT:     draw_night_mode(s_buffer, &car); break;
                case MODE_COUNT: break;
            }
        }

        ssd1309_display_buffer(screen_handle, s_buffer);
        vTaskDelay(pdMS_TO_TICKS(30)); // 100 FPS target (system permitting)
    }
}
