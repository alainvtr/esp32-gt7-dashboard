// 21/09/2026 14:54 - Nouveau profil LovyanGFX pour la carte Sunton ESP32-3248S035R (ST7796 320x480 + tactile resistif XPT2046), ajoute pour pouvoir utiliser cette carte sans modifier les themes.
#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

// Carte Sunton ESP32-3248S035 (variante R = tactile RESISTIF XPT2046).
// Selectionnee via le flag de build -DBOARD_ESP32_3248S035=1 (voir platformio.ini,
// environnement esp32-3248s035) et incluse depuis SHCustomProtocol.h.
//
// Differences par rapport a la carte ESP32-2432S028 (fichier LGFX_ESP32_esp32-2432s028.hpp) :
//  - Controleur ST7796, 320x480 en natif (au lieu de ILI9341/ST7789 240x320)
//  - Retroeclairage sur GPIO 27 (et non 21)
//  - Le tactile XPT2046 partage le bus SPI de l'ecran (CLK 14, MOSI 13, MISO 12),
//    seul son CS (GPIO 33) lui est propre. Il n'a plus de bus dedie 25/32/39.
//  - RST de l'ecran relie a EN (pas de GPIO) => pin_rst = -1
//
// Les themes dessinent toujours dans une zone 320x240 (coin haut-gauche de l'ecran
// une fois en paysage 480x320) : le reste de l'ecran reste noir pour l'instant.

// Valeurs de depart, a valider sur la carte reelle (memes valeurs que l'environnement
// par defaut ILI9341 de la carte 2.8"). Si l'affichage est a l'envers : essayer 1 au lieu de 3.
// Si le tactile est inverse/miroir : ajuster DASHBOARD_TOUCH_ROTATION_OFFSET (0 a 7).
static constexpr uint8_t DASHBOARD_DISPLAY_ROTATION = 3;
static constexpr uint8_t DASHBOARD_TOUCH_ROTATION_OFFSET = 6;

class LGFX : public lgfx::LGFX_Device{
  lgfx::Panel_ST7796  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;
//----------------------------------------------------------------------
public:LGFX(void){
  {                            // Configuration du bus SPI de l'ecran (partage avec le tactile).
  auto cfg = _bus_instance.config();

  cfg.spi_host   = SPI2_HOST;  // HSPI
  cfg.spi_mode   = 0;
  cfg.freq_write = 40000000;   // 40 MHz (l'ST7796 supporte plus, mais on reste prudent avec le bus partage)
  cfg.freq_read  = 16000000;
  cfg.spi_3wire  = false;      // MISO (GPIO 12) est utilise par le tactile
  cfg.use_lock   = true;       // indispensable : bus partage avec le tactile
  cfg.dma_channel = 1;
  cfg.pin_sclk = 14;
  cfg.pin_mosi = 13;
  cfg.pin_miso = 12;
  cfg.pin_dc   = 2;

  _bus_instance.config(cfg);
  _panel_instance.setBus(&_bus_instance);
  }
  {                            // Configuration du panneau ST7796.
  auto cfg = _panel_instance.config();
  cfg.pin_cs   = 15;
  cfg.pin_rst  = -1;           // RST relie a EN sur cette carte
  cfg.pin_busy = -1;
  cfg.memory_width  = 320;
  cfg.memory_height = 480;
  cfg.panel_width   = 320;
  cfg.panel_height  = 480;
  cfg.offset_x = 0;
  cfg.offset_y = 0;
  cfg.offset_rotation = 0;
  cfg.dummy_read_pixel = 8;
  cfg.dummy_read_bits  = 1;
  cfg.readable   = true;
  cfg.invert     = false;      // si les couleurs sont inversees (fond blanc au lieu de noir) : passer a true
  cfg.rgb_order  = false;      // si rouge et bleu sont echanges : passer a true
  cfg.dlen_16bit = false;
  cfg.bus_shared = true;       // bus partage avec le tactile XPT2046
  _panel_instance.config(cfg);
  }
  {                            // Retroeclairage (GPIO 27 sur cette carte)
  auto cfg = _light_instance.config();
  cfg.pin_bl = 27;
  cfg.invert = false;
  cfg.freq   = 44100;
  cfg.pwm_channel = 7;
  _light_instance.config(cfg);
  _panel_instance.setLight(&_light_instance);
  }
  {                            // Tactile resistif XPT2046 sur le meme bus SPI que l'ecran
  auto cfg = _touch_instance.config();
  cfg.x_min = 200;             // valeurs brutes min/max (a affiner si besoin)
  cfg.x_max = 3900;
  cfg.y_min = 200;
  cfg.y_max = 3900;
  cfg.pin_int = -1;            // IRQ = GPIO 36 sur cette carte ; -1 = interrogation continue (comme la carte 2.8")
  cfg.bus_shared = true;       // meme bus que l'ecran
  cfg.offset_rotation = DASHBOARD_TOUCH_ROTATION_OFFSET;
  cfg.spi_host = SPI2_HOST;    // meme hote que l'ecran
  cfg.freq = 1000000;
  cfg.pin_sclk = 14;           // idem bus ecran
  cfg.pin_mosi = 13;
  cfg.pin_miso = 12;
  cfg.pin_cs   = 33;           // seul signal propre au tactile
  _touch_instance.config(cfg);
  _panel_instance.setTouch(&_touch_instance);
  }
  setPanel(&_panel_instance);
  }
};

//=====================================================================
