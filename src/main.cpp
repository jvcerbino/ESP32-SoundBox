#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "BluetoothA2DPSink.h"

/* MACROS */
#define TFT_HOR_RES 320
#define TFT_VER_RES 240
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

/* OBJETOS */
static TFT_eSPI tft = TFT_eSPI();
static lv_display_t *disp;
void *draw_buf_1; 
unsigned long lastTickMillis = 0;
lv_obj_t *menu; /* screen */
static BluetoothA2DPSink a2dp_sink; 

lv_obj_t *artist_label;
lv_obj_t *song_label;  
lv_obj_t *vol_bar;
lv_obj_t *vol_img;

String lv_song = "Waiting...";
String lv_artist = " ";
String lv_duration = " ";
int lv_vol = 0;
bool meta_update = false;
bool vol_update = false;

/* IMAGENS */
LV_IMAGE_DECLARE(pp);
LV_IMAGE_DECLARE(vol);
LV_IMAGE_DECLARE(no_vol);

/* INTERFACE GRÁFICA falta parse do ESP32-A2DP pra lvgl */
void gui_draw()
{

  menu = lv_obj_create(NULL);
  lv_obj_remove_flag(menu, LV_OBJ_FLAG_CLICKABLE); 
  lv_obj_set_style_radius(menu, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(menu, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_screen_load(menu);

  lv_obj_t *music_time = lv_bar_create(menu);
  lv_bar_set_value(music_time, 25, LV_ANIM_OFF);
  lv_bar_set_start_value(music_time, 0, LV_ANIM_OFF);
  lv_obj_set_width(music_time, 200);
  lv_obj_set_height(music_time, 10);
  lv_obj_set_x(music_time, 0);
  lv_obj_set_y(music_time, -15);
  lv_obj_set_align(music_time, LV_ALIGN_BOTTOM_MID);
  lv_obj_set_style_bg_color(music_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(music_time, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(music_time, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(music_time, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

  song_label = lv_label_create(menu);
  lv_obj_set_width(song_label, LV_SIZE_CONTENT); 
  lv_obj_set_height(song_label, LV_SIZE_CONTENT); 
  lv_obj_set_x(song_label, 0);
  lv_obj_set_y(song_label, -10);
  lv_obj_set_align(song_label, LV_ALIGN_CENTER);
  lv_label_set_text(song_label, lv_song.c_str());
  lv_obj_set_style_text_color(song_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(song_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(song_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

  artist_label = lv_label_create(menu);
  lv_obj_set_width(artist_label, LV_SIZE_CONTENT);
  lv_obj_set_height(artist_label, LV_SIZE_CONTENT);
  lv_obj_set_x(artist_label, 0);
  lv_obj_set_y(artist_label, 20);
  lv_obj_set_align(artist_label, LV_ALIGN_CENTER);
  lv_label_set_text(artist_label, lv_artist.c_str());
  lv_obj_set_style_text_color(artist_label, lv_color_hex(0xCCC3C3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(artist_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(artist_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *pp_img = lv_image_create(menu);
  lv_image_set_src(pp_img, &pp);
  lv_obj_set_width(pp_img, LV_SIZE_CONTENT);
  lv_obj_set_height(pp_img, LV_SIZE_CONTENT);
  lv_obj_set_x(pp_img, -130);
  lv_obj_set_y(pp_img, -9);
  lv_obj_set_align(pp_img, LV_ALIGN_BOTTOM_MID);

  vol_bar = lv_bar_create(menu);
  lv_bar_set_value(vol_bar, 25, LV_ANIM_OFF);
  lv_bar_set_start_value(vol_bar, 0, LV_ANIM_OFF);
  lv_obj_set_width(vol_bar, 150);
  lv_obj_set_height(vol_bar, 6);
  lv_obj_set_x(vol_bar, 0);
  lv_obj_set_y(vol_bar, -100);
  lv_obj_set_align(vol_bar, LV_ALIGN_CENTER);
  lv_obj_set_style_bg_color(vol_bar, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(vol_bar, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(vol_bar, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(vol_bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_bar_set_range(vol_bar, 0, 127);

  lv_obj_t *timeleft_label = lv_label_create(menu);
  lv_obj_set_width(timeleft_label, LV_SIZE_CONTENT);
  lv_obj_set_height(timeleft_label, LV_SIZE_CONTENT);
  lv_obj_set_x(timeleft_label, 130);
  lv_obj_set_y(timeleft_label, -14);
  lv_obj_set_align(timeleft_label, LV_ALIGN_BOTTOM_MID);
  lv_label_set_text(timeleft_label, "-5:00");
  lv_obj_set_style_text_color(timeleft_label, lv_color_hex(0xCCC3C3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(timeleft_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(timeleft_label, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

  vol_img = lv_image_create(menu);
  lv_image_set_src(vol_img, &vol);
  lv_obj_set_width(vol_img, LV_SIZE_CONTENT);
  lv_obj_set_height(vol_img, LV_SIZE_CONTENT);
  lv_obj_set_x(vol_img, -100);
  lv_obj_set_y(vol_img, -100);
  lv_obj_set_align(vol_img, LV_ALIGN_CENTER);
}

/* Callback de atualização */

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  String valor = String((char*)text);
  
  switch (id) {
    case ESP_AVRC_MD_ATTR_TITLE:
      lv_song = valor;
      break;
    case ESP_AVRC_MD_ATTR_ARTIST:
      lv_artist = valor;
      break;
    case ESP_AVRC_MD_ATTR_PLAYING_TIME:
      lv_duration = valor; 
      break;
  }
  meta_update = true;
}

void volume_change_callback(int volume){
  lv_vol = volume;
  vol_update = true;
}


void setup()
{
  tft.init();
  lv_init();
  draw_buf_1 = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf_1, DRAW_BUF_SIZE);

  static const i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = 44100,                        
      .bits_per_sample = (i2s_bits_per_sample_t)16, 
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = 0, 
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false};

  a2dp_sink.set_i2s_config(i2s_config);
  a2dp_sink.start("ESP32-SoundBox");
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_volumechange(volume_change_callback);
  gui_draw();
}

void loop()
{

  if(meta_update){
    lv_label_set_text(song_label, lv_song.c_str());
    lv_label_set_text(artist_label, lv_artist.c_str());
    meta_update = false;
  }

  if(vol_update){
    if(lv_vol == 0){
      lv_image_set_src(vol_img, &no_vol);
    }
    else{
      lv_image_set_src(vol_img, &vol);
    }
    lv_bar_set_value(vol_bar, lv_vol, LV_ANIM_ON);
    vol_update = false;
  }

  unsigned int tickPeriod = millis() - lastTickMillis;
  lv_tick_inc(tickPeriod);
  lastTickMillis = millis();
  lv_task_handler();
  delay(10);
}
